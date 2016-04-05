#include "internetconnection.h"
#include "ui_internetconnection.h"

InternetConnection::InternetConnection(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::InternetConnection)
{
    ui->setupUi(this);
    ui->progressBar->setVisible(false);

    nmcli = new QProcess(this);
    connect(nmcli, SIGNAL(finished(int)), this, SLOT(processFinished(int)));

    //nmcli->start("nmcli d wifi rescan");
    //nmcli->waitForFinished();

    nmcli->start("nmcli -t -f SSID,SIGNAL,SECURITY d wifi list");
    nmcli->waitForFinished();
    QString output(nmcli->readAllStandardOutput());
    QStringList brokenOutput = output.split("\n");
    brokenOutput.removeAt(0);

    for (QString network : brokenOutput) {
        QString data;
        int i;
        for (QString part : network.split(":")) {
                switch (i) {
                case 0:
                    if (part == "--") {
                        data.append("Hidden network,");
                    } else {
                        data.append(part + ",");
                    }
                    break;
                case 1:
                    data.append(part + ",");
                    break;
                case 2:
                    if (part.contains("WPA2")) {
                        data.append("WPA2");
                    } else if (part.contains("WPA1")) {
                        data.append("WPA1");
                    } else if (part.contains("--")) {
                        data.append("NONE");
                    } else {
                        data.append("UNKNOWN");
                    }
                }
                i++;
        }
        if (data.split(",").count() >= 3) {
            networks.append(data);
        }
        i = 0;
    }

    for (QString network : networks) {
        if (network != "") {
            QStringList parts = network.split(",");
            if (parts.count() >= 3) {
            QListWidgetItem *item = new QListWidgetItem();

            item->setText(parts.at(0) + (parts.at(2) == "NONE" ? " Not Secured" : " Secured with " + parts.at(2)));

            ui->listWidget->addItem(item);
            }
        }
    }
}

InternetConnection::~InternetConnection()
{
    delete ui;
}

void InternetConnection::on_pushButton_clicked()
{
    ui->progressBar->setVisible(true);
    /*QNetworkConfiguration configuration;
    QList<QNetworkConfiguration> nc = manager.allConfigurations();

    for (QNetworkConfiguration &network : nc)
    {
        if (network.bearerType() == QNetworkConfiguration::BearerWLAN) {
            if (network.name() == ui->listWidget->selectedItems().at(0)->text())
                configuration = network;
        }
    }

    QNetworkSession *session = new QNetworkSession(configuration, this);
    session->open();
    session->waitForOpened();
    this->close();*/
    ui->listWidget->setEnabled(false);
    ui->pushButton->setEnabled(false);
    ui->pushButton_2->setEnabled(false);
    int selected = ui->listWidget->selectionModel()->selectedIndexes().at(0).row();
    QStringList parts = networks.at(selected).split(",");

    QString password;

    if (parts.at(2) != "NONE") {
        password = QInputDialog::getText(this, "Enter Key", parts.at(0) + " is protected with a key. Please enter the key to connect to this network.", QLineEdit::Password);
        if (password == "") {
            ui->listWidget->setEnabled(true);
            ui->pushButton->setEnabled(true);
            ui->pushButton_2->setEnabled(true);
            ui->progressBar->setVisible(false);
            return;
        }
    }

    nmcli->start("nmcli d wifi connect \"" + parts.at(0) + "\" password \"" + password + "\"");
    while (nmcli->state() != 0) {
        QApplication::processEvents();
    }

    qDebug() << nmcli->exitCode();
    //nmcli->waitForFinished();

    //qDebug() << nmcli->state();

    this->close();
}

void InternetConnection::on_pushButton_2_clicked()
{
    this->close();
}

void InternetConnection::on_listWidget_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous)
{

    if (ui->listWidget->selectedItems().length() != 0) {
        ui->pushButton->setEnabled(true);
    } else {
        ui->pushButton->setEnabled(false);
    }
}

void InternetConnection::on_listWidget_itemClicked(QListWidgetItem *item)
{
    if (ui->listWidget->selectedItems().length() != 0) {
        ui->pushButton->setEnabled(true);
    } else {
        ui->pushButton->setEnabled(false);
    }

}

void InternetConnection::processFinished(int exitCode) {

}
