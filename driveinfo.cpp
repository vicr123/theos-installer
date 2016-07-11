#include "driveinfo.h"

DriveInfo::DriveInfo(qulonglong size, DriveFormat type, QObject *parent) : QObject(parent)
{
    this->size = size;
    this->type = type;
}

DriveInfo::OperationError DriveInfo::addPartition(qlonglong size, PartitionFormat format, QString label, QString MountPoint) {
    int partitionCount = 0;
    for (QVariantList partition : partitions) {
        if (partition.at(1).value<DriveInfo::PartitionFormat>() != DriveInfo::freeSpace) {
            partitionCount++;
        }
    }
    if (this->type == mbr) {
        if (partitionCount >= 4) {
            return tooManyPartitions;
        }
    } else if (this->type == gpt) {
        if (partitionCount >= 128) {
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
    partitionInformation.append(MountPoint);
    partitions.append(partitionInformation);

    return success;
}

DriveInfo::OperationError DriveInfo::addPartitionInFreeSpace(int index, qlonglong size, PartitionFormat format, QString label, QString MountPoint) {
    int partitionCount = 0;
    for (QVariantList partition : partitions) {
        if (partition.at(1).value<DriveInfo::PartitionFormat>() != DriveInfo::freeSpace) {
            partitionCount++;
        }
    }
    if (this->type == mbr) {
        if (partitionCount >= 4) {
            return tooManyPartitions;
        }
    } else if (this->type == gpt) {
        if (partitionCount >= 128) {
            return tooManyPartitions;
        }
    }

    qulonglong driveEndSize = size;
    for (QVariantList partition : partitions) {
        driveEndSize = driveEndSize + partition.first().toLongLong();
    }

    if (size == -1) {

    } else {
        QVariantList partitionInformation;
        partitionInformation.append(size);
        partitionInformation.append(format);
        partitionInformation.append(label);
        partitionInformation.append(MountPoint);
        partitions.insert(index, partitionInformation);

        qulonglong freeSpaceSize = partitions.at(index + 1).first().toLongLong() - size;
        if (freeSpaceSize == 0) {
            partitions.removeAt(index + 1);
        } else {
            QVariantList partitionInformation = partitions.at(index + 1);
            partitionInformation.replace(0, freeSpaceSize);
            partitions.replace(index + 1, partitionInformation);
        }

        operations.append(QVariantList() << "new" << index << size << format << label);
        qDebug() << operations;
    }

    return success;
}

DriveInfo::OperationError DriveInfo::removePartition(int index) {
    if (addToOperationList) {
        QString label = partitions.at(index).at(2).toString();
        qulonglong space = partitions.at(index).first().toLongLong();
        bool nextPartitionIsFreeSpace = false, previousPartitionIsFreeSpace = false;
        if (partitions.count() > index + 1) {
            if (partitions.at(index + 1).at(1).value<DriveInfo::PartitionFormat>() == DriveInfo::freeSpace) {
                nextPartitionIsFreeSpace = true;
            }
        }

        if (index > 0) {
            if (partitions.at(index - 1).at(1).value<DriveInfo::PartitionFormat>() == DriveInfo::freeSpace) {
                previousPartitionIsFreeSpace = true;
            }
        }

        if (nextPartitionIsFreeSpace && previousPartitionIsFreeSpace) {
            qulonglong nextPartitionSpace = partitions.at(index + 1).first().toLongLong();
            partitions.removeAt(index + 1);

            partitions.removeAt(index);
            qulonglong previousPartitionSpace = partitions.at(index - 1).first().toLongLong();

            QVariantList partitionInformation = partitions.at(index - 1);
            partitionInformation.replace(1, DriveInfo::freeSpace);
            partitionInformation.replace(0, previousPartitionSpace + nextPartitionSpace + space);
            partitions.replace(index - 1, partitionInformation);
        } else if (nextPartitionIsFreeSpace) {
            qulonglong nextPartitionSpace = partitions.at(index + 1).first().toLongLong();
            partitions.removeAt(index + 1);

            QVariantList partitionInformation = partitions.at(index);
            partitionInformation.replace(1, DriveInfo::freeSpace);
            partitionInformation.replace(0, nextPartitionSpace + space);
            partitions.replace(index, partitionInformation);
        } else if (previousPartitionIsFreeSpace) {
            partitions.removeAt(index);
            qulonglong previousPartitionSpace = partitions.at(index - 1).first().toLongLong();

            QVariantList partitionInformation = partitions.at(index - 1);
            partitionInformation.replace(1, DriveInfo::freeSpace);
            partitionInformation.replace(0, previousPartitionSpace + space);
            partitions.replace(index - 1, partitionInformation);
        } else {
            QVariantList partitionInformation = partitions.at(index);
            partitionInformation.replace(1, DriveInfo::freeSpace);
            partitions.replace(index, partitionInformation);
        }

        operations.append(QVariantList() << "del" << index << label);
        qDebug() << operations;
        return success;
    } else {
        if (partitions.count() > index) {
            partitions.removeAt(index);
            return success;
        } else {
            return indexOutOfRange;
        }
    }
}

DriveInfo::OperationError DriveInfo::applyToDriveErase(QString drive) {
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
        qDebug() << "Creating file systems and mounting...";
        qDebug() << "";

        int i = 1;
        for (QVariantList partition : partitions) {
            PartitionFormat format = partition.at(1).value<PartitionFormat>();
            QString mountPoint = partition.at(3).toString();
            QProcess* formatProc = new QProcess();
            switch (format) {
            case ext4:
                formatProc->start("mkfs.ext4 /dev/" + drive + QString::number(i) + " -F");
                formatProc->waitForFinished(-1);
                formatProc->start("e2label /dev/" + drive + QString::number(i) + " \"" + partition.at(2).toString() + "\"");
                formatProc->waitForFinished(-1);

                if (mountPoint != "") {
                    formatProc->start("mount /dev/" + drive + QString::number(i) + " " + mountPoint);
                    formatProc->waitForFinished(-1);
                }
                break;
            case swap:
                formatProc->start("mkswap /dev/" + drive + QString::number(i));
                formatProc->waitForFinished(-1);
                formatProc->start("swapon /dev/" + drive + QString::number(i));
                formatProc->waitForFinished(-1);
                break;
            }

            while (formatProc->state() == QProcess::Running) {
                QApplication::processEvents();
            }

            i++;
        }

        qDebug() << "Done.";
        qDebug() << "-----------------------------------------";
        return success;
    }
}

DriveInfo::OperationError DriveInfo::applyOperationList(QString drive) {
    qDebug() << "-----------------------------------------";
    qDebug() << "Starting partitioning on /dev/" + drive;
    PedDevice* device = ped_device_get(QString("/dev/" + drive).toLocal8Bit()); //Create device
    ped_device_open(device); //Open device
    qDebug() << "Sector size:    " + QString::number(device->sector_size) + "B";
    qDebug() << "No. of sectors: " + QString::number(device->length) + " sectors";

    PedDisk* disk = ped_disk_new(device); //Create disk

    QProcess* operationProcess = new QProcess();
    QList<int> newPartitionNumbers;
    for (QVariantList params : operations) {
        if (params.first().toString() == "new") {
            qDebug() << "Now creating a new partition.";

            PedFileSystemType* fstype;
            switch (params.at(3).value<PartitionFormat>()) {
            case DriveInfo::ext4:
                fstype = ped_file_system_type_get("ext4");
                break;
            case DriveInfo::swap:
                fstype = ped_file_system_type_get("linuxswap");
                break;
            case DriveInfo::ntfs:
                fstype = ped_file_system_type_get("ntfs");
                break;
            case DriveInfo::fat32:
                fstype = ped_file_system_type_get("fat32");
                break;
            }

            PedSector startSector, endSector;
            PedPartition* partition = ped_disk_get_partition(disk, params.at(1).toInt() - 1);
            if (partition == NULL) {
                startSector = 2048;
            } else {
                startSector = partition->geom.end + 1;
            }

            if (startSector < 2048) { //Make sure partition starts at at least 2048
                size = size - 2048;
                startSector = 2048;
            }

            PedGeometry* geometry = ped_geometry_new(device, startSector, params.at(2).toLongLong() / device->sector_size);

            if (geometry->end > device->length) {
                endSector = device->length - 2048;
            } else {
                endSector = geometry->end;
            }

            PedPartition* diskPartition = ped_partition_new(disk, PED_PARTITION_NORMAL, fstype, startSector, endSector);
            ped_disk_add_partition(disk, diskPartition, ped_constraint_exact(&diskPartition->geom));
            newPartitionNumbers.append(diskPartition->num);
        } else if (params.first().toString() == "del") {
            qDebug() << "Now removing a partition.";
            PedPartition* partition = ped_disk_get_partition(disk, params.at(1).toInt() - 1);
            qDebug() << "Returns " << ped_disk_delete_partition(disk, partition);
        } else if (params.first().toString() == "size") {
            qDebug() << "Now resizing a partition.";
            //ped_disk_get_partition(disk, params.at(1).toInt());
        }
    }

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
        qDebug() << "Creating file systems...";
        qDebug() << "";

        int i = 0;
        for (QVariantList params : operations) {
            if (params.first().toString() == "new") {
                QString label = params.at(4).toString();
                PedFileSystemType* fstype;
                switch (params.at(3).value<PartitionFormat>()) {
                case DriveInfo::ext4:
                    operationProcess->start("mkfs.ext4 /dev/" + drive + QString::number(newPartitionNumbers.at(i)));
                    operationProcess->waitForFinished(-1);
                    operationProcess->start("e2label /dev/" + drive + QString::number(newPartitionNumbers.at(i)) + " \"" + label + "\"");
                    operationProcess->waitForFinished(-1);
                    break;
                case DriveInfo::swap:
                    operationProcess->start("mkswap /dev/" + drive + QString::number(newPartitionNumbers.at(i)));
                    operationProcess->waitForFinished(-1);
                    break;
                case DriveInfo::ntfs:
                    operationProcess->start("mkfs.ntfs -Q -L " + label + " /dev/" + drive + QString::number(newPartitionNumbers.at(i)));
                    operationProcess->waitForFinished(-1);
                    break;
                case DriveInfo::fat32:
                    operationProcess->start("mkfs.fat -F 32 /dev/" + drive + QString::number(newPartitionNumbers.at(i)));
                    operationProcess->waitForFinished(-1);
                    operationProcess->start("fatlabel /dev/" + drive + QString::number(newPartitionNumbers.at(i)) + " \"" + label + "\"");
                    operationProcess->waitForFinished(-1);
                    break;
                }
                i++;
            }
        }

        qDebug() << "Mounting file systems...";

        i = 1;
        for (QVariantList partition : partitions) {
            PartitionFormat format = partition.at(1).value<PartitionFormat>();
            if (format == freeSpace) {
                continue;
            }
            QString mountPoint = partition.at(3).toString();
            QString label = partition.at(2).toString();
            QProcess* mntProc = new QProcess();
            mntProc->setProcessChannelMode(QProcess::ForwardedChannels);
            switch (format) {
            case swap:
                mntProc->start("swapon /dev/" + drive + QString::number(i));
                mntProc->waitForFinished(-1);
                break;
            default:
                if (mountPoint != "") {
                    if (mountPoint != "/") {
                        QDir("/mnt/").mkdir(mountPoint.mid(1));
                    }
                    if (label == "") {
                        mntProc->start("mount /dev/" + drive + QString::number(i) + " /mnt" + mountPoint);
                    } else {
                        mntProc->start("mount /dev/disk/by-label/" + label + " /mnt" + mountPoint);
                    }
                    mntProc->waitForFinished(-1);
                }
                break;
            }

            i++;
        }

        qDebug() << "Done.";
        qDebug() << "-----------------------------------------";
        return success;
    }
}

