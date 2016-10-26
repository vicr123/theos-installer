#ifndef DRIVEINFO_H
#define DRIVEINFO_H

#include <QObject>
#include <QVariant>
#include <QRect>
#include <parted/parted.h>
#include <QDebug>
#include <QProcess>
#include <QApplication>
#include <QDir>
#include <QIcon>
#include "doingpartitioning.h"

class DriveInfo : public QObject
{
    Q_OBJECT
public:
    enum DriveFormat {
        gpt = 0,
        mbr = 1
    };

    enum PartitionFormat {
        ext4 = 0,
        swap = 1,
        ntfs = 2,
        hfsplus = 3,
        fat32 = 4,
        efisys = 5,
        freeSpace = -1
    };

    enum OperationError {
        success,
        unknown,
        driveExtendsPastSize,
        tooManyPartitions,
        indexOutOfRange
    };
    explicit DriveInfo(qulonglong size, DriveFormat type, QObject *parent = 0);

    static DriveInfo* loadDrive(QString drive);
    static void clobberDrive(QString drive, DriveFormat format);

signals:

public slots:
    OperationError addPartition(qlonglong size, PartitionFormat format, QString label = "", QString MountPoint = "");
    OperationError addPartitionInFreeSpace(int index, qlonglong size, PartitionFormat format, QString label = "", QString MountPoint = "");
    OperationError removePartition(int index);
    OperationError resizePartition(int index, qulonglong newSize);

    qulonglong getPartitionSize(int index);
    PartitionFormat getPartitionType(int index);
    QString getPartitionLabel(int index);
    QString getPartitionMountPoint(int index);
    QList<QVariantList> getOperations();

    void setPartitionMountPoint(int index, QString mountPoint);

    OperationError applyToDriveErase(QString drive);
    OperationError applyOperationList(QString drive);
    int partitionCount();
    QList<QRect> getPanelSizes(int width, int height);

private:
    qulonglong size;
    DriveFormat type;

    bool addToOperationList = false;

    QList<QVariantList> partitions;
    QList<QVariantList> operations;

};

Q_DECLARE_METATYPE(DriveInfo::PartitionFormat)

#endif // DRIVEINFO_H
