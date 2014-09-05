obj-m :=  usb2000.o
KDIR := /lib/modules/`uname -r`/build
PWD := `pwd`
default:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules
