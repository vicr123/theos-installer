#ifndef INSTALLWORKER_H
#define INSTALLWORKER_H

#include <QObject>
#include <QString>
#include <QProcess>
#include <QDir>
#include <QDirIterator>
#include <QStorageInfo>
#include "mainwindow.h"

class installWorker : public QObject
{
    Q_OBJECT
public:
    installWorker(MainWindow *parent);
    ~installWorker();
public slots:
    void process(bool useOfflineMethod = true);

    void lastProcessFinished(int);

    void outputAvaliable();

    void errorAvaliable();

signals:
    void finished();
    void output(QString output);
    void message(QString message);
    void error(QString err);
    void failed();
    void progress(qulonglong progress, qulonglong max);

private:
    MainWindow *parentWindow;

    bool lastProcessDone = false;
    QString standardOutput;
    QProcess *p;

    bool WaitUntilProcessFinished();
};

#endif // INSTALLWORKER_H
