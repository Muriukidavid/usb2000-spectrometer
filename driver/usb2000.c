/*  usb2000.c : USB device driver for ocean optics spectrometers.

    The driver currently supports USB2000/200+ devices, but the set of commands
    which really works under the USB200 is not entirely clear, since the
    syntax was mostly taken from the USB2000+ device descripion.
    Not all USB calls seem to work, but the core features do for the usb2000.

    Device control currently is through ioctl() calls, but see below.

 Copyright (C) 2009      Christian Kurtsiefer, National University
                         of Singapore <christian.kurtsiefer@gmail.com>

 This source code is free software; you can redistribute it and/or
 modify it under the terms of the GNU Public License as published
 by the Free Software Foundation; either version 2 of the License,
 or (at your option) any later version.

 This source code is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 Please refer to the GNU Public License for more details.

 You should have received a copy of the GNU Public License along with
 this source code; if not, write to:
 Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

--

   usb2000.c  - version for kernel version 2.6. All calls to the driver are
           currently made via ioctls, which are described in the usb2000.h file.

   STATUS: 26.4.2009 first attempt

   ToDo: * Implement read/write methods similarly to the proc devices such
           that a read attempt results in a ASCII text spectrum, and write
           attempts into the device node allows to set some control parameters.
         * implement the bus control and read options announced in the
           USB2000+ OEM specs. For now, this is only a very basic driver.
 
*/

#include <linux/module.h>
#include <linux/usb.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <asm/ioctl.h>
#include <asm/uaccess.h>
#include <linux/string.h>
#include <linux/version.h>


#include "usb2000.h"    /* contains all the ioctls */


/* dirty fixes for broken usb_driver definitions */
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,11) )
#define HAS_NO_OWNER so_sad
#define HAS_NO_DEVFS_MODE
#endif


/* Module parameters*/
MODULE_AUTHOR("Christian Kurtsiefer <christian.kurtsiefer@gmail.com>");
MODULE_DESCRIPTION("Experimental driver for Ocean Optics USB2000(+) spectrometer\n");
MODULE_LICENSE("GPL");  

#define USBDEV_NAME "usb2000"   /* used everywhere... */
#define USB_VENDOR_ID_OCEANOPTICS 0x2457
#define USB_DEVICE_ID_USB2000 0x1002
#define USB_DEVICE_ID_USB2PLUS 0x101E

/* timeout in milliseconds */
#define DEFAULT_TIMEOUT 100

/* local status variables for cards */
typedef struct cardinfo {
    int iocard_opened;
    int major;
    int minor;
    struct usb_device *dev;
    struct device *hostdev; /* roof device */
    unsigned int outpipe1; /* contains pipe ID */
    unsigned int inpipe1;
    unsigned int inpipe2; /* EP7/EP2 large input pipe */
    unsigned int inpipe3; /* needs docu!!! large input pipe */

    struct cardinfo *next, *previous; /* for device management */

    /* device info */
    int deviceID;

    /* status data */
    int timeout_value; /* wait for a spectrum request */

    /* for proper disconnecting behaviour */
    wait_queue_head_t closingqueue; /* for the unload to wait until closed */
    int reallygone;  /* gets set to 1 just before leaving the close call */
   
    /* read buffer; we may allocate that separately but the buffer is just
       really not large enough for really justifying a separate kmalloc */
    char returnbuffer[4100];

} cdi;

static struct cardinfo *cif=NULL; /* no device registered */

/* search cardlists for a particular minor number */
static struct cardinfo *search_cardlist(int index) {
    struct cardinfo *cp;
    for (cp=cif;cp;cp=cp->next) if (cp->minor==index) break;
    return cp; /* pointer to current private device data */
}


