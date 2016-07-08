#include "partitionwindow.h"
#include "ui_partitionwindow.h"

PartitionWindow::PartitionWindow(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PartitionWindow)
{
    ui->setupUi(this);

    /*
    ui->partitions->clear();
    ui->forwardButton->setEnabled(false);

    QProcess *lsblk = new QProcess(this);
    lsblk->start("lsblk -rf");

    lsblk->waitForFinished();
    QByteArray output = lsblk->readAllStandardOutput();
    for (QString part : QString(output).split("\n")) {
        if (part != "") {
            QStringList parse = part.split(" ");
            if (parse.length() > 3) {
                if (part.split(" ")[1] == "ext4") {
                    if (parse[4] != "/") {

                        QListWidgetItem *i = new QListWidgetItem(ui->partitions);
                        i->setText("/dev/" + parse[0] + " " + parse[2]);
                        ui->partitions->addItem(i);
                    }
                }
            }
        }
    }

    if (ui->partitions->count() == 0) {
        ui->noPartitions->setVisible(true);
    } else {
        ui->noPartitions->setVisible(false);
    }*/
}

PartitionWindow::~PartitionWindow()
{
    delete ui;
}
