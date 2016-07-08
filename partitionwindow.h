#ifndef PARTITIONWINDOW_H
#define PARTITIONWINDOW_H

#include <QDialog>

namespace Ui {
class PartitionWindow;
}

class PartitionWindow : public QDialog
{
    Q_OBJECT

public:
    explicit PartitionWindow(QWidget *parent = 0);
    ~PartitionWindow();

private:
    Ui::PartitionWindow *ui;
};

#endif // PARTITIONWINDOW_H
