#ifndef INTERNETCONNECTION_H
#define INTERNETCONNECTION_H

#include <QDialog>
#include <QNetworkConfiguration>
#include <QNetworkConfigurationManager>
#include <QNetworkSession>
#include <QListWidgetItem>
#include <QProcess>
#include <QStringList>
#include <QInputDialog>
#include <QThread>
//#include <KF5/NetworkManagerQt/NetworkManagerQt/WirelessDevice>
//#include <KF5/NetworkManagerQt/NetworkManagerQt/WiredDevice>
//#include <KF5/NetworkManagerQt/NetworkManagerQt/WirelessNetwork>
//#include <KF5/NetworkManagerQt/NetworkManagerQt/WirelessSecuritySetting>
//#include <KF5/NetworkManagerQt/NetworkManagerQt/WirelessSetting>


namespace Ui {
class InternetConnection;
}

class InternetConnection : public QDialog
{
    Q_OBJECT

public:

    explicit InternetConnection(QWidget *parent = 0);
    ~InternetConnection();

private slots:
    void on_pushButton_clicked();

    void on_pushButton_2_clicked();

    void on_listWidget_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous);

    void on_listWidget_itemClicked(QListWidgetItem *item);

    void processFinished(int exitCode);

private:
    Ui::InternetConnection *ui;
    QNetworkConfigurationManager manager;

    QProcess* nmcli;
    QStringList networks;
};

#endif // INTERNETCONNECTION_H
