#include "erasedrivedialog.h"
#include "ui_erasedrivedialog.h"

extern QString calculateSize(quint64 size);

EraseDriveDialog::EraseDriveDialog(QString drive, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::EraseDriveDialog)
{
    ui->setupUi(this);
    this->drive = drive;

    ui->explanationLabel->setText("/dev/" + drive + " will be erased and formatted like the following.");


    QProcess *lsblk = new QProcess(this);
    lsblk->start("lsblk -rb");

    lsblk->waitForFinished();
    QByteArray output = lsblk->readAllStandardOutput();
    for (QString part : QString(output).split("\n")) {
        if (part != "") {
            QStringList parse = part.split(" ");
            if (parse.first() == drive) {
                driveSpace = parse.at(3).toLongLong();
            }
        }
    }

    qulonglong ramSize;
    QFile ramFile("/proc/meminfo");
    ramFile.open(QFile::ReadOnly);
    QString ramInfo(ramFile.readAll());
    for (QString line : ramInfo.split("\n")) {
        if (line.contains("MemTotal:")) {
            ramSize = line.split(" ", QString::SkipEmptyParts).at(1).toLongLong() * 1024;
        }
    }

    if (driveSpace < 6442450944) {//6 GB
        showError("There isn't enough space on /dev/" + drive + " to install theOS.");
    }

    driveInfo = new DriveInfo(driveSpace, DriveInfo::gpt);

    if (driveInfo->addPartition(driveSpace - ramSize, DriveInfo::ext4, "theOS") == DriveInfo::driveExtendsPastSize) {
        showError("There isn't enough space on /dev/" + drive + " to install theOS.");
        return;
    }
    if (driveInfo->addPartition(-1, DriveInfo::swap) == DriveInfo::driveExtendsPastSize) {
        showError("There isn't enough space on /dev/" + drive + " to install theOS.");
        return;
    }
}

EraseDriveDialog::~EraseDriveDialog()
{
    delete ui;
}

void EraseDriveDialog::showError(QString error) {
    ui->explanationLabel->setText(error);
    ui->driveLayout->setVisible(false);
    ui->pushButton->setEnabled(false);
    this->error = true;
}

void EraseDriveDialog::resizeEvent(QResizeEvent *event) {
    if (!error) {
        for (QObject* child : driveInfo->children()) {
            delete child;
        }

        QList<QRect> partitionSizes = driveInfo->getPanelSizes(ui->driveLayout->width(), ui->driveLayout->height());
        PartitionFrame* ext4Partition = new PartitionFrame(DriveInfo::ext4, driveInfo->getPartitionSize(0), "theOS", ui->driveLayout);
        PartitionFrame* swapPartition = new PartitionFrame(DriveInfo::swap, driveInfo->getPartitionSize(1), "Swap", ui->driveLayout);

        ext4Partition->setGeometry(partitionSizes.at(0));
        swapPartition->setGeometry(partitionSizes.at(1));
    }
}

void EraseDriveDialog::on_pushButton_2_clicked()
{
    this->reject();
}

void EraseDriveDialog::on_pushButton_clicked()
{
    DriveInfo::OperationError error = driveInfo->applyToDrive(this->drive);
    if (error == DriveInfo::success) {
        QProcess::execute("mount /dev/" + this->drive + "1 /mnt");
        QProcess::execute("swapon /dev/" + this->drive + "2");
        this->accept();
    } else {
        QMessageBox::critical(this, "Couldn't change partitions!", "We couldn't apply your partition changes.");
    }
}
