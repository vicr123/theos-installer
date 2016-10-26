#include "partitionframe.h"

extern QString calculateSize(quint64 size);

PartitionFrame::PartitionFrame(DriveInfo::PartitionFormat type, qulonglong size, QString label, QString mountPoint, QWidget *parent) : QFrame(parent)
{
    this->type = type;
    this->size = size;
    this->label = label;
    this->mountPoint = mountPoint;
    this->setToolTip(this->label + "\n" + calculateSize(this->size));
    this->setFocusPolicy(Qt::ClickFocus);
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
        if (this->checked) {
            painter.setBrush(QBrush(QColor::fromRgb(0, 50, 255)));
        } else {
            painter.setBrush(QBrush(QColor::fromRgb(0, 100, 255)));
        }
        textColor = QColor::fromRgb(255, 255, 255);
        break;
    case DriveInfo::swap:
        if (this->checked) {
            painter.setBrush(QBrush(QColor::fromRgb(0, 200, 0)));
        } else {
            painter.setBrush(QBrush(QColor::fromRgb(0, 255, 0)));
        }
        textColor = QColor::fromRgb(0, 0, 0);
        break;
    case DriveInfo::freeSpace:
        if (this->checked) {
            painter.setBrush(QBrush(QColor::fromRgb(50, 50, 50)));
        } else {
            painter.setBrush(QBrush(QColor::fromRgb(0, 0, 0)));
        }
        textColor = QColor::fromRgb(255, 255, 255);
        break;
    case DriveInfo::ntfs:
        if (this->checked) {
            painter.setBrush(QBrush(QColor::fromRgb(200, 200, 0)));
        } else {
            painter.setBrush(QBrush(QColor::fromRgb(255, 255, 0)));
        }
        textColor = QColor::fromRgb(0, 0, 0);
        break;
    case DriveInfo::hfsplus:
        if (this->checked) {
            painter.setBrush(QBrush(QColor::fromRgb(200, 0, 0)));
        } else {
            painter.setBrush(QBrush(QColor::fromRgb(255, 0, 0)));
        }
        textColor = QColor::fromRgb(0, 0, 0);
        break;
    case DriveInfo::fat32:
    case DriveInfo::efisys:
        if (this->checked) {
            painter.setBrush(QBrush(QColor::fromRgb(0, 200, 200)));
        } else {
            painter.setBrush(QBrush(QColor::fromRgb(0, 255, 255)));
        }
        textColor = QColor::fromRgb(0, 0, 0);
        break;
    }
    QRect paintGeometry = this->geometry();
    paintGeometry.moveTo(0, 0);
    painter.drawRect(paintGeometry.adjusted(-1, -1, 0, 0));
    painter.setPen(textColor);

    if (this->type == DriveInfo::freeSpace) {
        painter.drawText(paintGeometry, Qt::AlignCenter, calculateSize(this->size));
    } else {
        if (this->mountPoint == "") {
            painter.drawText(paintGeometry, Qt::AlignCenter, this->label + "\n" + calculateSize(this->size));
        } else {
            painter.drawText(paintGeometry, Qt::AlignCenter, this->label + " (" + this->mountPoint + ")\n" + calculateSize(this->size));
        }
    }

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

void PartitionFrame::mouseReleaseEvent(QMouseEvent *event) {
    if (this->geometry().contains(this->mapToParent(event->pos()))) {
        emit uncheckAll();
        this->checked = true;
        this->repaint();
        emit nowChecked(this->number);
    }
}

void PartitionFrame::focusInEvent(QFocusEvent *event) {
    //hasFocus = true;
    this->repaint();
    event->accept();
}

void PartitionFrame::focusOutEvent(QFocusEvent *event) {
    //hasFocus = false;
    this->repaint();
    event->accept();
}

void PartitionFrame::setNumber(int number) {
    this->number = number;
}

void PartitionFrame::setChecked(bool checked) {
    this->checked = checked;
    this->repaint();
}
