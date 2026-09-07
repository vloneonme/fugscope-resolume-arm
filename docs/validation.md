# Проверки и состояние разработки — 2026-09-07

## Выполнено

- Пользователь подтвердил работу Mac-версии в Resolume 7.16. Агент не запускал macOS/Resolume; полная матрица Intel/ARM и аудиоустройств не проверена.
- После реорганизации повторно прошли ASan/UBSan core/audio tests: режимы, граничные размеры, очередь, конкурентные экземпляры, callback и ошибки драйвера. LeakSanitizer отключён из-за ptrace.
- Повторно прошли 21 сочетание Config × Arrange через настоящий FFGL и Mesa OpenGL 4.1 Core на Linux, метаданные, прозрачность, GL state, resize и teardown.
- Windows x64 Release DLL и три Windows test executables собраны MinGW-w64 GCC 13 / CMake 3.28.3. PE32+ x86-64, экспорты `plugMain` и `SetLogCallback` подтверждены.
- DLL зависит только от системных KERNEL32, msvcrt, ole32, OPENGL32, WINMM. PortAudio/WASAPI, GLEW и runtime MinGW статические.
- Проверены shell syntax, контрольные суммы vendored-файлов и Git diff. SDK содержит единственную правку регистра `Windows.h` → `windows.h`, описанную в NOTICE.
- Добавлены Windows MSVC CI с core/audio/LoadLibrary тестами и упаковкой DLL с исходниками, macOS Universal CI и Linux OpenGL CI.

## GitHub Actions

Для исходников коммита `cc1f662` успешно завершены:

- [Windows x64 MSVC, три теста и ZIP](https://github.com/vloneonme/fugscope-resolume-arm/actions/runs/34111615311).
- [macOS Universal и core/audio](https://github.com/vloneonme/fugscope-resolume-arm/actions/runs/34111615397).
- [Linux ASan/UBSan и OpenGL](https://github.com/vloneonme/fugscope-resolume-arm/actions/runs/34111615295).

Windows-артефакт содержит DLL/установщик/лицензии и отдельный ZIP соответствующих исходников. Бинарники и локальные архивы в Git не добавлены.

## Ограничения

Windows executables в Linux не запущены: локальный Wine упёрся в недоступный runtime-каталог и ограничения сокетов sandbox. Это ограничение среды, не результат теста плагина. Нативная сборка MSVC, PowerShell build/package, core/audio/LoadLibrary тесты и загрузка артефактов успешно прошли в Windows CI.

Реальная отрисовка, WASAPI-микрофон, несколько экземпляров и повторное открытие композиции в Windows Resolume ещё требуют ручной проверки. Linux audio-test использует модель драйвера. Universal Intel отдельно не подтверждён.

## Структура и решения

Исходники перенесены в корень репозитория. Полные upstream-копии, старые архивы, результаты и инструменты исключены из текущего дерева Git и сохранены в локальной `.local/`; прошлые коммиты не переписывались. Сохранены Mac renderer, параметры и ID FSAR. Windows имя — FugScope; Mac — FugScope ARM. CMake ограничен Windows x64, Mac использует прежний xcrun build.

## Следующий шаг

Проверить результат GitHub Actions, затем Windows DLL в Resolume: Test Signal, все режимы, системный ввод, несколько экземпляров и сохранение/открытие композиции. При ошибке сохранить версию Windows/Resolume, название устройства и текст загрузчика; исправлять по фактическому логу.