/* minor device 0 (simple access) structures */
static int usbdev_flat_open(struct inode *inode, struct file *filp) {
    struct cardinfo *cp;

    cp= search_cardlist(iminor(inode));
    if (!cp) return -ENODEV;
    if (cp->iocard_opened)
        return -EBUSY;
    cp->iocard_opened = 1;
    filp->private_data = (void *)cp; /* store card info in file structure */

    /* USB device is presumably in correct alternate mode, so no action */

    cp->timeout_value = DEFAULT_TIMEOUT;
   
    return 0;
}
static int usbdev_flat_close(struct inode *inode, struct file *filp) {
    struct cardinfo *cp = (struct cardinfo *)filp->private_data;
 
    cp->iocard_opened = 0;

    /* eventually tell the unloader that we are about to close */
    cp->reallygone=0;
    wake_up(&cp->closingqueue);
    /* don't know if this is necessary but just to make sure that we have
       really left this call */
    cp->reallygone=1;
    return 0;
}
/* here goes the old version of ioctl, and gets replaced with the new one..
old definition:

static int usbdev_flat_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
*/
static int usbdev_flat_ioctl(struct file *filp, unsigned int cmd, unsigned long arg) {
    struct cardinfo *cp = (struct cardinfo *)filp->private_data;
    unsigned char data[6]; /* send stuff */
    unsigned char len=1;
    int err;
    int atrf; /* actually transferred data */
    char *argp = NULL;
       
    if (!cp->dev) return -ENODEV;

    argp = (void __user *) arg; /* wherever this is need */

    /* retreive possible argument */
    switch (cmd) {
/* we don't need these ioctls in this argument parsing tree:
        case RequestSpectra:
        case SetIntegrationTime:
        case SetStrobeEnable:
        case SetShutdownMode:
        case SetTriggerMode:
        break;
*/
        case QueryInformation: case ReadRegister: /* extract it out
                                                     of the first byte */
            get_user(data[0], argp);
            arg=data[0];
            break;
        case SetTimeout: /* internal command: set timeout */
            if (arg>0 && arg<100000) {
                cp->timeout_value = arg;
                return 0;
            } else {
                return -EINVAL;
            }
            break;
    }

    switch (cmd) {
        /* simple commands which send direct control urbs to the device */
        /* commands with a 32 bit (4 byte) argument */
        case SetIntegrationTime: /* takes a 32 bit value */
            data[4]=(arg>>24) & 0xff;
            data[3]=(arg>>16) & 0xff;
            len += 2;
        /* commands with a 16 bit (2 byte) argument */
        case SetIntegrationTime2: /* 16 bit version, not confirmed yet */
        case SetTriggerMode:  /* not confirmed yet */
        case SetShutdownMode: /* not confirmed yet */
        case SetStrobeEnable: /* not confirmed yet */
            data[2]=(arg>> 8) & 0xff;
            len += 1;
        case QueryInformation: /* confirmed to work */
        case ReadRegister:     /* yet unconfirmed */
            data[1]= arg      & 0xff;
            len += 1;
        /* commands with a 8 bit (1 byte) argument */
        case InitializeUSB2000: /* seems to work, no idea how to test it */
        case RequestSpectra:    /* confirmed to work */
        case QueryStatus:       /* confirmed to work */
        case ReadPCBTemperature:/* not confirmed yet */
        case TriggerPacket:     /* confirmed to work */
            data[0]=cmd & 0xff;
            /* just send the last significant byte to the device */
            err=usb_bulk_msg(cp->dev, cp->outpipe1, data, len, &atrf, 100);
            if (err) {
              printk("error in sending cmd 0x%x; err: %d", cmd, err);
                return -EFAULT;
            }
            break;
        /* here are dummy entries for commands which don't need to send sth
           to the USB device. This is to trap illegal ioctls. */
        case EmptyPipe:     /* confirmed to work */
        case GetDeviceID:
            break;

        default:
            return -ENOSYS; /* function not implemented */
    }

    /* continue processing the commands which receive return data */
    switch (cmd) {
        case QueryInformation:    /* confirmed to work */
            usb_bulk_msg(cp->dev,cp->inpipe2, cp->returnbuffer, 18, &atrf, 100);
            if (copy_to_user(argp, cp->returnbuffer, 18)) return -EFAULT;
            break;
        case QueryStatus:         /* partly confirmed */
            usb_bulk_msg(cp->dev,cp->inpipe2, cp->returnbuffer, 16, &atrf, 100);
            if (copy_to_user(argp, cp->returnbuffer, 16)) return -EFAULT;
            break;

        /* commands wich return 3 bytes */
        case ReadRegister:        /* not confirmed yet */  
        case ReadPCBTemperature:  /* not confirmed yet, something comes back */
            err=usb_bulk_msg(cp->dev,cp->inpipe2, cp->returnbuffer, 3, &atrf, 1000);
            if (copy_to_user(argp, cp->returnbuffer, 3)) return -EFAULT;
            break;

        /* commands which return 4097 bytes into user mem */
        case RequestSpectra:      /* confirmed to work */
        case EmptyPipe:           /* confirmed to work */
            err=usb_bulk_msg(cp->dev,cp->inpipe1, cp->returnbuffer,
                             4097, &atrf, cp->timeout_value);      
            if (err) return -err; /* are there better options ? */
            if (copy_to_user(argp, cp->returnbuffer, 4097)) return -EFAULT;
            break;
        /* commands which do not involve a USB interaction */
        case GetDeviceID:
            if (copy_to_user(argp, &cp->deviceID, sizeof(int))) return -EFAULT;
            break;
    }

    return 0; /* went ok... */
}

