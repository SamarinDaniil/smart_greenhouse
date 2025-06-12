#!/bin/bash

# ANSI цвета для вывода
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

# Конфигурация из YAML
BROKER_IP="0.0.0.0"
PORT="1883"
USERNAME="iot_user"
PASSWORD="secure_password_123"
PASSWORD_FILE="/etc/mosquitto/passwd"
CONFIG_DIR="/etc/mosquitto/conf.d"
CONFIG_FILE="$CONFIG_DIR/greenhouse.conf"

# Проверка root-привилегий
if [ "$EUID" -ne 0 ]; then
  echo -e "${RED}Ошибка: Скрипт требует привилегий root. Запустите с sudo${NC}"
  exit 1
fi

# Функция для вывода разделителя
print_section() {
  echo -e "${GREEN}=== $1 ===${NC}"
}

# Функция для вывода ошибки и выхода
error_exit() {
  echo -e "${RED}$1${NC}"
  exit 1
}

# Проверка поддержки IPv6
check_ipv6() {
  if ip link show | grep -q 'inet6'; then
    echo "listener $PORT ::"
  fi
}

# Очистка старых конфигураций
clean_old_configs() {
  print_section "Очистка старых конфигураций"
  if [ -d "$CONFIG_DIR" ]; then
    rm -f "$CONFIG_DIR"/*.conf
    echo -e "${GREEN}✓ Старые конфигурации удалены${NC}"
  fi
}

# Установка Mosquitto
install_mosquitto() {
  print_section "Проверка установки Mosquitto"
  if command -v mosquitto > /dev/null; then
    echo -e "${YELLOW}Mosquitto уже установлен${NC}"
    return 0
  fi

  echo -e "${YELLOW}Обновляю список пакетов...${NC}"
  apt-get update -y || error_exit "Не удалось обновить пакеты"

  echo -e "${YELLOW}Устанавливаю Mosquitto...${NC}"
  apt-get install -y mosquitto mosquitto-clients || error_exit "Не удалось установить Mosquitto"
  echo -e "${GREEN}✓ Mosquitto установлен${NC}"
}

# Создание необходимых директорий
setup_directories() {
  print_section "Создание необходимых директорий"
  DIRS=("/etc/mosquitto" "/var/lib/mosquitto" "/var/log/mosquitto" "/run/mosquitto")

  for dir in "${DIRS[@]}"; do
    if [ ! -d "$dir" ]; then
      mkdir -p "$dir" || error_exit "Не удалось создать директорию $dir"
    fi
    chown mosquitto:mosquitto "$dir"
    chmod 750 "$dir"
  done

  # Создание файла логов
  touch /var/log/mosquitto/mosquitto.log
  chown mosquitto:mosquitto /var/log/mosquitto/mosquitto.log
  chmod 640 /var/log/mosquitto/mosquitto.log
  echo -e "${GREEN}✓ Все необходимые директории и файлы созданы${NC}"
}

# Создание файла паролей
setup_password() {
  print_section "Настройка аутентификации"
  if [ -f "$PASSWORD_FILE" ]; then
    echo -e "${YELLOW}Файл паролей уже существует${NC}"
    return 0
  fi

  echo -e "${YELLOW}Создаю файл паролей...${NC}"
  touch "$PASSWORD_FILE" || error_exit "Не удалось создать файл паролей"
  chown mosquitto:mosquitto "$PASSWORD_FILE"
  chmod 600 "$PASSWORD_FILE"

  mosquitto_passwd -b "$PASSWORD_FILE" "$USERNAME" "$PASSWORD" || error_exit "Не удалось создать учетную запись"
  echo -e "${GREEN}✓ Пароль пользователя $USERNAME установлен${NC}"
}

# Создание файла базы данных
setup_database() {
  print_section "Настройка базы данных"
  DB_FILE="/var/lib/mosquitto/mosquitto.db"
  if [ ! -f "$DB_FILE" ]; then
    touch "$DB_FILE" || error_exit "Не удалось создать файл базы данных"
    chown mosquitto:mosquitto "$DB_FILE"
    chmod 600 "$DB_FILE"
    echo -e "${GREEN}✓ База данных создана${NC}"
  else
    echo -e "${YELLOW}Файл базы данных уже существует${NC}"
  fi
}

# Настройка конфигурации
configure_broker() {
  print_section "Настройка конфигурации"
  # Основной конфиг
  cat > /etc/mosquitto/mosquitto.conf << EOL
pid_file /run/mosquitto/mosquitto.pid

persistence true


log_dest file /var/log/mosquitto/mosquitto.log
log_type all

include_dir /etc/mosquitto/conf.d
EOL

  # Конфиг для greenhouse
  cat > "$CONFIG_FILE" << EOL
# Прослушивание портов
listener $PORT $BROKER_IP
$(check_ipv6)

# Аутентификация
password_file $PASSWORD_FILE
allow_anonymous false

# Хранение данных
persistence true
persistence_location /var/lib/mosquitto/
persistence_file mosquitto.db

# Безопасность
autosave_interval 600
autosave_on_changes false
EOL

  echo -e "${GREEN}✓ Конфигурация сохранена в $CONFIG_FILE${NC}"
}

# Настройка фаервола
setup_firewall() {
  print_section "Настройка фаервола"
  if command -v ufw > /dev/null; then
    echo -e "${YELLOW}Разрешаю порт $PORT в фаерволе...${NC}"
    ufw allow "$PORT/tcp" > /dev/null 2>&1
    ufw allow from 192.168.1.0/24 > /dev/null 2>&1
    echo -e "${GREEN}✓ Порт $PORT разрешен в фаерволе${NC}"
  else
    echo -e "${YELLOW}UFW не установлен. Пропускаю настройку фаервола${NC}"
  fi
}

# Перезапуск сервиса
restart_service() {
  print_section "Перезапуск Mosquitto"
  # Очистка занятых портов
  if lsof -i :$PORT > /dev/null 2>&1; then
    echo -e "${YELLOW}Очищаю занятые порты...${NC}"
    kill -9 $(lsof -t -i :$PORT) > /dev/null 2>&1 || true
    sleep 2
  fi

  systemctl daemon-reload
  systemctl restart mosquitto || error_exit "Не удалось перезапустить Mosquitto"

  # Проверка статуса
  if ! systemctl is-active --quiet mosquitto; then
    echo -e "${RED}Ошибка: Mosquitto не запущен! Проверьте логи:${NC}"
    journalctl -u mosquitto.service -n 20
    exit 1
  fi
  echo -e "${GREEN}✓ Mosquitto успешно перезапущен${NC}"
}

# Тестовое подключение
test_connection() {
  print_section "Проверка подключения"
  timeout 5 mosquitto_pub -h 127.0.0.1 -p $PORT -t "test" -m "connection_ok" \
    -u "$USERNAME" -P "$PASSWORD" > /dev/null 2>&1

  if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓ Подключение успешно! Брокер готов к использованию${NC}"
  else
    echo -e "${RED}⚠️  Подключение не удалось. Рекомендуем проверить:${NC}"
    echo -e "1. journalctl -u mosquitto.service"
    echo -e "2. tail -n 20 /var/log/mosquitto/mosquitto.log"
  fi
}

# Основной процесс
echo -e "${GREEN}=== Автоматическая настройка Mosquitto для Smart Greenhouse ===${NC}"

clean_old_configs
install_mosquitto
setup_directories
setup_password
setup_database
configure_broker
setup_firewall
restart_service
test_connection

echo -e "${GREEN}✅ Настройка завершена успешно!${NC}"