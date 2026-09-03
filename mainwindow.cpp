#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QClipboard>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QDirIterator>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QIcon>
#include <QMimeData>
#include <QMessageBox>
#include <QPainter>
#include <QPixmap>
#include <QPolygonF>
#include <QRegularExpression>
#include <QSettings>
#include <QStandardPaths>
#include <QStorageInfo>
#include <QThread>
#include <QTranslator>
#include <QUrl>
#include <QVector>

#ifdef Q_OS_WIN
#  include <qt_windows.h>   // CREATE_NO_WINDOW, STARTF_USESHOWWINDOW, SW_HIDE
#  include <dwmapi.h>       // DwmSetWindowAttribute — тёмный заголовок окна
#endif

namespace {

// =====================================================================
//  ССЫЛКА НА TELEGRAM — меняется здесь, в одной строке.
// =====================================================================
const QString kTelegramUrl = QStringLiteral("https://t.me/Saidolimxoja");

/// Корневой префикс ресурсов, куда вшиты утилиты (см. resources.qrc).
const QString kResourceRoot = QStringLiteral(":/bin");

/// Имя временной папки внутри %TEMP%.
const QString kWorkDirName = QStringLiteral("Downloader_by_Saidolimxoja");

/// Размер порции при потоковом извлечении.
/// ffmpeg весит ~145 МБ, читать его целиком через readAll() — это 145 МБ в RAM
/// и заметный фриз окна, поэтому льём кусками.
constexpr qint64 kChunkSize = 4 * 1024 * 1024;   // 4 МиБ

/// Число потоков разгона. Держим в одном месте, чтобы значения в разных
/// аргументах yt-dlp не разъехались.
const QString kThreads = QStringLiteral("16");

// --- Регулярки для разбора вывода yt-dlp --------------------------------
// Типичная строка прогресса:
//   [download]  50.6% of ~  1.20GiB at   24.36MiB/s ETA 00:42
const QRegularExpression kRePercent(QStringLiteral(R"(\[download\]\s+(\d{1,3}(?:[.,]\d+)?)%)"));
const QRegularExpression kReSpeed(QStringLiteral(R"(\bat\s+~?\s*([\d.]+\s*(?:[KMGT]i?)?B/s|Unknown\s+B/s))"));
const QRegularExpression kReEta(QStringLiteral(R"(\bETA\s+([\d:]+))"));
const QRegularExpression kReDest(QStringLiteral(R"(Destination:\s*(.+)$)"));

/// Рисует круглую иконку Telegram — синий кружок с белым самолётиком.
///
/// Рисуем вручную, а не берём готовый .svg: QIcon из SVG требует модуль
/// QtSvg и плагин qsvg.dll рядом с приложением, а это лишняя зависимость
/// в папке. Символ ✈ из шрифта тоже не годится — в Segoe UI он рисуется
/// невнятной чёрточкой.
QPixmap makeTelegramIcon(int size, qreal dpr)
{
    const int px = int(size * dpr);
    QPixmap pm(px, px);
    pm.fill(Qt::transparent);
    pm.setDevicePixelRatio(dpr);

    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setPen(Qt::NoPen);

    // Кружок фирменного синего.
    p.setBrush(QColor(0x22, 0x9E, 0xD9));
    p.drawEllipse(0, 0, px, px);

    // Самолётик в долях от размера — чтобы иконка масштабировалась.
    const auto pt = [px](qreal x, qreal y) { return QPointF(x * px, y * px); };

    const QPolygonF body {
        pt(0.17, 0.49), pt(0.83, 0.22), pt(0.68, 0.79), pt(0.46, 0.61)
    };
    const QPolygonF flap {
        pt(0.46, 0.61), pt(0.39, 0.77), pt(0.37, 0.56)
    };

    p.setBrush(Qt::white);
    p.drawPolygon(body);
    // Загнутый уголок — чуть прозрачнее, иначе самолётик читается как клякса.
    p.setBrush(QColor(255, 255, 255, 170));
    p.drawPolygon(flap);

    return pm;
}

/// Рисует аккуратную иконку вставки из буфера (планшет с листом бумаги).
QPixmap makePasteIcon(int size, qreal dpr)
{
    const int px = int(size * dpr);
    QPixmap pm(px, px);
    pm.fill(Qt::transparent);
    pm.setDevicePixelRatio(dpr);

    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);

    // Рамка планшета
    p.setPen(QPen(QColor(0x3B, 0x82, 0xF6), qMax(1.5 * dpr, 1.0)));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(QRectF(px * 0.22, px * 0.25, px * 0.56, px * 0.65), px * 0.08, px * 0.08);

    // Верхняя защёлка
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0x3B, 0x82, 0xF6));
    p.drawRoundedRect(QRectF(px * 0.35, px * 0.15, px * 0.30, px * 0.18), px * 0.04, px * 0.04);

    return pm;
}

/// Рисует аккуратную векторную иконку глобуса для переключения языка.
QPixmap makeGlobeIcon(int size, qreal dpr)
{
    const int px = int(size * dpr);
    QPixmap pm(px, px);
    pm.fill(Qt::transparent);
    pm.setDevicePixelRatio(dpr);

    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);

    QPen pen(QColor(0x64, 0x74, 0x8B), qMax(1.2 * dpr, 1.0));
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);

    // Внешний круг
    const QRectF r(px * 0.12, px * 0.12, px * 0.76, px * 0.76);
    p.drawEllipse(r);

    // Экватор (горизонтальная линия)
    p.drawLine(QPointF(px * 0.12, px * 0.50), QPointF(px * 0.88, px * 0.50));

    // Меридиан (вертикальный эллипс)
    p.drawEllipse(QRectF(px * 0.30, px * 0.12, px * 0.40, px * 0.76));

    return pm;
}

