#include "partitionwindow.h"
#include "ui_partitionwindow.h"

extern QString calculateSize(quint64 size);

PartitionWindow::PartitionWindow(QString drive, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PartitionWindow)
{
    ui->setupUi(this);
    this->layout()->removeWidget(ui->newPartitionFrame);
    this->layout()->removeWidget(ui->resizePartitionFrame);
    ui->newPartitionFrame->setParent(this);
    ui->newPartitionFrame->setGeometry(10, this->height(), this->width() - 20, this->height() - 20);
    ui->resizePartitionFrame->setParent(this);
    ui->resizePartitionFrame->setGeometry(10, this->height(), this->width() - 20, this->height() - 20);
    ui->partitionTableRequiredFrame->setVisible(false);

    ui->newPartitionType->addItem("ext4");
    ui->newPartitionType->addItem("linuxswap");
    ui->newPartitionType->addItem("ntfs");
    ui->newPartitionType->addItem("fat32");

    this->drive = drive;
    driveInfo = DriveInfo::loadDrive(drive);
    if (driveInfo == NULL) { //No partition table found on device.
        bool ok = false;
        QString format = QInputDialog::getItem(parent, "Initialize Drive", "No valid partition table was found on this device. You'll need to initialize it first.\n\n"
                                                                           "All data on the disk will be erased.", QStringList() << "GPT" << "MBR", 0, false, &ok);
        if (ok) {
            DriveInfo::clobberDrive(drive, format == "GPT" ? DriveInfo::gpt : DriveInfo::mbr);
            driveInfo = DriveInfo::loadDrive(drive);
        } else {
            ui->MainFrame->setVisible(false);
            ui->pushButton->setEnabled(false);
            ui->partitionTableRequiredFrame->setVisible(true);
        }
    }

    this->reloadPartitions();

}

PartitionWindow::~PartitionWindow()
{
    delete ui;
}

void PartitionWindow::reloadPartitions() {
    if (driveInfo != 0x0) {
        int currentIndex = this->currentIndex;
        for (QObject* object : ui->driveLayout->layout()->children()) {
            frames.removeFirst();
            delete object;
        }

        if (partitionsFrame != NULL) {
            delete partitionsFrame;
        }

        partitionsFrame = new QFrame();

        QList<QRect> partitionSizes = driveInfo->getPanelSizes(ui->driveLayout->width(), ui->driveLayout->height());
        for (int i = 0; i < driveInfo->partitionCount(); i++) {
            DriveInfo::PartitionFormat type = driveInfo->getPartitionType(i);
            qulonglong size = driveInfo->getPartitionSize(i);
            QString label = driveInfo->getPartitionLabel(i);
            QString mountPoint = driveInfo->getPartitionMountPoint(i);
            PartitionFrame* frame = new PartitionFrame(type, size, label, mountPoint, partitionsFrame);
            frame->setGeometry(partitionSizes.at(i));
            frame->setNumber(i);
            if (i == currentIndex) {
                frame->setChecked(true);
            } else {
                frame->setChecked(false);
            }
            connect(frame, SIGNAL(uncheckAll()), this, SLOT(uncheckAllFrames()));
            connect(frame, SIGNAL(nowChecked(int)), this, SLOT(checked(int)));
            frames.append(frame);
        }
        ui->driveLayout->layout()->addWidget(partitionsFrame);

        ui->listWidget->clear();
        for (QVariantList operation : driveInfo->getOperations()) {
            if (operation.first().toString() == "del") {
                QListWidgetItem* item = new QListWidgetItem();
                item->setIcon(QIcon::fromTheme("edit-delete"));
                item->setText(operation.at(2).toString());
                ui->listWidget->addItem(item);
            } else if (operation.first().toString() == "new") {
                QListWidgetItem* item = new QListWidgetItem();
                item->setIcon(QIcon::fromTheme("list-add"));

                QString format;
                switch (operation.at(3).value<DriveInfo::PartitionFormat>()) {
                case DriveInfo::ext4:
                    format = "ext4";
                    break;
                case DriveInfo::swap:
                    format = "Swap";
                    break;
                case DriveInfo::ntfs:
                    format = "NTFS";
                    break;
                case DriveInfo::fat32:
                    format = "FAT32";
                }

                if (operation.at(4).toString() == "") {
                    item->setText(calculateSize(operation.at(2).toLongLong()) + " " + format);
                } else {
                    item->setText(calculateSize(operation.at(2).toLongLong()) + " " + format + " with label " + operation.at(4).toString());
                }
                ui->listWidget->addItem(item);
            } else if (operation.first().toString() == "size") {
                QListWidgetItem* item = new QListWidgetItem();
                item->setIcon(QIcon::fromTheme("go-last"));

                qulonglong newSize = operation.at(2).toLongLong();
                QString label = operation.at(3).toString();
                qulonglong currentSize = operation.at(4).toLongLong();

                item->setText(label + ": " + calculateSize(currentSize) + " > " + calculateSize(newSize));

                ui->listWidget->addItem(item);

            }
        }
        this->currentIndex = currentIndex;
    }
}

