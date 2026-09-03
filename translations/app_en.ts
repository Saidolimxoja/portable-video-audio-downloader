<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="en_US" sourcelanguage="ru_RU">

<!-- ================================================================
     MainWindow — programmatic tr() strings from mainwindow.cpp
     ================================================================ -->
<context>
    <name>MainWindow</name>

    <!-- Constructor / status -->
    <message>
        <source>Вставьте ссылку и нажмите «Разобрать»</source>
        <translation>Paste a link and click "Analyze"</translation>
    </message>

    <!-- extractResourceFile errors -->
    <message>
        <source>не читается ресурс %1</source>
        <translation>cannot read resource %1</translation>
    </message>
    <message>
        <source>не создать папку %1</source>
        <translation>cannot create folder %1</translation>
    </message>
    <message>
        <source>не записать %1: %2</source>
        <translation>cannot write %1: %2</translation>
    </message>
    <message>
        <source>обрыв записи в %1 (нет места на диске?)</source>
        <translation>write interrupted for %1 (disk full?)</translation>
    </message>
    <message>
        <source>не сбросить буфер в %1</source>
        <translation>cannot flush buffer to %1</translation>
    </message>

    <!-- ensureToolsExtracted -->
    <message>
        <source>Ошибка сборки</source>
        <translation>Build Error</translation>
    </message>
    <message>
        <source>В .exe не вшиты утилиты — ресурс %1 пуст.

Проверьте, что resources.qrc подключён в .pro и что файлы
tools/yt-dlp_win/yt-dlp.exe и tools/yt-dlp_win/ffmpeg.exe
существовали на момент компиляции.</source>
        <translation>No tools embedded in .exe — resource %1 is empty.

Make sure resources.qrc is included in the .pro file and that
tools/yt-dlp_win/yt-dlp.exe and tools/yt-dlp_win/ffmpeg.exe
existed at compile time.</translation>
    </message>
    <message>
        <source>Ошибка</source>
        <translation>Error</translation>
    </message>
    <message>
        <source>Не удалось создать временную папку:
%1</source>
        <translation>Failed to create temporary folder:
%1</translation>
    </message>
    <message>
        <source>Ошибка распаковки</source>
        <translation>Extraction Error</translation>
    </message>
    <message>
        <source>Не удалось подготовить утилиты: %1</source>
        <translation>Failed to prepare tools: %1</translation>
    </message>
    <message>
        <source>Подготовка утилит... %1%</source>
        <translation>Preparing tools... %1%</translation>
    </message>

    <!-- Paste action -->
    <message>
        <source>Вставить из буфера обмена</source>
        <translation>Paste from clipboard</translation>
    </message>

    <!-- onAnalyzeClicked -->
    <message>
        <source>Вставьте ссылку на видео</source>
        <translation>Paste a video link</translation>
    </message>
    <message>
        <source>Это не похоже на ссылку. Нужен адрес вида https://...</source>
        <translation>This doesn't look like a link. Enter a URL like https://...</translation>
    </message>
    <message>
        <source>Не удалось подготовить утилиты</source>
        <translation>Failed to prepare tools</translation>
    </message>
    <message>
        <source>Разбираю ролик...</source>
        <translation>Analyzing video...</translation>
    </message>

    <!-- handleAnalyzeFinished -->
    <message>
        <source>Авторизация: пробую куки %1...</source>
        <translation>Authorization: trying %1 cookies...</translation>
    </message>
    <message>
        <source>Для просмотра этого видео требуется авторизация в браузере или файл cookies.txt</source>
        <translation>This video requires browser authorization or a cookies.txt file</translation>
    </message>
    <message>
        <source>Не удалось разобрать ответ yt-dlp. Проверьте ссылку.</source>
        <translation>Failed to parse yt-dlp response. Check the link.</translation>
    </message>
    <message>
        <source>Ошибка: %1</source>
        <translation>Error: %1</translation>
    </message>
    <message>
        <source>Без названия</source>
        <translation>Untitled</translation>
    </message>
    <message>
        <source>Лучшее доступное</source>
        <translation>Best available</translation>
    </message>
    <message>
        <source>превью...</source>
        <translation>loading...</translation>
    </message>
    <message>
        <source>Готово к скачиванию — выберите формат</source>
        <translation>Ready to download — choose a format</translation>
    </message>

    <!-- handleThumbnailFinished -->
    <message>
        <source>без превью</source>
        <translation>no preview</translation>
    </message>
    <message>
        <source>превью</source>
        <translation>preview</translation>
    </message>

    <!-- onDownloadClicked / handleDownloadFinished -->
    <message>
        <source>Подключение к серверу...</source>
        <translation>Connecting to server...</translation>
    </message>
    <message>
        <source>Готово: %1 — сохранено в папку %2.</source>
        <translation>Done: %1 — saved to %2.</translation>
    </message>
    <message>
        <source>Готово! Файл сохранён.</source>
        <translation>Done! File saved.</translation>
    </message>
    <message>
        <source>Файл не найден после скачивания. Возможно, на диске не хватает места.</source>
        <translation>File not found after download. You may be out of disk space.</translation>
    </message>
    <message>
        <source>Ошибка: yt-dlp завершился с кодом %1</source>
        <translation>Error: yt-dlp exited with code %1</translation>
    </message>

    <!-- handleOutputLine -->
    <message>
        <source>Скачивание %1%</source>
        <translation>Downloading %1%</translation>
    </message>
    <message>
        <source>, осталось %1</source>
        <translation>, %1 remaining</translation>
    </message>
    <message>
        <source>Склейка видео и звука через ffmpeg...</source>
        <translation>Merging video and audio with ffmpeg...</translation>
    </message>
    <message>
        <source>Извлечение звука в MP3...</source>
        <translation>Extracting audio to MP3...</translation>
    </message>
    <message>
        <source>Обработка файла через ffmpeg...</source>
        <translation>Processing file with ffmpeg...</translation>
    </message>

    <!-- onProcessErrorOccurred -->
    <message>
        <source>Не удалось запустить yt-dlp.exe (%1). Возможно, антивирус заблокировал распакованный файл.</source>
        <translation>Failed to start yt-dlp.exe (%1). Your antivirus may have blocked the extracted file.</translation>
    </message>

    <!-- UI actions -->
    <message>
        <source>Операция отменена пользователем.</source>
        <translation>Operation cancelled by user.</translation>
    </message>
    <message>
        <source>Буфер обмена пуст</source>
        <translation>Clipboard is empty</translation>
    </message>
    <message>
        <source>Выберите папку для сохранения</source>
        <translation>Choose download folder</translation>
    </message>
    <message>
        <source>Файл не найден на диске</source>
        <translation>File not found on disk</translation>
    </message>

    <!-- setBusy button text -->
    <message>
        <source>Разбираю...</source>
        <translation>Analyzing...</translation>
    </message>
    <message>
        <source>Разобрать</source>
        <translation>Analyze</translation>
    </message>
    <message>
        <source>Качаю...</source>
        <translation>Downloading...</translation>
    </message>
    <message>
        <source>Скачать</source>
        <translation>Download</translation>
    </message>

    <!-- cookies -->
    <message>
        <source>Выберите файл cookies.txt</source>
        <translation>Select cookies.txt file</translation>
    </message>
    <message>
        <source>Файлы cookies (*.txt);;Все файлы (*.*)</source>
        <translation>Cookie files (*.txt);;All files (*.*)</translation>
    </message>
    <message>
        <source>🍪 Куки: %1</source>
        <translation>🍪 Cookies: %1</translation>
    </message>
    <message>
        <source>Загружен файл куков %1. Повторный разбор...</source>
        <translation>Loaded cookie file %1. Re-analyzing...</translation>
    </message>

    <!-- update engine -->
    <message>
        <source>Проверка и обновление парсеров yt-dlp...</source>
        <translation>Checking for yt-dlp updates...</translation>
    </message>
    <message>
        <source>Парсеры yt-dlp успешно обновлены до последней версии!</source>
        <translation>yt-dlp has been updated to the latest version!</translation>
    </message>
    <message>
        <source>Не удалось обновить yt-dlp (код %1)</source>
        <translation>Failed to update yt-dlp (code %1)</translation>
    </message>
    <message>
        <source>Ошибка обновления: %1</source>
        <translation>Update error: %1</translation>
    </message>
