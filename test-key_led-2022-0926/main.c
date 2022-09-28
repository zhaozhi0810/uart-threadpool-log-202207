/*
 * main.c
 *
 *  Created on: Nov 19, 2021
 *      Author: zlf
 */

#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>

#include "drv_22134_api.h"
#include "debug.h"

#define SCREEN_BRIGHTNESS_MIN		(0)
#define SCREEN_BRIGHTNESS_MAX		(0xff)
#define PANEL_KEY_BRIGHTNESS_MIN	(0)
#define PANEL_KEY_BRIGHTNESS_MAX	(100)

#define KEY_VALUE_MIN				(0x1)
#define KEY_VALUE_MAX				(45)

#define WATCHDOG_TIMEOUT			(20)	// s

static const char* g_build_time_str = "Buildtime :"__DATE__" "__TIME__;   //获得编译时间



enum {
	TEST_ITEM_WATCHDOG_ENABLE,
	TEST_ITEM_WATCHDOG_DISABLE,
	TEST_ITEM_SET_KEYBOARD_LED_BRIGHTNESS,
	TEST_ITEM_GET_KEYBOARD_LED_ON_OFF,
	TEST_ITEM_SET_KEYBOARD_LED_ON,    //设置led开
	TEST_ITEM_SET_KEYBOARD_LED_OFF,    //设置led关
	TEST_ITEM_SET_LEDS_FLASH,      //设置闪烁，大于一个则开启线程
	TEST_ITEM_SET_LEDS_FLASH_STOP, //设置停止闪烁，小于1个则关闭线程
	TEST_ITEM_SET_HWTD_TIMEOUT,   //设置硬件看门狗超时时间
	TEST_ITEM_GET_HWTD_TIMEOUT,   //,  //获得硬件看门狗超时时间
	TEST_PRINT_VERSION            //打印版本，保持为最后一项
};

//extern int drvCoreBoardExit(void);

static bool s_main_thread_exit = false;
static bool s_watchdog_feed_thread_exit = false;
static bool s_leds_flash_thread_exit = false;
//static int sg_leds_flash_contol;   //每一位表示一个灯，最多为0-31号灯。对应的位为1表示闪烁，0表示停止闪烁。

static void s_show_usage(void) {
	printf("Usage:\n");
	printf("\t%2d - Watchdog enable\n", TEST_ITEM_WATCHDOG_ENABLE);
	printf("\t%2d - Watchdog disable\n", TEST_ITEM_WATCHDOG_DISABLE);
	printf("\t%2d - Set keyboard led brightness\n", TEST_ITEM_SET_KEYBOARD_LED_BRIGHTNESS);
	printf("\t%2d - Get keyboard led on/off status\n", TEST_ITEM_GET_KEYBOARD_LED_ON_OFF);
	printf("\t%2d - Set a keyboard led on status\n", TEST_ITEM_SET_KEYBOARD_LED_ON);
	printf("\t%2d - Set a keyboard led off status\n", TEST_ITEM_SET_KEYBOARD_LED_OFF);
	printf("\t%2d - Set  leds flash\n", TEST_ITEM_SET_LEDS_FLASH);
	printf("\t%2d - Set  leds flash stop (nothing)\n", TEST_ITEM_SET_LEDS_FLASH_STOP);
	printf("\t%2d - Set Hard watchdog timerout\n", TEST_ITEM_SET_HWTD_TIMEOUT);
	printf("\t%2d - Get Hard watchdog timerout\n", TEST_ITEM_GET_HWTD_TIMEOUT);
	printf("\t%2d - Print program build time\n", TEST_PRINT_VERSION);  //版本打印，保持为最后一项
	printf("\tOther - Exit\n");
}

static void s_sighandler(int signum) {
	INFO("Receive signal %d, program will be exit!\n", signum);
}

