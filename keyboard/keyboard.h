/*
 * keyboard.h
 *
 *  Created on: Nov 8, 2021
 *      Author: zlf
 */

#ifndef KEYBOARD_KEYBOARD_H_
#define KEYBOARD_KEYBOARD_H_

//#include "drv_22134_api.h"

extern int keyboard_init(void);
extern int keyboard_exit(void);

//按键处理线程函数
//void *s_recv_event_thread(void *arg);

//按键处理回调函数指针
//GPIO_NOTIFY_KEY_FUNC s_key_notify_func;
typedef void (*GPIO_NOTIFY_KEY_FUNC)(int gpio, int val);
void __drvSetGpioKeyCbk(GPIO_NOTIFY_KEY_FUNC cbk);

#endif /* KEYBOARD_KEYBOARD_H_ */
