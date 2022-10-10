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



// enum {
// 	TEST_ITEM_WATCHDOG_ENABLE,
// 	TEST_ITEM_WATCHDOG_DISABLE,
// 	TEST_ITEM_SCREEN_ENABLE,
// 	TEST_ITEM_SPEAKER_ENABLE,
// 	TEST_ITEM_SPEAKER_DISABLE,
// 	TEST_ITEM_SPEAKER_VOLUME_ADD,
// 	TEST_ITEM_SPEAKER_VOLUME_SUB,
// 	TEST_ITEM_WARNING_ENABLE,
// 	TEST_ITEM_WARNING_DISABLE,
// 	TEST_ITEM_HAND_ENABLE,
// 	TEST_ITEM_HAND_DISABLE,
// 	TEST_ITEM_HAND_VOLUME_ADD,
// 	TEST_ITEM_HAND_VOLUME_SUB,
// 	TEST_ITEM_HEADSET_ENABLE,
// 	TEST_ITEM_HEADSET_DISABLE,
// 	TEST_ITEM_HEADSET_VOLUME_ADD,
// 	TEST_ITEM_HEADSET_VOLUME_SUB,
// 	TEST_ITEM_USB0_ENABLE,
// 	TEST_ITEM_USB0_DISABLE,
// 	TEST_ITEM_USB1_ENABLE,
// 	TEST_ITEM_USB1_DISABLE,
// 	TEST_ITEM_GET_CPU_TEMPERATURE,
// 	TEST_ITEM_GET_INTERFACE_BOARD_TEMPERATURE,
// 	TEST_ITEM_SET_KEYBOARD_LED_BRIGHTNESS,
// 	TEST_ITEM_GET_KEYBOARD_LED_ON_OFF,
// 	TEST_ITEM_SET_KEYBOARD_LED_ON,    //设置led开
// 	TEST_ITEM_SET_KEYBOARD_LED_OFF,    //设置led关
// 	TEST_ITEM_SET_SCREEN_BRIGHTNESS,
// 	TEST_ITEM_RESET_KEYBOARD,
// 	TEST_ITEM_RESET_SCREEN,
// 	TEST_ITEM_RESET_CORE_BOARD,
// 	TEST_ITEM_GET_VOLGATE,
// 	TEST_ITEM_GET_ELECTRICITY,
// 	TEST_ITEM_GET_KEYBOARD_MODEL,
// 	TEST_ITEM_GET_LCD_MODEL,
// 	TEST_ITEM_AUDIO_SELECT_PANEL_MIC,
// 	TEST_ITEM_AUDIO_SELECT_HAND_MIC,
// 	TEST_ITEM_AUDIO_SELECT_HEADSET_MIC,
// 	TEST_ITEM_TOUCHSCREEN_ENABLE,
// 	TEST_ITEM_TOUCHSCREEN_DISABLE,
// 	TEST_ITEM_GET_RTC,
// 	TEST_ITEM_SET_RTC,
// 	TEST_ITEM_GET_HEADSET_INSERT_STATUS,
// 	TEST_ITEM_GET_HANDLE_INSERT_STATUS,
// 	TEST_ITEM_SET_TUNE_UP,
// 	TEST_ITEM_SET_TUNE_DOWN,
// 	TEST_ITEM_SET_HWTD_TIMEOUT,   //设置硬件看门狗超时时间
// 	TEST_ITEM_GET_HWTD_TIMEOUT,   //,  //获得硬件看门狗超时时间
// 	TEST_ITEM_SET_LEDS_FLASH,      //设置闪烁，大于一个则开启线程
// 	TEST_PRINT_VERSION            //打印版本，保持为最后一项
// };

//extern int drvCoreBoardExit(void);

static bool s_main_thread_exit = false;
static bool s_watchdog_feed_thread_exit = false;
static 	pthread_t watchdog_feed_thread_id = 0;


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