static int s_signal_init(void) {
	struct sigaction act;
	bzero(&act, sizeof(struct sigaction));
	CHECK(!sigemptyset(&act.sa_mask), -1, "Error sigemptyset with %d: %s\n", errno, strerror(errno));
	CHECK(!sigaddset(&act.sa_mask, SIGINT), -1, "Error sigaddset with %d: %s\n", errno, strerror(errno));
	CHECK(!sigaddset(&act.sa_mask, SIGQUIT), -1, "Error sigaddset with %d: %s\n", errno, strerror(errno));
	CHECK(!sigaddset(&act.sa_mask, SIGTERM), -1, "Error sigaddset with %d: %s\n", errno, strerror(errno));
	act.sa_handler = s_sighandler;
	CHECK(!sigaction(SIGINT, &act, NULL), -1, "Error sigaction with %d: %s\n", errno, strerror(errno));
	CHECK(!sigaction(SIGQUIT, &act, NULL), -1, "Error sigaction with %d: %s\n", errno, strerror(errno));
	CHECK(!sigaction(SIGTERM, &act, NULL), -1, "Error sigaction with %d: %s\n", errno, strerror(errno));
	return 0;
}

static void s_gpio_notify_func(int gpio, int val) {
	INFO("Key %#x, value %#x\n", gpio, val);
}

static void s_panel_key_notify_func(int gpio, int val) {
	INFO("panel_Key %#x, value %#x\n", gpio, val);
}

static void *s_watchdog_feed_thread(void *param) {
	INFO("Start feed watchdog!");
	unsigned int index = 0;
	s_watchdog_feed_thread_exit = false;
	while(!s_watchdog_feed_thread_exit) {
		if(!(index % WATCHDOG_TIMEOUT)) {
			drvWatchDogFeeding();
			INFO("Watchdog feed success!");
		}
		index ++;
		sleep(1);

	}
	INFO("Stop feed watchdog!");
	return NULL;
}


// static void *s_leds_flash_thread(void *param) {
// 	INFO("Start leds_flash_thread!");
// 	unsigned int index = 0;
// 	s_leds_flash_thread_exit = false;
// 	while(!s_leds_flash_thread_exit) {
// 		for(index = 0;index < 32; index++)
// 			if(sg_leds_flash_contol & (1<<index))
// 			{
// 				drvLightLED(index);  //点亮
// 			}
// 		usleep(500000);  //休眠500ms
// 		for(index = 0;index < 32; index++)
// 			if(sg_leds_flash_contol & (1<<index))
// 			{
// 				drvDimLED(index);  //熄灭
// 			}
// 		usleep(500000);  //休眠500ms
// 	}
// 	INFO("Stop leds_flash_thread!");
// 	return NULL;
// }




