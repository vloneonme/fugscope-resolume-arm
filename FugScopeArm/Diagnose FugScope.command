#!/bin/bash
# Read-only checks of the plugin and narrowly filtered Resolume logs.
# Writes a report beside this script; does not change plugins, settings or signatures.
set -u
if [[ "$(uname -s)" != Darwin ]]; then
    echo 'Эта диагностика предназначена для Mac.' >&2
    exit 1
fi
script_dir="$(cd "$(dirname "$0")" && pwd)"
report_dir="$script_dir/FugScope-Diagnostics"
mkdir -p "$report_dir" || exit 1
report="$report_dir/FugScope-$(date +%Y%m%d-%H%M%S)-$$.txt"
shopt -s nullglob
inspect_bundle() {
    local bundle="$1"
    local executable="$bundle/Contents/MacOS/FugScopeArm"
    echo
    echo "BUNDLE: $bundle"
    /usr/bin/plutil -lint "$bundle/Contents/Info.plist" 2>&1
    if [[ ! -f "$executable" ]]; then
        echo 'ERROR: Contents/MacOS/FugScopeArm отсутствует'
        return
    fi
    /usr/bin/file "$executable"
    /usr/bin/xcrun lipo "$executable" -archs 2>&1
    /usr/bin/codesign --verify --strict --verbose=2 "$bundle" 2>&1
    echo "codesign status: $? (0 = OK)"
    echo 'FFGL exported symbols:'
    /usr/bin/xcrun nm -gU "$executable" 2>&1 | /usr/bin/grep -E '(_plugMain|_SetLogCallback|error:)' || echo 'ERROR: FFGL symbols not found, or nm failed'
    echo 'Linked libraries:'
    /usr/bin/xcrun otool -L "$executable" 2>&1
    echo 'Bundle quarantine attribute:'
    /usr/bin/xattr -p com.apple.quarantine "$bundle" 2>/dev/null || echo '(none or unreadable)'
    echo 'Binary quarantine attribute:'
    /usr/bin/xattr -p com.apple.quarantine "$executable" 2>/dev/null || echo '(none or unreadable)'
}
{
    echo 'FugScope ARM — диагностика обнаружения в Resolume'
    /usr/bin/sw_vers
    echo "Terminal architecture: $(uname -m)"
    echo 'Проверьте также Activity Monitor → CPU → Kind для процесса Resolume: Apple или Intel.'
    echo 'Наличие ARM64 в плагине не означает, что сам Resolume запущен нативно.'
    found=0
    for bundle in "$HOME"/Documents/Resolume*/"Extra Effects"/FugScopeArm.bundle; do
        found=$((found+1))
        inspect_bundle "$bundle"
    done
    if [[ "$found" == 0 ]]; then
        echo 'ERROR: FugScopeArm.bundle не найден в Documents/Resolume*/Extra Effects.'
    fi
    # Show existing build outputs separately; their presence is not installation.
    for bundle in "$script_dir"/build/*/FugScopeArm.bundle "$HOME"/Downloads/FugScopeArm/build/*/FugScopeArm.bundle; do
        echo "BUILD OUTPUT (не подтверждает установку): $bundle"
    done
    for app in "$HOME"/Applications/Resolume*.app /Applications/Resolume*.app /Applications/Resolume*/*.app; do
        case "$app" in *Arena*|*Avenue*)
            echo "APP: $app"
            /usr/libexec/PlistBuddy -c 'Print :CFBundleShortVersionString' "$app/Contents/Info.plist" 2>&1
            ;;
        esac
    done
    for product in 'Resolume Arena' 'Resolume Avenue'; do
        logfile="$HOME/Library/Logs/$product/$product log.txt"
        if [[ -f "$logfile" ]]; then
            echo
            echo "LOG: $logfile"
            echo 'Последние сведения о каталогах сканирования:'
            /usr/bin/grep -i 'Scanning directory for plugins' "$logfile" | /usr/bin/tail -n 20
            echo 'Строки FugScope и ближайший контекст:'
            /usr/bin/grep -inE -C 2 'Fug[ _-]*Scope|FSAR' "$logfile" | /usr/bin/tail -n 100
        else
            echo "Нет журнала по стандартному пути: $logfile"
        fi
    done
    echo
    echo 'Если журнал не найден: Resolume → Preferences → Feedback → View Log; найдите FugScope.'
    echo 'Отчёт не выполняет рендеринг и не подтверждает загрузку плагина в Resolume.'
} > "$report" 2>&1
cat "$report"
echo
echo "Отчёт сохранён: $report"
echo 'Пришлите этот отчёт в текущий чат для определения причины.'
if [[ -t 0 ]]; then
    read -r -p 'Нажмите Enter, чтобы закрыть окно…' || true
fi
