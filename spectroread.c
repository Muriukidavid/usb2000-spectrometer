/* program to read a spectrum from the ocean optics USB2000/USB2000+ device.

   usage: spectroread [-o fnam] [-i integrationtime] [-d devicefile] [-s serial]
                      [-v verbosity]

   -o fnam:             output file name. if the name - is specified, output
                        is sent to stdout - this is also the default.
   -i integrationtime:  specifies the integration time in ms. Default value
                        is 100.
   -d devicefile        specifies a USB device file. Default is
                        /dev/ioboards/Spectrometer0
   -s serial:           select a specific serial number (not implemented yet)

   -V verbosity:        commenting level. adds details at the end of a spectrum
                        add the values below to have the following texts:
                        1: serial number
                        2: date/time of data taking
                        4: integration time
                        8: generic comment
                        16: ccd dark pixel level
                        32: stored wavelength coefficients
                        64: USB device ID

   The program emits to stdout or the target file name a space-separated list
   with the following entries:
   pixel index, wavelength in nm, raw amplitude and a few comment options


   Status: first version 26.4.09chk
           translation to work also with usb2000+ 17.7.09chk

   ToDo: Keep it so general that a usb200+ or 400+ can be used as well.

 */

#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

#include "usb2000.h"

#define USB_DEVICE_ID_USB2000 0x1002

#define DEFAULT_DEVICENAME "/dev/Spectrometer0"
#define DEFAULT_INTEGRATIONTIME 100
#define DEFAULT_VERBOSITY 31 /* sernum, date/time, integtime, gencomment */
#define FILENAMLEN 100

/* error handling */
char *errormessage[] = {
  "No error.",
  "Error parsing verbosity argument option.", /* 1 */
  "Error parsing target file name option.",
  "Error parsing device file name option.",
  "Error parsing integration time option.",
  "Integration time out of range (1-10000 ms)", /* 5 */
  "; error opening spectrometer device.",
  "Error opening target file.",
  "; error when retreiving spectrum from device.",
};

int emsg(int code) {
  fprintf(stderr,"%s\n",errormessage[code]);
  return code;
};

/* some global variables */
FILE* outhandle; /* for output of data */
double lam_coeff[4]; /* coefficients to convert into wavelength */

/*function to convert the return string of a USB2000 into a list of integers */
void generate_numbers_USB2000(unsigned char *data, int *values) {
    int i;
    for (i=0; i<2048; i++)
        values[i] = data[(i%64)+(i>>6)*128+64]*256 + data[(i%64)+(i>>6)*128];
}
void generate_numbers_USB2000p(unsigned char *data, int *values) {
    int i;
    for (i=0; i<2048; i++)
        values[i] = data[i*2+1]*256 + data[2*i];
}


#define BLACKLEVEL_START 6
#define BLACKLEVEL_END 20
float baselevel_USB2000( int *values) {
    int i;
    int sum=0;
    for (i=BLACKLEVEL_START;i<=BLACKLEVEL_END;i++) sum += values[i];
    return ((float) sum)/(BLACKLEVEL_END-BLACKLEVEL_START+1);
}

