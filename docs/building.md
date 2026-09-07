# Сборка из исходников

Обычным пользователям сборка не нужна: готовые архивы находятся в [Releases](https://github.com/vloneonme/fugscope-resolume-arm/releases/latest).

## macOS

Требуются Apple Silicon, Resolume 7 с поддержкой ARM-плагинов и Xcode Command Line Tools (`xcode-select --install`). Пользователь подтвердил работу в Resolume 7.16.

```bash
bash scripts/build-macos.sh arm64
bash scripts/install-macos.sh arm64
```

Можно запустить `Build and Install.command`. Для Intel и Apple Silicon вместе используйте `universal` вместо `arm64`. Добавьте папку установки в Preferences → Video → FFGL Plugin Directories, перезапустите Resolume и найдите **FugScope ARM** в **Sources**.

## Windows x64

Требуются Windows 10/11 x64, 64-битный Resolume 7 и видеодрайвер с OpenGL 4.1. Плагин использует WASAPI и системный аудиовход по умолчанию. PortAudio, GLEW и C++ runtime встроены в DLL.

Для сборки из исходников установите Visual Studio 2022 Build Tools с **Desktop development with C++**, Windows SDK и CMake 3.20+. Выполните в PowerShell из корня проекта:

```powershell
./scripts/build-windows.ps1
./scripts/install-windows.ps1
```

Результат — `dist/FugScope-windows-x64.zip`. Сборка включает тесты алгоритмов, аудиологики и загрузки DLL; сборщик останавливается при их ошибке. Если скрипты заблокированы политикой PowerShell, можно собрать через CMake вручную, без изменения политики:

```powershell
cmake -S . -B build/windows-x64 -G "Visual Studio 17 2022" -A x64
cmake --build build/windows-x64 --config Release --parallel
ctest --test-dir build/windows-x64 -C Release --output-on-failure
```

DLL будет в `build/windows-x64/Release/FugScope.dll`. Установите её вручную по шагам выше.