#ifdef Q_OS_WIN
/// Красит системный заголовок окна в тон приложения.
void applyLightTitleBar(HWND hwnd)
{
    if (!hwnd)
        return;

    const BOOL useDark = FALSE;
    // Номер атрибута менялся: 19 в ранних сборках Windows 10,
    // 20 (DWMWA_USE_IMMERSIVE_DARK_MODE) начиная с 20H1 и в Windows 11.
    DwmSetWindowAttribute(hwnd, 20, &useDark, sizeof(useDark));
    DwmSetWindowAttribute(hwnd, 19, &useDark, sizeof(useDark));
}
#endif

} // namespace


// =======================================================================
//  Конструктор / деструктор
// =======================================================================

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setAcceptDrops(true);

    // Плашка версии в шапке. Значение приходит из .pro через APP_VERSION.
    ui->versionChip->setText(QStringLiteral("v%1").arg(QCoreApplication::applicationVersion()));

    // Иконка Telegram рисуется в рантайме.
    ui->telegramButton->setIcon(QIcon(makeTelegramIcon(16, devicePixelRatioF())));
    ui->telegramButton->setIconSize(QSize(16, 16));

    // Иконка переключения языка (глобус)
    ui->languageButton->setIcon(QIcon(makeGlobeIcon(14, devicePixelRatioF())));
    ui->languageButton->setIconSize(QSize(14, 14));

    ui->infoCard->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    ui->videoRow->setAlignment(ui->thumbLabel, Qt::AlignTop);

#ifdef Q_OS_WIN
    applyLightTitleBar(reinterpret_cast<HWND>(winId()));
#endif

    // Изоляция %TEMP%/said_downloader_<PID> для исключения конфликтов параллельного запуска
    m_workDir = QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation))
                    .filePath(QStringLiteral("%1_%2").arg(kWorkDirName).arg(QCoreApplication::applicationPid()));
    m_ytDlpPath  = QDir(m_workDir).filePath(QStringLiteral("yt-dlp.exe"));
    m_ffmpegPath = QDir(m_workDir).filePath(QStringLiteral("ffmpeg.exe"));

    setupEmbeddedPasteAction();
    loadSettings();

    connect(ui->analyzeButton,      &QPushButton::clicked, this, &MainWindow::onAnalyzeClicked);
    connect(ui->downloadButton,     &QPushButton::clicked, this, &MainWindow::onDownloadClicked);
    connect(ui->cancelButton,       &QPushButton::clicked, this, &MainWindow::onCancelClicked);
    connect(ui->browseButton,       &QPushButton::clicked, this, &MainWindow::onBrowseFolderClicked);
    connect(ui->cookiesButton,      &QPushButton::clicked, this, &MainWindow::onSelectCustomCookiesClicked);
    connect(ui->openFileButton,     &QPushButton::clicked, this, &MainWindow::onOpenFileClicked);
    connect(ui->openFolderButton,   &QPushButton::clicked, this, &MainWindow::onOpenFolderClicked);
    connect(ui->updateEngineButton, &QPushButton::clicked, this, &MainWindow::onUpdateEngineClicked);
    connect(ui->telegramButton,     &QPushButton::clicked, this, &MainWindow::onTelegramClicked);
    connect(ui->languageButton,     &QPushButton::clicked, this, &MainWindow::onLanguageChanged);
    connect(ui->videoModeButton,    &QPushButton::toggled, this, &MainWindow::onModeChanged);

    // Enter в поле ввода = «Разобрать».
    connect(ui->urlLineEdit, &QLineEdit::returnPressed, this, &MainWindow::onAnalyzeClicked);

    // Новая ссылка обесценивает старый разбор — прячем карточку.
    connect(ui->urlLineEdit, &QLineEdit::textEdited, this, [this](const QString &text) {
        if (text.trimmed() != m_analyzedUrl) {
            showCard(false);
            showCompletionActions(false);
        }
    });

    showCard(false);
    showCompletionActions(false);
    setStatus(tr("Вставьте ссылку и нажмите «Разобрать»"));
}

MainWindow::~MainWindow()
{
    stopProcessTree();
    cleanupWorkDir();
    delete ui;
}


// =======================================================================
//  Извлечение утилит из ресурсов
// =======================================================================

bool MainWindow::extractResourceFile(const QString &resourcePath,
                                     const QString &destPath,
                                     QString *errorOut)
{
    QFile src(resourcePath);
    if (!src.open(QIODevice::ReadOnly)) {
        if (errorOut)
            *errorOut = tr("не читается ресурс %1").arg(resourcePath);
        return false;
    }

    // Гарантируем наличие подкаталогов (_internal/Cryptodome/Cipher/... и т.п.)
    const QFileInfo destInfo(destPath);
    if (!QDir().mkpath(destInfo.absolutePath())) {
        if (errorOut)
            *errorOut = tr("не создать папку %1").arg(destInfo.absolutePath());
        return false;
    }

    QFile dst(destPath);
    if (!dst.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (errorOut)
            *errorOut = tr("не записать %1: %2").arg(destPath, dst.errorString());
        return false;
    }

    while (!src.atEnd()) {
        const QByteArray chunk = src.read(kChunkSize);
        if (chunk.isEmpty())
            break;
        if (dst.write(chunk) != chunk.size()) {
            if (errorOut)
                *errorOut = tr("обрыв записи в %1 (нет места на диске?)").arg(destPath);
            return false;
        }
    }

    if (!dst.flush()) {
        if (errorOut)
            *errorOut = tr("не сбросить буфер в %1").arg(destPath);
        return false;
    }
    dst.close();

    dst.setPermissions(dst.permissions()
                       | QFileDevice::ExeOwner
                       | QFileDevice::ExeUser);
    return true;
}

