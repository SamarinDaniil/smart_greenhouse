#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="build/Release"

mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

# Шаг 1: Устанавливаем зависимости через Conan
conan install ../../conanfile.txt \
    --build=missing \
    -s build_type=Release \
    -g CMakeToolchain \
    -g CMakeDeps

# Шаг 2: Конфигурируем CMake с переданной toolchain-утилитой от Conan
cmake ../.. \
    -DCMAKE_TOOLCHAIN_FILE="${PWD}/generators/conan_toolchain.cmake" \
    -DCMAKE_BUILD_TYPE=Release

# Шаг 3: Собираем проект в режиме Release
cmake --build . --config Release

# Возвращаемся в корень проекта
cd - > /dev/null

echo "Сборка успешно завершена. Артефакты лежат в ${BUILD_DIR}"