static int get_stdin_a_num(void)
{
	char buf[32];
	int i;

	while(fgets(buf,sizeof buf,stdin))
	{
		i = 0;
		buf[31] = '\0';
		while((buf[i] == ' ') || (buf[i] == '\t'))   //去除输入的空格
			i++;
		if(buf[i]>='0' && buf[i] <= '9')
			return atoi(buf+i);
		else if(buf[i] =='\n')   //对回车则继续
			continue;
		else   //输入了非数字开头
			return -1;
	}	
	return -1;
}



static void printf_audio_menu(void)
{
	printf("音频控制菜单：（使用方法: 请输入对应功能的数字） \n");
	printf("1. 开启扬声器\n");
	printf("2. 关闭扬声器\n");
	printf("3. 扬声器音量增加10%%\n");
	printf("4. 扬声器音量减小10%%\n");
	printf("5. 开启警示器\n");
	printf("6. 关闭警示器\n");
	printf("7. 开启手柄输出\n");
	printf("8. 关闭手柄输出\n");
	printf("9. 手柄音量增加10%%\n");
	printf("10. 手柄音量减小10%%\n");
	printf("11. 使能头戴耳机\n");
	printf("12. 禁止头戴耳机\n");
	printf("13. 头戴耳机音量增加10%%\n");
	printf("14. 头戴耳机音量减小10%%\n");
	printf("15. 选择面板音频输入\n");
	printf("16. 选择手柄音频输入\n");
	printf("17. 选择头戴耳机输入\n");
	printf("18. 获取头戴耳机插入状态\n");
	printf("19. 获取面板音频插入状态\n");
	printf("20. 设置PCM音量增加10%%\n");
	printf("21. 设置PCM音量减小10%%\n");
	printf("0. 退出测试程序\n");
	printf("非数字-返回主菜单\n");
	printf("其他数字. 继续该测试\n");
} 






static int audio_menu_control(void)
{
	int test_item_index = 0;
	int status = 0;
	while(1)
	{
		printf_audio_menu();
		INFO("请输入对应功能的数字:");

		if((test_item_index = get_stdin_a_num())==-1)
			return 1;	 //不再进行该测试，返回主菜单
	//	if(scanf("%d", &test_item_index) != 1) {
	//		return 1;   //不再进行该测试，返回主菜单
		//	ERR("您的输入有误，请重新输入\n");
		//	s_main_thread_exit = true;
		//	continue;
		

		switch(test_item_index)
		{
			case 0:
				return 0;  //函数退出，测试程序退出
			case 1:      //1. 开启扬声器
				drvEnableSpeaker();
				break;
			case 2:      //2. 关闭扬声器
				drvDisableSpeaker();
				break;
			case 3:     //3. 扬声器音量增加10%
				drvAddSpeakVolume(10);
				break;
			case 4:     //4. 扬声器音量减小10%
				drvSubSpeakVolume(10);
				break;
			case 5:     //5. 开启警示器
				drvEnableWarning();
				break;
			case 6:     //6. 关闭警示器
				drvDisableWarning();
				break;
			case 7:     //7. 开启手柄输出
				drvEnableHandout();
				break;
			case 8:     //8. 关闭手柄输出
				drvDisableHandout();
				break;
			case 9:     //9. 手柄音量增加10%
				drvAddHandVolume(10);
				break;
			case 10:     //10. 手柄音量减小10%
				drvSubHandVolume(10);
				break;
			case 11:     //11. 使能头戴耳机
				drvEnableEarphout();
				break;
			case 12:     //12. 禁止头戴耳机
				drvDisableEarphout();
				break;
			case 13:     //13. 头戴耳机音量增加10%
				drvAddEarphVolume(10);
				break;
			case 14:     //14. 头戴耳机音量减小10%
				drvSubEarphVolume(10);
				break;
			case 15:     //15. 选择面板音频输入
				drvSelectHandFreeMic();
				break;
			case 16:     //16. 选择手柄音频输入
				drvSelectHandMic();
				break;
			case 17:     //17. 选择头戴耳机输入
				drvSelectEarphMic();
				break;	
			case 18:     //18. 获取头戴耳机插入状态
				//int status = 0;
				if((status = drvGetMicStatus()) < 0) {
					ERR("Error drvGetMicStatus!");
					break;
				}
				INFO("Headset %s!\n", status? "insert":"no insert");
				break;
			case 19:     //19. 获取面板音频插入状态
				
				if((status = drvGetHMicStatus()) < 0) {
					ERR("Error drvGetMicStatus!");
					break;
				}
				INFO("Hand %s!\n", status? "insert":"no insert");
				break;
			case 20:     //20. 设置PCM音量增加10%
				drvSetTuneUp();
				break;
			case 21:     //21. 设置PCM音量减小10%
				drvSetTuneDown();
				break;	
			default:				
				break;
		}

	}
	return 1;
}



