# Авторство и лицензии

Это неофициальный перенос **fugScopeGL** на FFGL 2 для macOS Apple Silicon.
Автор оригинала — **Alex May / bigfug**, © 2015, https://www.bigfug.com/.
Исходники: https://github.com/bigfug/Freeframe, commit
`cbecd9e4dd0fa148a2cb33ebbd19822bde15c247` (получен 2026-09-06).
Оригинальный репозиторий выпущен под GPL-3.0; полный текст сохранён в `LICENSE`.
Перенос и его изменения распространяются под GPL-3.0-only, без гарантий.
При распространении бинарника необходимо предоставлять соответствующие исходники
и сохранять авторство и лицензии. Название и ID отдельные: FugScope ARM / FSAR.
Проект не является официальным релизом bigfug или Resolume.

В `reference/` сохранены исходные `Plugin.h`, `Plugin.cpp` и `Instance.cpp` для
сопоставления поведения. Они не компилируются. Сохранены режимы, порядок первых
четырёх параметров, signed-max выборка 1024 mono samples и логика зеркалирования.
Переписаны FFGL-интеграция, OpenGL-отрисовка и обмен аудио между потоками.
Исправлены неполная обработка нечётной ширины, чтение общего аудиобуфера без
синхронизации, null input и нестандартные размеры callback. Толстые линии и
заливки реализованы треугольниками; пиксельная идентичность старому GL не заявлена.
Добавлен параметр Test Signal. Старые композиции автоматически не мигрируют.

## Включённые зависимости

- Resolume FFGL SDK: https://github.com/resolume/ffgl, commit
  `46758d72dd1e2bfbc29a44bc7b4d033e51a35348`, получен 2026-09-06.
  Копия `source/lib` без изменений. BSD-3-Clause, см. `licenses/FFGL-BSD-3-Clause.txt`
  и уведомления в отдельных файлах SDK.
- PortAudio v19.7.0: https://github.com/PortAudio/portaudio/releases/tag/v19.7.0.
  Commit `147dd722548358763a8b649b3e4b41dfffbcfbb6`.
  Включены неизменённые `include`, `src/common`, `src/os/unix`, `src/hostapi/coreaudio`.
  MIT, см. `licenses/PortAudio-MIT.txt` и уведомления в отдельных файлах.
  В macOS-бинарник компилируется статически только CoreAudio backend.

SHA-256 включённых файлов: `vendor/SHA256SUMS` (пути от корня проекта).
SHA-256 исходных архивов:

```text
b8aee302f5bb2cd371d5fbd1c6c97aaee3f088ba9eb993bd36fac387e20a1371  bigfug-Freeframe.tar.gz
cea452dd9dcb7559e7039d384317b5946495cb04a4c182155aaf8e5fafd16805  resolume-ffgl.tar.gz
5af29ba58bbdbb7bbcefaaecc77ec8fc413f0db6f4c4e286c40c3e1b83174fa0  portaudio-v19.7.0.tar.gz
```
