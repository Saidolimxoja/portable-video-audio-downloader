#-------------------------------------------------
# Said Downloader — портативный монолитный .exe
#
# Сборка:  Qt 6.x  +  MSVC 2019/2022 x64  (release)
#          Qt Creator -> Открыть проект -> SaidDownloader.pro
#
# ВАЖНО ПРО РАЗМЕР:
#   В .exe вшивается ~176 МБ бинарников (ffmpeg + yt-dlp).
#   CONFIG += resources_big обязателен: он заставляет rcc
#   генерировать объектный файл напрямую (режим -pass 1/-pass 2),
#   иначе компилятор задохнётся на гигантском .cpp-массиве.
#-------------------------------------------------

QT       += core gui widgets

CONFIG   += c++17
CONFIG   += resources_big          # <-- критично для огромного .qrc

#-------------------------------------------------
# СЖАТИЕ РЕСУРСОВ — без этих флагов .exe весит на 100 МБ больше.
#
# rcc по умолчанию идёт с -threshold 70: файл сжимается, только если
# экономия БОЛЬШЕ 70%. ffmpeg.exe жмётся до 40% (экономия 60%) — и молча
# остаётся сырым. Опускаем порог до 5% и выкручиваем уровень zlib.
#
# Замерено на ffmpeg.exe: 139.1 МБ -> 53.3 МБ.
#
# Распаковка прозрачна: QFile(":/bin/...") отдаёт уже распакованные данные,
# в коде менять нечего. Плата — сборка идёт дольше, и при извлечении
# QResource разжимает файл целиком в память.
#-------------------------------------------------
QMAKE_RESOURCE_FLAGS += -compress 9 -threshold 5

TARGET    = SaidDownloader
TEMPLATE  = app

# Сборка готового .exe сразу в папку dist, где лежат все библиотеки DLL
DESTDIR   = $$PWD/dist

#-------------------------------------------------
# ВЕРСИЯ ПРИЛОЖЕНИЯ — меняется ровно в одном месте.
#
# Отсюда она разъезжается сразу в три места:
#   1) свойства .exe в проводнике (ПКМ -> Свойства -> Подробно),
#      qmake сам сгенерирует ресурс VERSIONINFO;
#   2) макрос APP_VERSION в C++;
#   3) плашка «v1.0.0» в шапке окна.
#-------------------------------------------------
VERSION = 2.0.0

# ВНИМАНИЕ: только латиница. Эти строки qmake кладёт в сгенерированный .rc,
# а windres/rc.exe компилируют его не в UTF-8 — кириллица превращается
# в «Р СџР С•РЎР‚...» прямо в свойствах файла. Проверено.
QMAKE_TARGET_PRODUCT     = Downloader by Saidolimxoja
QMAKE_TARGET_DESCRIPTION = Portable video downloader
QMAKE_TARGET_COMPANY     = Saidolimxoja
QMAKE_TARGET_COPYRIGHT   = Copyright (c) 2026 Saidolimxoja

# Пробрасываем версию в код. Тройное экранирование — чтобы кавычки пережили
# и qmake, и make, и командную строку компилятора.
DEFINES += APP_VERSION=\\\"$$VERSION\\\"

# Тёмный заголовок окна через DwmSetWindowAttribute
win32: LIBS += -ldwmapi

# Кириллица в исходниках: заставляем MSVC читать файлы как UTF-8
msvc {
    QMAKE_CXXFLAGS += /utf-8
    # Огромные объектные файлы от rcc
    QMAKE_CXXFLAGS += /bigobj
}

SOURCES += \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    mainwindow.h

FORMS += \
    mainwindow.ui

RESOURCES += \
    resources.qrc

# Не тащим deprecated-API
DEFINES += QT_DEPRECATED_WARNINGS

# Иконка приложения (вшивается в свойства и ярлык .exe на Windows)
RC_ICONS = app.ico
