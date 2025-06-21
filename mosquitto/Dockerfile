# mosquitto/Dockerfile
FROM eclipse-mosquitto:latest

# Копируем кастомную конфигурацию
COPY config/mosquitto.conf /mosquitto/config/mosquitto.conf

# Копируем данные (если нужно)
COPY data/ /mosquitto/data/

# Открываем порт MQTT
EXPOSE 1883

# Запускаем Mosquitto с кастомной конфигурацией
CMD ["mosquitto", "-c", "/mosquitto/config/mosquitto.conf"]