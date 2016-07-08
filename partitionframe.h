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
    explicit PartitionFrame(DriveInfo::PartitionFormat type, qulonglong size, QString label, QWidget *parent = 0);

    void setGeometry(int x, int y, int height, int width);
    void setGeometry(QRect rect);
    QSize sizeHint();
signals:

public slots:
    void setPartitionType(DriveInfo::PartitionFormat type);
    void setPartitionSize(qulonglong size);
    void setPartitionLabel(QString label);

private:
    QString label;
    qulonglong size;
    DriveInfo::PartitionFormat type;

    void paintEvent(QPaintEvent* event);
};

#endif // PARTITIONFRAME_H
