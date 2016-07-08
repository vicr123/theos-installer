#ifndef ERASEDRIVEDIALOG_H
#define ERASEDRIVEDIALOG_H

#include <QDialog>
#include <QProcess>
#include <QMessageBox>
#include "driveinfo.h"
#include "partitionframe.h"

namespace Ui {
class EraseDriveDialog;
}

class EraseDriveDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EraseDriveDialog(QString drive, QWidget *parent = 0);
    ~EraseDriveDialog();

    void resizeEvent(QResizeEvent* event);

private slots:
    void on_pushButton_2_clicked();

    void on_pushButton_clicked();

private:
    Ui::EraseDriveDialog *ui;
    qlonglong driveSpace;
    DriveInfo* driveInfo;
    QString drive;

    void showError(QString error);
    bool error = false;
};

#endif // ERASEDRIVEDIALOG_H
