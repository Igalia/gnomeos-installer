This repository contains the necessary bits to generate an ISO image containing the GnomeOS installer

Files and directories structure
-------------------------------

* cdcontent            -> directory with the files that will be added to the ISO image
  * isolinux           -> directory with the isolinux files
    * boot.cat         -> boot catalog
    * isolinux.bin     -> bootloader binary
    * isolinux.cfg     -> bootloader menu configuration 
    * splash.png       -> menu background image
    * vesamenu.c32     -> module used for the menu
  * boot               -> directory with the kernel to boot
    * vmlinuz          -> kernel
    * initrd.gz        -> initrd image for the kernel to load
  * image              -> directory with the gnomeos image
    * image.gz         -> gnomeos image (binary compressed with gzip)
* installer            -> installer application directory
  * Makefile
  * main.c
  * mainwindow.h
  * mainwindow.c
* extra                -> directory with extra bits needed
  * isohdpfx.bin       -> binary needed for xorriso to create usb bootable images
* unpackinitrd         -> script to unpack the initrd image in cdcontent/boot in an "initrd" directory so it can be manipulated
* packinitrd           -> script that packs the "initrd" directory again into the cdcontent/boot dir
* generateiso          -> script that creates the ISO files from the files in cdcontent
* ddiso                -> script that dd's the iso image to an usb device that will be used to boot


The boot process and the installer
----------------------------------

It's pretty simple. First, the syslinux bootloader gets launched, which shows the boot menu (defined in isolinux.cfg). When selecting the
install option from the menu, the kernel in boot is launched with the initrd image there. That initrd will launch a busybox that will start
several needed processes, then launch Xorg and finally the installer application.

Once the installer is launched, it will check the space needed for the image, and let the user select a disk from all the available in the system
to perform the installation. The disks with not enough space are disabled. When the user selects a disk, the installer dd's the image
to it and it's done.


Modifying the installer (or the initrd image)
---------------------------------------------

* Modify the installer app at will and compile it *for i386* architecture
* From the toplevel dir, aunch the unpackinitrd script. It will create a initrd directory
* Copy the compiled installer to initrd/usr/bin with the name launch-gnomeos-installer
  or
* Modify the files in the initrd image at will (editing the init file and replacing "exec $init" with "exec /bin/ash" will launch a shell at boot) 
* From the toplevel dir, launch the packinitrd script. It will pack the inird directory and put it in the cdcontent/boot dir


Creating the image and putting it into the usb
----------------------------------------------

After having modified the installer/initrd files and packed them again

* From the toplevel dir launch the generateiso script, that will create the image from the contents in the cdcontent dir.
  This script uses the xorriso command so you need to have it installed
* Launch the ddiso script specifying the usb device. Notice that you'll need to sudo it (eg: sudo ./ddiso /dev/sdb)


GnomeOS image details
---------------------

The GnomeOS image that is being installed was downloaded from https://build.gnome.org/work/images/current/
From there you download a qemu disk image that needs to be converted to raw with

qemu-img convert -f qcow2 -O raw gnome-ostree-x86_64-runtime.qcow2 gnomeos.img 

This will generate a huge file (8GB in the image I used) because it's the image of a 8GB disk, despite only 1.5GB are being
used (the rest is empty). That's why, in order to use it in the installer, it must be compressed before putting it in
the cdcontent/image directory.

gzip -c gnomeos.raw > image.gz

The installer knows that the image is compressed and will check the space needed when uncompressed in order to disable
the disks that are not big enough.

NOTE: Currently at build.gnome.org there's only a 64 bits image for download. I can't remember whether it's the image that
I downloaded or the one I used is a 32 bits one that I can't find now.


Known problems
--------------

Sometimes the generated usb image hangs during boot. It shows the boot menu, but once the installer is selected it gets blank. It's weird because
this happens only in the wetab, while the usb boots in any other pc or in VBox, and I couldn't find the reason.
Usually, removing the "quiet" parameter from the boot command fixes the problem (it not, try several times). If it doesn't work anyway, you can
try regenerating the image and dding it again.

