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
#include <QProgressDialog>
#include "internetconnection.h"
#include "worker.h"
#include "erasedrivedialog.h"
#include "partitionwindow.h"
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
    enum stageType {
        Welcome = 0,
        SystemCheck = 1,
        Partition = 2,
        Mirrorlist = 3,
        Confirm = 4,
        UserInfo = 5,
        InstallProgress = 6,
        Finish = 7
    };

    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

public:
    bool performNetworkCheck = true;
    bool dryrun = false;
    bool noProbe = false;

    QString partition;
    bool formatPartition = false;
    QString drive;
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

    void install_error();

    void on_hostname_textEdited(const QString &arg1);

    void on_fullname_textEdited(const QString &arg1);

    void on_partition_eraseDrive_clicked();

    void on_partition_manualMount_clicked();

    void on_partition_somethingElse_clicked();

private:
    stageType stage = Welcome;
    QString mirrors;
    QStringList detectedOperatingSystems;

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
