#include "mainwindow.h"
#include "ui_mainwindow.h"

QString calculateSize(quint64 size) {
    QString ret;
    if (size > 1073741824) {
        ret = QString::number(((float) size / 1024 / 1024 / 1024), 'f', 2).append(" GiB");
    } else if (size > 1048576) {
        ret = QString::number(((float) size / 1024 / 1024), 'f', 2).append(" MiB");
    } else if (size > 1024) {
        ret = QString::number(((float) size / 1024), 'f', 2).append(" KiB");
    } else {
        ret = QString::number((float) size, 'f', 2).append(" B");
    }

    return ret;
}

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
    ui->label_26->setVisible(false);
    ui->pushButton->setEnabled(false);
    ui->progressBar->setVisible(false);
    ui->slideshow->setVisible(false);

    QGraphicsScene* s = new QGraphicsScene();
    s->addPixmap(QPixmap(":/slides/slide02.svg"));
    s->setSceneRect(0, 0, ui->slideshow->width(), ui->slideshow->height());
    ui->slideshow->setScene(s);

    ui->flag->setPixmap(QIcon::fromTheme("flag").pixmap(24,24));

    this->setFixedSize(this->size());

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event) {
    if (stage == UserInfo || stage == InstallProgress) {
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
    if (stage == SystemCheck) {
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

        ui->driveBox->clear();
        QProcess *lsblk = new QProcess(this);
        lsblk->start("lsblk -rb");

        lsblk->waitForFinished();
        QByteArray output = lsblk->readAllStandardOutput();
        for (QString part : QString(output).split("\n")) {
            if (part != "") {
                QStringList parse = part.split(" ");
                if (parse.first().startsWith("sd") && parse.first().length() == 3) {
                    ui->driveBox->addItem("/dev/" + parse.first() + " (" + calculateSize(parse.at(3).toLongLong()) + ")", parse.first());
                }
            }
        }



        ui->forwardButton->setEnabled(false);
        ui->partition_dualBoot->setVisible(false);
        delete lsblk;

    } else if (stage == Partition) {
        /*if (QDir("/sys/firmware/efi").exists()) {
            QProcess* lsblk = new QProcess();
            lsblk->start("lsblk --output PARTTYPE");
            lsblk->waitForFinished();
            if (!QString(lsblk->readAllStandardOutput()).contains("C12A7328-F81F-11D2-BA4B-00A0C93EC93B", Qt::CaseInsensitive)) {
                if (QMessageBox::warning(this, "EFI System Partition", "We've detected that you're booting theOS on an EFI system. To make theOS bootable once you've installed it, you need to create an EFI System Partition. Do you want to continue despite not having a detectable EFI System Partition?", QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::No) {
                    return;
                }
            }
        }*/
    } else if (stage == Mirrorlist) {
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
    } else if (stage == Confirm) {
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
        connect(w, SIGNAL(failed()), this, SLOT(install_error()));
        installThread->start();

        ui->forwardButton->setText("Next");
        ui->forwardButton->setIcon(QIcon::fromTheme("go-next"));
    } else if (stage == UserInfo) {
        fullname = ui->fullname->text();
        loginname = ui->loginname->text();
        password = ui->password->text();
        hostname = ui->hostname->text();
    } else if (stage == Finish) {
        QProcess::startDetached("shutdown -r now");
        return;
    }
    stage = (stageType) (stage + 1);
    changeScreen(stage, true);
}

void MainWindow::on_backButton_clicked()
{
    if (stage == Finish) {
        this->close();
    }
    stage = (stageType) (stage - 1);
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

    QParallelAnimationGroup* animationGroup = new QParallelAnimationGroup();

    QPropertyAnimation* welcomeAnim = new QPropertyAnimation(ui->WelcomeFrame, "geometry");
    welcomeAnim->setStartValue(QRect(160, 10, ui->WelcomeFrame->width(), ui->WelcomeFrame->height()));
    welcomeAnim->setEndValue(QRect(-291, 10, ui->WelcomeFrame->width(), ui->WelcomeFrame->height()));
    welcomeAnim->setDuration(500);
    welcomeAnim->setEasingCurve(QEasingCurve::InOutCubic);
    animationGroup->addAnimation(welcomeAnim);

    QPropertyAnimation* confirmAnim = new QPropertyAnimation(ui->ConfirmFrame, "geometry");
    confirmAnim->setStartValue(QRect(160, 10, ui->ConfirmFrame->width(), ui->ConfirmFrame->height()));
    confirmAnim->setEndValue(QRect(-291, 10, ui->ConfirmFrame->width(), ui->ConfirmFrame->height()));
    confirmAnim->setDuration(500);
    confirmAnim->setEasingCurve(QEasingCurve::InOutCubic);
    animationGroup->addAnimation(confirmAnim);

    QPropertyAnimation* installAnim = new QPropertyAnimation(ui->InstallFrame, "geometry");
    installAnim->setStartValue(QRect(160, 10, ui->InstallFrame->width(), ui->InstallFrame->height()));
    installAnim->setEndValue(QRect(-291, 10, ui->InstallFrame->width(), ui->InstallFrame->height()));
    installAnim->setDuration(500);
    installAnim->setEasingCurve(QEasingCurve::InOutCubic);
    animationGroup->addAnimation(installAnim);

    QPropertyAnimation* mirrorAnim = new QPropertyAnimation(ui->MirrorlistFrame, "geometry");
    mirrorAnim->setStartValue(QRect(160, 10, ui->MirrorlistFrame->width(), ui->MirrorlistFrame->height()));
    mirrorAnim->setEndValue(QRect(-291, 10, ui->MirrorlistFrame->width(), ui->MirrorlistFrame->height()));
    mirrorAnim->setDuration(500);
    mirrorAnim->setEasingCurve(QEasingCurve::InOutCubic);
    animationGroup->addAnimation(mirrorAnim);

    QPropertyAnimation* readyAnim = new QPropertyAnimation(ui->ReadyToInstallFrame, "geometry");
    readyAnim->setStartValue(QRect(160, 10, ui->ReadyToInstallFrame->width(), ui->ReadyToInstallFrame->height()));
    readyAnim->setEndValue(QRect(-291, 10, ui->ReadyToInstallFrame->width(), ui->ReadyToInstallFrame->height()));
    readyAnim->setDuration(500);
    readyAnim->setEasingCurve(QEasingCurve::InOutCubic);
    animationGroup->addAnimation(readyAnim);

    QPropertyAnimation* infoAnim = new QPropertyAnimation(ui->UserInfoFrame, "geometry");
    infoAnim->setStartValue(QRect(160, 10, ui->UserInfoFrame->width(), ui->UserInfoFrame->height()));
    infoAnim->setEndValue(QRect(-291, 10, ui->UserInfoFrame->width(), ui->UserInfoFrame->height()));
    infoAnim->setDuration(500);
    infoAnim->setEasingCurve(QEasingCurve::InOutCubic);
    animationGroup->addAnimation(infoAnim);

    switch (switchTo) {
    case 0:
        welcomeAnim->setStartValue(QRect(619, 10, ui->WelcomeFrame->width(), ui->WelcomeFrame->height()));
        welcomeAnim->setEndValue(QRect(160, 10, ui->WelcomeFrame->width(), ui->WelcomeFrame->height()));
        break;
    case 1:
        confirmAnim->setStartValue(QRect(619, 10, ui->ConfirmFrame->width(), ui->ConfirmFrame->height()));
        confirmAnim->setEndValue(QRect(160, 10, ui->ConfirmFrame->width(), ui->ConfirmFrame->height()));

        performSanityChecks(true);
        break;
    case 2:
        installAnim->setStartValue(QRect(619, 10, ui->InstallFrame->width(), ui->InstallFrame->height()));
        installAnim->setEndValue(QRect(160, 10, ui->InstallFrame->width(), ui->InstallFrame->height()));

        updatePartitionList();
        break;
    case 3:
        mirrorAnim->setStartValue(QRect(619, 10, ui->MirrorlistFrame->width(), ui->MirrorlistFrame->height()));
        mirrorAnim->setEndValue(QRect(160, 10, ui->MirrorlistFrame->width(), ui->MirrorlistFrame->height()));

        break;
    case 4:
        readyAnim->setStartValue(QRect(619, 10, ui->ReadyToInstallFrame->width(), ui->ReadyToInstallFrame->height()));
        readyAnim->setEndValue(QRect(160, 10, ui->ReadyToInstallFrame->width(), ui->ReadyToInstallFrame->height()));

        break;

    case 5:
        infoAnim->setStartValue(QRect(619, 10, ui->UserInfoFrame->width(), ui->UserInfoFrame->height()));
        infoAnim->setEndValue(QRect(160, 10, ui->UserInfoFrame->width(), ui->UserInfoFrame->height()));

        break;
    case 6:
        QPropertyAnimation* installingAnim = new QPropertyAnimation(ui->InstallingFrame, "geometry");
        installingAnim->setStartValue(QRect(160, 10, ui->InstallingFrame->width(), ui->InstallingFrame->height()));
        installingAnim->setEndValue(QRect(-291, 10, ui->InstallingFrame->width(), ui->InstallingFrame->height()));
        installingAnim->setDuration(500);
        installingAnim->setEasingCurve(QEasingCurve::InOutCubic);
        installingAnim->setStartValue(QRect(619, 10, ui->InstallingFrame->width(), ui->InstallingFrame->height()));
        installingAnim->setEndValue(QRect(160, 10, ui->InstallingFrame->width(), ui->InstallingFrame->height()));

        animationGroup->addAnimation(installingAnim);
        ui->InstallingFrame->setVisible(true);

        break;
    }

    connect(animationGroup, &QParallelAnimationGroup::finished, [=]() {
        ui->WelcomeFrame->setVisible(false);
        ui->ConfirmFrame->setVisible(false);
        ui->InstallFrame->setVisible(false);
        ui->MirrorlistFrame->setVisible(false);
        ui->ReadyToInstallFrame->setVisible(false);
        ui->InstallingFrame->setVisible(false);
        ui->CompleteFrame->setVisible(false);
        ui->UserInfoFrame->setVisible(false);

        switch (switchTo) {
        case Welcome:
            ui->WelcomeFrame->setVisible(true);
            ui->forwardButton->setEnabled(true);
            ui->backButton->setVisible(false);

            ui->WelcomeFrame->raise();
            break;
        case SystemCheck:
            ui->ConfirmFrame->setVisible(true);
            ui->backButton->setVisible(true);

            ui->ConfirmFrame->raise();
            break;
        case Partition:
            ui->InstallFrame->setVisible(true);

            ui->InstallFrame->raise();

            break;
        case Mirrorlist:
            ui->MirrorlistFrame->setVisible(true);
            ui->forwardButton->setText("Next");
            ui->forwardButton->setIcon(QIcon::fromTheme("go-next"));
            ui->forwardButton->setEnabled(false);

            ui->MirrorlistFrame->raise();
            break;
        case Confirm:
            ui->ReadyToInstallFrame->setVisible(true);
            ui->forwardButton->setText("Install!");
            ui->forwardButton->setIcon(QIcon::fromTheme("dialog-ok"));

            ui->ReadyToInstallFrame->raise();
            break;

        case UserInfo:
            ui->UserInfoFrame->setVisible(true);
            ui->UserInfoFrame->raise();
            break;
        case InstallProgress:
            ui->InstallingFrame->setVisible(true);
            ui->InstallingFrame->raise();
            break;
        }

        ui->waitingFrame->raise();
        ui->sideFrame->raise();
    });

    animationGroup->start();
    ui->WelcomeFrame->setVisible(true);
    ui->ConfirmFrame->setVisible(true);
    ui->InstallFrame->setVisible(true);
    ui->MirrorlistFrame->setVisible(true);
    ui->ReadyToInstallFrame->setVisible(true);
    //ui->CompleteFrame->setVisible(true);
    ui->UserInfoFrame->setVisible(true);
}

void MainWindow::updatePartitionList() {

}

bool MainWindow::performSanityChecks(bool progressBar) {
    ui->backButton->setEnabled(false);
    ui->forwardButton->setEnabled(false);

    ui->SpaceLabel->setPixmap(QIcon::fromTheme("go-next").pixmap(16, 16));
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
        ui->PowerLabel->setPixmap(QIcon::fromTheme("dialog-ok").pixmap(16, 16));
    } else {
        ui->PowerLabel->setPixmap(QIcon::fromTheme("dialog-cancel").pixmap(16, 16));
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



    /*bool readLine;
    for (QString line : readMirrorlist) {
        if (readLine) {
            if (line == "") {
                readLine = false;
            } else {
                int index = readMirrorlist.indexOf(line);
                line.remove("#");
                readMirrorlist.replace(index, line);
            }
        } else {
            if (line.endsWith(ui->CountryDropDown->currentText(), Qt::CaseInsensitive)) {
                readLine = true;
            }
        }
    }*/

    QString finalMirrorList(editedMirrorlist.join("\n"));

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
    stage = Finish;

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


void MainWindow::on_partition_eraseDrive_clicked()
{
    EraseDriveDialog* dialog = new EraseDriveDialog(ui->driveBox->currentData().toString(), this);
    if (dialog->exec() == QDialog::Accepted) {
        on_forwardButton_clicked();
    }
}

void MainWindow::on_partition_manualMount_clicked()
{
    QProcess::startDetached("theterminal");
    if (QMessageBox::information(this, "Manual Mount", "Mount your drives on /mnt, and click OK when done.", QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Ok) == QMessageBox::Ok) {
        on_forwardButton_clicked();
    }
}

void MainWindow::install_error() {
    QMessageBox::critical(this, "Error", "An unexpected error occurred while trying to install theOS.\n\ntheOS Setup will now exit.", QMessageBox::Ok, QMessageBox::Ok);
    QApplication::exit(1);
}