static void printf_temperature_rtc_menu(void)
{
	printf("温度RTC菜单：（使用方法: 请输入对应功能的数字） \n");
	printf("1. 读取CPU温度\n");
	printf("2. 读取主板温度\n");
	printf("3. 读取RTC\n");
	printf("4. 设置RTC\n");
	printf("0. 退出测试程序\n");
	printf("其他. 返回主菜单\n");
}



static int temperature_rtc_menu_control(void)
{
	int test_item_index = 0;
	float temp;
	long secs;
	while(1)
	{
		printf_temperature_rtc_menu();
		INFO("请输入对应功能的数字:");
		if((test_item_index = get_stdin_a_num())==-1)
			return 1;	 //不再进行该测试，返回主菜单
		//	ERR("您的输入有误，请重新输入\n");
		//	s_main_thread_exit = true;
		//	continue;
		

		switch(test_item_index)
		{
			case 0:
				return 0;  //函数退出，测试程序退出
			case 1:      //1. 读取CPU温度
				temp = drvGetCPUTemp();
				if(temp) {
					INFO("CPU temperature is %0.3f\n", temp);
				}
				else {
					ERR("Error get CPU temperature!");
				}
				break;
			case 2:      //2. 读取主板温度
				temp = drvGetBoardTemp();
				if(temp) {
					INFO("Interface board temperature is %0.3f\n", temp);
				}
				else {
					ERR("Error get interface board temperature!");
				};
				break;
			case 3:     //3. 读取RTC
				secs = 0;
				if((secs = drvGetRTC()) == EXIT_FAILURE) {
					ERR("Error drvGetRTC!");
					break;
				}
				INFO("RTC seconds: %ld\n", secs);
				break;
			case 4:     //4. 设置RTC
				secs = 0;
				INFO("Please input RTC seconds:");
				if((secs = get_stdin_a_num()) == -1) {//if(scanf("%ld", &secs) != 1) {
					ERR("您的输入有误，请重新输入\n");
					continue;
				}
				if(drvSetRTC(secs) == EXIT_FAILURE) {
					ERR("Error drvSetRTC!");
					break;
				}
				break;
			default:
				break;
		}		
	}
	return 1;
}



static void printf_key_lights_menu(void)
{
	printf("键灯控制菜单：（使用方法: 请输入对应功能的数字） \n");
	printf("1. 获取面板键灯的状态\n");
	printf("2. 设置对应键灯点亮\n");
	printf("3. 设置对应键灯熄灭\n");
	printf("4. 获取键灯类型\n");
	printf("5. 设置键灯闪烁\n");
	printf("6. 设置键灯亮度\n");
	printf("0. 退出测试程序\n");
	printf("其他. 返回主菜单\n");
}



