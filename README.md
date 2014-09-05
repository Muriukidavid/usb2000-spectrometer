usb2000-spectrometer
====================

driver and sample application for USB2000 Oceanoptics spectrometer written in C and tcl scripts :)

the following message is copied from original poster of this code. I just put this code here for easier 
reference and updates in future

"Status of this document: 15.7.2009 christian kurtsiefer

This is a program set to run a Ocean optics USB2000/2000+ spectrometer. It
consists of the following parts:

1. Device driver

At this point, there is only a driver version for kernel 2.6 available. The
code is located in the 2.6 directory. In order to install the driver on a
machine, do the following:

1.1 say make in the main directory. This compiles the driver and
application. This directory needs to stay where it is for later on since
it will host the driver code.
1.2 as superuser, use the "make udev" command to install the device rule on
the target machine. This should install a proper hotplug script. If you
now attach a USB2000(+) module, there should be a "Spectrometer0" entry in
the /dev and in the /dev/ioboards directory.
1.3 That's it. There should be no further things necessary to run the
applications.

The driver is intended to provide a file tree entry through which the
spectrometer functions are carried out via ioctl() commands; a list of these
commands and their description is in the header file usb2000.h, which also
needs to be linked to an application program for having access to the command
names. The main driver code is the C program usb2000.c which is able to
handle the USB2000 and the USB200+ spectrometer, possibly more. Currently,
there is also only a subset of all commands of the USB2000+ implemented from
which I thought they may be available under the USB2000 as well.

1.4 possible issues

The driver is only almost complete. Its main instability is probably
connected to disconnection activities while a transfer of data is in
progress. I have seen a few kernel oopses when disconnecting the device
while running.

1.5 other linux distributions than SuSE
The driver should work on other contemporary distros as well - there may
be a little problem in the permission of the devices - the hotplug file
currently has the group name for the device files hard-coded in the source
file, udevsrc. If you have problems with the group (namely, if no group
"users" exists), then try to change this by hand.

2. Documentation

I took the OEM description from ocean optics to get an idea how the USB
interface of the device works. Have a look if you want to understand what the
device does. Take the comments with a grain of salt, I think they changed the
onboard microcontroller from the 2000 version to the 2000+ version.

3. Applications

In the apps directory there are two applications: One is a C program with a
simple command line option set to take a spectrum with a given exposure
time. It generates normal ASCII text as an output which can be used to feed
into gnuplot or something similar. It keeps a few spectrometer parameters in
the comments text as well. It does also the translation from pixel into
wavelength according to the calibration table in the device EEPROM - assuming
I got the translation polynomial right. At the moment, the program allows up
to 10 seconds integration time, and operates the device in free-running
mode.

There is a GUI script named spectrumscript (apologies, it is still in Tcl/Tk)
which should allow an online observation of the spectrum. It seems reasonably
robust.


4. Feedback

Any comments are welcome to make this more general than it is at the moment 8)
At this point in time, I have not set up a dedicated directory for this code,
so please contact me via email at christian.kurtsiefer@gmail.com"