bool MainWindow::ensureToolsExtracted()
{
    if (m_toolsReady)
        return true;

    // Собираем полный список вшитого в :/bin — yt-dlp.exe, ffmpeg.exe и всё
    // дерево _internal. Обход рекурсивный, чтобы добавление файлов в .qrc
    // не требовало правок кода.
    struct Entry { QString res; QString dest; qint64 size; };
    QVector<Entry> entries;
    qint64 totalBytes   = 0;
    qint64 pendingBytes = 0;

    QDirIterator it(kResourceRoot, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QString resPath = it.next();
        const QString relPath = resPath.mid(kResourceRoot.length() + 1); // без ":/bin/"
        const QString dest    = QDir(m_workDir).filePath(relPath);
        const qint64  size    = it.fileInfo().size();

        totalBytes += size;

        // Уже распакован и совпадает по размеру — пропускаем.
        // Это же условие ловит обрыв прошлой распаковки: битый файл имеет
        // другой размер и будет перезалит.
        const QFileInfo destInfo(dest);
        if (destInfo.exists() && destInfo.size() == size)
            continue;

        pendingBytes += size;
        entries.append({ resPath, dest, size });
    }

    if (entries.isEmpty()) {
        if (totalBytes == 0) {
            QMessageBox::critical(this, tr("Ошибка сборки"),
                tr("В .exe не вшиты утилиты — ресурс %1 пуст.\n\n"
                   "Проверьте, что resources.qrc подключён в .pro и что файлы\n"
                   "tools/yt-dlp_win/yt-dlp.exe и tools/yt-dlp_win/ffmpeg.exe\n"
                   "существовали на момент компиляции.").arg(kResourceRoot));
            return false;
        }
        m_toolsReady = true;
        return true;   // всё уже лежит на месте
    }

    if (!QDir().mkpath(m_workDir)) {
        QMessageBox::critical(this, tr("Ошибка"),
            tr("Не удалось создать временную папку:\n%1").arg(m_workDir));
        return false;
    }

    // Распаковка с живым прогрессом: ~170 МБ пишутся не мгновенно.
    ui->progressBar->setRange(0, 100);
    ui->progressBar->setValue(0);

    // Блокируем UI на время распаковки, чтобы processEvents()
    // не привёл к reentrancy (повторному входу в обработчики).
    ui->centralwidget->setEnabled(false);

    qint64 written = 0;
    QString error;
    for (const Entry &e : entries) {
        if (!extractResourceFile(e.res, e.dest, &error)) {
            ui->centralwidget->setEnabled(true);
            QMessageBox::critical(this, tr("Ошибка распаковки"),
                tr("Не удалось подготовить утилиты: %1").arg(error));
            return false;
        }
        written += e.size;

        const int percent = pendingBytes > 0
                                ? int((written * 100) / pendingBytes)
                                : 100;
        ui->progressBar->setValue(percent);
        setStatus(tr("Подготовка утилит... %1%").arg(percent));

        // Держим окно отзывчивым: распаковка идёт в UI-потоке.
        // UI заблокирован выше — пользователь не может вызвать reentrancy.
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    }

    ui->centralwidget->setEnabled(true);

    ui->progressBar->setValue(0);
    m_toolsReady = true;
    return true;
}


void MainWindow::setupEmbeddedPasteAction()
{
    QAction *pasteAction = ui->urlLineEdit->addAction(
        QIcon(makePasteIcon(16, devicePixelRatioF())),
        QLineEdit::TrailingPosition
    );
    pasteAction->setToolTip(tr("Вставить из буфера обмена"));
    connect(pasteAction, &QAction::triggered, this, &MainWindow::onPasteClicked);
}

bool MainWindow::isSocialOrPrivateUrl(const QString &url) const
{
    const QString lower = url.toLower();
    return lower.contains(QStringLiteral("instagram.com")) ||
           lower.contains(QStringLiteral("instagr.am")) ||
           lower.contains(QStringLiteral("tiktok.com")) ||
           lower.contains(QStringLiteral("facebook.com")) ||
           lower.contains(QStringLiteral("fb.watch")) ||
           lower.contains(QStringLiteral("twitter.com")) ||
           lower.contains(QStringLiteral("x.com"));
}

bool MainWindow::isBrokenSslDomain(const QString &url) const
{
    const QString lower = url.toLower();
    return lower.contains(QStringLiteral("rutube.ru")) ||
           lower.contains(QStringLiteral("vk.com")) ||
           lower.contains(QStringLiteral("vkvideo.ru"));
}

QString MainWindow::currentCookieBrowser() const
{
    static const QStringList browsers = {
                        QStringLiteral("chrome"),
                        QStringLiteral("edge"),
                        QStringLiteral("firefox"),
                        QStringLiteral("brave"),
                        QStringLiteral("opera")
                    };
    if (m_cookieFallbackIndex >= 0 && m_cookieFallbackIndex < browsers.size())
        return browsers.at(m_cookieFallbackIndex);
    return QStringLiteral("chrome");
}

QStringList MainWindow::commonArgs() const
{
    const QString url = ui->urlLineEdit->text().trimmed();

    QStringList args {
        QStringLiteral("--no-playlist"),            // ссылка на ролик из плейлиста
        QStringLiteral("--no-colors"),              // без ANSI-мусора в парсере
    };

    // Отключаем проверку SSL только для доменов с известными проблемами.
    if (isBrokenSslDomain(url))
        args << QStringLiteral("--no-check-certificate");

    const QString embeddedCookies = QDir(m_workDir).filePath(QStringLiteral("cookies.txt"));

    if (!m_customCookiesPath.isEmpty() && QFile::exists(m_customCookiesPath)) {
        args << QStringLiteral("--cookies") << m_customCookiesPath;
    } else if (QFile::exists(embeddedCookies) && QFileInfo(embeddedCookies).size() > 50) {
        args << QStringLiteral("--cookies") << embeddedCookies;
    } else if (m_useCookieFallback || isSocialOrPrivateUrl(ui->urlLineEdit->text().trimmed())) {
        args << QStringLiteral("--cookies-from-browser") << currentCookieBrowser();
    }

    return args;
}

