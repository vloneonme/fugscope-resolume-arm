#!/bin/bash
set -euo pipefail
project_dir="$(cd "$(dirname "$0")" && pwd)"
trap 'status=$?; if [[ $status != 0 ]]; then echo "Операция не завершена. Смотрите ошибку выше."; fi; read -r -p "Нажмите Enter, чтобы закрыть окно…" || true' EXIT
bash "$project_dir/scripts/build-macos.sh" arm64
bash "$project_dir/scripts/install-macos.sh" arm64