static int key_lights_menu_control(void)
{
	int test_item_index = 0;
	int i;
	int nBrtVal = 0;
	int KeyIndex;
	while(1)
	{
		printf_key_lights_menu();
		INFO("请输入对应功能的数字:");
		if((test_item_index = get_stdin_a_num())==-1)
			return 1;	 //不再进行该测试，返回主菜单
		//	ERR("您的输入有误，请重新输入\n");
		//	s_main_thread_exit = true;
		//	continue;
		

		switch(test_item_index)
		{
			case 0:
				return 0;  //函数退出，测试程序退出
			case 1:      //1.获取面板键灯的状态
			//	int i = KEY_VALUE_MIN;
				for(i = KEY_VALUE_MIN; i <= KEY_VALUE_MAX; i ++) {
					INFO("Key %#02x is %s\n", i, drvGetLEDStatus(i)? "on":"off");
				}
				break;
			case 2:      //2. 设置对应键灯点亮
				INFO("Please input KeyIndex: (%u-%u)", KEY_VALUE_MIN, KEY_VALUE_MAX);
				if((KeyIndex = get_stdin_a_num()) == -1) {//if(scanf("%d", &KeyIndex) != 1) {
					ERR("您的输入有误，请重新输入\n");
					continue;
				}
				if(KeyIndex<KEY_VALUE_MIN || KeyIndex > KEY_VALUE_MAX){
					ERR("Error KeyIndex<KEY_VALUE_MIN || KeyIndex > KEY_VALUE_MAX");
					continue;
				}			
				drvLightLED(KeyIndex);
				break;
			case 3:     //3. 设置对应键灯熄灭
				INFO("Please input KeyIndex: (%u-%u)", KEY_VALUE_MIN, KEY_VALUE_MAX);
				if((KeyIndex = get_stdin_a_num()) == -1) {//if(scanf("%d", &KeyIndex) != 1) {
					ERR("您的输入有误，请重新输入\n");
					continue;
				}
				if(KeyIndex<KEY_VALUE_MIN || KeyIndex > KEY_VALUE_MAX){
					ERR("Error KeyIndex<KEY_VALUE_MIN || KeyIndex > KEY_VALUE_MAX");
					continue;
				}
				drvDimLED(KeyIndex);
				break;
			case 4:     //4. 获取键灯类型				
				INFO("Keyboard model is %#x\n", getKeyboardType());			
				break;
			case 5:  //5. 设置键灯闪烁
				INFO("Please input KeyIndex: (%u-%u)", KEY_VALUE_MIN, KEY_VALUE_MAX);
				if((KeyIndex = get_stdin_a_num()) == -1) {//if(scanf("%d", &KeyIndex) != 1) {
					ERR("您的输入有误，请重新输入\n");
					continue;
				}
				if(KeyIndex<KEY_VALUE_MIN || KeyIndex > KEY_VALUE_MAX){
					ERR("Error KeyIndex<KEY_VALUE_MIN || KeyIndex > KEY_VALUE_MAX");
					continue;
				}
				drvFlashLEDs(KeyIndex);
				break;
			case 6:  //6. 设置键灯亮度
				nBrtVal = 0;
				INFO("Please input brightness: (%u-%u)\n", PANEL_KEY_BRIGHTNESS_MIN, PANEL_KEY_BRIGHTNESS_MAX);
				if((nBrtVal = get_stdin_a_num()) == -1) {//if(scanf("%d", &nBrtVal) != 1) {
					ERR("您的输入有误，请重新输入\n");
					continue;
				}
				if(nBrtVal > 100 || nBrtVal < 0) {
					ERR("Error brightness out of range!");
					break;
				}
				drvSetLedBrt(nBrtVal);
				break;
			default:
				break;
		}		
	}
	return 1;
}




static void printf_Watchdog_menu(void)
{
	printf("看门狗控制菜单：（使用方法: 请输入对应功能的数字） \n");
	printf("1. 使能看门狗\n");
	printf("2. 禁止看门狗\n");
	printf("3. 设置看门狗超时时间\n");
	printf("4. 获取看门狗超时时间\n");
	printf("0. 退出测试程序\n");
	printf("其他. 返回主菜单\n");
}



