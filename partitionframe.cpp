#include "partitionframe.h"

extern QString calculateSize(quint64 size);

PartitionFrame::PartitionFrame(DriveInfo::PartitionFormat type, qulonglong size, QString label, QWidget *parent) : QFrame(parent)
{
    this->type = type;
    this->size = size;
    this->label = label;
    this->setToolTip(this->label + "\n" + calculateSize(this->size));
}

void PartitionFrame::setPartitionType(DriveInfo::PartitionFormat type) {
    this->type = type;
}

void PartitionFrame::setPartitionSize(qulonglong size) {
    this->size = size;
    this->setToolTip(this->label + "\n" + calculateSize(this->size));
}

void PartitionFrame::setPartitionLabel(QString label) {
    this->label = label;
    this->setToolTip(this->label + "\n" + calculateSize(this->size));
}

void PartitionFrame::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    QColor textColor;
    switch(this->type) {
    case DriveInfo::ext4:
        painter.setBrush(QBrush(QColor::fromRgb(0, 100, 255)));
        textColor = QColor::fromRgb(255, 255, 255);
        break;
    case DriveInfo::swap:
        painter.setBrush(QBrush(QColor::fromRgb(0, 255, 0)));
        textColor = QColor::fromRgb(0, 0, 0);
        break;
    case DriveInfo::freeSpace:
        painter.setBrush(QBrush(QColor::fromRgb(0, 0, 0)));
        textColor = QColor::fromRgb(255, 255, 255);
        break;
    case DriveInfo::ntfs:
        painter.setBrush(QBrush(QColor::fromRgb(255, 255, 0)));
        textColor = QColor::fromRgb(0, 0, 0);
        break;
    }
    QRect paintGeometry = this->geometry();
    paintGeometry.moveTo(0, 0);
    painter.drawRect(paintGeometry.adjusted(-1, -1, 0, 0));
    painter.setPen(textColor);
    painter.drawText(paintGeometry, Qt::AlignCenter, this->label + "\n" + calculateSize(this->size));

    event->accept();
}

void PartitionFrame::setGeometry(int x, int y, int width, int height) {
    QFrame::setGeometry(x, y, height, width);

    this->setFixedSize(width, height);
}

void PartitionFrame::setGeometry(QRect rect) {
    this->setGeometry(rect.x(), rect.y(), rect.width(), rect.height());
}

QSize PartitionFrame::sizeHint() {
    return this->geometry().size();
}