int main(int args, char *argv[]) {
	int test_item_index = -1;
	pthread_t watchdog_feed_thread_id = 0;
	pthread_t leds_flash_thread_id = 0;
	int KeyIndex = 0;


	printf("%s running,Buildtime %s\n",argv[0],g_build_time_str);

	INFO("Enter %s\n", __func__);
	CHECK(!s_signal_init(), -1, "Error s_signal_init!");
	CHECK(!drvCoreBoardInit(), -1, "Error drvCoreBoardInit!");

	drvSetGpioCbk(s_gpio_notify_func);
	drvSetGpioKeyCbk(s_panel_key_notify_func);


	//首先要设置看门狗超时时间（220）--> 22秒
	if(drvWatchdogSetTimeout(220) == EXIT_FAILURE) {
		ERR("Error drvWatchdogSetTimeout!");
	}

	if(drvWatchDogEnable()) {
		ERR("Error drvWatchDogEnable!");
		drvCoreBoardExit();
		return -1;
	}
	if(pthread_create(&watchdog_feed_thread_id, NULL, s_watchdog_feed_thread, NULL)) {
		ERR("Error pthread_create with %d: %s\n", errno, strerror(errno));
		drvWatchDogDisable();
		drvCoreBoardExit();
		return -1;
	}

	while(!s_main_thread_exit) {
		test_item_index = -1;
		s_show_usage();

		INFO("Please input test item:");
		if(scanf("%d", &test_item_index) != 1) {
			ERR("Error scanf with %d: %s\n", errno, strerror(errno));
			s_main_thread_exit = true;
			continue;
		}
		switch(test_item_index) {
		case TEST_ITEM_WATCHDOG_ENABLE:
			if(watchdog_feed_thread_id) {
				INFO("Watchdog is running!");
				break;
			}
			if(drvWatchDogEnable() == EXIT_FAILURE) {
				ERR("Error drvWatchDogEnable!");
				break;
			}
			if(pthread_create(&watchdog_feed_thread_id, NULL, s_watchdog_feed_thread, NULL)) {
				ERR("Error pthread_create with %d: %s\n", errno, strerror(errno));
				drvWatchDogDisable();
				break;
			}
			INFO("Execute enable watchdog success!");
			break;
		case TEST_ITEM_WATCHDOG_DISABLE:
			if(!watchdog_feed_thread_id) {
				INFO("Watchdog is not running!");
				break;
			}
			if(drvWatchDogDisable() == EXIT_FAILURE) {
				ERR("Error drvWatchDogDisable!");
				break;
			}
			s_watchdog_feed_thread_exit = true;
			if(pthread_join(watchdog_feed_thread_id, NULL)) {
				ERR("Error pthread_join with %d: %s\n", errno, strerror(errno));
				break;
			}
			watchdog_feed_thread_id = 0;
			INFO("Execute disable watchdog success!");
			break;

		case TEST_ITEM_SET_KEYBOARD_LED_BRIGHTNESS: {
			int nBrtVal = 0;
			INFO("Please input brightness: (%u-%u)\n", PANEL_KEY_BRIGHTNESS_MIN, PANEL_KEY_BRIGHTNESS_MAX);
			if(scanf("%d", &nBrtVal) != 1) {
				ERR("Error scanf with %d: %s\n", errno, strerror(errno));
				continue;
			}
			if(nBrtVal > PANEL_KEY_BRIGHTNESS_MAX || nBrtVal < PANEL_KEY_BRIGHTNESS_MIN) {
				ERR("Error brightness out of range!");
				break;
			}
			drvSetLedBrt(nBrtVal);
			break;
		}
		case TEST_ITEM_GET_KEYBOARD_LED_ON_OFF: {
			int i = KEY_VALUE_MIN;
			for(; i <= KEY_VALUE_MAX; i ++) {
				INFO("Key %#02x is %s\n", i, drvGetLEDStatus(i)? "on":"off");
			}
			break;
		}

		case TEST_ITEM_SET_KEYBOARD_LED_ON:
		{
		//	int KeyIndex = 0;
			INFO("Please input KeyIndex: (%u-%u)", KEY_VALUE_MIN, KEY_VALUE_MAX);
			if(scanf("%d", &KeyIndex) != 1) {
				ERR("Error scanf with %d: %s", errno, strerror(errno));
				continue;
			}
			if(KeyIndex<KEY_VALUE_MIN || KeyIndex > KEY_VALUE_MAX){
				ERR("Error KeyIndex<KEY_VALUE_MIN || KeyIndex > KEY_VALUE_MAX");
				continue;
			}

			// if(KeyIndex>=0 && KeyIndex <32)
			// {
			// 	sg_leds_flash_contol &= ~(1<<KeyIndex);
			// 	if(!sg_leds_flash_contol && !s_leds_flash_thread_exit)
			// 	{
			// 		s_leds_flash_thread_exit = 1;  //线程退出
			// 		pthread_join(leds_flash_thread_id, NULL);
			// 		leds_flash_thread_id = 0;
			// 	}	
			// }
			drvLightLED(KeyIndex);
			break;
		}		
		case TEST_ITEM_SET_KEYBOARD_LED_OFF:
		{
			
			INFO("Please input KeyIndex: (%u-%u)", KEY_VALUE_MIN, KEY_VALUE_MAX);
			if(scanf("%d", &KeyIndex) != 1) {
				ERR("Error scanf with %d: %s", errno, strerror(errno));
				continue;
			}
			if(KeyIndex<KEY_VALUE_MIN || KeyIndex > KEY_VALUE_MAX){
				ERR("Error KeyIndex<KEY_VALUE_MIN || KeyIndex > KEY_VALUE_MAX");
				continue;
			}
			
			// if(KeyIndex>=0 && KeyIndex <32)
			// {
			// 	sg_leds_flash_contol &= ~(1<<KeyIndex);
			// 	if(!sg_leds_flash_contol && !s_leds_flash_thread_exit)
			// 	{
			// 		s_leds_flash_thread_exit = 1;  //线程退出
			// 		pthread_join(leds_flash_thread_id, NULL);
			// 		leds_flash_thread_id = 0;
			// 	}	
			// }				
			
			drvDimLED(KeyIndex); 									
			break;
		}

		case TEST_ITEM_SET_LEDS_FLASH:
			// if(!leds_flash_thread_id) {
			// 	if(pthread_create(&leds_flash_thread_id, NULL, s_leds_flash_thread, NULL)) {
			// 		ERR("Error pthread_create with %d: %s\n", errno, strerror(errno));
			// 	//	drvWatchDogDisable();
			// 	//	break;
			// 	}
			// }
			// else
			// {

			// }
			INFO("Please input KeyIndex: (%u-%u)", KEY_VALUE_MIN, KEY_VALUE_MAX);
			if(scanf("%d", &KeyIndex) != 1) {
				ERR("Error scanf with %d: %s", errno, strerror(errno));
				continue;
			}
			if(KeyIndex<KEY_VALUE_MIN || KeyIndex > KEY_VALUE_MAX){
				ERR("Error KeyIndex<KEY_VALUE_MIN || KeyIndex > KEY_VALUE_MAX");
				continue;
			}

			// if(KeyIndex>=0 && KeyIndex <41)
			// 	sg_leds_flash_contol |= (1<<KeyIndex);
			// else
			// 	sg_leds_flash_contol |= ~0;//(1<<KeyIndex);  全部

			drvFlashLEDs(KeyIndex);

		break;
		case TEST_ITEM_SET_LEDS_FLASH_STOP:
		break;

		// 
		
		case TEST_ITEM_SET_HWTD_TIMEOUT:  //设置硬件看门狗超时时间
		{

			int timeout = 0;
			INFO("Please input TIMEOUT[1-250]:");
			if(scanf("%d", &timeout) != 1) {
				ERR("Error scanf with %d: %s\n", errno, strerror(errno));
				continue;
			}
			//对取值进行合法设置
			if(timeout < 1)
				timeout = 1;
			else if(timeout > 250)
				timeout = 250;
			INFO("Set hard watchdog timeout = %d!\n", timeout);
			drvWatchdogSetTimeout(timeout);
			break;
		}
		case TEST_ITEM_GET_HWTD_TIMEOUT:  //获取硬件看门狗超时时间
		{
			int timeout = drvWatchdogGetTimeout();
			INFO("Get hard watchdog timeout = %d!\n", timeout);
			break;	
		}
		case TEST_PRINT_VERSION:
			printf("%s running,Buildtime %s\n",argv[0],g_build_time_str);
			drvShowVersion();
		break;






		default:
			s_main_thread_exit = true;
			printf("%s exit,Buildtime %s\n",argv[0],g_build_time_str);
			break;
		}
	}

	if(watchdog_feed_thread_id) {
		s_watchdog_feed_thread_exit = true;
		pthread_join(watchdog_feed_thread_id, NULL);
		watchdog_feed_thread_id = 0;
		drvWatchDogDisable();
	}
	if(leds_flash_thread_id) {
		s_leds_flash_thread_exit = true;
		pthread_join(leds_flash_thread_id, NULL);
		leds_flash_thread_id = 0;
		//drvWatchDogDisable();
	}
	drvCoreBoardExit();
	INFO("Exit %s\n", __func__);
	return 0;
}