void PartitionWindow::resizeEvent(QResizeEvent *event) {
    this->reloadPartitions();
}

void PartitionWindow::on_pushButton_2_clicked()
{
    this->reject();
}

void PartitionWindow::on_pushButton_clicked()
{
    //Do some checks
    bool hasRootMount = false;
    for (int i = 0; i < driveInfo->partitionCount(); i++) {
        if (driveInfo->getPartitionMountPoint(i) == "/") {
            hasRootMount = true;
        }
    }

    if (!hasRootMount) {
        QMessageBox::warning(this, "No Root Partition", "You don't have a partition to be mounted on root. Select your root partition and set \"/\" as the mount point.", QMessageBox::Ok, QMessageBox::Ok);
        return;
    }

    if (QMessageBox::warning(this, "Applying Changes", "We're about to make changes to your disk. Ensure that the operations are correct, then click OK to apply the changes.\n\n"
                             "It's advisable to make a backup because we're doing some dangerous things here.", QMessageBox::Ok | QMessageBox::Abort, QMessageBox::Ok) == QMessageBox::Ok) {
        if (driveInfo->applyOperationList(this->drive) == DriveInfo::success) {
            this->accept();
        } else {
            QMessageBox::critical(this, "Error", "We couldn't apply your changes. You can try again, or you can use KDE Partition Manager from the Gateway or if you haven't started theShell, you can quit setup and launch it from the Utilities section", QMessageBox::Ok, QMessageBox::Ok);
        }
    }
}

void PartitionWindow::uncheckAllFrames() {
    for (QObject* object : partitionsFrame->children()) {
        ((PartitionFrame*) object)->setChecked(false);
    }
    this->currentIndex = -1;
    ui->mountPoint->setText("");
}

void PartitionWindow::checked(int index) {
    this->currentIndex = index;
    if (driveInfo->getPartitionType(index) == DriveInfo::freeSpace) {
        ui->removePartition->setVisible(false);
        ui->newPartition->setVisible(true);
        ui->mountPoint->setEnabled(false);
    } else if (driveInfo->getPartitionType(index) == DriveInfo::swap) {
        ui->removePartition->setVisible(true);
        ui->newPartition->setVisible(false);
        ui->mountPoint->setEnabled(false);
    } else {
        ui->removePartition->setVisible(true);
        ui->newPartition->setVisible(false);
        ui->mountPoint->setEnabled(true);
    }
    ui->mountPoint->setText(driveInfo->getPartitionMountPoint(index));
}

void PartitionWindow::on_removePartition_clicked()
{
    driveInfo->removePartition(this->currentIndex);
    reloadPartitions();
}

void PartitionWindow::on_newPartition_clicked()
{
    ui->newPartitionSize->setMaximum((double) driveInfo->getPartitionSize(this->currentIndex) / 1024 / 1024);
    ui->newPartitionSize->setValue((double) driveInfo->getPartitionSize(this->currentIndex) / 1024 / 1024);
    ui->newPartitionLabel->setText("");
    ui->newPartitionMount->setText("");

    QPropertyAnimation* anim = new QPropertyAnimation(ui->newPartitionFrame, "geometry");
    anim->setStartValue(ui->newPartitionFrame->geometry());
    anim->setEndValue(QRect(10, 10, this->width() - 20, this->height() - 20));
    anim->setEasingCurve(QEasingCurve::OutCubic);
    anim->setDuration(500);
    anim->start();
}

