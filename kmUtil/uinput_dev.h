

#ifndef __UINPUT_DEV_H__
#define __UINPUT_DEV_H__



//返回0 成功，非0表示失败
/* Setup the uinput device */ 
int setup_uinput_device(void) ;


//KeyState : 1按下， 0弹起
//key_index : 键值【0-35】
void send_a_button_ievent(unsigned char key_index, char KeyState)  ; //1°´ÏÂ£¬ 0µ¯Æð



//设备关闭
void uinput_device_close(void);
#endif
