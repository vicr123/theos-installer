#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMessageBox>
#include <QCloseEvent>
#include <QLayout>
#include <QProcess>
#include <QThread>
#include <QDebug>
#include <QTimer>
#include <QException>
#include <QScrollBar>
#include <QDesktopWidget>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include "internetconnection.h"
#include "worker.h"
#include <stdio.h>
#include <iostream>
#include <unistd.h>
#include <linux/reboot.h>
#include <sys/reboot.h>
//#include <QSystemBatteryInfo>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

public:
    bool performNetworkCheck = true;
    bool dryrun = false;

    QString partition;
    bool formatPartition = false;
    QString fullname;
    QString loginname;
    QString password;
    QString hostname;

private slots:
    void on_forwardButton_clicked();

    void on_backButton_clicked();

    void on_quitButton_clicked();

    void on_recheck_clicked();

    void on_connectInternet_clicked();

    void on_part_clicked();

    void on_environment_check_done(int returnVal);

    void on_formatCheck_toggled(bool checked);

    void on_recheck_2_clicked();

    void on_partitions_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous);

    void on_pushButton_clicked();

    void on_pushButton_2_clicked();

    void on_radioButton_toggled(bool checked);

    void on_radioButton_2_toggled(bool checked);

    void installOutput(QString output);

    void installMessage(QString message);

    void on_fullname_textChanged(const QString &arg1);

    void on_hostname_textChanged(const QString &arg1);

    void on_loginname_textEdited(const QString &arg1);

    void on_password_textChanged(const QString &arg1);

    void on_passwordconfirm_textChanged(const QString &arg1);

    void install_complete();

    void on_hostname_textEdited(const QString &arg1);

    void on_fullname_textEdited(const QString &arg1);

private:
    int stage = 0;
    QString mirrors;

    QThread *installThread;

    Ui::MainWindow *ui;
    void closeEvent(QCloseEvent *);
    void changeScreen(int switchTo, bool movingForward);
    bool performSanityChecks(bool progressBar = false);
    void updatePartitionList();
    void checkUserInfo();

};

#include "installworker.h"
#endif // MAINWINDOW_H