void MainWindow::startTask(Task task, const QStringList &args, bool mergeChannels)
{
    // Защита от утечки: если предыдущий процесс ещё жив (например,
    // cookie fallback вызывает startTask повторно).
    if (m_process) {
        m_process->disconnect();
        m_process->deleteLater();
        m_process = nullptr;
    }

    m_task = task;
    m_outBuffer.clear();
    m_jsonBuffer.clear();
    m_lastError.clear();

    m_process = new QProcess(this);
    m_process->setWorkingDirectory(m_workDir);

    // Для скачивания прогресс идёт в stdout, а предупреждения в stderr —
    // сливаем в один канал, чтобы ничего не потерять.
    // Для разбора наоборот: в stdout должен остаться ЧИСТЫЙ JSON, иначе
    // любое предупреждение yt-dlp сломает разбор.
    m_process->setProcessChannelMode(mergeChannels ? QProcess::MergedChannels
                                                   : QProcess::SeparateChannels);

#ifdef Q_OS_WIN
    // Прячем чёрное окно консоли: yt-dlp — консольное приложение.
    m_process->setCreateProcessArgumentsModifier(
        [](QProcess::CreateProcessArguments *cpa) {
            cpa->flags |= CREATE_NO_WINDOW;
            if (cpa->startupInfo) {
                cpa->startupInfo->dwFlags    |= STARTF_USESHOWWINDOW;
                cpa->startupInfo->wShowWindow = SW_HIDE;
            }
        });
#endif

    connect(m_process, &QProcess::readyReadStandardOutput,
            this, &MainWindow::onReadyReadOutput);
    connect(m_process, &QProcess::finished,
            this, &MainWindow::onProcessFinished);
    connect(m_process, &QProcess::errorOccurred,
            this, &MainWindow::onProcessErrorOccurred);

    m_process->start(m_ytDlpPath, args);
}


// =======================================================================
//  Шаг 1: разбор ролика
// =======================================================================

void MainWindow::onAnalyzeClicked()
{
    if (m_process && m_process->state() != QProcess::NotRunning)
        return;

    const QString url = ui->urlLineEdit->text().trimmed();
    if (url.isEmpty()) {
        setStatus(tr("Вставьте ссылку на видео"));
        ui->urlLineEdit->setFocus();
        return;
    }
    if (!QUrl(url).isValid() || !url.startsWith(QStringLiteral("http"))) {
        setStatus(tr("Это не похоже на ссылку. Нужен адрес вида https://..."));
        ui->urlLineEdit->setFocus();
        return;
    }

    setBusy(true);
    showCard(false);

    if (!ensureToolsExtracted()) {
        setBusy(false);
        setStatus(tr("Не удалось подготовить утилиты"));
        return;
    }

    m_analyzedUrl = url;
    m_cookieFallbackIndex = 0;
    m_useCookieFallback = false;
    ui->cookiesButton->setVisible(false);

    setStatus(tr("Разбираю ролик..."));
    ui->progressBar->setRange(0, 0);   // «бегущая» полоса на время разбора

    QStringList args = commonArgs();
    args << QStringLiteral("--dump-single-json")
         << QStringLiteral("--no-warnings")
         << url;

    startTask(Task::Analyzing, args, /*mergeChannels=*/false);
}

void MainWindow::handleAnalyzeFinished()
{
    QJsonParseError parseError {};
    const QJsonDocument doc = QJsonDocument::fromJson(m_jsonBuffer, &parseError);

    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        const QString errLower = m_lastError.toLower();
        const bool isAuthError = errLower.contains(QStringLiteral("login")) ||
                                 errLower.contains(QStringLiteral("private")) ||
                                 errLower.contains(QStringLiteral("cookie")) ||
                                 errLower.contains(QStringLiteral("unauthorized")) ||
                                 errLower.contains(QStringLiteral("instagram")) ||
                                 isSocialOrPrivateUrl(m_analyzedUrl);

        if (isAuthError && m_cookieFallbackIndex < 4) {
            m_cookieFallbackIndex++;
            m_useCookieFallback = true;

            setStatus(tr("Авторизация: пробую куки %1...").arg(currentCookieBrowser()));
            ui->progressBar->setRange(0, 0);

            QStringList args = commonArgs();
            args << QStringLiteral("--dump-single-json")
                 << QStringLiteral("--no-warnings")
                 << m_analyzedUrl;

            startTask(Task::Analyzing, args, /*mergeChannels=*/false);
            return;
        }

        resetProgress();
        if (isAuthError) {
            setStatus(tr("Для просмотра этого видео требуется авторизация в браузере или файл cookies.txt"));
            ui->cookiesButton->setVisible(true);
        } else {
            setStatus(m_lastError.isEmpty()
                          ? tr("Не удалось разобрать ответ yt-dlp. Проверьте ссылку.")
                          : tr("Ошибка: %1").arg(m_lastError));
        }
        setBusy(false);
        return;
    }

    const QJsonObject root = doc.object();

    // --- название, канал, длительность ---------------------------------
    const QString title = root.value(QStringLiteral("title"))
                              .toString(tr("Без названия"));
    QString channel = root.value(QStringLiteral("uploader")).toString();
    if (channel.isEmpty())
        channel = root.value(QStringLiteral("channel")).toString();
    if (channel.isEmpty())
        channel = root.value(QStringLiteral("extractor_key")).toString();

    const qint64 duration = qint64(root.value(QStringLiteral("duration")).toDouble());

    ui->videoTitleLabel->setText(title);

    QStringList meta;
    if (!channel.isEmpty())
        meta << channel;
    if (duration > 0)
        meta << formatDuration(duration);
    ui->videoMetaLabel->setText(meta.join(QStringLiteral("  ·  ")));

    // --- доступные качества --------------------------------------------
    // Собираем высоты только у дорожек с видео. QSet не берём: нужен
    // предсказуемый порядок, а высот тут максимум десяток.
    QVector<int> heights;
    const QJsonArray formats = root.value(QStringLiteral("formats")).toArray();
    for (const QJsonValue &v : formats) {
        const QJsonObject f = v.toObject();
        if (f.value(QStringLiteral("vcodec")).toString() == QStringLiteral("none"))
            continue;   // это чистое аудио
        const int h = f.value(QStringLiteral("height")).toInt();
        if (h > 0 && !heights.contains(h))
            heights.append(h);
    }
    std::sort(heights.begin(), heights.end(), std::greater<int>());

    ui->qualityCombo->clear();
    ui->qualityCombo->addItem(tr("Лучшее доступное"), 0);
    for (const int h : heights)
        ui->qualityCombo->addItem(QStringLiteral("%1p").arg(h), h);

    // --- превью ---------------------------------------------------------
    m_thumbUrl = root.value(QStringLiteral("thumbnail")).toString();
    ui->thumbLabel->setText(tr("превью..."));
    ui->thumbLabel->setPixmap(QPixmap());

    showCard(true);
    onModeChanged();          // расставить доступность выбора качества
    resetProgress();
    setStatus(tr("Готово к скачиванию — выберите формат"));

    // Превью тянем отдельным запуском: оно не критично, и ждать его
    // перед показом карточки незачем.
    if (!m_thumbUrl.isEmpty())
        startThumbnailFetch();
    else
        setBusy(false);
}


