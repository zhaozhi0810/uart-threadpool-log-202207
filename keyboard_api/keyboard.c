/*
 * keyboard.c
 *
 *  Created on: Dec 2, 2021
 *      Author: zlf
 */

#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <linux/input.h>
#include <stdlib.h>

#include "debug.h"
// #include "drv722.h"
// #include "fs.h"
#include "keyboard.h"

#define JC_KEYBOARD_DRIVER_NAME		"jc_uart_keyboard"   //注意与uart_to_mcu保持一致
// #define JC_KEYBOARD_BRIGHTNESS_MIN	(0)
// #define JC_KEYBOARD_BRIGHTNESS_MAX	(100)

// typedef struct {
// 	char event_dev_name[32];
// 	char keyboard_dev_name[32];
// 	char model;
// 	char version;
// } KEYBOARD_INFO_S;

static bool s_recv_event_thread_exit = false;
static pthread_t s_recv_key_event_thread_id = 0;
static GPIO_NOTIFY_KEY_FUNC s_key_notify_func = NULL;
static GPIO_NOTIFY_FUNC s_gpio_notify_func = NULL;
// static KEYBOARD_INFO_S s_keyboard_info;

#define PROC_INPUT_DEVICES		"/proc/bus/input/devices"

static char event_dev_name[128];
static char gpio_event_dev[64] = "\0";   //2022-08-15

static void *s_recv_event_thread(void *arg) {
	fd_set readfds;
	int event_dev_fd = 0;
	int gpio_event_dev_fd = 0;  //2022-08-15增加
	struct input_event ts;
	int max_fd = 0;

	CHECK((gpio_event_dev_fd = open(gpio_event_dev, O_RDONLY)) > 0, NULL, "Error open with %d: %s", errno, strerror(errno));
	CHECK((event_dev_fd = open(event_dev_name, O_RDONLY, 0)) > 0, NULL, "Error open with %d: %s", errno, strerror(errno));
	max_fd = event_dev_fd;
	if(gpio_event_dev_fd > max_fd)
		max_fd = gpio_event_dev_fd;
	INFO("Enter %s", __func__);
	
	//线程主循环
	while(!s_recv_event_thread_exit) {
	//	struct timeval timeout = {1, 0};
		FD_ZERO(&readfds);
		FD_SET(event_dev_fd, &readfds);
		FD_SET(gpio_event_dev_fd, &readfds);
		if(select(max_fd + 1, &readfds, NULL, NULL, NULL) < 0) {   //不需要超时设置，阻塞型
			ERR("Error select with %d: %s", errno, strerror(errno));
			s_recv_event_thread_exit = true;
		}
		else if(FD_ISSET(event_dev_fd, &readfds)) {			
		//	printf("s_recv_event_thread event_dev_fd\n");
			bzero(&ts, sizeof(struct input_event));
			if(read(event_dev_fd, &ts, sizeof(struct input_event)) != sizeof(struct input_event)) {
				ERR("Error read with %d: %s", errno, strerror(errno));
				if(errno == ENODEV) //这个错误表示服务进程退出了！！！！
					exit(-1);  //进程退出 //s_recv_event_thread_exit = true;  //设备出现问题，退出
				continue;
			}
			if(ts.type == EV_KEY) {	
				INFO("panel : Key %#x, value %#x", ts.code, ts.value);
			//	printf("KEY_P = %d\n",KEY_P);
				if((ts.code == KEY_P) && s_gpio_notify_func){  //对PTT按键特殊处理一下！！					
					s_gpio_notify_func(ts.code, ts.value);					
				//	printf("ptt1\n");
				}
				else //除了PTT以外的按键
				{
					if(s_key_notify_func) {
						s_key_notify_func(ts.code, ts.value);
					}
				}
			}
			
		}
		else if(FD_ISSET(gpio_event_dev_fd, &readfds))  //2022-08-15
		{
		//	printf("s_recv_event_thread gpio_event_dev_fd\n");
			bzero(&ts, sizeof(struct input_event));
			if(read(gpio_event_dev_fd, &ts, sizeof(struct input_event)) != sizeof(struct input_event)) {
				ERR("Error read with %d: %s\n", errno, strerror(errno));
				if(errno == ENODEV)
						exit(-1);  //进程退出 //s_recv_event_thread_exit = true;  //设备出现问题，退出
					continue;
			}
			if(ts.type == EV_KEY) {
				if(ts.code == KEY_A) //KEY_A == 30  hand PTT 按键值
				{
					//通知单片机，控制micctrl （PD6）引脚
					api_handptt_change(ts.value);	
				}

				//if(gpio_notify_func(ts.code, ts.value)) {
				INFO("Key %#x, value %#x", ts.code, ts.value);
				//}

				if(s_gpio_notify_func)
				{
			//		printf("if(s_gpio_notify_func)\n");
					s_gpio_notify_func(ts.code,ts.value);
				}
				// if(input_event.code == egn_earmic) {   //耳机插入
				// 	s_egn_earmic_insert = input_event.value? true:false;
				// }
				// else if(input_event.code == egn_hmic) {  //手柄插入状态
				// 	s_egn_hmic_insert = input_event.value? true:false;
				// }
			}
		}
	}
	CHECK(!close(event_dev_fd), NULL, "Error close with %d: %s", errno, strerror(errno));
	CHECK(!close(gpio_event_dev_fd), NULL, "Error close with %d: %s", errno, strerror(errno));
	INFO("Exit %s", __func__);
	return NULL;
}