void PartitionWindow::on_newPartitionCancel_clicked()
{
    QPropertyAnimation* anim = new QPropertyAnimation(ui->newPartitionFrame, "geometry");
    anim->setStartValue(ui->newPartitionFrame->geometry());
    anim->setEndValue(QRect(10, this->height(), this->width() - 20, this->height() - 20));
    anim->setEasingCurve(QEasingCurve::OutCubic);
    anim->setDuration(500);
    anim->start();
}

void PartitionWindow::on_newPartitionAdd_clicked()
{
    DriveInfo::PartitionFormat format;
    if (ui->newPartitionType->currentText() == "ext4") {
        format = DriveInfo::ext4;
    } else if (ui->newPartitionType->currentText() == "linuxswap") {
        format = DriveInfo::swap;
    } else if (ui->newPartitionType->currentText() == "ntfs") {
        format = DriveInfo::ntfs;
    } else if (ui->newPartitionType->currentText() == "fat32") {
        format = DriveInfo::fat32;
    }

    driveInfo->addPartitionInFreeSpace(this->currentIndex, ui->newPartitionSize->value() * 1024 * 1024, format, ui->newPartitionLabel->text(), ui->newPartitionMount->text());
    reloadPartitions();

    QPropertyAnimation* anim = new QPropertyAnimation(ui->newPartitionFrame, "geometry");
    anim->setStartValue(ui->newPartitionFrame->geometry());
    anim->setEndValue(QRect(10, this->height(), this->width() - 20, this->height() - 20));
    anim->setEasingCurve(QEasingCurve::OutCubic);
    anim->setDuration(500);
    anim->start();
}

void PartitionWindow::on_mountPoint_textEdited(const QString &arg1)
{
    if (this->currentIndex != -1) {
        driveInfo->setPartitionMountPoint(this->currentIndex, arg1);
        reloadPartitions();
    }
}

void PartitionWindow::on_resizePartition_clicked()
{
    bool nextPartitionIsFreeSpace;
    if (driveInfo->partitionCount() > this->currentIndex + 1) {
        if (driveInfo->getPartitionType(this->currentIndex + 1) == DriveInfo::freeSpace) {
            nextPartitionIsFreeSpace = true;
        }
    }

    if (nextPartitionIsFreeSpace) {
        ui->resizePartitionSize->setMaximum((driveInfo->getPartitionSize(this->currentIndex) + driveInfo->getPartitionSize(this->currentIndex + 1)) / 1024 / 1024);
    } else {
        ui->resizePartitionSize->setMaximum(driveInfo->getPartitionSize(this->currentIndex) / 1024 / 1024);
    }

    ui->resizePartitionSize->setValue(driveInfo->getPartitionSize(this->currentIndex) / 1024 / 1024);

    QPropertyAnimation* anim = new QPropertyAnimation(ui->resizePartitionFrame, "geometry");
    anim->setStartValue(ui->resizePartitionFrame->geometry());
    anim->setEndValue(QRect(10, 10, this->width() - 20, this->height() - 20));
    anim->setEasingCurve(QEasingCurve::OutCubic);
    anim->setDuration(500);
    anim->start();
}


void PartitionWindow::on_resizePartitionCancel_clicked()
{
    QPropertyAnimation* anim = new QPropertyAnimation(ui->resizePartitionFrame, "geometry");
    anim->setStartValue(ui->resizePartitionFrame->geometry());
    anim->setEndValue(QRect(10, this->height(), this->width() - 20, this->height() - 20));
    anim->setEasingCurve(QEasingCurve::OutCubic);
    anim->setDuration(500);
    anim->start();
}

void PartitionWindow::on_resizePartitionButton_clicked()
{
    driveInfo->resizePartition(this->currentIndex, ui->resizePartitionSize->value() * 1024 * 1024);
    reloadPartitions();

    QPropertyAnimation* anim = new QPropertyAnimation(ui->resizePartitionFrame, "geometry");
    anim->setStartValue(ui->resizePartitionFrame->geometry());
    anim->setEndValue(QRect(10, this->height(), this->width() - 20, this->height() - 20));
    anim->setEasingCurve(QEasingCurve::OutCubic);
    anim->setDuration(500);
    anim->start();
}

void PartitionWindow::on_newPartitionType_currentTextChanged(const QString &arg1)
{
    if (arg1 == "linuxswap") {
        ui->newPartitionMount->setEnabled(false);
    } else {
        ui->newPartitionMount->setEnabled(true);
    }
}
