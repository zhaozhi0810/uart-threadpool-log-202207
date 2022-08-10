#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <linux/kd.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>             // exit
#include <sys/ioctl.h>          // ioctl
#include <string.h>             // bzero


#include <stdarg.h>


static int uinp_fd = -1; 
//static int uart_fd[MAX_PS2_COM_NUM] = {-1, -1};
static struct uinput_user_dev uinp; // uInput device structure 
//static uunsigned int key_leds_val = 0;


static const int key_map[]={KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6, 
KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_F11, KEY_F12, 
KEY_C, KEY_LEFTCTRL, KEY_H, KEY_R, KEY_P, KEY_1, 
KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, 
KEY_8, KEY_9, KEY_0, KEY_KPASTERISK, KEY_KPSLASH, KEY_UP,
KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_KPENTER, KEY_KPPLUS, KEY_KPMINUS};

#define JC_KEYBOARD_DRIVER_NAME		"jc_uart_keyboard"


// int get_uintput_device_fd( void )
// {
// 	return uinp_fd;
// }

// unsigned char get_key_led_val( void )
// {
// 	return (unsigned char)key_leds_val;
// }

//KeyState : 1按下， 0弹起
//key_index : 键值【0-35】
void send_a_button_ievent(unsigned char key_index, char KeyState)  
{ 
	// Report BUTTON CLICK - PRESS event 
	ssize_t ret;
	struct input_event event; // Input device structure 

	if(uinp_fd < 0)
	{
		printf(" ERROR: send_a_button_ievent-->uinput dev not open!!\n");
		return;
	}

	if(key_index > sizeof(key_map)/sizeof(key_map[0]))
	{
		printf(" ERROR: send_a_button_ievent-->key_index = %d too large\n",key_index);
		return ;
	}	

	memset(&event, 0, sizeof(event)); 
	
	gettimeofday(&event.time, NULL); 
	
	event.type = EV_KEY; 
	event.code = key_map[key_index]; 
	event.value = KeyState; 
	
	ret=write(uinp_fd, &event, sizeof(event)); 	
	if( -1 == ret )
	{
		printf("Error, send_a_button EV_KEY\n");
	}

	event.type = EV_SYN; 
	event.code = SYN_REPORT; 
	event.value = 0; 
	ret=write(uinp_fd, &event, sizeof(event)); 
	if( -1 == ret )
	{
		printf("Error, send_a_button  EV_SYN\n");
	}
//	printf("send_a_button VK= %d VKState = %d\n",key_map[key_index],KeyState);
} 


/* Setup the uinput device */ 
//返回0 成功，非0表示失败
int setup_uinput_device(void) 
{ 
	// Temporary variable 
	int i=0; 
	// Open the input device 
	uinp_fd = open("/dev/uinput", O_WRONLY | O_NDELAY); 
	if (uinp_fd == -1) 
	{ 
		printf("Unable to open /dev/uinput, err=%s\n", strerror(errno)); 
		return -1; 
	} 
//	printf("debug : uinp_fd=%d\n", uinp_fd);
	
	memset(&uinp, 0, sizeof(uinp)); // Intialize the uInput device to NULL 
	strncpy(uinp.name, JC_KEYBOARD_DRIVER_NAME, UINPUT_MAX_NAME_SIZE); 
	uinp.id.version = 4; 
	uinp.id.bustype = BUS_USB;
	
	// Setup the uinput device 
	ioctl (uinp_fd, UI_SET_EVBIT, EV_KEY); //按键按下
	ioctl (uinp_fd, UI_SET_EVBIT, EV_REP); //按键重复
	ioctl (uinp_fd, UI_SET_EVBIT, EV_SYN); //同步事件
//	ioctl(uinp_fd, UI_SET_EVBIT, EV_REL);  //按键释放
//	ioctl (uinp_fd, UI_SET_EVBIT, EV_LED); //LEDÊÂ¼þ
#if 0      // 鼠标
	ioctl(uinp_fd, UI_SET_EVBIT, EV_REL); 
   //ioctl(uinp_fd , UI_SET_EVBIT, EV_ABS);
	ioctl(uinp_fd, UI_SET_RELBIT, REL_X); 
	ioctl(uinp_fd, UI_SET_RELBIT, REL_Y);
	ioctl(uinp_fd, UI_SET_RELBIT, REL_WHEEL); //¹öÂÖ
#if 0
	ioctl(uinp_fd , UI_SET_ABSBIT, ABS_X);
	ioctl(uinp_fd , UI_SET_ABSBIT, ABS_Y);
	ioctl(uinp_fd , UI_SET_ABSBIT, ABS_PRESSURE);
#endif
#endif

	for (i=0; i < sizeof(key_map)/sizeof(key_map[0]); i++) 
	{ 
		ioctl(uinp_fd, UI_SET_KEYBIT, key_map[i]); 
	}
	
#if 0
	//ioctl(uinp_fd, UI_SET_KEYBIT, BTN_MOUSE); 
	//ioctl(uinp_fd, UI_SET_KEYBIT, BTN_TOUCH); 
	ioctl(uinp_fd, UI_SET_KEYBIT, BTN_MOUSE); 
	ioctl(uinp_fd, UI_SET_KEYBIT, BTN_LEFT); 
	ioctl(uinp_fd, UI_SET_KEYBIT, BTN_MIDDLE); 
	ioctl(uinp_fd, UI_SET_KEYBIT, BTN_RIGHT); 
	//ioctl(uinp_fd, UI_SET_KEYBIT, BTN_FORWARD); 
	//ioctl(uinp_fd, UI_SET_KEYBIT, BTN_BACK);
#endif
#if 0
    //LEDS
	ioctl(uinp_fd, UI_SET_LEDBIT, LED_NUML);
	ioctl(uinp_fd, UI_SET_LEDBIT, LED_CAPSL);
	ioctl(uinp_fd, UI_SET_LEDBIT, LED_SCROLLL);
#endif
	/* Create input device into input sub-system */ 
	if ( write(uinp_fd, &uinp, sizeof(uinp)) == -1)
	{
		printf("ERROR, setup_uinput_device --> write failed\n");
		return -1;
	}
	
	if (ioctl(uinp_fd, UI_DEV_CREATE)) 
	{ 
		 printf("Unable to create UINPUT device.errno=%s\n", strerror(errno)); 
		 return -1; 
	} 

	return 0; 
}


//设备关闭
void uinput_device_close(void)
{
	ioctl(uinp_fd, UI_DEV_DESTROY);
	close(uinp_fd); 
}