// =======================================================================
//  Превью
// =======================================================================

void MainWindow::startThumbnailFetch()
{
    // Скачиваем превью силами самого yt-dlp: у него уже настроен SSL и
    // прокси. Через QNetworkAccessManager пришлось бы тащить QtNetwork
    // и OpenSSL-DLL, а это ломает портативность.
    //
    // Конвертация обязательна — YouTube отдаёт .webp, который Qt без
    // плагина qwebp не откроет. И именно PNG, а не JPEG: поддержка PNG
    // вкомпилирована в QtGui, а JPEG — это отдельный плагин
    // imageformats/qjpeg.dll, который пришлось бы класть рядом с exe.
    QStringList args = commonArgs();
    args << QStringLiteral("--skip-download")
         << QStringLiteral("--write-thumbnail")
         << QStringLiteral("--convert-thumbnails") << QStringLiteral("png")
         << QStringLiteral("--ffmpeg-location") << m_ffmpegPath
         << QStringLiteral("-o")
         << QDir(m_workDir).filePath(QStringLiteral("thumb.%(ext)s"))
         << m_analyzedUrl;

    startTask(Task::Thumbnail, args, /*mergeChannels=*/true);
}

void MainWindow::handleThumbnailFinished()
{
    const QString pngPath = QDir(m_workDir).filePath(QStringLiteral("thumb.png"));

    QPixmap pix;
    if (pix.load(pngPath) && !pix.isNull()) {
        // Кадрируем по центру: превью бывают и 16:9, и 4:3, и квадратные.
        // Учитываем devicePixelRatio для чёткости на HiDPI-мониторах.
        const qreal dpr = devicePixelRatioF();
        const QSize target(int(ui->thumbLabel->width() * dpr),
                           int(ui->thumbLabel->height() * dpr));
        QPixmap scaled = pix.scaled(target, Qt::KeepAspectRatioByExpanding,
                                    Qt::SmoothTransformation);
        const int x = (scaled.width()  - target.width())  / 2;
        const int y = (scaled.height() - target.height()) / 2;
        QPixmap cropped = scaled.copy(x, y, target.width(), target.height());
        cropped.setDevicePixelRatio(dpr);
        ui->thumbLabel->setPixmap(cropped);
        ui->thumbLabel->setText(QString());
    } else {
        ui->thumbLabel->setText(tr("без превью"));
    }

    setBusy(false);
}


// =======================================================================
//  Шаг 2: скачивание
// =======================================================================

void MainWindow::onDownloadClicked()
{
    if (m_process && m_process->state() != QProcess::NotRunning)
        return;
    if (m_analyzedUrl.isEmpty())
        return;

    setBusy(true);
    showCompletionActions(false);
    m_lastFileName.clear();
    m_lastFilePath.clear();

    const QString outputTemplate = QDir(m_downloadDir).filePath(QStringLiteral("%(title)s.%(ext)s"));
    const bool audioOnly = ui->audioModeButton->isChecked();

    QStringList args = commonArgs();

    if (audioOnly) {
        args << QStringLiteral("-x")
             << QStringLiteral("--audio-format")  << QStringLiteral("mp3")
             << QStringLiteral("--audio-quality") << QStringLiteral("0");
    } else {
        const int height = ui->qualityCombo->currentData().toInt();
        if (height > 0) {
            args << QStringLiteral("-f")
                 << QStringLiteral("bv*[height<=%1]+ba/b[height<=%1]").arg(height);
        } else {
            args << QStringLiteral("-f") << QStringLiteral("bv*+ba/b");
        }
        args << QStringLiteral("--merge-output-format") << QStringLiteral("mp4");
    }

    args << QStringLiteral("--concurrent-fragments") << kThreads
         << QStringLiteral("--buffer-size")     << QStringLiteral("16K")
         << QStringLiteral("--http-chunk-size") << QStringLiteral("10M")
         << QStringLiteral("--retries")         << QStringLiteral("10")
         << QStringLiteral("--fragment-retries")<< QStringLiteral("10")
         << QStringLiteral("--ffmpeg-location") << m_ffmpegPath
         << QStringLiteral("--newline")
         << QStringLiteral("-o") << outputTemplate
         << m_analyzedUrl;

    setStatus(tr("Подключение к серверу..."));
    ui->progressBar->setRange(0, 0);

    startTask(Task::Downloading, args, /*mergeChannels=*/true);
}

