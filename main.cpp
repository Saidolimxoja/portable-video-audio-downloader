#include "mainwindow.h"

#include <QApplication>
#include <QFile>
#include <QIcon>
#include <QStyleFactory>

// Версия приезжает из .pro (DEFINES += APP_VERSION=...).
// Фолбэк нужен, чтобы файл собирался и при сборке в обход qmake.
#ifndef APP_VERSION
#  define APP_VERSION "0.0.0-dev"
#endif

/**
 * Точка входа. Ничего лишнего: вся логика — в MainWindow.
 */
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QApplication::setApplicationName(QStringLiteral("Said Downloader"));
    QApplication::setApplicationVersion(QStringLiteral(APP_VERSION));
    QApplication::setOrganizationName(QStringLiteral("Said"));
    QApplication::setWindowIcon(QIcon(QStringLiteral(":/ui/app.ico")));

    // Fusion — единая база под тему. Нативный windowsvista-стиль рисует
    // собственные рамки поверх QSS, и половина правил просто не применяется.
    QApplication::setStyle(QStyleFactory::create(QStringLiteral("Fusion")));

    QFile qss(QStringLiteral(":/ui/style.qss"));
    if (qss.open(QIODevice::ReadOnly | QIODevice::Text))
        app.setStyleSheet(QString::fromUtf8(qss.readAll()));

    MainWindow window;
    window.show();

    return app.exec();
}
