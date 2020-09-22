#ifndef __CONFIG_H
#define __CONFIG_H

#define COUNTDOWN_TIME 5 //Countdown time, in seconds. Note that print function expects at most a 2-digit number
#define BACKGROUND_IMG_PATH "ur0:tai/BootMenu.bmp"      //Location of the background .bmp
#define LINUX_MODULE_PATH "ux0:linux/bootstrap.skprx"   //Location of the kernel module that loads the baremetal loader (DEPRECATED)
#define BAREMETAL_PAYLOAD_PATH "ux0:linux/payload.bin"            //Location of the Linux baremetal payload
#define BAREMETAL_PAYLOAD_PADDR 0x40300000                        //Phyiscal address at which the payload will be loaded


#endif