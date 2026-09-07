# FugScope for Resolume 7

Неофициальный порт аудиовизуализатора **fugScopeGL** Alex May / bigfug на FFGL 2.2 и OpenGL 4.1 Core. Семь режимов осциллограммы, три варианта расположения, прозрачный фон и встроенный тестовый сигнал.

![Режимы FugScope](docs/preview.png)

## macOS

Требуются Apple Silicon, Resolume 7 с поддержкой ARM-плагинов и Xcode Command Line Tools (`xcode-select --install`). Пользователь подтвердил работу в Resolume 7.16.

```bash
bash scripts/build-macos.sh arm64
bash scripts/install-macos.sh arm64
```

Можно запустить `Build and Install.command`. Для Intel и Apple Silicon вместе используйте `universal` вместо `arm64`. Добавьте папку установки в Preferences → Video → FFGL Plugin Directories, перезапустите Resolume и найдите **FugScope ARM** в **Sources**.

## Windows x64

Требуются Windows 10/11 x64, 64-битный Resolume 7 и видеодрайвер с OpenGL 4.1. Плагин использует WASAPI и системный аудиовход по умолчанию. PortAudio, GLEW и C++ runtime встроены в DLL.

Готовая сборка: откройте [GitHub Actions](https://github.com/vloneonme/fugscope-resolume-arm/actions/workflows/windows.yml), выберите успешный запуск и скачайте артефакт **FugScope-Windows-x64**. Внутри находятся архив плагина и соответствующие исходники.

1. Закройте Resolume и распакуйте `FugScope-windows-x64.zip`.
2. Скопируйте `FugScope.dll` в `Documents\Resolume\Extra Effects` либо выполните `install-windows.ps1` из распакованного архива. Скрипт сохраняет предыдущую DLL в `Documents\FugScope Backups`.
3. Добавьте каталог в Preferences → Video → FFGL Plugin Directories и перезапустите Resolume.
4. Найдите **FugScope** в **Sources**, поместите в клип и включите **Test Signal**.

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

## Использование

Перетащите источник в клип. Включите **Test Signal**, чтобы проверить изображение без микрофона. Отключите его для захвата первого канала системного устройства ввода по умолчанию; разрешите Resolume доступ к микрофону. Аудио не берётся автоматически из композиции или системного выхода: для этого нужен настроенный виртуальный аудиовход.

- **Config** — семь режимов отображения.
- **Arrange** — три варианта расположения.
- **Scale** — усиление амплитуды от 0 до 10.
- **Width** — толщина от 1 до 50.

ID плагина — `FSAR`; он не заменяет оригинальный `BF24`. Старые композиции оригинала не мигрируют автоматически.

## Структура и разработка

`src/` — реализация; `vendor/` — необходимые исходники зависимостей; `scripts/` — сборка и установка; `tests/` — проверки; `resources/` — метаданные; `reference/` — исходные алгоритмы для сравнения; `docs/` — документация.

```bash
bash scripts/test-core.sh
```

На Linux графические проверки запускаются через `scripts/test-linux-gl.sh` с GLEW/EGL development libraries. Сборки создаются в `build/`, архивы — в `dist/`; эти каталоги не входят в Git.

## Статус

Mac-версия работает по подтверждению пользователя. Windows x64 DLL собрана MinGW и нативно MSVC в GitHub Actions; core/audio/LoadLibrary тесты прошли. macOS Universal и Linux OpenGL CI также прошли. Проверка изображения и реального аудиовхода в Windows Resolume ещё требуется. Контекст и границы проверок: [docs/validation.md](docs/validation.md). Следующий шаг проверки — запуск Windows-источника в Resolume с Test Signal и реальным аудиовходом.

## Лицензия

GPL-3.0-only. Автор оригинала — Alex May / bigfug. Это неофициальный проект, не релиз bigfug или Resolume. При распространении бинарников предоставляйте соответствующие исходники. Версии и лицензии зависимостей перечислены в [NOTICE.md](NOTICE.md).
