/*CommFunc.h*/
#ifndef __COMM_FUNC_H__
#define __COMM_FUNC_H__

#define COM_KEYBOARD_NO 0
#define COM_MOUSE_NO 1
#define MAX_PS2_COM_NUM 2 //串口鼠标和串口键盘，每个都占用一个串口 

int get_uintput_device_fd( void );

QQUEUE get_key_cmd_queue( void );

QQUEUE get_mouse_cmd_queue( void );

uint8_t get_key_led_val( void );

#endif
