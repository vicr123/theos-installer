#include "driveinfo.h"

DriveInfo::DriveInfo(qulonglong size, DriveFormat type, QObject *parent) : QObject(parent)
{
    this->size = size;
    this->type = type;
}

DriveInfo::OperationError DriveInfo::addPartition(qlonglong size, PartitionFormat format, QString label) {
    if (this->type == mbr) {
        if (partitions.count() >= 4) {
            return tooManyPartitions;
        }
    } else if (this->type == gpt) {
        if (partitions.count() >= 4) {
            return tooManyPartitions;
        }
    }

    qulonglong driveEndSize = size;
    for (QVariantList partition : partitions) {
        driveEndSize = driveEndSize + partition.first().toLongLong();
    }

    if (size == -1) {
        if (driveEndSize >= this->size) {
            return driveExtendsPastSize;
        }
    } else if (size < -1) {
        return driveExtendsPastSize;
    } else {
        if (driveEndSize > this->size) {
            return driveExtendsPastSize;
        }
    }

    if (size == -1) {
        size = this->size - driveEndSize;
    }

    QVariantList partitionInformation;
    partitionInformation.append(size);
    partitionInformation.append(format);
    partitionInformation.append(label);
    partitions.append(partitionInformation);

    return success;
}

DriveInfo::OperationError DriveInfo::removePartition(int index) {
    if (partitions.count() > index) {
        partitions.removeAt(index);
        return success;
    } else {
        return indexOutOfRange;
    }
}

DriveInfo::OperationError DriveInfo::applyToDrive(QString drive) {
    qDebug() << "-----------------------------------------";
    qDebug() << "Starting partitioning on /dev/" + drive;
    PedDevice* device = ped_device_get(QString("/dev/" + drive).toLocal8Bit()); //Create device
    ped_device_open(device); //Open device
    ped_disk_clobber(device); //Delete everything on device
    qDebug() << "Sector size:    " + QString::number(device->sector_size) + "B";
    qDebug() << "No. of sectors: " + QString::number(device->length) + " sectors";

    PedDisk* disk = ped_disk_new_fresh(device, ped_disk_type_get(this->type == gpt ? "gpt" : "msdos")); //Create disk

    qulonglong currentSize = 2048 * device->sector_size;
    for (QVariantList partition : partitions) { //Iterate over all partitions
        qulonglong size = partition.first().toLongLong(); //Get partition size
        PartitionFormat format = partition.at(1).value<PartitionFormat>();
        QString label = partition.at(2).toString();
        qDebug() << "New partition: " + label;
        qDebug() << "Size:          " + QString::number(size);

        PedSector startSector, endSector;
        startSector = currentSize / device->sector_size;
        if (startSector < 2048) { //Make sure partition starts at at least 2048
            size = size - 2048;
            startSector = 2048;
        }
        PedGeometry* geometry = ped_geometry_new(device, startSector, size / device->sector_size);
        if (geometry->end > device->length) {
            endSector = device->length - 2048;
        } else {
            endSector = geometry->end;
        }

        currentSize = currentSize + size;

        qDebug() << "Start Sector:  " + QString::number(startSector);
        qDebug() << "End Sector:    " + QString::number(endSector);
        qDebug() << "Length:        " + QString::number(size / device->sector_size);
        qDebug() << "Type:          " + QString::number(format);

        PedPartition* pedPartition;
        if (format == ext4) {
            pedPartition = ped_partition_new(disk, PED_PARTITION_NORMAL, ped_file_system_type_get("ext4"), startSector, endSector);
        } else if (format == swap) {
            pedPartition = ped_partition_new(disk, PED_PARTITION_NORMAL, ped_file_system_type_get("linux-swap"), startSector, endSector);
        }
        ped_partition_set_name(pedPartition, label.toLocal8Bit()); //Set label on disk
        if (ped_disk_add_partition(disk, pedPartition, ped_constraint_exact(&pedPartition->geom)) == 0) {
            qDebug() << "--FAILED TO ADD PARTITION.--";
        }

        qDebug() << "";
    }

    //ped_disk_print(disk);
    qDebug() << "Committing changes to disk...";
    int commit = ped_disk_commit(disk); //Commit all the changes
    ped_disk_destroy(disk);
    ped_device_close(device);
    ped_device_destroy(device);
    commit = 0; //libparted seems to not be working :(

    if (commit != 0) {
        qDebug() << "--COULDN'T COMMIT TO DISK--";
        qDebug() << "-----------------------------------------";
        return unknown;
    } else {
        qDebug() << "";
        qDebug() << "Creating file systems.";
        qDebug() << "";

        int i = 1;
        for (QVariantList partition : partitions) {
            PartitionFormat format = partition.at(1).value<PartitionFormat>();
            switch (format) {
            case ext4:
                QProcess::execute("mkfs.ext4 /dev/" + drive + QString::number(i) + " -F");
                break;
            case swap:
                QProcess::execute("mkswap /dev/" + drive + QString::number(i));
                break;
            }
            i++;
        }

        qDebug() << "Done.";
        qDebug() << "-----------------------------------------";
        return success;
    }
}

int DriveInfo::partitionCount() {
    return partitions.count();
}

QList<QRect> DriveInfo::getPanelSizes(int width, int height) {
    QList<QRect> ret;

    int currentWidth = 0;
    for (QVariantList info : partitions) {
        int panWidth = ((double) info.first().toLongLong() / (double) size) * width;
        QRect rect(currentWidth, 0, panWidth, height);
        ret.append(rect);
        currentWidth = currentWidth + panWidth;
    }

    return ret;
}

qulonglong DriveInfo::getPartitionSize(int index) {
    return partitions.at(index).first().toLongLong();
}
