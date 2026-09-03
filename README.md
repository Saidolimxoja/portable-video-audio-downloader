# Downloader by Saidolimxoja

**Free, open-source, portable video downloader** built with Qt 6.
Downloads from YouTube, Instagram, TikTok, Rutube, VK Video and [1000+ other sites](https://github.com/yt-dlp/yt-dlp/blob/master/supportedsites.md).

**Бесплатный, портативный загрузчик видео с открытым исходным кодом** на Qt 6.
Скачивает с YouTube, Instagram, TikTok, Rutube, VK Видео и [1000+ других сайтов](https://github.com/yt-dlp/yt-dlp/blob/master/supportedsites.md).

Everything is bundled inside a single `.exe` — no installation required.
Всё вшито в один `.exe` — установка не нужна.

---

## ⬇️ Download / Скачать

**[Download Latest Release / Скачать последнюю версию (v2.0.0)](https://github.com/Saidolimxoja/Downloader-QT/releases/latest)**

> Just unzip and run `SaidDownloader.exe` — no installer, no setup, completely portable.
> Просто распакуйте и запустите `SaidDownloader.exe` — без установщика, полностью портативно.

| File / Файл | Size / Размер |
|---|---|
| `SaidDownloader_v2.0.zip` | ~66 MB |
| Unpacked folder / Распакованная папка | ~96 MB |

---

## ✨ Features / Возможности

- 🎬 **Video + Audio / Видео + Аудио** — download in MP4 with quality selection (360p–4K) / скачивание в MP4 с выбором качества
- 🎵 **Audio only / Только аудио** — extracts MP3 at best quality / извлечение MP3 в лучшем качестве
- 🔍 **Video analysis / Разбор ролика** — shows title, channel, duration, thumbnail / показывает название, канал, длительность, превью
- 📋 **Paste & Go / Вставить и вперёд** — paste from clipboard with one click / вставка из буфера одним кликом
- 🖱️ **Drag & Drop / Перетаскивание** — drop a link onto the window / перетащите ссылку на окно
- 🍪 **Smart cookies / Умные куки** — auto-tries Chrome, Edge, Firefox, Brave, Opera for private content / автоперебор браузеров для закрытого контента
- 🔄 **Self-updating / Самообновление** — update yt-dlp parsers from within the app / обновление парсеров yt-dlp из приложения
- 🌐 **Bilingual UI / Двуязычный интерфейс** — switch between English and Russian in one click / переключение EN ↔ RU одной кнопкой
- ⚡ **Fast downloads / Быстрое скачивание** — 16 concurrent fragments for HLS/DASH streams / 16 параллельных фрагментов
- 🛡️ **Portable / Портативность** — no installation, no registry, no admin rights needed / без установки, реестра и прав администратора

---

## 🖼️ How It Works / Как это работает

1. **Paste a link** (or drag & drop it) and click **Analyze**
   **Вставьте ссылку** (или перетащите) и нажмите **Разобрать**
2. Choose **Video + Audio** or **Audio only**, select quality
   Выберите **Видео + Аудио** или **Только аудио**, укажите качество
3. Click **Download** — progress bar shows speed and ETA
   Нажмите **Скачать** — прогресс-бар покажет скорость и оставшееся время
4. **Open file** or **Show in folder** when done
   По завершении — **Открыть файл** или **Показать в папке**

---

## 🏗️ Architecture / Архитектура

The app is a Qt 6 / C++17 application that wraps `yt-dlp` and `ffmpeg`.
Приложение на Qt 6 / C++17, обёртка над `yt-dlp` и `ffmpeg`.

1. On first run, `yt-dlp.exe` and `ffmpeg.exe` are extracted from embedded Qt resources into `%TEMP%\Downloader_by_Saidolimxoja_<PID>/`
   При первом запуске `yt-dlp.exe` и `ffmpeg.exe` извлекаются из вшитых ресурсов Qt в `%TEMP%\Downloader_by_Saidolimxoja_<PID>/`
2. `yt-dlp` is launched as a hidden subprocess (no console window)
   `yt-dlp` запускается скрытым подпроцессом (без окна консоли)
3. **Analysis**: `--dump-single-json` returns video metadata as JSON
   **Разбор**: `--dump-single-json` возвращает метаданные ролика в JSON
4. **Download**: output is parsed line-by-line with regex for progress, speed, ETA
   **Скачивание**: вывод парсится построчно регулярками — прогресс, скорость, ETA
5. On exit, the temp folder and all child processes are cleaned up
   При выходе временная папка и все дочерние процессы удаляются

---

## 📝 Download Formats / Форматы скачивания

| Mode / Режим | yt-dlp arguments / аргументы |
|---|---|
| Video + Audio, quality N / Видео + аудио, качество N | `-f "bv*[height<=N]+ba/b[height<=N]" --merge-output-format mp4` |
| Video + Audio, best / Видео + аудио, лучшее | `-f "bv*+ba/b" --merge-output-format mp4` |
| Audio only / Только аудио | `-x --audio-format mp3 --audio-quality 0` |

---

## 🌐 Localization / Локализация

The UI supports **Russian** (default) and **English**. Click the 🌐 button in the header to switch.
Интерфейс поддерживает **русский** (по умолчанию) и **английский**. Кнопка 🌐 в шапке переключает язык.

The translation system uses Qt's built-in `QTranslator`:
Система переводов использует встроенный `QTranslator`:

- Source strings are in Russian (in the code via `tr()`) / Исходные строки на русском
- English translations are in `translations/app_en.ts` / Английский перевод в `translations/app_en.ts`
- Compiled `.qm` file is embedded in resources / Скомпилированный `.qm` вшит в ресурсы

**To add a new language / Добавление нового языка:**
1. Copy / Скопируйте `translations/app_en.ts` → `translations/app_XX.ts`
2. Translate the strings / Переведите строки
3. Run / Запустите `lrelease translations/app_XX.ts`
4. Add to `resources.qrc` and load in `loadSettings()` / Добавьте в `resources.qrc` и загрузите в `loadSettings()`

---

## 🔧 Building from Source / Сборка из исходников

### Prerequisites / Требования

- **Qt 6.x** (tested with / проверено на 6.5.3)
- **MSVC 2019/2022 x64** or / или **MinGW x64**
- **yt-dlp.exe** and / и **ffmpeg.exe** in / в `tools/yt-dlp_win/`

### Build Steps / Шаги сборки

```bash
# Qt Creator: Open SaidDownloader.pro → Kit: Qt 6.x MSVC/MinGW x64 → Release
# Qt Creator: Откройте SaidDownloader.pro → Комплект: Qt 6.x MSVC/MinGW x64 → Release

# From command line (MinGW) / Из командной строки (MinGW):
mkdir build && cd build
C:\Qt\6.5.3\mingw_64\bin\qmake.exe ..\SaidDownloader.pro
C:\Qt\Tools\mingw1120_64\bin\mingw32-make.exe

# Compile translations / Компиляция переводов:
C:\Qt\6.5.3\mingw_64\bin\lrelease.exe translations\app_en.ts
```

### Why `CONFIG += resources_big`? / Зачем `resources_big`?

The binary embeds ~176 MB of tools. Without `resources_big`, `rcc` generates a massive C++ array that crashes the compiler. This flag uses a two-pass object file approach instead.

В бинарник вшито ~176 МБ утилит. Без `resources_big` rcc генерирует гигантский C++-массив, на котором компилятор задыхается. Этот флаг использует двухпроходную генерацию объектного файла.

### Resource compression / Сжатие ресурсов

```qmake
QMAKE_RESOURCE_FLAGS += -compress 9 -threshold 5
```

Measured / Замерено: ffmpeg.exe 139 MB → 53 MB in the final binary / в итоговом бинарнике.

---

## 📦 Distribution / Дистрибуция

The built `.exe` requires Qt DLLs alongside it:
Собранный `.exe` требует DLL рядом с собой:

```
dist/
├── SaidDownloader.exe         55.6 MB
├── Qt6Core.dll                 6.5 MB
├── Qt6Gui.dll                 10.0 MB
├── Qt6Widgets.dll              6.6 MB
├── libgcc_s_seh-1.dll          0.1 MB
├── libstdc++-6.dll             2.0 MB
├── libwinpthread-1.dll         0.1 MB
├── platforms/qwindows.dll      1.0 MB
└── styles/qwindowsvistastyle.dll 0.2 MB
```

The entire folder is portable — just copy it to any Windows machine.
Всю папку можно носить целиком — на чужой машине ничего доустанавливать не нужно.

### Single-file packaging / Упаковка в один файл

**Option 1 — Static Qt build** (recommended): Link Qt statically so the `.exe` has zero dependencies. Takes 2–3 hours to build Qt from source.

**Вариант 1 — Статическая сборка Qt** (рекомендуется): Слинковать Qt статически, чтобы `.exe` не зависел ни от чего. Занимает 2–3 часа на сборку Qt из исходников.

**Option 2 — Enigma Virtual Box**: Pack DLLs inside the `.exe` at the cost of potential antivirus false positives.

**Вариант 2 — Enigma Virtual Box**: Упаковать DLL внутрь `.exe` ценой возможных ложных срабатываний антивируса.

---

## ⚠️ Known Limitations / Известные ограничения

- **Windows only / Только Windows** — uses Windows-specific APIs (`taskkill`, `DwmSetWindowAttribute`) / использует API Windows
- Tools are extracted to `%TEMP%` — antivirus may flag newly extracted `.exe` files / Утилиты извлекаются в `%TEMP%` — антивирус может заблокировать
- Extraction runs in the UI thread (with progress) — may feel slow on HDDs / Распаковка идёт в UI-потоке — на HDD может быть медленно
- The `--update` command updates yt-dlp in the temp folder; the update is lost on app restart / Обновление yt-dlp теряется при перезапуске

---

## 📄 License / Лицензия

This project is **open source**. You are free to use, modify, and distribute it.
Этот проект с **открытым исходным кодом**. Вы можете свободно использовать, изменять и распространять его.

---

## 🤝 Contributing / Вклад в проект

Contributions are welcome! / Вклад приветствуется!

- Open issues for bugs or feature requests / Создавайте issues для багов и предложений
- Submit pull requests / Отправляйте pull requests
- Fork and customize for your needs / Форкайте и настраивайте под себя

---

## 📱 Contact / Контакты

Created by **Saidolimxoja** — [Telegram](https://t.me/Saidolimxoja)
