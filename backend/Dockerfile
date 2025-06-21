# === Stage 1: Сборка проекта ===
FROM ubuntu:22.04 AS builder

# Установка зависимостей для сборки
RUN apt-get update && \
    apt-get install -y \
        build-essential \
        cmake \
        git \
        pkg-config \
        python3-pip \
        software-properties-common \
        curl \
        openssl \
        libssl-dev \
        libssl3 \
        libcrypto++-dev \
        zlib1g-dev \
        ca-certificates \
        libbz2-dev \
        libffi-dev \
        libsqlite3-dev \
        uuid-dev \
        libboost-all-dev \
    && rm -rf /var/lib/apt/lists/*

# Установка GCC 13
RUN add-apt-repository ppa:ubuntu-toolchain-r/test -y && \
    apt-get update && \
    apt-get install -y gcc-13 g++-13 && \
    update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-13 60 \
        --slave /usr/bin/g++ g++ /usr/bin/g++-13

# Установка Conan 2.15.1
RUN pip3 install "conan==2.15.1"

# Проверка версии Conan
RUN conan --version

# Настройка Conan профиля
RUN conan profile detect --force

# Рабочая директория
WORKDIR /workspace

# Копируем только файлы конфигурации сначала для лучшего кэширования
COPY conanfile.txt .

# Очищаем кэш Conan
RUN conan remove "*" --confirm || true

# Устанавливаем зависимости Conan с принудительной пересборкой OpenSSL
RUN mkdir -p build/Release && cd build/Release && \
    conan install ../../conanfile.txt \
        --build=openssl* \
        --build=cpp-jwt* \
        --build=missing \
        -s build_type=Release \
        -s compiler=gcc \
        -s compiler.version=13 \
        -s compiler.libcxx=libstdc++11 \
        -s compiler.cppstd=20 \
        -c tools.system.package_manager:mode=install \
        -c tools.system.package_manager:sudo=True

# Теперь копируем остальные файлы
COPY . .

# Сборка проекта (очищаем кэш CMake перед сборкой)
RUN cd build/Release && \
    # Удаляем старый кэш CMake
    rm -f CMakeCache.txt && \
    # Устанавливаем переменные окружения для поиска системных библиотек
    export PKG_CONFIG_PATH=/usr/lib/x86_64-linux-gnu/pkgconfig:$PKG_CONFIG_PATH && \
    export OPENSSL_ROOT_DIR=/usr && \
    export OPENSSL_INCLUDE_DIR=/usr/include/openssl && \
    export OPENSSL_CRYPTO_LIBRARY=/usr/lib/x86_64-linux-gnu/libcrypto.so && \
    export OPENSSL_SSL_LIBRARY=/usr/lib/x86_64-linux-gnu/libssl.so && \
    # Генерируем проект
    cmake ../.. \
        -DCMAKE_TOOLCHAIN_FILE="${PWD}/generators/conan_toolchain.cmake" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_CXX_COMPILER=g++-13 \
        -DCMAKE_VERBOSE_MAKEFILE=ON \
        -DCMAKE_FIND_DEBUG_MODE=ON \
        -DOPENSSL_ROOT_DIR=/usr \
        -DOPENSSL_INCLUDE_DIR=/usr/include/openssl \
        -DOPENSSL_CRYPTO_LIBRARY=/usr/lib/x86_64-linux-gnu/libcrypto.so \
        -DOPENSSL_SSL_LIBRARY=/usr/lib/x86_64-linux-gnu/libssl.so && \
    # Собираем проект
    cmake --build . --config Release --parallel $(nproc)

# === Stage 2: Финальный образ ===
FROM ubuntu:22.04

# Установка runtime-зависимостей
RUN apt-get update && \
    apt-get install -y \
        libssl3 \
        libsqlite3-0 \
        libc6 \
        libgcc-s1 \
        libstdc++6 \
        curl \
    && rm -rf /var/lib/apt/lists/*

# Создание пользователя для безопасности
RUN groupadd -r appuser && useradd -r -g appuser appuser

# Создание необходимых директорий
RUN mkdir -p /app/config /app/data /app/logs && \
    chown -R appuser:appuser /app

# Копирование скомпилированного бинарника и конфигов
COPY --from=builder --chown=appuser:appuser /workspace/build/Release/smart_greenhouse /app/
COPY --from=builder --chown=appuser:appuser /workspace/config /app/config/
COPY --from=builder --chown=appuser:appuser /workspace/data /app/data/
COPY --from=builder --chown=appuser:appuser /workspace/logs /app/logs/

# Установка прав доступа
RUN chmod +x /app/smart_greenhouse

# Переключение на непривилегированного пользователя
USER appuser

# Рабочая директория
WORKDIR /app

# Открытие портов
EXPOSE 8080 1883

# Проверка здоровья приложения
HEALTHCHECK --interval=30s --timeout=3s --start-period=5s --retries=3 \
    CMD curl -f http://localhost:8080/health || exit 1

# Запуск сервера
CMD ["./smart_greenhouse", "--run"]