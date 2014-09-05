- to build the loadable kernel module driver, while in this folder, run the command:

make

- to load the module in the kernel, run the command:

sudo insmod usb2000.ko

- to load the module everytime the spectrometer is plugged in and unload everytime its unplugged, run the command:

sudo nano /etc/udev/rules.d/10-oceanoptics.rules

add the following 2 lines in that file, save and unplug the plug the spectrometer

SUBSYSTEM=="usb", ACTION=="add", ENV{PRODUCT}=="2457/1002/*", RUN+="/sbin/insmod /path/to/the/module/usb2000.ko"
SUBSYSTEM=="usb", ACTION=="remove", ENV{PRODUCT}=="2457/1002/*", RUN+="/sbin/rmmod usb2000"

NB:// /path/to/the/module/ should be replaced with the correct path to the .ko loadable module file :) 

Enjoy capturing spectra...
