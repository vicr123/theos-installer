#ifndef PARTITIONFRAME_H
#define PARTITIONFRAME_H

#include <QFrame>
#include <QPainter>
#include <QPaintEvent>
#include "driveinfo.h"

class PartitionFrame : public QFrame
{
    Q_OBJECT
public:
    explicit PartitionFrame(DriveInfo::PartitionFormat type, qulonglong size, QString label, QString mountPoint = "", QWidget *parent = 0);

    void setGeometry(int x, int y, int height, int width);
    void setGeometry(QRect rect);
    QSize sizeHint();
signals:
    void uncheckAll();
    void nowChecked(int index);
    void focused();

public slots:
    void setPartitionType(DriveInfo::PartitionFormat type);
    void setPartitionSize(qulonglong size);
    void setPartitionLabel(QString label);
    void setNumber(int number);
    void setChecked(bool checked);

private:
    QString label;
    QString mountPoint;
    qulonglong size;
    DriveInfo::PartitionFormat type;
    int number;

    bool checked;

    void paintEvent(QPaintEvent* event);
    void focusInEvent(QFocusEvent* event);
    void focusOutEvent(QFocusEvent* event);
    void mouseReleaseEvent(QMouseEvent* event);
};

#endif // PARTITIONFRAME_H