static int get_event_dev(char *name) {
	CHECK(name, -1, "Error name is null!");
	CHECK(strlen(name), -1, "Error name lenth is 0");

	FILE *input_device_fd = NULL;
	char *lineptr = NULL;
	int input_device_num = -1;
	size_t n = 0;

	CHECK(input_device_fd = fopen(PROC_INPUT_DEVICES, "r"), -1, "Error fopen with %d: %s", errno, strerror(errno));
	while(getline(&lineptr, &n, input_device_fd) != EOF) {
	//	printf("APILIB: 1debug lineptr = %s name = %s\n",lineptr,name);
		if(strstr(lineptr, "Name") && strstr(lineptr, name)) {
	//		printf("APILIB: 2debug lineptr = %s name = %s\n",lineptr,name);
			char *strstrp = NULL;
			while(getline(&lineptr, &n, input_device_fd) != EOF) {
				if((strstrp = strstr(lineptr, "Handlers"))) {
					if((strstrp = strstr(strstrp, "event"))) {
						strstrp += strlen("event");
						input_device_num = atoi(strstrp);
					}
					break;
				}
			}
			break;
		}
	}
	if(lineptr) {
		free(lineptr);
		lineptr = NULL;
	}

//	printf("APILIB: input_device_num = %d\n",input_device_num);

	CHECK(!fclose(input_device_fd), -1, "Error fclose with %d: %s", errno, strerror(errno));
	return input_device_num;
}


static int gpio_event_init(void) {	
	int input_device_num = 0;
	INFO("Enter %s", __func__);
	CHECK((input_device_num = get_event_dev("gpio-keys")) >= 0, -1, "Error s_get_event_dev!");
	snprintf(gpio_event_dev, sizeof(gpio_event_dev), "/dev/input/event%d", input_device_num);
	return 0;
}



int keyboard_init(void) {
	int input_device_num = 0;

	gpio_event_init();   //GPIO event 初始化

	//memset(&s_keyboard_info, -1, sizeof(KEYBOARD_INFO_S));
	CHECK((input_device_num = get_event_dev(JC_KEYBOARD_DRIVER_NAME)) >= 0, -1, "Error get_event_dev! \nPlease make sure drvserver is running!!\n");
	printf("debug : input_device_num = %d\n",input_device_num);
	snprintf(event_dev_name, sizeof(event_dev_name), "/dev/input/event%d", input_device_num);
//	snprintf(s_keyboard_info.keyboard_dev_name, sizeof(s_keyboard_info.keyboard_dev_name), "/dev/%s", JC_KEYBOARD_DRIVER_NAME);
	CHECK(!pthread_create(&s_recv_key_event_thread_id, NULL, s_recv_event_thread, NULL), -1, "Error pthread_create with %d: %s", errno, strerror(errno));
	pthread_detach(s_recv_key_event_thread_id);  //设置分离模式，自动释放资源
	return 0;
}






int keyboard_exit(void) {
	s_recv_event_thread_exit = true;
//	CHECK(!pthread_join(s_recv_key_event_thread_id, NULL), -1, "Error pthread_join with %d: %s", errno, strerror(errno));
	return 0;
}

// static int s_ioctl_cmd(int cmd, char *val) {
// 	// int dev_fd = 0;
// 	// CHECK((dev_fd = open(s_keyboard_info.keyboard_dev_name, O_RDWR, 0)) > 0, -1, "Error open with %d: %s", errno, strerror(errno));
// 	// if(ioctl(dev_fd, cmd, val)) {
// 	// 	ERR("Error ioctl with %d: %s", errno, strerror(errno));
// 	// 	CHECK(!close(dev_fd), -1, "Error close with %d: %s", errno, strerror(errno));
// 	// 	return -1;
// 	// }
// 	// CHECK(!close(dev_fd), -1, "Error close with %d: %s", errno, strerror(errno));
// 	return 0;
// }



void __drvSetGpioKeyCbk(GPIO_NOTIFY_KEY_FUNC cbk) {
	s_key_notify_func = cbk;
}


void __drvSetGpioCbk(GPIO_NOTIFY_FUNC cbk) {
//	printf("__drvSetGpioCbk\n");
	s_gpio_notify_func = cbk;
}

// void drvSetLedBrt(int nBrtVal) {
// 	CHECK(nBrtVal >= JC_KEYBOARD_BRIGHTNESS_MIN && nBrtVal <= JC_KEYBOARD_BRIGHTNESS_MAX, , "Error nBrtVal %d out of range!", nBrtVal);
// 	char val = nBrtVal;
// 	CHECK(!s_ioctl_cmd(KEYBOARD_IOC_SET_BRIGHTNESS, &val), , "Error s_ioctl_cmd!");
// 	nBrtVal = val;
// }

// void drvLightLED(int nKeyIndex) {
// 	char val = nKeyIndex;
// 	CHECK(!s_ioctl_cmd(KEYBOARD_IOC_KEY_LED_ON, &val), , "Error s_ioctl_cmd!");
// }

// void drvDimLED(int nKeyIndex) {
// 	char val = nKeyIndex;
// 	CHECK(!s_ioctl_cmd(KEYBOARD_IOC_KEY_LED_OFF, &val), , "Error s_ioctl_cmd!");
// }

// void drvLightAllLED(void) {
// 	ERR("Error non-supported!");
// }

// void drvDimAllLED(void) {
// 	ERR("Error non-supported!");
// }

// int drvGetLEDStatus(int nKeyIndex) {
// 	ERR("Error non-supported!");
// 	return -1;
// }

// int drvIfBrdReset(void) {
// 	CHECK(!s_ioctl_cmd(KEYBOARD_IOC_RESET, NULL), -1, "Error s_ioctl_cmd!");
// 	return 0;
// }