/* minor device 0 (simple access) file options */
static struct file_operations usbdev_simple_fops = {
    open:    usbdev_flat_open,
    release: usbdev_flat_close,
    /* new version for ioctl without BKL */
    unlocked_ioctl:   usbdev_flat_ioctl,
};


/* static structures for the class  entries for udev */
static char classname[]="Spectrometer%d";


/* when using the usb major device number */
static struct usb_class_driver spectrometerclass = {
    name: classname,
    fops: &usbdev_simple_fops,
#ifndef HAS_NO_DEVFS_MODE
    mode: S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP ,
#endif
    minor_base: 80, /* somewhat arbitrary choice... */
};


/* initialisation of the driver: getting resources etc. */
static int usbdev_init_one(struct usb_interface *intf, const struct usb_device_id *id ) {
    int iidx; /* index of different interfaces */
    struct usb_host_interface *setting; /* pointer to one alt setting */
    struct cardinfo *cp; /* pointer to this card */
    int found=0; /* hve found interface & setting w correct ep */
    int epi; /* end point index */
    int retval;

    /* make sure there is enough card space */
    cp = (struct cardinfo *)kmalloc(sizeof(struct cardinfo),GFP_KERNEL);
    if (!cp) {
        printk("%s: Cannot kmalloc device memory\n",USBDEV_NAME);
        return -ENOMEM;
    }

    /* store device ID in a device variable for later */
    cp->deviceID = id->idProduct;
   
    cp->iocard_opened = 0; /* no open */

    retval=usb_register_dev(intf, &spectrometerclass);
    if (retval) { /* coul not get minor */
        printk("%s: could not get minor for a device.\n",USBDEV_NAME);
        goto out2;
    }
    cp->minor = intf->minor;


    /* find device */
    for (iidx=0;iidx<intf->num_altsetting;iidx++){ /* probe interfaces */
        setting = &(intf->altsetting[iidx]);
        if (setting->desc.bNumEndpoints==4) {
            for (epi=0;epi<5;epi++) {
                /* printk("epi: %d, ead: %d\n",epi,
                   setting->endpoint[epi].desc.bEndpointAddress); */
                switch (setting->endpoint[epi].desc.bEndpointAddress) {
                    case 0x81: /* EP1 in */
                        found |=1; break;
                    case 0x01: /* EP1 out  */
                        found |=2; break;
                    case 0x82: /* EP2 in */
                        found |=4; break;
                    case 0x02: /* EP2 out */
                        found |=8; break;
                    case 0x86: /* EP6 in  */
                        found |=16; break;
                    case 0x87: /* EP 7 in */
                        found |=64; break;
                }
                if (found == ((cp->deviceID==USB_DEVICE_ID_USB2000)?0x4c:0x17))
                    break;
            }
        }
    }
    if (found != ((cp->deviceID==USB_DEVICE_ID_USB2000)?0x4c:0x17)) {
        /* have not found correct interface */
        printk("incompete intf; find code: %x. See source for details\n",found);
        goto out1; /* no device found */
    }

    /* generate usbdevice */
    cp->dev = interface_to_usbdev(intf);
    cp->hostdev = cp->dev->bus->controller; /* for nice cleanup */

    /* construct endpoint pipes */
    if (cp->deviceID == USB_DEVICE_ID_USB2000) {
        cp->outpipe1 = usb_sndbulkpipe(cp->dev, 02); /* bulk EP2 out */
        cp->inpipe1 = usb_rcvbulkpipe(cp->dev, 0x82); /*  EP2i pipe; spectra */
        cp->inpipe2 = usb_rcvbulkpipe(cp->dev, 0x87); /*  EP7i pipe; misc */
    } else { /* here we are in usb2000+ or better land */
        cp->outpipe1 = usb_sndbulkpipe(cp->dev, 1); /* construct bulk EP1 out */
        cp->inpipe1 = usb_rcvbulkpipe(cp->dev, 0x82); /*  EP2i pipe; spectra */
        cp->inpipe2 = usb_rcvbulkpipe(cp->dev, 0x81); /*  EP1i pipe; misc */
        cp->inpipe3 = usb_rcvbulkpipe(cp->dev, 0x86); /*  EP6; what is this? */
    }
       
    /* construct a wait queue for proper disconnect action */
    init_waitqueue_head(&cp->closingqueue);

    /* insert in list */
    cp->next=cif;cp->previous=NULL;
    if (cif) cif->previous = cp;
    cif=cp;/* link into chain */
    usb_set_intfdata(intf, cp); /* save private data */

    return 0; /* everything is fine */
 out1:
    usb_deregister_dev(intf, &spectrometerclass);
 out2:
    /* first give back stuff */
    kfree(cp);
    printk("%s: dev alloc went wrong, give back %p\n",USBDEV_NAME,cp);

    return -EBUSY;
}

