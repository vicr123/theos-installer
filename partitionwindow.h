#ifndef PARTITIONWINDOW_H
#define PARTITIONWINDOW_H

#include <QDialog>
#include "driveinfo.h"
#include "partitionframe.h"
#include <QPropertyAnimation>
#include <QMessageBox>
#include <QInputDialog>

namespace Ui {
class PartitionWindow;
}

class PartitionWindow : public QDialog
{
    Q_OBJECT

public:
    explicit PartitionWindow(QString drive, QWidget *parent = 0);
    ~PartitionWindow();

public slots:
    void reloadPartitions();

private slots:
    void on_pushButton_2_clicked();

    void on_pushButton_clicked();

    void uncheckAllFrames();

    void checked(int index);

    void on_removePartition_clicked();

    void on_newPartition_clicked();

    void on_newPartitionCancel_clicked();

    void on_newPartitionAdd_clicked();

    void on_mountPoint_textEdited(const QString &arg1);

    void on_resizePartition_clicked();

    void on_resizePartitionCancel_clicked();

    void on_resizePartitionButton_clicked();

private:
    QString drive;
    DriveInfo* driveInfo;
    int currentIndex = -1;
    QFrame* partitionsFrame = NULL;
    Ui::PartitionWindow *ui;


    QList<PartitionFrame*> frames;

    void resizeEvent(QResizeEvent* event);
};

#endif // PARTITIONWINDOW_H
