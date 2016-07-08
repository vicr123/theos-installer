#ifndef DRIVEINFO_H
#define DRIVEINFO_H

#include <QObject>
#include <QVariant>
#include <QRect>
#include <parted/parted.h>
#include <QDebug>
#include <QProcess>

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



signals:

public slots:
    OperationError addPartition(qlonglong size, PartitionFormat format, QString label = "");
    OperationError removePartition(int index);

    qulonglong getPartitionSize(int index);

    OperationError applyToDrive(QString drive);
    int partitionCount();
    QList<QRect> getPanelSizes(int width, int height);

private:
    qulonglong size;
    DriveFormat type;

    QList<QVariantList> partitions;

};

Q_DECLARE_METATYPE(DriveInfo::PartitionFormat)

#endif // DRIVEINFO_H
