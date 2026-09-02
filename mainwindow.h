#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QProcess>
#include <QString>
#include <QStringList>

#include <QDragEnterEvent>
#include <QDropEvent>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

/**
 * @brief Главное (и единственное) окно портативного загрузчика.
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    /// Чем именно занят текущий QProcess.
    enum class Task {
        Idle,
        Extracting,   ///< Фоновая распаковка ресурсов
        Analyzing,    ///< yt-dlp --dump-single-json
        Thumbnail,    ///< выкачивание превью
        Downloading   ///< собственно скачивание
    };

private slots:
    void onAnalyzeClicked();
    void onDownloadClicked();
    void onCancelClicked();
    void onPasteClicked();
    void onBrowseFolderClicked();
    void onOpenFileClicked();
    void onOpenFolderClicked();
    void onModeChanged();
    void onTelegramClicked();

    void onReadyReadOutput();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessErrorOccurred(QProcess::ProcessError error);

private:
    // ---- запуск yt-dlp -------------------------------------------------
    void startTask(Task task, const QStringList &args, bool mergeChannels);
    QStringList commonArgs() const;

    // ---- шаги сценария -------------------------------------------------
    void handleAnalyzeFinished();
    void handleThumbnailFinished();
    void handleDownloadFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void startThumbnailFetch();

    // ---- утилиты и файловая часть -------------------------------------
    bool ensureToolsExtracted();
    static bool extractResourceFile(const QString &resourcePath,
                                    const QString &destPath,
                                    QString *errorOut);
    void stopProcessTree();
    void cleanupWorkDir();

    // ---- настройки и пути ---------------------------------------------
    void loadSettings();
    void saveSettings();
    void updateDownloadDirLabel();
    void showCompletionActions(bool show);

    // ---- разбор вывода -------------------------------------------------
    void handleOutputLine(const QString &line);

    // ---- UI ------------------------------------------------------------
    void setBusy(bool busy);
    void setStatus(const QString &text);
    void showCard(bool visible);
    void resetProgress();
    static QString formatDuration(qint64 seconds);

    Ui::MainWindow *ui;

    QProcess *m_process = nullptr;   ///< живёт только на время одной задачи
    Task      m_task = Task::Idle;

    QString m_workDir;               ///< %TEMP%/said_downloader_<PID>
    QString m_ytDlpPath;
    QString m_ffmpegPath;

    QString m_outBuffer;             ///< «хвост» неполной строки между чтениями
    QByteArray m_jsonBuffer;         ///< накопленный stdout при разборе
    QString m_lastError;
    QString m_lastFileName;
    QString m_lastFilePath;

    QString m_analyzedUrl;           ///< ссылка, которую реально разобрали
    QString m_thumbUrl;
    QString m_downloadDir;           ///< Пользовательская папка загрузки
    bool    m_toolsReady = false;
    Task    m_pendingTaskAfterExtract = Task::Idle;
};

#endif // MAINWINDOW_H