static void usbdev_remove_one(struct usb_interface *interface) {
    struct cardinfo *cp=NULL; /* to retreive card data */
    /* do the open race condition protection later on, perhaps with a
       semaphore */
    cp = (struct cardinfo *)usb_get_intfdata(interface);
    if (!cp) {
        printk("usbdev: Cannot find device entry \n");
        return;
    }

    /* try to find out if it is running */
    if (cp->iocard_opened) {
        printk("%s: device got unplugged while open. How messy.....\n",
               USBDEV_NAME);
    }

    /* remove from local device list */
    if (cp->previous) {
        cp->previous->next = cp->next;
    } else {
        cif=cp->next;
    }
    if (cp->next) cp->next->previous = cp->previous;

    /* mark interface as dead */
    usb_set_intfdata(interface, NULL);
    usb_deregister_dev(interface, &spectrometerclass);

    kfree(cp); /* give back card data container structure */
   
}

/* driver description info for registration; more details?  */

static struct usb_device_id usbdev_tbl[] = {
    {USB_DEVICE(USB_VENDOR_ID_OCEANOPTICS, USB_DEVICE_ID_USB2000)},
    {USB_DEVICE(USB_VENDOR_ID_OCEANOPTICS, USB_DEVICE_ID_USB2PLUS)},
    {},
};

MODULE_DEVICE_TABLE(usb, usbdev_tbl);

static struct usb_driver usbdev_driver = {
#ifndef HAS_NO_OWNER
    .owner =     THIS_MODULE,
#endif
    .name =      USBDEV_NAME, /* "usbdev-driver", */
    .id_table =  usbdev_tbl,
    .probe =     usbdev_init_one,
    .disconnect =    usbdev_remove_one,
};

static void  __exit usbdev_clean(void) {
    usb_deregister( &usbdev_driver );
}

static int __init usbdev_init(void) {
    int rc;
    cif=NULL;
    rc = usb_register( &usbdev_driver );
    if (rc)
        printk("%s: usb_register failed. Err: %d",USBDEV_NAME,rc);
    return rc;
}

module_init(usbdev_init);
module_exit(usbdev_clean);

