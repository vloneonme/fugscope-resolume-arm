# Проверки — 2026-09-06

## Выполнено

Рабочая среда: Linux x86_64, g++, Mesa llvmpipe, EGL surfaceless.

| Проверка | Результат |
|---|---|
| Production C++ + полный FFGL SDK | Linux shared library успешно собрана |
| Signed-max downsampling и Scale | PASS, известные входы и ожидаемые результаты |
| Три Arrange | PASS, включая 1 px и нечётную ширину |
| 7 режимов × 3 Arrange × граничные ширины × Width | PASS, конечная геометрия и корректные размеры |
| NaN input, null callback, блоки 2500 + 572 samples | PASS |
| Переполнение и конкурентный оборот SPSC-очереди | PASS, 20000 кадров без разрыва данных |
| Аудиозахват с моделью драйвера | PASS: первый канал, callback, refcount двух экземпляров, повторный start/stop |
| Ошибки init/open/start, отсутствие входа, fallback 48000 Hz | PASS, очистка ресурсов |
| ASan + UBSan | PASS для собственного core/audio кода |
| FFGL через настоящий plugMain | PASS: metadata, ID, source, параметры и dropdown элементы |
| GLSL 4.10 в OpenGL 4.1 Core | PASS, все 21 комбинации реально отрисованы |
| Прозрачный фон и белая волна | PASS, проверены пиксели каждого режима |
| Viewport с offset, сохранность пикселей вне viewport | PASS |
| Восстановление draw/read FBO, viewport, scissor, blend, depth, cull, color mask | PASS |
| Resize через FFGL до 1×1, 2×2, 3×3 и 127×127 | PASS |
| Отсутствие аудиодрайвера и уничтожение FFGL экземпляра | PASS |
| Shell syntax / Info.plist | PASS: bash -n / Python plistlib |
| Визуальный просмотр 21 сочетания | PASS, `preview.png` — реальные тестовые кадры |

Команды воспроизведения:

```bash
bash scripts/test-core.sh
# В sandbox с ptrace (как при этой проверке):
ASAN_OPTIONS=detect_leaks=0 bash scripts/test-core.sh
# Linux, с GLEW/EGL headers и библиотеками в локальном sysroot:
GL_TEST_SYSROOT=/absolute/path/to/sysroot bash scripts/test-linux-gl.sh
```

LeakSanitizer в этой среде отказывается работать под ptrace, поэтому **проверка
утечек отключена**; AddressSanitizer и UndefinedBehaviorSanitizer остались включены.
Отдельный ThreadSanitizer не запускался. Аудиотест использует модель драйвера;
Linux OpenGL host линкуется с PortAudio без аппаратного backend. Это не тест
микрофона, CoreAudio или Resolume. Ограничение OpenGL 4.1 задано через
`MESA_GL_VERSION_OVERRIDE=4.1` и `MESA_GLSL_VERSION_OVERRIDE=410`.

## Осталось выполнить на Mac

1. `bash scripts/build-macos.sh arm64`. Сохранить вывод Apple clang,
   `file`, `lipo -verify_arch`, `codesign --verify`. При ошибках SDK исправить
   их, обновить документацию и повторить затронутые тесты.
2. Установить в Resolume 7.11+ нативного ARM64 процесса. Убедиться, что источник
   виден в Sources как FugScope ARM; отдельно проверить повторный запуск хоста.
3. Включить Test Signal, проверить семь Config, три Arrange, Scale/Width,
   прозрачность на цветном слое, resize композиции, сохранение и открытие проекта.
4. Отключить Test Signal, разрешить микрофон хосту, проверить реальный системный
   аудиовход. Сравнить 44.1/48 kHz; отключить устройство и проверить восстановление
   после переключения Test Signal или повторного открытия клипа.
5. Проверить несколько одновременно активных экземпляров и закрытие одного,
   не прерывающее остальные; проверить завершение Resolume без зависания.
6. Сравнить внешнее поведение с оригиналом на известном тестовом аудио. Здесь
   установлено соответствие исходным формулам, но оригинальный бинарник не запускался.
7. При необходимости выполнить Universal build и проверить Intel/Rosetta.
8. После успешного теста приложить `.bundle` вместе с этим исходным проектом,
   зафиксировать версии macOS, Resolume, Xcode и обновить README/VALIDATION.

Статус: **готов исходный кандидат; macOS-бинарник не собран, совместимость с
конкретной версией Resolume на Apple Silicon пока не подтверждена**.
