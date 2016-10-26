#include "mainwindow.h"
#include <QApplication>
#include <QIcon>
#include <QStyleFactory>
#include <QPalette>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    qDebug() << QIcon::themeName();
    qDebug() << QStyleFactory::keys();
    if (QIcon::themeName() == "hicolor") {
        QApplication::setStyle(QStyleFactory::create("Breeze"));
        //a.setStyleSheet("/usr/share/color-schemes/BreezeDark.colors");
        QPalette pal(QColor::fromRgb(49, 54, 59));
        pal.setBrush(QPalette::Highlight, QBrush(QColor::fromRgb(61,174,233)));
        a.setPalette(pal);

        QIcon::setThemeName("breeze-dark");
    }

    MainWindow w;

    QStringList args = a.arguments();
    args.removeAt(0);
    for (QString arg : args) {
        if (arg == "--no-network-check" || arg == "-n") {
            w.performNetworkCheck = false;
        } else if (arg == "--dry" || arg == "-d") {
            w.dryrun = true;
        } else if (arg == "--no-probe-os" || arg == "-o") {
            w.noProbe = true;
        } else if (arg == "--help" || arg == "-h") {
            qDebug() << "theOS Installer";
            qDebug() << "Usage: theos_installer [OPTIONS]";
            qDebug() << "  -n, --no-network-check       Don't perform network sanity check";
            qDebug() << "  -d, --dry                    Don't edit any files (until we actually install)";
            qDebug() << "  -o, --no-probe-os            Don't search for other operating systems on disk.";

            return 0;
        } else {
            qDebug() << arg + " is an invalid option.";
            qDebug() << "Use -h or --help to get help options.";
            return 0;
        }
    }


    w.show();

    return a.exec();
}
