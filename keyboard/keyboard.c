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
// static KEYBOARD_INFO_S s_keyboard_info;

#define PROC_INPUT_DEVICES		"/proc/bus/input/devices"

static char event_dev_name[128];


static void *s_recv_event_thread(void *arg) {
	fd_set readfds;
	int event_dev_fd = 0;
	struct input_event ts;

	CHECK((event_dev_fd = open(event_dev_name, O_RDONLY, 0)) > 0, NULL, "Error open with %d: %s", errno, strerror(errno));
	INFO("Enter %s", __func__);
	while(!s_recv_event_thread_exit) {
		struct timeval timeout = {1, 0};
		FD_ZERO(&readfds);
		FD_SET(event_dev_fd, &readfds);
		if(select(event_dev_fd + 1, &readfds, NULL, NULL, &timeout) < 0) {
			ERR("Error select with %d: %s", errno, strerror(errno));
			s_recv_event_thread_exit = true;
		}
		else if(FD_ISSET(event_dev_fd, &readfds)) {
			if(s_key_notify_func) {
				bzero(&ts, sizeof(struct input_event));
				if(read(event_dev_fd, &ts, sizeof(struct input_event)) != sizeof(struct input_event)) {
					ERR("Error read with %d: %s", errno, strerror(errno));
					continue;
				}
				if(ts.type == EV_KEY) {
					s_key_notify_func(ts.code, ts.value);
				}
			}
		}
	}
	CHECK(!close(event_dev_fd), NULL, "Error close with %d: %s", errno, strerror(errno));
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
		if(strstr(lineptr, "Name") && strstr(lineptr, name)) {
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
	CHECK(!fclose(input_device_fd), -1, "Error fclose with %d: %s", errno, strerror(errno));
	return input_device_num;
}



int keyboard_init(void) {
	int input_device_num = 0;
	//memset(&s_keyboard_info, -1, sizeof(KEYBOARD_INFO_S));
	CHECK((input_device_num = get_event_dev(JC_KEYBOARD_DRIVER_NAME)) >= 0, -1, "Error get_event_dev! \nPlease make sure drvserver is running!!\n");
	printf("debug : input_device_num = %d\n",input_device_num);
	snprintf(event_dev_name, sizeof(event_dev_name), "/dev/input/event%d", input_device_num);
//	snprintf(s_keyboard_info.keyboard_dev_name, sizeof(s_keyboard_info.keyboard_dev_name), "/dev/%s", JC_KEYBOARD_DRIVER_NAME);
	CHECK(!pthread_create(&s_recv_key_event_thread_id, NULL, s_recv_event_thread, NULL), -1, "Error pthread_create with %d: %s", errno, strerror(errno));
	return 0;
}

int keyboard_exit(void) {
	s_recv_event_thread_exit = true;
	CHECK(!pthread_join(s_recv_key_event_thread_id, NULL), -1, "Error pthread_join with %d: %s", errno, strerror(errno));
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
