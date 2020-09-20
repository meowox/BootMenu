#include <taihen.h>

#include <string.h>
#include <stdio.h>

#include "blit/blit.h"

#include <psp2/usbstorvstor.h>
#include <psp2kern/power.h> 
#include <psp2kern/ctrl.h> 
#include <psp2kern/display.h>
#include <psp2kern/io/fcntl.h> 
#include <psp2kern/io/dirent.h> 
#include <psp2kern/udcd.h>
#include <psp2kern/sblaimgr.h> 
#include <psp2kern/kernel/cpu.h> 
#include <psp2kern/kernel/sysmem.h> 
#include <psp2kern/kernel/suspend.h>
#include <psp2kern/kernel/modulemgr.h> 
#include <psp2kern/kernel/threadmgr.h> 
#include <psp2kern/kernel/dmac.h> 
#include <psp2/rtc.h>

int ksceRtcGetCurrentClockLocalTime(SceDateTime*);

const char BACKGROUND_IMG_PATH[] = "ur0:tai/BootMenu.bmp";
const char LINUX_MODULE_PATH[] = "ux0:linux/bootstrap.skprx";
const unsigned int MENU_LAST_LINE_Y = 80;
const unsigned int COUNTDOWN_Y = 220;
const unsigned int LINUX_MESSAGE_Y = 250;
const char menu[4][20] = {"Boot Linux", "Boot Vita OS", "Reboot", "Shutdown"};

static SceUID fb_uid = -1;

SceCtrlData ctrl_peek, ctrl_press;

void *fb_addr = NULL;
void *bmp_addr = NULL;
int rgbuid, bgfd;
int menusize;
int menu_index = 1;
int hasbg;

SceDateTime current_time;
int target_time;
int is_pstv = 0;
int do_countdown = 1;
int linux_module_exists;
int secs = 8;

#define ALIGN(x, a) (((x) + ((a) - 1)) & ~((a) - 1))

#define SCREEN_PITCH 960
#define SCREEN_W 960
#define SCREEN_H 544

void drawScreen() {
	//ksceDmacMemset(fb_addr, 0x00, SCREEN_PITCH * SCREEN_H * 4);
		if (is_pstv) return; //PSTV doesn't have a menu
		blit_set_color(0x00000000, 0x00000000);
		for(int i = 1; i <= menusize; i++) blit_stringf((strlen(menu[i - 1]) + 2) * 16, i * 20 + MENU_LAST_LINE_Y, "<");
		blit_set_color(0x00ffffff, 0x00000000);
		blit_stringf((strlen(menu[menu_index - 1]) + 2) * 16, menu_index * 20 + MENU_LAST_LINE_Y, "<");
}

int isSkipComboPressed() {
	SceCtrlData ctrl;

	ksceCtrlPeekBufferPositive(0, &ctrl, 1);

	ksceDebugPrintf("Buttons held: 0x%08X\n", ctrl.buttons);

	if(ctrl.buttons & (SCE_CTRL_POWER | SCE_CTRL_TRIANGLE)) {
		return 1;
	} else {
		return 0;
	}

}

void boot_linux(){
	if (!linux_module_exists) {
		blit_set_color(RGBT(0xFF,0,0,0), 0x00000000);
		blit_stringf(20,LINUX_MESSAGE_Y,"No module at %s !",LINUX_MODULE_PATH);
		blit_set_color(0x00FFFFFF, 0x00000000);
		return;
	}
	else {
		SceUID linux_uid;
		int status;
		linux_uid = ksceKernelLoadStartModule(LINUX_MODULE_PATH, 0, NULL, 0, NULL, &status);
		if (linux_uid < 0){
			blit_set_color(RGBT(0xFF,0,0,0), 0x00000000);
			blit_stringf(20,LINUX_MESSAGE_Y,"Error loading %s :", LINUX_MODULE_PATH);
			blit_stringf(20,LINUX_MESSAGE_Y + 20,"UID : 0x%08X Status : 0x%08X",linux_uid,status);
			blit_set_color(0x00FFFFFF, 0x00000000);
		}
		else {
			blit_set_color(RGBT(0,0xFF,0,0), 0x00000000);
			blit_stringf(20,LINUX_MESSAGE_Y,"Module loaded, Linux should start soon !");
			ksceKernelDelayThread(5 * 1000 * 1000);
			blit_set_color(0x00FFFFFF, 0x00000000);
		}
	}
	return;
}

