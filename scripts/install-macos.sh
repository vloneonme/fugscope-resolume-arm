#!/bin/bash
set -euo pipefail
project_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
[[ "$(uname -s)" == Darwin ]] || { echo 'Установка выполняется на Mac.' >&2; exit 1; }
architecture="${1:-arm64}"
case "$architecture" in arm64|universal) ;; *) exit 2 ;; esac
source_bundle="$project_dir/build/$architecture/FugScopeArm.bundle"
[[ -f "$source_bundle/Contents/MacOS/FugScopeArm" ]] || { echo 'Сначала выполните сборку.' >&2; exit 1; }
codesign --verify --strict "$source_bundle"
target_dir="$HOME/Documents/Resolume/Extra Effects"
mkdir -p "$target_dir"
target_bundle="$target_dir/FugScopeArm.bundle"
if [[ -e "$target_bundle" ]]; then
    backup_dir="$HOME/Documents/Resolume/FugScope Backups/$(date +%Y%m%d-%H%M%S)-$$"
    mkdir -p "$backup_dir"
    mv "$target_bundle" "$backup_dir/"
    echo "Предыдущая версия сохранена в $backup_dir"
fi
ditto "$source_bundle" "$target_bundle"
echo "Скопировано в: $target_dir"
echo 'В Resolume → Preferences → Video → FFGL Plugin Directories добавьте эту папку, если её нет в списке.'
echo 'Полностью перезапустите Resolume → Sources → FugScope ARM.'