void MainWindow::handleDownloadFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    ui->progressBar->setRange(0, 100);

    if (exitStatus == QProcess::NormalExit && exitCode == 0) {
        // Валидация: проверяем, что файл действительно существует и не пуст.
        const bool fileValid = !m_lastFilePath.isEmpty()
                               && QFile::exists(m_lastFilePath)
                               && QFileInfo(m_lastFilePath).size() > 0;

        if (fileValid) {
            ui->progressBar->setValue(100);
            setStatus(tr("Готово: %1 — сохранено в папку %2.")
                          .arg(m_lastFileName, QDir::toNativeSeparators(m_downloadDir)));
            showCompletionActions(true);
        } else if (m_lastFileName.isEmpty()) {
            // yt-dlp не сообщил имя файла — показываем общее сообщение.
            ui->progressBar->setValue(100);
            setStatus(tr("Готово! Файл сохранён."));
            showCompletionActions(false);
        } else {
            ui->progressBar->setValue(0);
            setStatus(tr("Файл не найден после скачивания. Возможно, на диске не хватает места."));
            showCompletionActions(false);
        }
    } else {
        ui->progressBar->setValue(0);
        setStatus(m_lastError.isEmpty()
                      ? tr("Ошибка: yt-dlp завершился с кодом %1").arg(exitCode)
                      : tr("Ошибка: %1").arg(m_lastError));
        showCompletionActions(false);
    }

    setBusy(false);
}


// =======================================================================
//  Чтение вывода
// =======================================================================

void MainWindow::onReadyReadOutput()
{
    if (!m_process)
        return;

    // При разборе stdout — это чистый JSON, резать его на строки нельзя.
    if (m_task == Task::Analyzing) {
        m_jsonBuffer += m_process->readAllStandardOutput();
        return;
    }

    // Приклеиваем новую порцию к недочитанному «хвосту» прошлой.
    m_outBuffer += QString::fromLocal8Bit(m_process->readAllStandardOutput());

    // Даже с --newline часть строк приходит через \r — приводим к одному
    // разделителю, иначе прогресс склеится в одну гигантскую строку.
    m_outBuffer.replace(QLatin1Char('\r'), QLatin1Char('\n'));

    // Последний элемент может быть незавершённой строкой — оставляем
    // в буфере до следующего чтения.
    QStringList lines = m_outBuffer.split(QLatin1Char('\n'));
    m_outBuffer = lines.takeLast();

    for (const QString &line : lines) {
        const QString trimmed = line.trimmed();
        if (!trimmed.isEmpty())
            handleOutputLine(trimmed);
    }
}

void MainWindow::handleOutputLine(const QString &line)
{
    // Превью качается молча — в статус лезть незачем.
    if (m_task == Task::Thumbnail) {
        if (line.startsWith(QStringLiteral("ERROR:")))
            m_lastError = line.mid(6).trimmed();
        return;
    }

    // --- проценты ------------------------------------------------------
    const QRegularExpressionMatch mPercent = kRePercent.match(line);
    if (mPercent.hasMatch()) {
        // Локаль может отдать «50,6» — приводим к точке для toDouble().
        const double percent = QString(mPercent.captured(1))
                                   .replace(QLatin1Char(','), QLatin1Char('.'))
                                   .toDouble();

        if (ui->progressBar->maximum() == 0)      // выходим из «бегущей» полосы
            ui->progressBar->setRange(0, 100);
        ui->progressBar->setValue(int(percent + 0.5));

        // Скорость и ETA в начале скачивания могут отсутствовать.
        QString status = tr("Скачивание %1%").arg(percent, 0, 'f', 1);

        const QRegularExpressionMatch mSpeed = kReSpeed.match(line);
        if (mSpeed.hasMatch())
            status += QStringLiteral(" (%1)").arg(mSpeed.captured(1).simplified());

        const QRegularExpressionMatch mEta = kReEta.match(line);
        if (mEta.hasMatch())
            status += tr(", осталось %1").arg(mEta.captured(1));

        setStatus(status);
        return;
    }

    // --- имя файла -----------------------------------------------------
    const QRegularExpressionMatch mDest = kReDest.match(line);
    if (mDest.hasMatch()) {
        const QString rawPath = mDest.captured(1).trimmed();
        m_lastFilePath = QFileInfo(rawPath).isAbsolute()
                             ? rawPath
                             : QDir(m_downloadDir).filePath(rawPath);
        m_lastFileName = QFileInfo(m_lastFilePath).fileName();
        return;
    }

    // --- этапы постобработки -------------------------------------------
    if (line.startsWith(QStringLiteral("[Merger]"))) {
        ui->progressBar->setRange(0, 0);
        setStatus(tr("Склейка видео и звука через ffmpeg..."));
        return;
    }
    if (line.startsWith(QStringLiteral("[ExtractAudio]"))) {
        ui->progressBar->setRange(0, 0);
        setStatus(tr("Извлечение звука в MP3..."));
        return;
    }
    if (line.startsWith(QStringLiteral("[VideoConvertor]"))
        || line.startsWith(QStringLiteral("[Fixup"))) {
        ui->progressBar->setRange(0, 0);
        setStatus(tr("Обработка файла через ffmpeg..."));
        return;
    }

    // --- ошибки --------------------------------------------------------
    if (line.startsWith(QStringLiteral("ERROR:"))) {
        m_lastError = line.mid(6).trimmed();
        return;
    }
}


// =======================================================================
//  Завершение процесса
// =======================================================================

void MainWindow::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    // Добираем остаток буфера — последняя строка могла прийти без перевода.
    if (!m_outBuffer.trimmed().isEmpty()) {
        handleOutputLine(m_outBuffer.trimmed());
        m_outBuffer.clear();
    }

    // При разборе или обновлении ошибки лежат в stderr — забираем их оттуда.
    if ((m_task == Task::Analyzing || m_task == Task::UpdatingEngine) && m_process) {
        const QString err = QString::fromLocal8Bit(m_process->readAllStandardError());
        for (const QString &line : err.split(QLatin1Char('\n'))) {
            const QString t = line.trimmed();
            if (t.startsWith(QStringLiteral("ERROR:")))
                m_lastError = t.mid(6).trimmed();
        }
    }

    const Task finished = m_task;
    m_task = Task::Idle;

    if (m_process) {
        m_process->deleteLater();
        m_process = nullptr;
    }

    switch (finished) {
    case Task::Extracting:                                               break;
    case Task::Analyzing:      handleAnalyzeFinished();                  break;
    case Task::Thumbnail:      handleThumbnailFinished();                break;
    case Task::Downloading:    handleDownloadFinished(exitCode, exitStatus); break;
    case Task::UpdatingEngine: handleUpdateEngineFinished(exitCode);    break;
    case Task::Idle:                                                     break;
    }
}

