#ifndef WORKER_H
#define WORKER_H

#include <QObject>
#include <QString>
#include <QProcess>
#include <QMainWindow>

class Worker : public QObject
{
    Q_OBJECT
public:
    Worker();
    ~Worker();

    QMainWindow *mainWindow;
public slots:
    void process();

signals:
    void finished(int);
    void error(QString err);
};

#endif // WORKER_H