void updateAndDrawCountdown(){
	if (!do_countdown){
		blit_rect(20,COUNTDOWN_Y,100,20,0x00000000);
		return;
	}
	else {
		blit_set_color(RGBT(0,0xFF,0,0), 0x00000000);
		blit_stringf(20,COUNTDOWN_Y,"Booting Linux in %1ds ...",--secs);
		blit_set_color(0x00FFFFFF,0x00000000);

		ksceDebugPrintf("current_time.second : %d\n",current_time.second);

		target_time = (current_time.second + 1 == 60) ? 0 : current_time.second + 1;

		ksceDebugPrintf("New target time : %d\n",target_time);
		if (secs == 0){
			do_countdown = 0;
			boot_linux();
		}
	}
	return;
}

void _start() __attribute__ ((weak, alias ("module_start")));
int module_start(SceSize argc, const void *args) {
	ksceDebugPrintf("-----------------------\n"\
					"VitaBootMenu by CreepNT\n"\
					"Based on EmergencyMount\n"\
					"      by Team CBPS     \n"\
					"-----------------------\n");

	menusize = sizeof(menu) / sizeof(menu[0]);
	is_pstv = ksceSblAimgrIsGenuineDolce();
	ksceDebugPrintf("is_pstv : %c\n", is_pstv ? 'y' : 'n');

	if( isSkipComboPressed() ) 
		return SCE_KERNEL_START_SUCCESS; //Skip menu if Triangle/Power is pressed

	int stat_ret;
  	SceIoStat stat;
	stat_ret = ksceIoGetstat(BACKGROUND_IMG_PATH, &stat);
	if ( (stat_ret < 0) || ((uint32_t)stat.st_size != 0x17E836) || (SCE_STM_ISDIR(stat.st_mode) != 0) ) hasbg = 0;
	else hasbg = 1;

	stat_ret = ksceIoGetstat(LINUX_MODULE_PATH, &stat);
	if ( (stat_ret < 0) || (SCE_STM_ISDIR(stat.st_mode) != 0) ) linux_module_exists = 0;
	else linux_module_exists = 1;
  	
	unsigned int fb_size = ALIGN(4 * SCREEN_PITCH * SCREEN_H, 256 * 1024);

	fb_uid = ksceKernelAllocMemBlock("fb", 0x40404006 , fb_size, NULL);

	ksceKernelGetMemBlockBase(fb_uid, &fb_addr);

	if(hasbg) {
		if( (rgbuid = ksceKernelAllocMemBlock("rgb", 0x1050D006, 0x200000, NULL)) < 0) {
			hasbg = 0;
			ksceDebugPrintf("background memory allocation failed\n");
		} else ksceDebugPrintf("rgb okke!\n");
	}

	if(hasbg) {
		ksceKernelGetMemBlockBase(rgbuid, (void**)&bmp_addr);
		bgfd = ksceIoOpen(BACKGROUND_IMG_PATH, SCE_O_RDONLY, 0);
		ksceIoLseek(bgfd, 54,	SCE_SEEK_SET); // 54 byte bmp header
		ksceIoRead(bgfd, bmp_addr, 0x17E7E2); // 0x17E836 - 54 = 0x17E7E2
		ksceIoClose(bgfd);
	    ksceDebugPrintf("bmp read okke!\n");

	    for(int i = 0; i < 544; i++) {
    		for(int j = 0; j < 960; j++) {
    			// A B G R
    	    	*(uint32_t *)(fb_addr + (((544 - i) * 960) + j) * 4) =
        	    	((((char *)bmp_addr)[((i * 960) + j) * 3 + 2]) <<  0) |
    	    	    ((((char *)bmp_addr)[((i * 960) + j) * 3 + 1]) <<  8) |
        	    	((((char *)bmp_addr)[((i * 960) + j) * 3 + 0]) << 16) |
    	        		0xFF000000;
			}
    	}

	ksceDebugPrintf("background okke!\n");

	ksceKernelFreeMemBlock(rgbuid);
	}

	SceDisplayFrameBuf fb;
	fb.size        = sizeof(fb);
	fb.base        = fb_addr;
	fb.pitch       = SCREEN_PITCH;
	fb.pixelformat = 0;
	fb.width       = SCREEN_W;
	fb.height      = SCREEN_H;

	ksceDisplaySetFrameBuf(&fb, 1);
	blit_set_frame_buf(&fb);
	
	blit_set_color(0x00ffffff, 0x00000000);

	blit_string(20, 20, "VitaBootMenu by CreepNT");
	blit_string(20, 40, "Based on EmergencyMount");
	blit_string(20, 60, "    by teakhanirons    ");
	blit_string(20, MENU_LAST_LINE_Y, "-----------------------"); //Update global var if modifying this menu
	if (!is_pstv){
		for(int i = 0; i < menusize; i++) { blit_stringf(20, ((i + 1) * 20) + MENU_LAST_LINE_Y, menu[i]); }
	}
	else {
		blit_string(20, MENU_LAST_LINE_Y + 20, "Press POWER to boot PS Vita OS.");
	}

	drawScreen();

	ksceRtcGetCurrentClockLocalTime(&current_time);
	target_time = (current_time.second + 1 == 60) ? 0 : current_time.second + 1;

	while(1) {
		ksceKernelPowerTick(SCE_KERNEL_POWER_TICK_DEFAULT);

		ctrl_press = ctrl_peek;
		ksceCtrlPeekBufferPositive(0, &ctrl_peek, 1);
		ctrl_press.buttons = ctrl_peek.buttons & ~ctrl_press.buttons;


		if (do_countdown) {
			if (ctrl_press.buttons){
				do_countdown = 0;
				updateAndDrawCountdown();
				if (is_pstv)
					goto exit_inf_loop; //For PSTV, cancelling countdown launches PSVita OS.
			}
			else {
				ksceRtcGetCurrentClockLocalTime(&current_time);
				if (current_time.second >= target_time)
					updateAndDrawCountdown();
			}
		}


		if(ctrl_press.buttons == SCE_CTRL_UP) {
			menu_index--;
			if(menu_index < 1) menu_index = menusize;
			drawScreen();
		} 
		else if(ctrl_press.buttons == SCE_CTRL_DOWN){
			menu_index++;
			if(menu_index > menusize) menu_index = 1;
			drawScreen();
		} 
		else if(ctrl_press.buttons == SCE_CTRL_CROSS || ctrl_press.buttons == SCE_CTRL_CIRCLE) {
			if(menu_index == 1) { //Boot Linux
				boot_linux();
			} 
			else if(menu_index == 2) { // Vita OS
				goto exit_inf_loop; //Exit infinite loop
			} 
			else if(menu_index == 3) { // reboot - kind of broken, sometimes doesnt reboot only shutdown
				kscePowerRequestColdReset();
				ksceKernelDelayThread(5 * 1000 * 1000); //fallback
			} 
			else if(menu_index == 4) { // shutdown - may be broken?
				kscePowerRequestStandby();
				ksceKernelDelayThread(5 * 1000 * 1000); //fallback
			} 
		}
	};
exit_inf_loop:
	ksceDebugPrintf("exit loop\n");
	if (fb_uid) {
		ksceKernelFreeMemBlock(fb_uid);
	}

	return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args) {
	if (fb_uid) {
		ksceKernelFreeMemBlock(fb_uid);
	}

	return SCE_KERNEL_STOP_SUCCESS;
}