#include "installworker.h"

installWorker::installWorker(MainWindow *parent) {
    parentWindow = parent;
}

// --- DECONSTRUCTOR ---
installWorker::~installWorker() {
}

void installWorker::process() {
    p = new QProcess(this);
    //p->setProcessChannelMode(QProcess::MergedChannels);
    connect(p, SIGNAL(finished(int)), this, SLOT(lastProcessFinished(int)));
    connect(p, SIGNAL(readyReadStandardOutput()), this, SLOT(outputAvaliable()));
    connect(p, SIGNAL(readyReadStandardError()), this, SLOT(errorAvaliable()));

    standardOutput.append("[theos_installer] Executing command umount /mnt\n");
    emit output(standardOutput);

    p->start("umount", QStringList() << "/mnt");
    p->waitForFinished(-1);

    if (parentWindow->formatPartition) {
        lastProcessDone = false;
        emit message("Formatting " + parentWindow->partition + "...");

        standardOutput.append("[theos_installer] Executing command mkfs -t ext4 -F -F " + parentWindow->partition);
        p->start("mkfs -t ext4 -F -F " + parentWindow->partition);
        if (!p->waitForStarted()) {
            standardOutput.append("[theos_installer] Error occurred executing command!\n");
        }

        p->waitForFinished(-1);
    }

    emit message("Mounting " + parentWindow->partition + "...");
    standardOutput.append("[theos_installer] Executing command mount " + parentWindow->partition + " /mnt\n");
    emit output(standardOutput);

    p->start("mount " + parentWindow->partition + " /mnt");
    p->waitForFinished(-1);

    if (QDir("/sys/firmware/efi").exists()) {
        standardOutput.append("This system is EFI, attempting to mount ESP onto /boot\n");
        emit output(standardOutput);

        QProcess* lsblk = new QProcess();
        lsblk->start("lsblk -r --output NAME,PARTTYPE");
        lsblk->waitForStarted(-1);
        lsblk->waitForFinished(-1);

        QString lsblkOutput(lsblk->readAllStandardOutput());

        for (QString partition : lsblkOutput.split("\n")) {
            if (partition.split(" ").count() != 1) {
                if (partition.split(" ").at(1).contains("C12A7328-F81F-11D2-BA4B-00A0C93EC93B", Qt::CaseInsensitive)) {
                    QDir("/mnt").mkdir("boot");

                    emit message("Mounting " + partition.split(" ").at(0) + "...");
                    standardOutput.append("[theos_installer] Executing command mount " + parentWindow->partition + " /mnt/boot\n");
                    emit output(standardOutput);


                    p->start("mount /dev/" + partition.split(" ").at(0) + " /mnt/boot");
                    p->waitForFinished(-1);
                    break;
                }
            }
        }


    }

    emit message("Downloading and copying new files...");
    standardOutput.append("[theos_installer] Executing command pacstrap /mnt base base-devel\n");
    emit output(standardOutput);

    p->start("pacstrap /mnt base base-devel");
    p->waitForFinished(-1);
    if (p->exitCode() != 0) {
        emit message("An error occurred. Inspect the output to see what happened.");
        emit failed();
        return;
    }

    emit message("Configuring system...");
    standardOutput.append("[theos_installer] Generating fstab...\n");
    emit output(standardOutput);
    QProcess *fstab = new QProcess(this);
    fstab->start("genfstab -p /mnt");
    fstab->waitForFinished();

    QFile fstabFile("/mnt/etc/fstab");
    fstabFile.open(QFile::WriteOnly);
    fstabFile.write(fstab->readAllStandardOutput());
    fstabFile.close();

    standardOutput.append("[theos_installer] Setting hostname...\n");

    while (parentWindow->hostname == "") {} //Stall until hostname is ready

    QFile hostnameFile("/mnt/etc/hostname");
    hostnameFile.open(QFile::WriteOnly);
    hostnameFile.write(parentWindow->hostname.toUtf8());

    standardOutput.append("[theos_installer] Generating locales...\n");
    QFile::copy("/etc/locale.gen", "/mnt/etc/locale.gen");

    standardOutput.append("[theos_installer] Executing command arch-chroot /mnt locale-gen \n");
    emit output(standardOutput);

    p->start("arch-chroot /mnt locale-gen");
    p->waitForFinished(-1);


    standardOutput.append("[theos_installer] Executing command arch-chroot /mnt mkinitcpio -p linux\n");
    emit output(standardOutput);

    p->start("arch-chroot /mnt mkinitcpio -p linux");
    p->waitForFinished(-1);

    emit message("Downloading and installing bootloader...");


    standardOutput.append("[theos_installer] Executing command pacstrap /mnt os-prober grub\n");
    emit output(standardOutput);

    p->start("pacstrap /mnt os-prober efibootmgr grub");
    p->waitForFinished(-1);

    QString disk = parentWindow->partition;
    disk.chop(1);

    if (QDir("/sys/firmware/efi").exists()) {
        standardOutput.append("[theos_installer] Executing command arch-chroot /mnt grub-install --target=x86_64-efi --efi-directory=/boot/ --bootloader-id=grub\n");

        p->start("arch-chroot /mnt grub-install --target=x86_64-efi --efi-directory=/boot/ --bootloader-id=grub");
        p->waitForFinished(-1);
    } else {
        standardOutput.append("[theos_installer] Executing command arch-chroot /mnt grub-install --target=i386-pc " + disk + "\n");
        emit output(standardOutput);
        p->start("arch-chroot /mnt grub-install --target=i386-pc " + disk);
        p->waitForFinished(-1);
    }

    QFile grubDefault("/mnt/etc/default/grub");
    grubDefault.open(QFile::ReadOnly);
    QString grubDefaults(grubDefault.readAll());
    grubDefault.close();

    QStringList grubDefaultsArray = grubDefaults.split("\n");
    for (QString line : grubDefaultsArray) {
        if (line.startsWith("GRUB_CMDLINE_LINUX_DEFAULT")) {
            int index = grubDefaultsArray.indexOf(line);
            grubDefaultsArray.removeAt(index);
            grubDefaultsArray.insert(index, "GRUB_CMDLINE_LINUX_DEFAULT=\"quiet splash\"");
        }
    }

    grubDefaults = "";
    for (QString line : grubDefaultsArray) {
        grubDefaults.append(line + "\n");
    }

    grubDefault.open(QFile::WriteOnly);
    grubDefault.write(grubDefaults.toUtf8());
    grubDefault.close();

    standardOutput.append("[theos_installer] Executing command arch-chroot /mnt grub-mkconfig -o /boot/grub/grub.cfg\n");
    emit output(standardOutput);

    p->start("arch-chroot /mnt grub-mkconfig -o /boot/grub/grub.cfg");
    p->waitForFinished(-1);

    QFile grubConfig("/mnt/boot/grub/grub.cfg");
    grubConfig.open(QFile::ReadWrite);
    QString grubConfiguration(grubConfig.readAll());
    grubConfig.close();
    grubConfiguration = grubConfiguration.replace("Arch Linux", "theOS");
    grubConfig.open(QFile::ReadWrite);
    grubConfig.write(grubConfiguration.toUtf8());
    grubConfig.close();

    emit message("Downloading and installing new files...");
    standardOutput.append("[theos_installer] Installing additional packages...\n");
    emit output(standardOutput);

    p->start(QString("pacstrap /mnt xf86-video-vesa xf86-video-intel xf86-video-nouveau xf86-video-vmware")
             .append(" virtualbox-guest-utils xorg-server xorg-xinit xf86-input-synaptics lightdm")
             .append(" networkmanager gtk3 breeze-gtk chromium konsole kinfocenter partitionmanager ntfs-3g")
             .append(" hfsprogs dolphin kate bluez bluedevil libreoffice-fresh hunspell hunspell-en amarok dragon kdegraphics-okular")
             .append(" ksuperkey kscreen user-manager kdeconnect gstreamer0.10 gstreamer0.10-bad gstreamer0.10-plugins")
             .append(" gstreamer0.10-base gstreamer0.10-base-plugins gstreamer0.10-ffmpeg gstreamer0.10-good")
             .append(" gstreamer0.10-good-plugins gstreamer0.10-ugly gstreamer0.10-ugly-plugins gst-plugins-good")
             .append(" gst-plugins-ugly kmail korganizer cups ark kcalc gwenview alsa-utils pulseaudio pulseaudio-alsa"));
    p->waitForFinished(-1);

    QDir localPackagesDir("/root/.packages/");
    QDirIterator *packageIterator = new QDirIterator(localPackagesDir);
    while (packageIterator->hasNext()) {
        QString packageName = packageIterator->next();
        QString packageToInstall = packageName;
        QString installLocation = "/mnt/var/cache/pacman/pkg/" + packageName.remove("/root/.packages/");
        if (!QFile::copy(packageToInstall, installLocation)) {
            standardOutput.append("[theos_installer] Error copying " + packageToInstall + " to " + installLocation);
            emit output(standardOutput);
        }
        p->start("arch-chroot /mnt pacman -U --noconfirm " + installLocation.remove(0, 4));
        p->waitForFinished(-1);
    }

    emit message("Configuring System...");
    standardOutput.append("[theos_installer] Configuring users...\n");

    QFile chfnDefault("/mnt/etc/login.defs");
    chfnDefault.open(QFile::ReadOnly);
    QString chfnDefaults(grubDefault.readAll());
    chfnDefault.close();

    QStringList chfnDefaultsArray = chfnDefaults.split("\n");
    for (QString line : chfnDefaultsArray) {
        if (line.startsWith("CHFN_RESTRICT")) {
            int index = chfnDefaultsArray.indexOf(line);
            chfnDefaultsArray.removeAt(index);
            chfnDefaultsArray.insert(index, "CHFN_RESTRICT           frwh");
        }
    }

    chfnDefaults = "";
    for (QString line : chfnDefaultsArray) {
        chfnDefaults.append(line + "\n");
    }

    chfnDefault.open(QFile::WriteOnly);
    chfnDefault.write(chfnDefaults.toUtf8());
    chfnDefault.close();

    p->start("useradd -R /mnt -g wheel -M " + parentWindow->loginname);
    p->waitForFinished(-1);

    p->start("arch-chroot /mnt chfn -f \"" + parentWindow->fullname + "\"" + parentWindow->loginname);
    p->waitForFinished(-1);

    p->start("chpasswd -R /mnt");
    p->write(QString("root:" + parentWindow->password + "\n").toUtf8());
    p->write(QString(parentWindow->loginname + ":" + parentWindow->password + "\n").toUtf8());
    p->closeWriteChannel();
    p->waitForFinished(-1);

    QFile sudoersConfig("/mnt/etc/sudoers");
    sudoersConfig.open(QFile::ReadWrite);
    QString sudoersConfiguration(sudoersConfig.readAll());
    sudoersConfig.close();
    sudoersConfiguration = sudoersConfiguration.replace("# %wheel ALL=(ALL) ALL", "%wheel ALL=(ALL) ALL");
    sudoersConfig.open(QFile::ReadWrite);
    sudoersConfig.write(sudoersConfiguration.toUtf8());
    sudoersConfig.close();

    standardOutput.append("[theos_installer] Configuring services...\n");
    p->start("arch-chroot /mnt systemctl enable NetworkManager");
    p->waitForFinished(-1);
    p->start("arch-chroot /mnt systemctl enable bluetooth");
    p->waitForFinished(-1);
    p->start("arch-chroot /mnt systemctl enable lightdm");
    p->waitForFinished(-1);

    QFile lightdmConf("/mnt/etc/lightdm/lightdm.conf");
    lightdmConf.open(QFile::ReadOnly);
    QString lightdmDefaults(lightdmConf.readAll());
    lightdmConf.close();

    QStringList lightdmDefaultsArray = lightdmDefaults.split("\n");
    for (QString line : lightdmDefaultsArray) {
        if (line.startsWith("#greeter-session=")) {
            int index = lightdmDefaultsArray.indexOf(line);
            lightdmDefaultsArray.removeAt(index);
            lightdmDefaultsArray.insert(index, "greeter-session=lightdm-webkit2-greeter");
        }
    }

    lightdmDefaults = "";
    for (QString line : lightdmDefaultsArray) {
        lightdmDefaults.append(line + "\n");
    }

    lightdmConf.open(QFile::WriteOnly);
    lightdmConf.write(lightdmDefaults.toUtf8());
    lightdmConf.close();

    QFile lightdmWebkitConf("/mnt/etc/lightdm/lightdm-webkit2-greeter.conf");
    lightdmWebkitConf.open(QFile::WriteOnly);
    lightdmWebkitConf.write(QString("[greeter]\nwebkit-theme=contemporary\n").toUtf8());
    lightdmWebkitConf.close();

    QFile initConf("/mnt/etc/mkinitcpio.conf");
    initConf.open(QFile::ReadOnly);
    QString init(initConf.readAll());
    initConf.close();

    QStringList initArray = init.split("\n");
    for (QString line : initArray) {
        if (line.startsWith("HOOKS")) {
            int index = initArray.indexOf(line);
            initArray.removeAt(index);
            initArray.insert(index, "HOOKS=\"base udev plymouth autodetect modconf block filesystems keyboard fsck\"");
        } else if (line.startsWith("MODULES")) {
            int index = initArray.indexOf(line);
            initArray.removeAt(index);
            initArray.insert(index, "MODULES=\"i915\"");
        }
    }

    init = "";
    for (QString line : initArray) {
        init.append(line + "\n");
    }

    initConf.open(QFile::WriteOnly);
    initConf.write(init.toUtf8());
    initConf.close();

    QFile("/etc/os-release").copy("/mnt/etc/os-release");

    p->start("arch-chroot /mnt plymouth-set-default-theme theos --rebuild-initrd" );
    p->waitForFinished(-1);

    p->start("cp -r /root /mnt/home/" + parentWindow->loginname);
    p->waitForFinished(-1);
    p->start("arch-chroot /mnt chown -R " + parentWindow->loginname + " /home/" + parentWindow->loginname + "/ " );
    p->waitForFinished(-1);

    emit finished();
}

void installWorker::lastProcessFinished(int ret) {
    qDebug() << "The last process returned" << ret;
}

void installWorker::outputAvaliable() {
    QString out(p->readAllStandardOutput());
    //qDebug() << out;
    standardOutput.append(out);
    emit output(standardOutput);
}

void installWorker::errorAvaliable() {
    standardOutput.append(QString(p->readAllStandardError()) + "\n");
    emit output(standardOutput);

}
