#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="build/Release"

if [ ! -d "$BUILD_DIR" ]; then
    echo "Ошибка: директория сборки '${BUILD_DIR}' не найдена."
    echo "Сначала запусти скрипт конфигурации проекта."
    exit 1
fi

cd "${BUILD_DIR}"

cmake --build . --config Release

cd - > /dev/null

echo "Пересборка успешно завершена. Артефакты находятся в ${BUILD_DIR}"