static int Watchdog_menu_control(void)
{
	int test_item_index = 0;
	int timeout = 0;
	while(1)
	{
		printf_Watchdog_menu();
		INFO("请输入对应功能的数字:");
		if((test_item_index = get_stdin_a_num())==-1)
			return 1;	 //不再进行该测试，返回主菜单
		//	ERR("您的输入有误，请重新输入\n");
		//	s_main_thread_exit = true;
		//	continue;
		

		switch(test_item_index)
		{
			case 0:
				return 0;  //函数退出，测试程序退出
			case 1:      //1. 使能看门狗
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
			case 2:      //2. 禁止看门狗
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
			case 3:     //3. 设置看门狗超时时间
				timeout = 0;
				INFO("Please input TIMEOUT[1-250]:");
				if((timeout = get_stdin_a_num()) == -1) {//if(scanf("%d", &timeout) != 1) {
					ERR("您的输入有误，请重新输入\n");
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
			case 4:     //4. 获取看门狗超时时间
				timeout = drvWatchdogGetTimeout();
				INFO("Get hard watchdog timeout = %d!\n", timeout);				
				break;
			default:
				break;
		}		
	}
	return 1;
}





static void printf_LCDmisc_menu(void)
{
	printf("LCD及其他控制菜单：（使用方法: 请输入对应功能的数字） \n");
	printf("1. LCD开启熄灭控制\n");
	printf("2. LCD屏幕重启\n");
	printf("3. 调节屏幕亮度\n");
	printf("4. 获取LCD屏幕类型\n");
	printf("0. 退出测试程序\n");
	printf("其他. 返回主菜单\n");
}



static int LCDmisc_menu_control(void)
{
	int test_item_index = 0;
	int i = 0;
	int nBrtVal = 0;
	int type;
	while(1)
	{
		printf_LCDmisc_menu();
		INFO("请输入对应功能的数字:");
		if((test_item_index = get_stdin_a_num())==-1)
			return 1;	 //不再进行该测试，返回主菜单
		//	ERR("您的输入有误，请重新输入\n");
		//	s_main_thread_exit = true;
		//	continue;
		

		switch(test_item_index)
		{
			case 0:
				return 0;  //函数退出，测试程序退出
			case 1:      //1. LCD开启熄灭控制
				
				for(i = 0; i < 3; i ++) {
					drvDisableLcdScreen();
					INFO("Disable LCD screen success!");
					sleep(1);
					drvEnableLcdScreen();
					INFO("Enable LCD screen success!");
					sleep(1);
				}
				break;
			case 2:      //2. LCD屏幕重启
				if(drvLcdReset()) {
					ERR("Error drvLcdReset!");
				}
				break;
			case 3:     //3. 调节屏幕亮度
				nBrtVal = 0;
				INFO("Please input brightness: (%u-%u)\n", SCREEN_BRIGHTNESS_MIN, SCREEN_BRIGHTNESS_MAX);
				if((nBrtVal = get_stdin_a_num()) == -1) {//if(scanf("%d", &nBrtVal) != 1) {
					ERR("您的输入有误，请重新输入\n");
					continue;
				}
				if(nBrtVal > SCREEN_BRIGHTNESS_MAX || nBrtVal < SCREEN_BRIGHTNESS_MIN) {
					ERR("Error input brightness out of range!");
					break;
				}
				drvSetLcdBrt(nBrtVal);				
			break;		
			case 4:     //4. 获取LCD屏幕类型
				type = drvGetLCDType();
				if(type == 5 || type == 6) {
					INFO("LCD model is %#x\n", type);
				}
				else {
					ERR("Error drvGetLCDType %d\n", type);
				}				
				break;
			default:
				break;
		}		
	}
	return 1;
}



static void printf_Unrealized_menu(void)
{
	printf("未实现功能菜单：\n");
	printf("1. 使能USB0\n");
	printf("2. 禁用USB0\n");
	printf("3. 使能USB1\n");
	printf("4. 禁用USB1\n");
	printf("5. 使能触摸屏\n");
	printf("6. 禁用触摸屏\n");
	printf("7. 获取电压值\n");
	printf("8. 获取电流值\n");
	printf("9. 重启按键板\n");
	printf("0. 退出测试程序\n");
	printf("其他. 返回主菜单\n");
}


static int Unrealized_menu_control(void)
{
	int test_item_index = 0;
	float val;
	while(1)
	{
		printf_Unrealized_menu();
		INFO("请输入对应功能的数字:");
		if((test_item_index = get_stdin_a_num())==-1)
			return 1;	 //不再进行该测试，返回主菜单
		//	ERR("您的输入有误，请重新输入\n");
		//	s_main_thread_exit = true;
		//	continue;
		

		switch(test_item_index)
		{
			case 0:
				return 0;  //函数退出，测试程序退出
			case 1:      //1. 使能USB0
				drvEnableUSB0();
				break;
			case 2:      //2. 禁用USB0
				drvDisableUSB0();
				break;
			case 3:     //3. 使能USB1
				drvEnableUSB1();				
			break;		
			case 4:     //4. 禁用USB1
				drvDisableUSB1();				
				break;
			case 5:    //5. 使能触摸屏
				drvEnableTouchModule();
				break;
			case 6:    //6. 禁用触摸屏
				drvDisableTouchModule();
				break;
			case 7:    //7. 获取电压值
				val = drvGetVoltage();
				if(val <= 0) {
					ERR("Error drvGetVoltage!");
					break;
				}
				INFO("Current voltage is %.3f\n", val);
				break;
			case 8:    //8. 获取电流值
				val = drvGetCurrent();
				if(val <= 0) {
					ERR("Error drvGetCurrent!");
					break;
				}
				INFO("Current electricity is %.3f\n", val);
				break;
			case 9:    //9. 重启按键板
				drvIfBrdReset();
				break;
			default:
				break;
		}		
	}
	return 1;
}



#if 0
static void s_show_usage(void) {
	printf("Usage:\n");
	printf("\t%2d - Watchdog enable\n", TEST_ITEM_WATCHDOG_ENABLE);
	printf("\t%2d - Watchdog disable\n", TEST_ITEM_WATCHDOG_DISABLE);
	printf("\t%2d - Screen wake on/off\n", TEST_ITEM_SCREEN_ENABLE);
	printf("\t%2d - Speaker enable\n", TEST_ITEM_SPEAKER_ENABLE);
	printf("\t%2d - Speaker disable\n", TEST_ITEM_SPEAKER_DISABLE);
	printf("\t%2d - Speaker volume increase 10%%\n", TEST_ITEM_SPEAKER_VOLUME_ADD);
	printf("\t%2d - Speaker volume decrease 10%%\n", TEST_ITEM_SPEAKER_VOLUME_SUB);
	printf("\t%2d - Warning enable\n", TEST_ITEM_WARNING_ENABLE);
	printf("\t%2d - Warning disable\n", TEST_ITEM_WARNING_DISABLE);
	printf("\t%2d - Hand enable\n", TEST_ITEM_HAND_ENABLE);
	printf("\t%2d - Hand disable\n", TEST_ITEM_HAND_DISABLE);
	printf("\t%2d - Hand volume increase 10%%\n", TEST_ITEM_HAND_VOLUME_ADD);
	printf("\t%2d - Hand volume decrease 10%%\n", TEST_ITEM_HAND_VOLUME_SUB);
	printf("\t%2d - Headset enable\n", TEST_ITEM_HEADSET_ENABLE);
	printf("\t%2d - Headset disable\n", TEST_ITEM_HEADSET_DISABLE);
	printf("\t%2d - Headset volume increase 10%%\n", TEST_ITEM_HEADSET_VOLUME_ADD);
	printf("\t%2d - Headset volume decrease 10%%\n", TEST_ITEM_HEADSET_VOLUME_SUB);
	printf("\t%2d - USB0 enable\n", TEST_ITEM_USB0_ENABLE);
	printf("\t%2d - USB0 disable\n", TEST_ITEM_USB0_DISABLE);
	printf("\t%2d - USB1 enable\n", TEST_ITEM_USB1_ENABLE);
	printf("\t%2d - USB1 disable\n", TEST_ITEM_USB1_DISABLE);
	printf("\t%2d - Get CPU temperature\n", TEST_ITEM_GET_CPU_TEMPERATURE);
	printf("\t%2d - Get interface board temperature\n", TEST_ITEM_GET_INTERFACE_BOARD_TEMPERATURE);
	printf("\t%2d - Set keyboard led brightness\n", TEST_ITEM_SET_KEYBOARD_LED_BRIGHTNESS);
	printf("\t%2d - Get keyboard led on/off status\n", TEST_ITEM_GET_KEYBOARD_LED_ON_OFF);
	printf("\t%2d - Set a keyboard led on status\n", TEST_ITEM_SET_KEYBOARD_LED_ON);
	printf("\t%2d - Set a keyboard led off status\n", TEST_ITEM_SET_KEYBOARD_LED_OFF);
	printf("\t%2d - Set screen brightness\n", TEST_ITEM_SET_SCREEN_BRIGHTNESS);
	printf("\t%2d - Reset keyboard\n", TEST_ITEM_RESET_KEYBOARD);
	printf("\t%2d - Reset screen\n", TEST_ITEM_RESET_SCREEN);
	printf("\t%2d - Reset core board\n", TEST_ITEM_RESET_CORE_BOARD);
	printf("\t%2d - Get voltage\n", TEST_ITEM_GET_VOLGATE);
	printf("\t%2d - Get electricity\n", TEST_ITEM_GET_ELECTRICITY);
	printf("\t%2d - Get keyboard model\n", TEST_ITEM_GET_KEYBOARD_MODEL);
	printf("\t%2d - Get lcd model\n", TEST_ITEM_GET_LCD_MODEL);
	printf("\t%2d - Select panel mic\n", TEST_ITEM_AUDIO_SELECT_PANEL_MIC);
	printf("\t%2d - Select hand mic\n", TEST_ITEM_AUDIO_SELECT_HAND_MIC);
	printf("\t%2d - Select headset mic\n", TEST_ITEM_AUDIO_SELECT_HEADSET_MIC);
	printf("\t%2d - Touchscreen enable\n", TEST_ITEM_TOUCHSCREEN_ENABLE);
	printf("\t%2d - Touchscreen disable\n", TEST_ITEM_TOUCHSCREEN_DISABLE);
	printf("\t%2d - Get RTC\n", TEST_ITEM_GET_RTC);
	printf("\t%2d - Set RTC\n", TEST_ITEM_SET_RTC);
	printf("\t%2d - Get headset insert status\n", TEST_ITEM_GET_HEADSET_INSERT_STATUS);
	printf("\t%2d - Get handle insert status\n", TEST_ITEM_GET_HANDLE_INSERT_STATUS);
	printf("\t%2d - Set tune up\n", TEST_ITEM_SET_TUNE_UP);
	printf("\t%2d - Set tune Down\n", TEST_ITEM_SET_TUNE_DOWN);
	printf("\t%2d - Set Hard watchdog timerout\n", TEST_ITEM_SET_HWTD_TIMEOUT);
	printf("\t%2d - Get Hard watchdog timerout\n", TEST_ITEM_GET_HWTD_TIMEOUT);
	printf("\t%2d - Set  leds flash\n", TEST_ITEM_SET_LEDS_FLASH);
	printf("\t%2d - Print program build time\n", TEST_PRINT_VERSION);  //版本打印，保持为最后一项
	printf("\tOther - Exit\n");
}
#endif


static void s_sighandler(int signum) {
	INFO("Receive signal %d, program will be exit!\n", signum);
	s_main_thread_exit = 1;
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
	static int gpio_last = 0,val_last = 0;
	if((gpio == gpio_last) && (val == val_last))
		return;
	else
	{
		gpio_last = gpio;
		val_last = val;
	}
	INFO("gpio_Key %#x, value %#x\n", gpio, val);
}

static void s_panel_key_notify_func(int gpio, int val) {
	static int gpio_last = 0,val_last = 0;
	if((gpio == gpio_last) && (val == val_last))
		return;
	else
	{
		gpio_last = gpio;
		val_last = val;
	}	
	INFO("Key %#x, value %#x\n", gpio, val);
}




static void printf_main_menu(void)
{
	printf("主菜单：（使用方法: 请输入对应功能的数字） \n");
	printf("1. 音频控制\n");
	printf("2. 温度读取，RTC设置及读取\n");
	printf("3. 键灯控制及状态获取\n");
	printf("4. 看门狗设置及状态读取\n");
	printf("5. LCD控制功能：屏幕点亮熄灭，屏幕重启，屏幕类型获取\n");
	printf("6. 重启核心板\n");
	printf("7. 查看测试程序编译的时间\n");
	printf("9. 未实现功能：USB控制，电压读取，触摸屏功能，重启按键板\n");
	printf("0. 测试程序退出\n");
	printf("其他. 打印该帮助信息\n");
} 






int main(int args, char *argv[]) {
	int test_item_index = -1;
//	int KeyIndex = 0;
	printf("%s running,%s\n",argv[0],g_build_time_str);

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
		printf_main_menu();

		INFO("Please input test item:");
		if((test_item_index = get_stdin_a_num())==-1)
			continue;
		//	s_main_thread_exit = true;
		// 	continue;
		// }
		switch(test_item_index) {

			case 0:       //1. 主循环退出
				s_main_thread_exit = true;  
				break;
			case 1:       //1. 音频控制
				if(!audio_menu_control())  //返回0 程序退出
					s_main_thread_exit = true;  
				break;
			case 2:      //2. 温度读取，RTC设置及读取
				if(!temperature_rtc_menu_control())  //返回0 程序退出
					s_main_thread_exit = true;  
				break;
			case 3:      //3. 键灯控制及状态获取
				if(!key_lights_menu_control())  //返回0 程序退出
					s_main_thread_exit = true;  
				break;
			case 4:      //4. 看门狗设置及状态读取
				if(!Watchdog_menu_control())  //返回0 程序退出
					s_main_thread_exit = true;  
				break;
			case 5:      //5. LCD控制功能：屏幕点亮熄灭，屏幕重启，屏幕类型获取
				if(!LCDmisc_menu_control())  //返回0 程序退出
					s_main_thread_exit = true;  
				break;	
			case 6:      //6. 重启核心板
				if(drvCoreBrdReset()) {
					ERR("Error drvCoreBrdReset!");
				}
				break;		
			case 7:      //7. 查看测试程序编译的时间
				printf("%s running,Buildtime %s\n",argv[0],g_build_time_str);
				drvShowVersion();
				break;
			case 9:      //9. 未实现功能：USB控制，电压读取，触摸屏功能，重启按键板
				if(!Unrealized_menu_control())  //返回0 程序退出
					s_main_thread_exit = true;  
				break;

		default:
			// s_main_thread_exit = true;
			// printf("%s exit,Buildtime %s\n",argv[0],g_build_time_str);
			break;
		}
	}

	if(watchdog_feed_thread_id) {
		s_watchdog_feed_thread_exit = true;
		pthread_join(watchdog_feed_thread_id, NULL);
		watchdog_feed_thread_id = 0;
		drvWatchDogDisable();
	}
	drvCoreBoardExit();
	INFO("Exit %s\n", __func__);
	return 0;
}