</context>

<!-- ================================================================
     MainWindow — .ui strings from mainwindow.ui (retranslateUi)
     ================================================================ -->
<context>
    <name>MainWindow</name>

    <message>
        <source>Downloader by Saidolimxoja</source>
        <translation>Downloader by Saidolimxoja</translation>
    </message>
    <message>
        <source>Rutube · VK Видео · YouTube — сначала разбор ролика, потом выбор формата</source>
        <translation>Rutube · VK Video · YouTube — analyze the video first, then choose format</translation>
    </message>
    <message>
        <source>Вставьте ссылку на Instagram, YouTube, Rutube, VK Видео (или перетащите сюда)...</source>
        <translation>Paste an Instagram, YouTube, Rutube, VK Video link (or drag &amp; drop here)...</translation>
    </message>
    <message>
        <source>Название ролика</source>
        <translation>Video title</translation>
    </message>
    <message>
        <source>канал · длительность</source>
        <translation>channel · duration</translation>
    </message>
    <message>
        <source>Видео + аудио</source>
        <translation>Video + Audio</translation>
    </message>
    <message>
        <source>Только аудио</source>
        <translation>Audio only</translation>
    </message>
    <message>
        <source>Качество</source>
        <translation>Quality</translation>
    </message>
    <message>
        <source>Папка сохранения:</source>
        <translation>Save folder:</translation>
    </message>
    <message>
        <source>Обзор...</source>
        <translation>Browse...</translation>
    </message>
    <message>
        <source>Отмена</source>
        <translation>Cancel</translation>
    </message>
    <message>
        <source>Готов к работе</source>
        <translation>Ready</translation>
    </message>
    <message>
        <source>🍪 Указать cookies.txt...</source>
        <translation>🍪 Select cookies.txt...</translation>
    </message>
    <message>
        <source>Открыть файл</source>
        <translation>Open file</translation>
    </message>
    <message>
        <source>Показать в папке</source>
        <translation>Show in folder</translation>
    </message>
    <message>
        <source>Выберите папку и формат перед скачиванием</source>
        <translation>Choose a folder and format before downloading</translation>
    </message>
    <message>
        <source>🔄 Обновить парсеры</source>
        <translation>🔄 Update parsers</translation>
    </message>
    <message>
        <source>Created by Saidolimxoja</source>
        <translation>Created by Saidolimxoja</translation>
    </message>
</context>
</TS>