void MainWindow::onProcessErrorOccurred(QProcess::ProcessError error)
{
    if (error != QProcess::FailedToStart)
        return;   // остальные случаи закроются через finished()

    resetProgress();
    setStatus(tr("Не удалось запустить yt-dlp.exe (%1). "
                 "Возможно, антивирус заблокировал распакованный файл.")
                  .arg(m_process ? m_process->errorString() : QString()));

    m_task = Task::Idle;
    if (m_process) {
        m_process->deleteLater();
        m_process = nullptr;
    }
    setBusy(false);
}


// =======================================================================
//  Очистка хвостов
// =======================================================================

void MainWindow::stopProcessTree()
{
    if (!m_process || m_process->state() == QProcess::NotRunning)
        return;

    const qint64 pid = m_process->processId();

#ifdef Q_OS_WIN
    // yt-dlp сам поднимает ffmpeg.exe. Убить только родителя недостаточно:
    // ffmpeg останется висеть и не даст удалить папку.
    // /T — всё дерево процессов, /F — принудительно.
    if (pid > 0) {
        QProcess::execute(QStringLiteral("taskkill"),
                          { QStringLiteral("/PID"), QString::number(pid),
                            QStringLiteral("/T"), QStringLiteral("/F") });
    }
#else
    Q_UNUSED(pid)
#endif

    m_process->terminate();
    if (!m_process->waitForFinished(3000)) {
        m_process->kill();
        m_process->waitForFinished(2000);
    }
}

void MainWindow::cleanupWorkDir()
{
    QDir dir(m_workDir);
    if (m_workDir.isEmpty() || !dir.exists())
        return;

    // Windows может ещё держать дескрипторы только что убитых процессов —
    // даём файловой системе до ~2 секунд, пробуя несколько раз.
    for (int attempt = 0; attempt < 10; ++attempt) {
        if (dir.removeRecursively())
            return;
        QThread::msleep(200);
    }
    // Не вышло — молча уходим: показывать диалоги из деструктора нельзя,
    // а остатки в %TEMP% рано или поздно подчистит система.
}


// =======================================================================
//  UI
// =======================================================================

void MainWindow::onModeChanged()
{
    // Для «только аудио» выбор разрешения бессмыслен.
    const bool videoMode = ui->videoModeButton->isChecked();
    ui->qualityCombo->setEnabled(videoMode);
    ui->optionCaption->setEnabled(videoMode);
}

void MainWindow::onTelegramClicked()
{
    QDesktopServices::openUrl(QUrl(kTelegramUrl));
}

void MainWindow::onCancelClicked()
{
    stopProcessTree();
    m_task = Task::Idle;
    if (m_process) {
        m_process->deleteLater();
        m_process = nullptr;
    }
    resetProgress();
    setStatus(tr("Операция отменена пользователем."));
    setBusy(false);
}

void MainWindow::onPasteClicked()
{
    const QString text = QApplication::clipboard()->text().trimmed();
    if (text.isEmpty()) {
        setStatus(tr("Буфер обмена пуст"));
        return;
    }
    ui->urlLineEdit->setText(text);
    onAnalyzeClicked();
}

void MainWindow::onBrowseFolderClicked()
{
    const QString dir = QFileDialog::getExistingDirectory(
        this, tr("Выберите папку для сохранения"), m_downloadDir);
    if (!dir.isEmpty()) {
        m_downloadDir = dir;
        saveSettings();
        updateDownloadDirLabel();
    }
}

void MainWindow::onOpenFileClicked()
{
    if (!m_lastFilePath.isEmpty() && QFile::exists(m_lastFilePath)) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(m_lastFilePath));
    } else {
        setStatus(tr("Файл не найден на диске"));
    }
}

void MainWindow::onOpenFolderClicked()
{
    if (m_lastFilePath.isEmpty() || !QFile::exists(m_lastFilePath)) {
        if (!m_downloadDir.isEmpty() && QDir(m_downloadDir).exists()) {
            QDesktopServices::openUrl(QUrl::fromLocalFile(m_downloadDir));
        }
        return;
    }
#ifdef Q_OS_WIN
    QProcess::startDetached(QStringLiteral("explorer.exe"),
                            { QStringLiteral("/select,"), QDir::toNativeSeparators(m_lastFilePath) });
#else
    QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(m_lastFilePath).absolutePath()));
#endif
}

void MainWindow::loadSettings()
{
    QSettings settings(QStringLiteral("Said"), QStringLiteral("SaidDownloader"));
    QString defaultDir = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    if (defaultDir.isEmpty())
        defaultDir = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);

    m_downloadDir = settings.value(QStringLiteral("DownloadDir"), defaultDir).toString();
    if (m_downloadDir.isEmpty() || !QDir(m_downloadDir).exists())
        m_downloadDir = defaultDir;

    // Язык интерфейса: по умолчанию русский (строки в коде на русском).
    m_language = settings.value(QStringLiteral("Language"), QStringLiteral("ru")).toString();
    if (m_language == QStringLiteral("en")) {
        m_translator = new QTranslator(this);
        if (m_translator->load(QStringLiteral(":/i18n/app_en.qm"))) {
            QCoreApplication::installTranslator(m_translator);
            ui->retranslateUi(this);
            retranslateCustomUi();
        }
    }

    updateDownloadDirLabel();
}

void MainWindow::saveSettings()
{
    QSettings settings(QStringLiteral("Said"), QStringLiteral("SaidDownloader"));
    settings.setValue(QStringLiteral("DownloadDir"), m_downloadDir);
    settings.setValue(QStringLiteral("Language"), m_language);
}

void MainWindow::updateDownloadDirLabel()
{
    ui->folderPathLabel->setText(QDir::toNativeSeparators(m_downloadDir));
}

