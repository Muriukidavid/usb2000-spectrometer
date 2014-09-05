/* usb2000.h:  IOCTL definitions for the ocean optics spectrometer
               driver. Details see below.
                 
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
   Definitions of ioctl commands passed to the driver. They essentially map
   to the USB commands of the device (TBD), but there are some generic
   commands which allow to play with the interface from user space.

   STATUS:  -first version 26.4.2009
            -ioctl comands more compatible with recommendation; there are
             still some commands which don't pass pointers but values
             directly. No sure if this will be trashed some day 23.2.10chk
 */

/* The following choices have been made to define the ioctls in the way it
   seems to be recommended in the kernel ioctl doc:
   the 'letter' identifier 0xaa and 0xab have been used. This causes perhaps
   a conflict with other commands at some point, but since there is no reserved
   set of letter codes with a barred tinkering from glibc I leave them for now
   We still keep the same LSB of the ioctl, since this is also used as the
   corresponding USB command token.
*/
#include <asm/ioctl.h>

/* confirmed operation: */
#define  InitializeUSB2000  _IO(0xaa, 1)              /* needs no argument */
#define  SetIntegrationTime _IOW(0xaa, 2, int)        /* takes a 32bit value
                                                         as argument */
#define  QueryInformation   _IOWR(0xaa, 5, char[17])  /* takes  arg in byte 0,
                                                         returns 17 bytes. The
                                                         argument must be a
                                                         pointer to an array
                                                         of char of at least
                                                         17 bytes. */
#define  RequestSpectra     _IOR(0xaa, 9, char[17])    /* returns 4097 bytes
                                                          of data. Argument
                                                          of the ioctl call
                                                          is a pointer to an
                                                          array of char of
                                                          at least 4097 bytes
                                                          size */
#define  QueryStatus        _IOR(0xaa, 0xfe , char[16])/* returns 16 bytes.
                                                          Argument is pointer
                                                          to an array of char
                                                          of at least 16 bytes
                                                          size */

/* ioctls for the USB driver which have no direct USB firmawre command
   equivalent */
#define EmptyPipe           _IOR(0xab, 0x00, char[4097]) /* reads the remainder
                                                            of the EP2 data */
#define TriggerPacket       _IO(0xab, 0x09)              /* issues a trigger
                                                            cmd, but does not
                                                            read. This is a
                                                            no-blocking version
                                                            of RequestSpectra */
/* unconfirmed operation */
#define  SetStrobeEnable    _IOW(0xaa, 3, int)    /* takes integer argument */
#define  SetShutdownMode    _IOW(0xaa, 4, int)    /* takes integer argument */
#define  SetTriggerMode     _IOW(0xaa, 10, int)   /* takes trigger mode as
                                                     argument */
#define  ReadRegister       _IOWR(0xaa, 0x6b, int)/* returns 2 bytes into a
                                                     user variable. Argument of
                                                     ioctl holds a pointer to
                                                     int, which initially also
                                                     contains the register
                                                     adr. */
#define  ReadPCBTemperature _IOR(0xaa, 0x6c, int) /* returns 2 bytes into a
                                                     user variable. Argument
                                                     of ioctl holds a pointer
                                                     to int. */
#define SetIntegrationTime2 _IOW(0xab, 0x02, int) /* takes a 16bit value as
                                                     argument */
#define SetTimeout          _IOW(0xab, 0x01, int) /* sets the timeout for a
                                                     EP2 read event. Argument
                                                     is in miliseconds */

#define GetDeviceID         _IO(0xab, 0x99) /* retrieve the USB device ID */