DriveInfo::OperationError DriveInfo::resizePartition(int index, qulonglong newSize) {
    if (addToOperationList) {
        QString label = partitions.at(index).at(2).toString();
        qulonglong currentSize = partitions.at(index).first().toLongLong();
        qlonglong difference = currentSize - newSize;
        bool nextPartitionIsFreeSpace = false;
        if (partitions.count() > index + 1) {
            if (partitions.at(index + 1).at(1).value<DriveInfo::PartitionFormat>() == DriveInfo::freeSpace) {
                nextPartitionIsFreeSpace = true;
            }
        }

        if (nextPartitionIsFreeSpace) {
            qulonglong nextPartitionSpace = partitions.at(index + 1).first().toLongLong();
            qulonglong nextPartitionFinalSpace = nextPartitionSpace + difference;
            if (nextPartitionFinalSpace == 0) {
                partitions.removeAt(index + 1);
            } else {
                QVariantList partitionInformation = partitions.at(index + 1);
                partitionInformation.replace(0, nextPartitionFinalSpace);
                partitions.replace(index + 1, partitionInformation);
            }

            QVariantList partitionInformation = partitions.at(index);
            partitionInformation.replace(0, newSize);
            partitions.replace(index, partitionInformation);
        } else {
            QVariantList newPartitionInformation;
            newPartitionInformation.append(difference);
            newPartitionInformation.append(freeSpace);
            newPartitionInformation.append("");
            newPartitionInformation.append("");
            partitions.insert(index + 1, newPartitionInformation);

            QVariantList partitionInformation = partitions.at(index);
            partitionInformation.replace(0, newSize);
            partitions.replace(index, partitionInformation);
        }

        operations.append(QVariantList() << "size" << index << newSize << label);
        qDebug() << operations;
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

DriveInfo::PartitionFormat DriveInfo::getPartitionType(int index) {
    return partitions.at(index).at(1).value<PartitionFormat>();
}

QString DriveInfo::getPartitionLabel(int index) {
    return partitions.at(index).at(2).toString();
}

QString DriveInfo::getPartitionMountPoint(int index) {
    return partitions.at(index).at(3).toString();
}

void DriveInfo::setPartitionMountPoint(int index, QString mountPoint) {
    QVariantList partitionInformation = partitions.at(index);
    partitionInformation.replace(3, mountPoint);
    partitions.replace(index, partitionInformation);
}

DriveInfo* DriveInfo::loadDrive(QString drive) {
    QProcess *lsblk = new QProcess();
    lsblk->start("lsblk -rb");

    qulonglong driveSpace;
    lsblk->waitForFinished();
    QByteArray output = lsblk->readAllStandardOutput();
    for (QString part : QString(output).split("\n")) {
        if (part != "") {
            QStringList parse = part.split(" ");
            if (parse.first() == drive) {
                driveSpace = parse.at(3).toLongLong();
            }
        }
    }

    //ped_device_probe_all();

    qDebug() << "-----------------------------------------";
    qDebug() << "Loading partition table on /dev/" + drive;

    PedDevice* device = ped_device_get(QString("/dev/" + drive).toLocal8Bit()); //Create device
    ped_device_open(device); //Open device

    PedDiskType* type = ped_disk_probe(device);

    DriveInfo* info;
    if (type == NULL || type == 0x0) { //No partition table on device.
        return NULL;
    } else if (strcmp(type->name, "msdos") == 0) {
        info = new DriveInfo(driveSpace, DriveInfo::mbr);
        qDebug() << "This is a MBR style drive.";
    } else {
        info = new DriveInfo(driveSpace, DriveInfo::gpt);
        qDebug() << "This is a GPT style drive.";
    }

    PedDisk* disk = ped_disk_new(device); //Open disk

    PedPartition* nextPartition = NULL;
    int i = 1;
    nextPartition = ped_disk_next_partition(disk, nextPartition);
    while (nextPartition != NULL) {
        qDebug() << "New Partition.";
        qDebug() << "Start Sector: " + QString::number(nextPartition->geom.start);
        qDebug() << "End Sector:   " + QString::number(nextPartition->geom.end);
        DriveInfo::PartitionFormat partitionType;
        PedFileSystemType* fs = ped_file_system_probe(&nextPartition->geom);

        QString label = "Drive Partition";
        QProcess* labelFinder = new QProcess();

        if (fs == NULL) {
            partitionType = DriveInfo::freeSpace;
        } else {
            if (fs == ped_file_system_type_get("ext4")) {
                partitionType = DriveInfo::ext4;
            } else if (fs == ped_file_system_type_get("linuxswap")) {
                partitionType = DriveInfo::swap;
                label = "Swap";
            } else if (fs == ped_file_system_type_get("ntfs")) {
                partitionType = DriveInfo::ntfs;
            } else if (fs == ped_file_system_type_get("hfs+")) {
                partitionType = DriveInfo::hfsplus;
            } else if (fs == ped_file_system_type_get("fat32")) {
                partitionType = DriveInfo::fat32;
            } else {
                partitionType = DriveInfo::swap;
            }

            for (QFileInfo info : QDir("/dev/disk/by-label/").entryInfoList()) {
                if (info.symLinkTarget() == "/dev/" + drive + QString::number(nextPartition->num)) {
                    label = info.fileName().replace("\\x20", " ");
                }
            }

            //ped_file_system_close(fs);
        }
        delete labelFinder;

        info->addPartition(nextPartition->geom.length * device->sector_size, partitionType, label);
        nextPartition = ped_disk_next_partition(disk, nextPartition);
        i++;
    }

    ped_disk_destroy(disk);
    ped_device_close(device);
    ped_device_destroy(device);

    info->addToOperationList = true;

    return info;
}

void DriveInfo::clobberDrive(QString drive, DriveFormat format) {
    PedDevice* device = ped_device_get(QString("/dev/" + drive).toLocal8Bit()); //Create device
    ped_device_open(device); //Open device
    PedDisk* disk = ped_disk_new_fresh(device, ped_disk_type_get(format == gpt ? "gpt" : "msdos")); //Create disk
    ped_disk_commit(disk);
    ped_disk_destroy(disk);
    ped_device_close(device);
    ped_device_destroy(device);
}

QList<QVariantList> DriveInfo::getOperations() {
    return this->operations;
}
