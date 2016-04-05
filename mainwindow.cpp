#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QRect screenGeometry = QApplication::desktop()->screenGeometry();
    int x = (screenGeometry.width() - this->width()) / 2;
    int y = (screenGeometry.height() - this->height()) / 2;
    this->move(x, y);

    ui->backButton->setVisible(false);
    ui->ConfirmFrame->setVisible(false);
    ui->InstallFrame->setVisible(false);
    ui->MirrorlistFrame->setVisible(false);
    ui->ReadyToInstallFrame->setVisible(false);
    ui->UserInfoFrame->setVisible(false);
    ui->InstallingFrame->setVisible(false);
    ui->CompleteFrame->setVisible(false);
    ui->connectInternet->setVisible(false);
    ui->noPartitions->setVisible(false);
    ui->label_26->setVisible(false);
    ui->pushButton->setEnabled(false);
    ui->progressBar->setVisible(false);

    ui->flag->setPixmap(QIcon::fromTheme("flag").pixmap(24,24));

    this->setFixedSize(this->size());

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event) {
    if (stage == 5 || stage == 6) {
        QMessageBox::warning(this, "Quit Setup", "We're currently installing theOS. You can't quit setup while we're doing this.", QMessageBox::Ok);
        event->ignore();
    } else if (stage == 7) {
        event->accept();
    } else {
        QMessageBox::StandardButton b = QMessageBox::question(this, "Quit Setup", "Do you really want to cancel installing theOS?", QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (b == QMessageBox::Yes) {
            event->accept();
        } else {
            event->ignore();
        }
    }
}

void MainWindow::on_forwardButton_clicked()
{
    if (stage == 1) {
        QFile *adapter = new QFile("/sys/class/power_supply/AC/online");
        if (!adapter->exists()) {
            adapter = new QFile("/sys/class/power_supply/ADP1/online");
        }
        adapter->open(QFile::ReadOnly);
        QByteArray info = adapter->read(1);
        adapter->close();
        qDebug() << info.at(0);
        if (QString(info).startsWith("0")) {
            if (QMessageBox::warning(this, "Continue?", "If power is lost during the theOS install, theOS won't install correctly. Are you sure you wish to continue?", QMessageBox::No, QMessageBox::Yes) == QMessageBox::No) {
                return;
            }
        }
    } else if (stage == 2) {
        if (QDir("/sys/firmware/efi").exists()) {
            QProcess* lsblk = new QProcess();
            lsblk->start("lsblk --output PARTTYPE");
            lsblk->waitForFinished();
            if (!QString(lsblk->readAllStandardOutput()).contains("C12A7328-F81F-11D2-BA4B-00A0C93EC93B", Qt::CaseInsensitive)) {
                if (QMessageBox::warning(this, "EFI System Partition", "We've detected that you're booting theOS on an EFI system. To make theOS bootable once you've installed it, you need to create an EFI System Partition. Do you want to continue despite not having a detectable EFI System Partition?", QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::No) {
                    return;
                }
            }
        }
    } else if (stage == 3) {
        if (ui->radioButton->isChecked()) {
            if (!dryrun) {
                QFile mirrorlist("/etc/pacman.d/mirrorlist");
                mirrorlist.open(QFile::ReadWrite);
                mirrorlist.write(mirrors.toUtf8());
                mirrorlist.close();
            }
        }

        QString summary = "Summary:\n";
        summary.append("Install theOS to " + partition + ".\n");
        if (formatPartition) {
            summary.append("Format the partition before installing.\n");
        }
        summary.append("--------------------");

        ui->textBrowser->setText(summary);
    } else if (stage == 4) {
        ui->quitButton->setVisible(false);
        //this->setWindowFlags(Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint);
        //this->setWindowFlags(this->windowFlags() ^ Qt::WindowCloseButtonHint);
        this->show();
        this->setFocus();
        ui->backButton->setVisible(false);


        installThread = new QThread();
        installWorker *w = new installWorker(this);
        w->moveToThread(installThread);
        connect(w, SIGNAL(error(QString)), this, SLOT(errorString(QString)));
        connect(installThread, SIGNAL(started()), w, SLOT(process()));
        connect(w, SIGNAL(finished()), installThread, SLOT(quit()));
        connect(w, SIGNAL(finished()), w, SLOT(deleteLater()));
        connect(installThread, SIGNAL(finished()), installThread, SLOT(deleteLater()));
        connect(w, SIGNAL(message(QString)), this, SLOT(installMessage(QString)));
        connect(w, SIGNAL(output(QString)), this, SLOT(installOutput(QString)));
        connect(w, SIGNAL(finished()), this, SLOT(install_complete()));
        installThread->start();

        ui->forwardButton->setText("Next");
        ui->forwardButton->setIcon(QIcon::fromTheme("go-next"));
    } else if (stage == 5) {
        fullname = ui->fullname->text();
        loginname = ui->loginname->text();
        password = ui->password->text();
        hostname = ui->hostname->text();
    } else if (stage == 7) {
        sync();
        reboot(LINUX_REBOOT_CMD_RESTART);
        return;
    }
    stage++;
    changeScreen(stage, true);
}

void MainWindow::on_backButton_clicked()
{
    if (stage == 7) {
        this->close();
    }
    stage--;
    changeScreen(stage, false);
}

void MainWindow::changeScreen(int switchTo, bool movingForward) {
    ui->progressThroughInstaller->setMaximum(1000);

    QPropertyAnimation* a = new QPropertyAnimation(ui->progressThroughInstaller, "value");
    a->setStartValue(ui->progressThroughInstaller->value());
    a->setEndValue(switchTo * (1000 / 6));
    a->setDuration(500);
    a->setEasingCurve(QEasingCurve::OutCubic);
    a->start();

    //ui->progressThroughInstaller->setValue(switchTo);
    switch (switchTo) {
    case 0:
            ui->WelcomeFrame->setVisible(true);
            ui->ConfirmFrame->setVisible(false);
            ui->InstallFrame->setVisible(false);
            ui->MirrorlistFrame->setVisible(false);
            ui->ReadyToInstallFrame->setVisible(false);
            ui->InstallingFrame->setVisible(false);
            ui->CompleteFrame->setVisible(false);
            ui->UserInfoFrame->setVisible(false);
            ui->backButton->setVisible(false);
            ui->forwardButton->setEnabled(true);
        break;
    case 1:
            ui->WelcomeFrame->setVisible(false);
            ui->ConfirmFrame->setVisible(true);
            ui->InstallFrame->setVisible(false);
            ui->MirrorlistFrame->setVisible(false);
            ui->ReadyToInstallFrame->setVisible(false);
            ui->InstallingFrame->setVisible(false);
            ui->CompleteFrame->setVisible(false);
            ui->UserInfoFrame->setVisible(false);
            ui->backButton->setVisible(true);
            performSanityChecks(true);
        break;
    case 2:
        ui->WelcomeFrame->setVisible(false);
        ui->ConfirmFrame->setVisible(false);
        ui->InstallFrame->setVisible(true);
        ui->MirrorlistFrame->setVisible(false);
        ui->ReadyToInstallFrame->setVisible(false);
        ui->InstallingFrame->setVisible(false);
        ui->CompleteFrame->setVisible(false);
        ui->UserInfoFrame->setVisible(false);

        updatePartitionList();
        break;
    case 3:
        ui->WelcomeFrame->setVisible(false);
        ui->ConfirmFrame->setVisible(false);
        ui->InstallFrame->setVisible(false);
        ui->MirrorlistFrame->setVisible(true);
        ui->ReadyToInstallFrame->setVisible(false);
        ui->InstallingFrame->setVisible(false);
        ui->CompleteFrame->setVisible(false);
        ui->UserInfoFrame->setVisible(false);
        ui->forwardButton->setText("Next");
        ui->forwardButton->setIcon(QIcon::fromTheme("go-next"));
        ui->forwardButton->setEnabled(false);
        break;
    case 4:
        ui->WelcomeFrame->setVisible(false);
        ui->ConfirmFrame->setVisible(false);
        ui->InstallFrame->setVisible(false);
        ui->MirrorlistFrame->setVisible(false);
        ui->ReadyToInstallFrame->setVisible(true);
        ui->InstallingFrame->setVisible(false);
        ui->CompleteFrame->setVisible(false);
        ui->UserInfoFrame->setVisible(false);
        ui->forwardButton->setText("Install!");
        ui->forwardButton->setIcon(QIcon::fromTheme("dialog-ok"));
        break;

    case 5:
        ui->WelcomeFrame->setVisible(false);
        ui->ConfirmFrame->setVisible(false);
        ui->InstallFrame->setVisible(false);
        ui->MirrorlistFrame->setVisible(false);
        ui->ReadyToInstallFrame->setVisible(false);
        ui->UserInfoFrame->setVisible(true);
        ui->InstallingFrame->setVisible(false);
        ui->CompleteFrame->setVisible(false);
        ui->backButton->setVisible(false);
        ui->forwardButton->setEnabled(false);
        break;
    case 6:
        ui->WelcomeFrame->setVisible(false);
        ui->ConfirmFrame->setVisible(false);
        ui->InstallFrame->setVisible(false);
        ui->MirrorlistFrame->setVisible(false);
        ui->ReadyToInstallFrame->setVisible(false);
        ui->UserInfoFrame->setVisible(false);
        ui->InstallingFrame->setVisible(true);
        ui->CompleteFrame->setVisible(false);
        ui->forwardButton->setVisible(false);
        break;
    }
}

void MainWindow::updatePartitionList() {
    ui->partitions->clear();
    ui->forwardButton->setEnabled(false);
    /*QProcess *osprober = new QProcess(this);
    osprober->start("os-prober");
    osprober->waitForStarted(-1);
    osprober->waitForFinished(-1);
    QString osproberoutput(osprober->readAllStandardOutput());
    qDebug() << osproberoutput;
    QStringList proberOutput = osproberoutput.split("\n");*/

    QProcess *lsblk = new QProcess(this);
    lsblk->start("lsblk -rf");

    lsblk->waitForFinished();
    QByteArray output = lsblk->readAllStandardOutput();
    for (QString part : QString(output).split("\n")) {
        if (part != "") {
            QStringList parse = part.split(" ");
            /*for (int i = parse.length() - 1; i > 1; i--) {
                if (parse[i] == "") {
                    parse.removeAt(i);
                }
            }*/
            if (parse.length() > 3) {
                if (part.split(" ")[1] == "ext4") {
                    if (parse[4] != "/") {
                        /*QString installedOs = "";

                        for (QString installed : proberOutput) {
                            QStringList split = installed.split(":");
                            if (split[0].contains(parse[0])) {
                                if (split[1] == "") {
                                    installedOs = split[2];
                                } else {
                                    installedOs = split[1];
                                }
                                break;
                            }
                        }*/

                        QListWidgetItem *i = new QListWidgetItem(ui->partitions);
                        //if (installedOs == "") {
                            i->setText("/dev/" + parse[0] + " " + parse[2]);
                        /*} else {
                            i->setText("/dev/" + parse[0] + " " + parse[2] + " (" + installedOs + " is installed here)");
                        }*/
                        ui->partitions->addItem(i);
                    }
                }
            }
        }
    }


    /*if (proberOutput.count() > 0 && proberOutput[0] != "") {
        QString installedOs = "Currently";
        for (QString installed : proberOutput) {
            if (installed == "") {
                continue;
            }
            qDebug() << installed;
            QStringList split = installed.split(":");
            qDebug() << split;
            if (split[1] == "") {
                installedOs.append(", " + split[2]);
            } else {
                installedOs.append(", " + split[1]);
            }
        }
        installedOs.append(" is installed.");

        ui->label_8->setText(installedOs);
    }*/

    if (ui->partitions->count() == 0) {
        ui->noPartitions->setVisible(true);
    } else {
        ui->noPartitions->setVisible(false);
    }
}

bool MainWindow::performSanityChecks(bool progressBar) {
    ui->backButton->setEnabled(false);
    ui->forwardButton->setEnabled(false);

    ui->SpaceLabel->setPixmap(QIcon::fromTheme("go-next").pixmap(22));
    if (progressBar) {
        ui->progressBar->setMaximum(0);
        ui->progressBar->setVisible(true);
        ui->recheck->setEnabled(false);
    }

    bool battery;
    QFile *adapter = new QFile("/sys/class/power_supply/AC/online");
    if (!adapter->exists()) {
        adapter = new QFile("/sys/class/power_supply/ADP1/online");
    }

    adapter->open(QFile::ReadOnly);
    QByteArray info = adapter->read(1);
    adapter->close();
    qDebug() << info.at(0);
    if (QString(info).startsWith("0")) {
        battery = false;
    } else {
        battery = true;
    }

    if (battery) {
        ui->PowerLabel->setPixmap(QIcon::fromTheme("dialog-ok").pixmap(22));
    } else {
        ui->PowerLabel->setPixmap(QIcon::fromTheme("dialog-cancel").pixmap(22));
    }

    if (performNetworkCheck) {
        QThread *t = new QThread();
        Worker *w = new Worker();
        w->moveToThread(t);
        connect(w, SIGNAL(error(QString)), this, SLOT(errorString(QString)));
        connect(t, SIGNAL(started()), w, SLOT(process()));
        connect(w, SIGNAL(finished(int)), this, SLOT(on_environment_check_done(int)));
        connect(w, SIGNAL(finished(int)), t, SLOT(quit()));
        connect(w, SIGNAL(finished(int)), w, SLOT(deleteLater()));
        connect(t, SIGNAL(finished()), t, SLOT(deleteLater()));
        t->start();
    } else {
        on_environment_check_done(0);
    }


    return true;
}

void MainWindow::on_quitButton_clicked()
{
    this->close();
}

void MainWindow::on_recheck_clicked()
{
    performSanityChecks(true);
}

void MainWindow::on_connectInternet_clicked()
{
    InternetConnection *internetWindow = new InternetConnection(this);
    internetWindow->exec();
    performSanityChecks(true);
    //internetWindow->show();
    //QMessageBox::information(this, "Connect to the Internet", "To connect to the internet using Wi-Fi, quit the installer now and click \"Test theOS.\" Then, use the network manager up the top.", QMessageBox::Ok);
}

void MainWindow::on_part_clicked()
{
    QProcess* proc = new QProcess();
    proc->start("partitionmanager");
    proc->waitForStarted();

    ui->label_10->setText("Before we continue, the partition manager needs to be closed so that we can do our stuff.");

    QPropertyAnimation* a = new QPropertyAnimation(ui->waitingFrame, "geometry");
    a->setStartValue(ui->waitingFrame->geometry());
    a->setEndValue(QRect(ui->waitingFrame->x(), 10, ui->waitingFrame->width(), ui->waitingFrame->height()));
    a->setDuration(500);
    a->setEasingCurve(QEasingCurve::OutCubic);
    a->start();

    while (proc->state() != 0) {
        QApplication::processEvents();
    }

    QPropertyAnimation* b = new QPropertyAnimation(ui->waitingFrame, "geometry");
    b->setStartValue(ui->waitingFrame->geometry());
    b->setEndValue(QRect(ui->waitingFrame->x(), 1360, ui->waitingFrame->width(), ui->waitingFrame->height()));
    b->setDuration(500);
    b->setEasingCurve(QEasingCurve::InCubic);
    b->start();

    updatePartitionList();
}

void MainWindow::on_environment_check_done(int returnVal) {
    ui->backButton->setEnabled(true);
    ui->recheck->setEnabled(true);
    if (returnVal == 0) {
        ui->InternetLabel->setPixmap(QIcon::fromTheme("dialog-ok").pixmap(22));
        ui->forwardButton->setEnabled(true);
        ui->connectInternet->setVisible(false);
    } else {
        ui->InternetLabel->setPixmap(QIcon::fromTheme("dialog-cancel").pixmap(22));
        ui->forwardButton->setEnabled(false);
        ui->connectInternet->setVisible(true);
    }

    //if (progressBar) {
        ui->progressBar->setVisible(false);
    //}
}

void MainWindow::on_formatCheck_toggled(bool checked)
{
    if (checked) {
        if (QMessageBox::warning(this, "Continue?", "Formatting a partition destroys all data on it.", QMessageBox::Cancel, QMessageBox::Ok) == QMessageBox::Cancel) {
            ui->formatCheck->setChecked(false);
        } else {
            formatPartition = true;
        }
    } else {
        formatPartition = false;
    }
}

void MainWindow::on_recheck_2_clicked()
{
    updatePartitionList();
}

void MainWindow::on_partitions_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous)
{
    ui->forwardButton->setEnabled(true);

    if (current != NULL) {
        partition = current->text().split(" ")[0];
    }
}

void MainWindow::on_pushButton_clicked()
{
    QProcess* proc = new QProcess();
    proc->start("kate /etc/pacman.d/mirrorlist");
    proc->waitForStarted();

    ui->label_10->setText("Before we continue, the text editor needs to be closed so that we can do our stuff.");

    QPropertyAnimation* a = new QPropertyAnimation(ui->waitingFrame, "geometry");
    a->setStartValue(ui->waitingFrame->geometry());
    a->setEndValue(QRect(ui->waitingFrame->x(), 10, ui->waitingFrame->width(), ui->waitingFrame->height()));
    a->setDuration(500);
    a->setEasingCurve(QEasingCurve::OutCubic);
    a->start();

    while (proc->state() != 0) {
        QApplication::processEvents();
    }

    QPropertyAnimation* b = new QPropertyAnimation(ui->waitingFrame, "geometry");
    b->setStartValue(ui->waitingFrame->geometry());
    b->setEndValue(QRect(ui->waitingFrame->x(), 1360, ui->waitingFrame->width(), ui->waitingFrame->height()));
    b->setDuration(500);
    b->setEasingCurve(QEasingCurve::InCubic);
    b->start();
}

void MainWindow::on_pushButton_2_clicked()
{
    QFile mirrorlist("/etc/pacman.d/mirrorlist");
    mirrorlist.open(QFile::ReadOnly);
    QStringList readMirrorlist(QString(mirrorlist.readAll()).split("\n"));
    QStringList editedMirrorlist;

    bool moveNextLineToTop = false;
    for (QString line : readMirrorlist) {
        if (moveNextLineToTop) {
            moveNextLineToTop = false;
            editedMirrorlist.append(line);
        } else if (line.contains(ui->CountryDropDown->currentText(), Qt::CaseInsensitive)) {
            moveNextLineToTop = true;
            editedMirrorlist.append(line);
        }
    }

    editedMirrorlist.append(readMirrorlist);

    QString finalMirrorList("");
    for (QString line : editedMirrorlist) {
        finalMirrorList.append(line + "\n");
    }

    if (finalMirrorList == "") {
        ui->modifiedMirrorList->setText("Unfortunately, a mirrorlist couldn't be generated from your country.");
        ui->forwardButton->setEnabled(false);
    } else {
        ui->modifiedMirrorList->setText(finalMirrorList);
        ui->forwardButton->setEnabled(true);
    }
    mirrors = finalMirrorList;
}


void MainWindow::on_radioButton_toggled(bool checked)
{
    if (checked) {
        ui->CountryDropDown->setEnabled(true);
        ui->pushButton_2->setEnabled(true);
        ui->modifiedMirrorList->setEnabled(true);
        ui->pushButton->setEnabled(false);
        ui->forwardButton->setEnabled(false);
    }
}

void MainWindow::on_radioButton_2_toggled(bool checked)
{
    if (checked) {
        ui->CountryDropDown->setEnabled(false);
        ui->pushButton_2->setEnabled(false);
        ui->modifiedMirrorList->setEnabled(false);
        ui->pushButton->setEnabled(true);
        ui->modifiedMirrorList->setText("");
        ui->forwardButton->setEnabled(true);
    }
}

void MainWindow::installMessage(QString message) {
    ui->installMessage->setText(message);
}

void MainWindow::installOutput(QString output) {
    QScrollBar *scroll = ui->standardOutput->verticalScrollBar();
    int scrollTo;
    if (scroll->value() == scroll->maximum()) {
        scrollTo = -1;
    } else {
        scrollTo = scroll->value();
    }

    //qDebug() << "Output:";
    std::cout << output.toStdString() << std::endl;
    ui->standardOutput->setText(output);
    if (scrollTo == -1) {
        scroll->setValue(scroll->maximum());
    } else {
        scroll->setValue(scrollTo);
    }
}

void MainWindow::on_fullname_textChanged(const QString &arg1)
{
    checkUserInfo();
}

void MainWindow::on_hostname_textChanged(const QString &arg1)
{
    checkUserInfo();
}

void MainWindow::on_loginname_textEdited(const QString &arg1)
{
    ui->loginname->setText(arg1.toLower());
    checkUserInfo();
}

void MainWindow::on_password_textChanged(const QString &arg1)
{
    checkUserInfo();
}

void MainWindow::on_passwordconfirm_textChanged(const QString &arg1)
{
    checkUserInfo();
}

void MainWindow::checkUserInfo() {
    if (ui->passwordconfirm->text() != "") {
        if (ui->passwordconfirm->text() != ui->password->text()) {
            ui->label_26->setVisible(true);
            return;
        } else {
            ui->label_26->setVisible(false);
        }
    }

    if (ui->fullname->text() != "" && ui->loginname->text() != "" && ui->password->text() != "" && ui->hostname->text() != "") {
        ui->forwardButton->setEnabled(true);
    } else {
        ui->forwardButton->setEnabled(false);
    }
}

void MainWindow::install_complete() {
    stage = 7;

    ui->CompleteFrame->setVisible(true);
    ui->InstallingFrame->setVisible(false);

    ui->forwardButton->setText("Reboot");
    ui->forwardButton->setIcon(QIcon::fromTheme("system-reboot"));
    ui->forwardButton->setVisible(true);
    ui->backButton->setText("Close");
    ui->backButton->setIcon(QIcon::fromTheme("dialog-ok"));
    ui->backButton->setVisible(true);
}

void MainWindow::on_hostname_textEdited(const QString &arg1)
{

}

void MainWindow::on_fullname_textEdited(const QString &arg1)
{
    if (arg1.split(" ").count() >= 1) {
        ui->loginname->setText(arg1.split(" ").at(0).toLower());
    }
}
