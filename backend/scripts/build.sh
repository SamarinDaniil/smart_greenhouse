#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="build/Release"

echo "Создание директории сборки..."
mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

# Настройка профиля Conan (важно для Docker)
echo "Настройка Conan профиля..."
conan profile detect --force

# Шаг 1: Установка зависимостей через Conan
echo "Установка зависимостей Conan..."
conan install ../../conanfile.txt \
    --build=missing \
    -s build_type=Release \
    -g CMakeToolchain \
    -g CMakeDeps

# Шаг 2: Конфигурация CMake
echo "Генерация проекта CMake..."
cmake ../.. \
    -DCMAKE_TOOLCHAIN_FILE="${PWD}/generators/conan_toolchain.cmake" \
    -DCMAKE_BUILD_TYPE=Release

# Шаг 3: Сборка проекта
echo "Компиляция проекта..."
cmake --build . --config Release

# Возврат в корневую директорию
cd - > /dev/null

echo "✅ Сборка успешно завершена. Артефакты: ${BUILD_DIR}"