void MainWindow::showCompletionActions(bool show)
{
    const bool fileExists = show && !m_lastFilePath.isEmpty() && QFile::exists(m_lastFilePath);
    ui->openFileButton->setVisible(fileExists);
    ui->openFolderButton->setVisible(fileExists);
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls() || event->mimeData()->hasText()) {
        event->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent *event)
{
    QString urlStr;
    if (event->mimeData()->hasUrls()) {
        const QList<QUrl> urls = event->mimeData()->urls();
        if (!urls.isEmpty())
            urlStr = urls.first().toString();
    } else if (event->mimeData()->hasText()) {
        urlStr = event->mimeData()->text().trimmed();
    }

    if (!urlStr.isEmpty()) {
        ui->urlLineEdit->setText(urlStr);
        onAnalyzeClicked();
    }
}

void MainWindow::setBusy(bool busy)
{
    ui->analyzeButton->setEnabled(!busy);
    ui->downloadButton->setEnabled(!busy);
    ui->urlLineEdit->setEnabled(!busy);
    ui->browseButton->setEnabled(!busy);
    ui->cookiesButton->setEnabled(!busy);
    ui->updateEngineButton->setEnabled(!busy);
    ui->videoModeButton->setEnabled(!busy);
    ui->audioModeButton->setEnabled(!busy);
    ui->qualityCombo->setEnabled(!busy && ui->videoModeButton->isChecked());
    ui->cancelButton->setVisible(busy);

    if (busy) {
        showCompletionActions(false);
    }

    ui->analyzeButton->setText(busy && m_task == Task::Analyzing
                                   ? tr("Разбираю...")
                                   : tr("Разобрать"));
    ui->downloadButton->setText(busy && m_task == Task::Downloading
                                    ? tr("Качаю...")
                                    : tr("Скачать"));
}

void MainWindow::onSelectCustomCookiesClicked()
{
    const QString file = QFileDialog::getOpenFileName(
        this,
        tr("Выберите файл cookies.txt"),
        QString(),
        tr("Файлы cookies (*.txt);;Все файлы (*.*)")
    );

    if (file.isEmpty())
        return;

    m_customCookiesPath = file;
    ui->cookiesButton->setText(tr("🍪 Куки: %1").arg(QFileInfo(file).fileName()));
    ui->cookiesButton->setVisible(true);

    setStatus(tr("Загружен файл куков %1. Повторный разбор...").arg(QFileInfo(file).fileName()));
    onAnalyzeClicked();
}

void MainWindow::onUpdateEngineClicked()
{
    if (m_process && m_process->state() != QProcess::NotRunning)
        return;

    setBusy(true);
    showCard(false);

    if (!ensureToolsExtracted()) {
        setBusy(false);
        return;
    }

    setStatus(tr("Проверка и обновление парсеров yt-dlp..."));
    ui->progressBar->setRange(0, 0);

    startTask(Task::UpdatingEngine, QStringList{ QStringLiteral("--update") }, /*mergeChannels=*/true);
}

void MainWindow::handleUpdateEngineFinished(int exitCode)
{
    resetProgress();
    if (exitCode == 0) {
        setStatus(tr("Парсеры yt-dlp успешно обновлены до последней версии!"));
    } else {
        setStatus(m_lastError.isEmpty()
                      ? tr("Не удалось обновить yt-dlp (код %1)").arg(exitCode)
                      : tr("Ошибка обновления: %1").arg(m_lastError));
    }
    setBusy(false);
}

void MainWindow::setStatus(const QString &text)
{
    ui->statusLabel->setText(text);
}

void MainWindow::showCard(bool visible)
{
    ui->infoCard->setVisible(visible);
    if (!visible) {
        m_analyzedUrl.clear();
        ui->thumbLabel->setPixmap(QPixmap());
        ui->thumbLabel->setText(tr("превью"));
    }
}

void MainWindow::resetProgress()
{
    ui->progressBar->setRange(0, 100);
    ui->progressBar->setValue(0);
}

QString MainWindow::formatDuration(qint64 seconds)
{
    const qint64 h = seconds / 3600;
    const qint64 m = (seconds % 3600) / 60;
    const qint64 s = seconds % 60;

    return h > 0
               ? QStringLiteral("%1:%2:%3").arg(h)
                     .arg(m, 2, 10, QLatin1Char('0'))
                     .arg(s, 2, 10, QLatin1Char('0'))
               : QStringLiteral("%1:%2").arg(m)
                     .arg(s, 2, 10, QLatin1Char('0'));
}


// =======================================================================
//  Переключение языка
// =======================================================================

void MainWindow::onLanguageChanged()
{
    if (m_language == QStringLiteral("ru")) {
        // Переключаемся на английский.
        m_language = QStringLiteral("en");
        if (!m_translator) {
            m_translator = new QTranslator(this);
        }
        if (m_translator->load(QStringLiteral(":/i18n/app_en.qm"))) {
            QCoreApplication::installTranslator(m_translator);
        }
    } else {
        // Переключаемся на русский (удаляем переводчик).
        m_language = QStringLiteral("ru");
        if (m_translator) {
            QCoreApplication::removeTranslator(m_translator);
        }
    }

    // Обновляем все тексты виджетов.
    ui->retranslateUi(this);
    retranslateCustomUi();
    saveSettings();
}

void MainWindow::retranslateCustomUi()
{
    // Тексты, выставленные программно (не из .ui), нужно обновить вручную.
    ui->versionChip->setText(QStringLiteral("v%1").arg(QCoreApplication::applicationVersion()));

    // Кнопка языка показывает, НА КАКОЙ язык можно переключиться.
    ui->languageButton->setText(m_language == QStringLiteral("ru")
                                    ? QStringLiteral("EN")
                                    : QStringLiteral("RU"));

    // Обновляем статус и путь.
    if (m_task == Task::Idle) {
        setStatus(tr("Вставьте ссылку и нажмите «Разобрать»"));
    }
    updateDownloadDirLabel();
}