int main(int argc, char *argv[]) {
    int handle; /* file handle for usb device */
    int retval;
    int i;
    int opterr, opt; /* for parsing options */
    unsigned char data2[4100];
    char *data = (char *) data2;
    int rawvalues[2048];  /* for storing numerical values */
    int integrationtime = DEFAULT_INTEGRATIONTIME; /* currently in millisec */
    char devicename[FILENAMLEN] = DEFAULT_DEVICENAME;
    int verbositylevel = DEFAULT_VERBOSITY;
    char outfilename[FILENAMLEN] = "-";
    float baselevel;  /* generated out of beginning pxels */
    double lambda;   /* for generating wavelength */
    time_t tme;
    int deviceID; /* stores the usb deviceID of the spectrometer */
   

    /* parsing options */
    opterr=0; /* be quiet when there are no options */
    while ((opt=getopt(argc, argv, "V:o:d:i:")) != EOF) {
        switch (opt) {
            case 'V': /* set verbosity level */
                if (sscanf(optarg,"%d",&verbositylevel)!=1 ) return -emsg(1);
                break;
            case 'o': /* set outout file name */
                if (sscanf(optarg,"%99s",outfilename)!=1 ) return -emsg(2);
                outfilename[FILENAMLEN-1]=0; /* close string */
                break;
            case 'd': /*enter device file name */
                if (sscanf(optarg,"%99s",devicename)!=1 ) return -emsg(3);
                outfilename[FILENAMLEN-1]=0; /* close string */
                break;
            case 'i': /* set integration time */
                if (sscanf(optarg,"%d",&integrationtime)!=1 ) return -emsg(4);
                if (integrationtime<1 || integrationtime >10000)
                    return -emsg(5); /* out of range */
                break;          
        }
    }

    /* opening device file */
    handle=open(devicename,O_RDWR);
    if (handle==-1) {
        perror("spectroread");
        return -emsg(6);
    }

    /* open target file */
    if (strcmp(outfilename,"-")) {
        outhandle = fopen(outfilename,"w+");
        if (!outhandle) return -emsg(7);
    } else {
        outhandle=stdout;
    }

    /* get device ID */
    ioctl(handle,GetDeviceID,&deviceID);

    /* prepare device */
    ioctl(handle,SetIntegrationTime,
          integrationtime * ((deviceID==USB_DEVICE_ID_USB2000)?1:1000));
    ioctl(handle,InitializeUSB2000);

    /* get wavelength conversion coefficients */
    for (i=0;i<4;i++) {
        data2[0]=i+1;
        ioctl(handle,QueryInformation,&data2);
        data2[17]=0;
        sscanf((char *)&data2[2],"%lf",&lam_coeff[i]);
    }
   
    /* clear input pipeline - this is still a bit dirty */
    ioctl(handle,SetTimeout,20); /* Let's not waste too much time */
    do {
        retval=ioctl(handle,EmptyPipe,&data2);
    } while (retval!=ETIMEDOUT);  /* wait until line is empty */

   
    /* now set timeout to match  for the spectrum to arrive. This is still
       a dirty choice, it seems to depend on the kernel interruption
       rate....it should match the time it takes to read in a spectrum: That
       information is actually available with the integration time.
       As there is no reason why the call should fail, the timeout
       could be reasonably long as well.... */
    ioctl(handle,SetTimeout,10000);

    /* do the actuall spectrum retrieval */
    retval=ioctl(handle,RequestSpectra,&data2);
   
    if (retval) {
        perror("specroread");
        return -emsg(8);
    } else {
        /* convert return string into a list of numbers */
        if (deviceID==USB_DEVICE_ID_USB2000) {
            generate_numbers_USB2000(data2, rawvalues);
        } else {
            generate_numbers_USB2000p(data2, rawvalues);
        }
        baselevel=baselevel_USB2000(rawvalues);

        /* generate first header */
        if (verbositylevel & 8) /* generic header */
            fprintf(outhandle,"# output of the ocean optics spectrometer.\n# comumn 1: pixel index, column 2: wavelength in nm\n# column 3: raw amplitude 4: baselevel-corrected ampl\n\n");

        /* output main spectrum */
        for (i=0;i<2048;i++) {
            lambda = lam_coeff[0] + i * (lam_coeff[1] +
                                         i*(lam_coeff[2]+i*lam_coeff[3]));
            fprintf(outhandle,"%d %7.2f %d %d\n",
                    i, lambda, rawvalues[i], rawvalues[i]-(int)(baselevel+0.5));
        }

        if (verbositylevel & 8) fprintf(outhandle,"\n"); /* some space */
        /* output the rest of the comments */
        if (verbositylevel & 1) {
            data2[0]=0; /* query serial no */
            ioctl(handle,QueryInformation,&data2); data2[17]=0;
            fprintf(outhandle,"# Serial No. %s\n",&data2[2]);
        }
        if (verbositylevel & 2 ) {
            tme=time(NULL);
            strftime(data,30,"%a %d %b %y %X %Z",localtime(&tme));
            fprintf(outhandle,"# %s\n",data);
        }
        if (verbositylevel & 4) {
            fprintf(outhandle,"# Integration time: %d ms\n",integrationtime);
        }
        if (verbositylevel & 16) {
            fprintf(outhandle,
                    "# Black level from blocked pixels (%d to %d): %8.2f\n",
                    BLACKLEVEL_START, BLACKLEVEL_END, baselevel);
        }
        if (verbositylevel & 32) {
            fprintf(outhandle, "# wavelength conversion coefficients, lam = sum_i c_i index**i\n");
            for (i=0;i<4;i++ ) fprintf(outhandle, "#  c%1d = %lf\n",
                                       i, lam_coeff[i]);
        }
        if (verbositylevel & 64) {
            fprintf(outhandle, "# USB device ID: 0x%x\n",deviceID);
        }
       
    }
    close(handle);
   
    /* close target file if necessary */
    if (strcmp(outfilename,"-")) fclose(outhandle);

    return 0;  
}

