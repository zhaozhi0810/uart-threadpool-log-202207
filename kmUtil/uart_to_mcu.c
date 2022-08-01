// main.c 
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
#include <pthread.h>
#include <semaphore.h>

#include <stdarg.h>

#include "ComFunc.h"
#include "queue.h"
#include "uart_to_mcu.h"
//#include "Common.h"


/*global defines*/
static QUEUE keyCmdQueue;
//static QUEUE mouseCmdQueue;
static int uinp_fd = -1; 
//static int uart_fd[MAX_PS2_COM_NUM] = {-1, -1};
static struct uinput_user_dev uinp; // uInput device structure 
//static uunsigned int key_leds_val = 0;

static 	int p_opt = 0;   //ÊÇ·ñÒª´òÓ¡³ö½ÓÊÕµ½µÄ×Ö·û£¿
static 	int uart_fd;

static volatile	unsigned short uart_recv_flag = 0;   //´®¿Ú½ÓÊÕ±êÖ¾£¬ÓÃÓÚµ¥Æ¬»úµÄÓ¦´ð£¬¸ß8Î»±íÊ¾×´Ì¬£¬µÍ8Î»±íÊ¾ÃüÁî


//#define FRAME_LENGHT (sizeof(com_frame_t)+1)    //Êý¾ÝÖ¡µÄ×Ö½ÚÊý
#define FRAME_HEAD 0xa5   //×¢ÒâÓëµ¥Æ¬»ú±£³ÖÒ»ÖÂ



#define COM_DATLEN 4 //´®¿ÚµÄÊý¾Ý³¤¶ÈÎª4¸ö×Ö½Ú 0x5f + dat1 + dat2 + checksum
#define FRAME_HEAD 0xa5
static unsigned char com_recv_data[COM_DATLEN*2];   //½ÓÊÕ»º´æ


const int key_map[]={KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6, 
KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_F11, KEY_F12, 
KEY_C, KEY_LEFTCTRL, KEY_H, KEY_R, KEY_P, KEY_1, 
KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, 
KEY_8, KEY_9, KEY_0, KEY_KPASTERISK, KEY_KPSLASH, KEY_UP,
KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_KPENTER, KEY_KPPLUS, KEY_KPMINUS};



// QQUEUE get_key_cmd_queue( void )
// {
// 	return (QQUEUE)&keyCmdQueue;
// }

// QQUEUE get_mouse_cmd_queue( void )
// {
// 	return (QQUEUE)&mouseCmdQueue;
// }

// int get_uintput_device_fd( void )
// {
// 	return uinp_fd;
// }

// unsigned char get_key_led_val( void )
// {
// 	return (unsigned char)key_leds_val;
// }



static void send_a_button_ievent(char VK, char VKState)  //1°´ÏÂ£¬ 0µ¯Æð
{ 
	// Report BUTTON CLICK - PRESS event 
	ssize_t ret;
	struct input_event event; // Input device structure 
//	int uinp_fd = get_uintput_device_fd();

	memset(&event, 0, sizeof(event)); 
	
	gettimeofday(&event.time, NULL); 
	
	event.type = EV_KEY; 
	event.code = VK; 
	event.value = VKState; 
	ret=write(uinp_fd, &event, sizeof(event)); 
	if( -1 == ret )
	{
		printf("Error, send_a_button\n");
	}

	event.type = EV_SYN; 
	event.code = SYN_REPORT; 
	event.value = 0; 
	ret=write(uinp_fd, &event, sizeof(event)); 
	if( -1 == ret )
	{
		printf("Error, send_a_button\n");
	}
	printf("send_a_button VK= %d VKState = %d\n",VK,VKState);
} 




/* Setup the uinput device */ 
static int setup_uinput_device( void ) 
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
	printf("uinp_fd=%d\n", uinp_fd);
	
	memset(&uinp, 0, sizeof(uinp)); // Intialize the uInput device to NULL 
	strncpy(uinp.name, "uart keyboard_mouse", UINPUT_MAX_NAME_SIZE); 
	uinp.id.version = 4; 
	uinp.id.bustype = BUS_USB;
	
	// Setup the uinput device 
	ioctl (uinp_fd, UI_SET_EVBIT, EV_KEY); //°´¼ü°´ÏÂ
	ioctl (uinp_fd, UI_SET_EVBIT, EV_REP); //°´¼üÊÍ·Å
	ioctl (uinp_fd, UI_SET_EVBIT, EV_SYN); //Í¬²½ÊÂ¼þ
//	ioctl (uinp_fd, UI_SET_EVBIT, EV_LED); //LEDÊÂ¼þ
#if 0  //Êó±êÊÂ¼þ
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

	for (i=0; i < 256; i++) 
	{ 
		ioctl(uinp_fd, UI_SET_KEYBIT, i); 
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
		printf("ERROR, write failed\n");
		return -1;
	}
	
	if (ioctl(uinp_fd, UI_DEV_CREATE)) 
	{ 
		 printf("Unable to create UINPUT device.errno=%s\n", strerror(errno)); 
		 return -1; 
	} 

	return 1; 
}



//´®¿ÚÏûÏ¢´¦Àí
static void com_message_handle(void)
{		
	if(com_recv_data[1]>0 && com_recv_data[1] < 37)   //°´¼üÖµ
	{
		printf("key = %d %s\n",com_recv_data[1],com_recv_data[2]?"press":"release");
		send_a_button_ievent(key_map[com_recv_data[1]-1], com_recv_data[2]);
	}
	else  //ÆäËû¶¨ÒåµÄ½Ó¿ÚÊý¾Ý
	{
		switch(com_recv_data[1])
		{
			case eMCU_LED_SETON_TYPE: //ÉèÖÃled ON			
			case eMCU_LED_SETOFF_TYPE: //ÉèÖÃled OFF		
			case eMCU_LCD_SETONOFF_TYPE:  //ÉèÖÃlcd ´ò¿ª»òÕß¹Ø±Õ		
			case eMCU_LEDSETALL_TYPE:  //ÉèÖÃËùÓÐµÄled ´ò¿ª»òÕß¹Ø±Õ			
			case eMCU_LED_STATUS_TYPE:		  //led ×´Ì¬»ñÈ¡			
				uart_recv_flag = com_recv_data[1] | (com_recv_data[2] <<8);  //¸ß8Î»±íÊ¾×´Ì¬		
				break;
			default:
				uart_recv_flag = 0;
				printf("ERROR:unknown uart recv\n");
				printf("com_recv_data[1] = %d com_recv_data[2] = %d\n",com_recv_data[1],com_recv_data[2]);
			break;
		}
		//调试信息
	//	printf("com_recv_data[1] = %d com_recv_data[2] = %d\n",com_recv_data[1],com_recv_data[2]);
	}
}


static unsigned char checksum(unsigned char *buf, unsigned char len)
{
	unsigned char sum;
	unsigned char i;

	for(i=0,sum=0; i<len; i++)
		sum += buf[i];
#ifdef PRINT_DEBUG
	printf("check sum = %x\n",sum);
#endif
	return sum;
}



//Ð£ÑéÊý¾Ý
static unsigned int verify_data(unsigned char *data,unsigned char len)
{
	unsigned char check;
	unsigned int ret = -1;
	int i;
	if(data == NULL)
		return -1;
#ifdef PRINT_DEBUG	
	for(i = 0;i<len;i++)
		printf("data[%d] = %x \n",i,data[i]);
	printf("\n\n");
#endif	
	//¶ÁÈ¡Ô­Êý¾ÝÖÐµÄÐ£ÑéÖµ
	check = data[len - 1];
	
	//ÖØÐÂ¼ÆËãÐ£ÑéÖµ
	if(check == checksum(data,len - 1))
		ret = 0;
	
	return ret;
}





static void ProcRecvKeyCmd(void)
{
//	unsigned int err;
	unsigned char length,i,j;
	static unsigned char datalen =COM_DATLEN,offset = 0;  //ÕâÁ½¸ö±äÁ¿ÓÃÓÚÖ¡´íÎóµÄÇé¿ö
	char ispt = 0;  //ÊÕµ½µÄÊý¾ÝÐèÒª´òÓ¡Âð


	while(1)  //¿¼ÂÇµ½¿ÉÄÜ½ÓÊÕµ½¶àÖ¡Òª´¦ÀíµÄÇé¿ö
	{		
		length = queue_length(&keyCmdQueue);	
		if(length < datalen)   //£¨°üÀ¨Ö¡Í·£©Ð¡ÓÚ4¸ö×Ö½Ú£¬²»ÊÇÍêÕûµÄÒ»Ö¡
		{	
			//com3_answer_3A3000(RECV_NO);   //Ó¦´ð½ÓÊÕµ½µÄ³¤¶È²»¶Ô
			return ;   //¼ÌÐøµÈ´ý
		}	
		length = datalen;   //¼ÆËãÐèÒª¶Á³öµÄ×Ö½ÚÊý
		for(i=0;i<length;i++)
		{
			//ÕâÀï²»ÅÐ¶Ï´íÎóÁË£¬Ç°ÃæÒÑ¾­¼ì²âÈ·ÊµÓÐÕâÃ´¶à×Ö½Ú¡£
			out_queue(&keyCmdQueue,com_recv_data+i+offset) ;  //com_data ¿Õ³ö1¸ö×Ö½Ú£¬ÎªÁË¼æÈÝÖ®Ç°µÄÐ£ÑéºÍËã·¨£¬¼°Êý¾Ý½âÎöËã·¨
			if(ispt)
				printf("com_recv_data[%d] = %x \n",i,com_recv_data[i+offset]);
		}
	//	com3_data[0] = FRAME_HEAD;  //¼ÓÈëÖ¡Í·½øÐÐÐ£ÑéºÍ¼ÆËã
		if((com_recv_data[0] == FRAME_HEAD) && (0 == verify_data(com_recv_data,COM_DATLEN)))   //µÚ¶þ²ÎÊýÊÇÊý¾Ý×Ü³¤¶È£¬°üÀ¨Ð£ÑéºÍ¹²7¸ö×Ö½Ú
		{
			//Ð£ÑéºÍÕýÈ·¡£
			//com3_answer_3A3000(RECV_OK);
			com_message_handle();		
		}	
		else  //Ð£ÑéºÍ²»ÕýÈ·£¬¿ÉÄÜÊÇÖ¡ÓÐ´íÎó¡£
		{
			for(i=1;i<COM_DATLEN;i++)   //Ç°ÃæµÄÅÐ¶Ï³öÎÊÌâ£¬¿¼ÂÇÊÇÖ¡´íÎó£¬Ñ°ÕÒÏÂÒ»¸öÖ¡Í·£¡£¡£¡
			{
				if(com_recv_data[i] == FRAME_HEAD)   //ÖÐ¼äÕÒµ½Ò»¸öÖ¡Í·
				{
					break;
				}
			}		
			if(i != COM_DATLEN) //ÔÚÊý¾ÝÖÐ¼äÕÒµ½Ö¡Í·£¡£¡£¡
			{
				datalen = i;   //ÏÂÒ»´ÎÐèÒª¶ÁµÄ×Ö½ÚÊý
				offset = COM_DATLEN-i;  //´æ´¢µÄÆ«ÒÆÎ»ÖÃµÄ¼ÆËã

				for(j=0;i<COM_DATLEN;i++,j++)   //ÓÐ¿ÉÄÜÖ¡Í·²»¶Ô£¬ËùÒÔµÚÒ»¸ö×Ö½Ú»¹ÊÇÒª¿½±´Ò»ÏÂ
				{
					com_recv_data[j] = com_recv_data[i];   //°ÑÊ£ÏÂµÄ¿½±´¹ýÈ¥
				}
			}
			else  //ÔÚÊý¾ÝÖÐ¼äÃ»ÓÐÕÒµ½Ö¡Í·
			{
				datalen = COM_DATLEN;  //	ÏÂÒ»´ÎÐèÒª¶ÁµÄ×Ö½ÚÊý
				offset = 0;
			}
		}	
	}//end while 1
}


static void* info_recv_proc_func(void)
{
	int readn=0, i=0;
	char rBuf[16]={0};
#ifdef PRINT_DEBUG
    printf("info_recv_proc_func\n");              
#endif
	while(1)
	{
		readn = PortRecv(uart_fd, rBuf,COM_DATLEN);
#ifdef PRINT_DEBUG
		printf("info_recv_proc_func readn = %d\n",readn);
#endif
		if(p_opt)
			printf("info_recv_proc_func recive printf \n");
		for (i=0; i<readn; i++)
		{
			if(p_opt)
				printf("rBuf[%d] = %x \n",i,rBuf[i]);
			if ( 0 == add_queue( &keyCmdQueue, rBuf[i]))
			{
				printf("Error, cann't add queue to keyCmdQueue\n");	
			}
		
			//´¦Àí½ÓÊÕµÄÏûÏ¢
			ProcRecvKeyCmd();
		}	
	}//while(1)

}



//¸ºÔð½ÓÊÕÊý¾ÝµÄÏß³Ì
/*
 * arg ´«³ö²ÎÊý£¬´®¿ÚÊÕµ½Êý¾Ý£¬ÒªÐÞ¸ÄÈ«¾Ö±äÁ¿£¬ËùÒÔargÖ¸ÏòÈ«¾ÖÊý¾Ý½á¹¹
 * 
 * ·µ»ØÖµ£º
 * 	¸ù¾ÝÏß³Ìº¯Êý¶¨Òå£¬ÎÞÒâÒå£¬¸Ãº¯Êý²»·µ»Ø£¡£¡£¡£¡
 * 
 * */
void* mcu_recvSerial_thread(void* arg)
{
	while(1)
	{
		info_recv_proc_func();
	}
}





//·¢ËÍÊý¾Ý£¬²»ÓÉµ¥¶ÀµÄÏß³Ì´¦ÀíÁË¡£dataÖ»ÐèÒª°üº¬Êý¾ÝÀàÐÍºÍÊý¾Ý¡£Í·²¿ºÍcrcÓÉ¸Ãº¯ÊýÍê³É¡£
/*
 * data ÓÃÓÚ·¢ËÍµÄÊý¾Ý£¬²»ÐèÒª°üÀ¨Ö¡Í·ºÍÐ£ÑéºÍ£¬Ö»Òª°üÀ¨Êý¾ÝÀàÐÍºÍÊý¾Ý£¨¹²2¸ö×Ö½Ú£©
 * ·µ»ØÖµ
 * 	0±íÊ¾³É¹¦£¬ÆäËû±íÊ¾Ê§°Ü
 * */
int send_mcu_data(const void* data)
{	
	unsigned char buf[8];  	
	int i;
	
	buf[0] = FRAME_HEAD;  //Ö¡Í·	
	memcpy(buf+1,data,sizeof(com_frame_t)-1);    //¿½±´

	//crcÖØÐÂ¼ÆËã
	buf[sizeof(com_frame_t)] = checksum(buf,sizeof(com_frame_t));  //Ð£ÑéºÍ£¬´æ´¢ÔÚµÚ7¸ö×Ö½ÚÉÏ£¬Êý×éÏÂ±ê6.

//	printf(PRINT_CPUTOMCU);
	// for(i=0;i<8;i++)
	// 	printf("%#x ",buf[i]);
	// printf("\n");
	uart_recv_flag = 0;  //ÇåÀí½ÓÊÕ±êÖ¾
	if(PortSend(uart_fd, buf, sizeof(com_frame_t)+1) == 0)   //com_frame_t²¢Ã»ÓÐ°üº¬Êý¾ÝÍ·£¬ËùÒÔ¼Ó1¸ö×Ö½Ú	
	{
		i = 0;
		//·¢ËÍ³É¹¦£¬µÈ´ýÓ¦´ð
		while(uart_recv_flag == 0)
		{
			usleep(100000);   //100ms
			i++;
			
			if(i>10)  //等待1s
			{
				uart_recv_flag = 0;  //超时清零
				printf("Error, send_mcu_data recv timeout !!cmd = %d\n",((unsigned char*)data)[0]);	
				return -1;
			}
		}
				
		if((uart_recv_flag &0xff) == ((unsigned char*)data)[0])   //·¢ËÍµÄÃüÁîÓë·µ»ØµÄÃüÁîÏàÍ¬
		{
			((unsigned char*)data)[0] = uart_recv_flag>>8;   //¸ß8Î»±íÊ¾×´Ì¬£¬°Ñ×´Ì¬Öµ·µ»Ø»ØÈ¥
			uart_recv_flag = 0;  //ÇåÁã½ÓÊÕ±êÖ¾
		}
		else
		{
			printf("Error, send_mcu_data recv uart_recv_flag =%d != cmd = %d\n",uart_recv_flag,((unsigned char*)data)[0]);
			uart_recv_flag = 0;  //ÇåÁã½ÓÊÕ±êÖ¾
			return -1;	
		}	
				
		return 0;   //ÔÝÊ±Ã»ÓÐµÈ´ýÓ¦´ð2021-11-23
	}
	printf("Error, send_mcu_data PortSend failed\n");	
	return -1;
}





static void show_version(char* name)
{
    printf( "%s Buildtime :"__DATE__" "__TIME__,name);
}
 
static void usage(char* name)
{
    show_version(name);
 
    printf("    -h,    short help\n");
    printf("    -v,    show version\n");
    printf("    -d /dev/ttyS0, select com device\n");
    printf("    -p , printf recv data\n");
    printf("    -b , set baudrate\n");
    printf("    -n , set com nonblock mode\n");
    exit(0);
}



static const char* my_opt = "vhpwb:d:";

/* This function will open the uInput device. Please make 
sure that you have inserted the uinput.ko into kernel. */ 
int uart_init(int argc, char *argv[]) 
{
	int nonblock=0;
	int i=0;
	char* com_port = "/dev/ttyS0";

	int c;
	int baudrate = 115200;


	create_queue(&keyCmdQueue);//´´½¨¼üÅÌÏûÏ¢»·ÐÎ¶ÓÁÐ
//	create_queue(&mouseCmdQueue);//´´½¨Êó±êÏûÏ¢»·ÐÎ¶ÓÁÐ
	printf("Program %s is running\n", argv[0]);
    if(argc != 1)
	{
	//	printf("usage: ./kmUtil keyboardComName mouseComName\n");		
	    while(1)
	    {
	        c = getopt(argc, argv, my_opt);
	        //printf("optind: %d\n", optind);
	        if (c < 0)
	        {
	            break;
	        }
	        //printf("option char: %x %c\n", c, c);
	        switch(c)
	        {
	        case 'p':
	        		p_opt = 1;
	                //debug_level = atoi(optarg);
	                printf("p_opt = 1\n");
	                break;
	        case 'd':
	        	//	com_port = 	
	                if(strncmp(optarg,"/dev/tty",8) == 0)
	             		com_port = optarg;
	             	else
	             		printf("select device error ,start with /dev/tty please!\n");
	                printf("com_port = %s.\n\n",com_port);
	                break;
	        case 'b':
	        		baudrate = atoi(optarg);
	        		if(baudrate < 1200)
	        			baudrate = 115200;
	                printf("set baudrate is: %d\n\n", baudrate);
	             //   p1 = optarg;
	                break;
	        case 'n':
	                printf("set com nonblock mode\n\n");
	                nonblock = 1;
	                break;
	        case ':':
	                fprintf(stderr, "miss option char in optstring.\n");
	                break;
	        case '?':
	        case 'h':
	        default:
	                usage(argv[0]);
	                break;
	                //return 0;
	        }
	    }
	    if (optind == 1)
	    {
	        usage(argv[0]);
	    }
	}
	// Return an error if device not found. 
	if (setup_uinput_device() < 0) 
	{ 
		printf("Unable to find uinput device \n"); 
		return -1; 
	} 
	
	uart_fd = PortOpen(com_port,nonblock);   //²ÎÊý2Îª0±íÊ¾Îª×èÈûÄ£Ê½£¬·Ç0Îª·Ç×èÈûÄ£Ê½
	if( uart_fd < 0 )
	{
		ioctl(uinp_fd, UI_DEV_DESTROY);
		close(uinp_fd); 
		printf("Error: open serial port(%s) error.\n", com_port);
	
		exit(1);
	}

	return PortSet(uart_fd,baudrate,1,'N');

	//Ñ­»·´¦Àí½ÓÊÕµÄÊý¾Ý
    // info_recv_proc_func(uart_fd,p_opt);	//p_opt Îª´òÓ¡Ñ¡Ïî£¬1±íÊ¾½ÓÊÕµÄÊý¾Ý½«´òÓ¡³öÀ´


    //³ÌÐòÍË³öµÄ´¦Àí
	/* Close the UINPUT device */
	// ioctl(uinp_fd, UI_DEV_DESTROY);
	// close(uinp_fd);       
 //    close(uart_fd);
	
}









#if 0

void show_version(char* name)
{
    printf( "%s Buildtime :"__DATE__" "__TIME__,name);
}
 
void usage(char* name)
{
    show_version(name);
 
    printf("    -h,    short help\n");
    printf("    -v,    show version\n");
    printf("    -d /dev/ttyS0, select com device\n");
    printf("    -p , printf recv data\n");
    printf("    -b , set baudrate\n");
    printf("    -n , set com nonblock mode\n");
    exit(0);
}



const char* my_opt = "vhpwb:d:";

/* This function will open the uInput device. Please make 
sure that you have inserted the uinput.ko into kernel. */ 
int main(int argc, char *argv[]) 
{
	int nonblock=0;
	int i=0;
	char* com_port = "/dev/ttyS0";
	int uart_fd;
	int c;
	int baudrate = 115200;
	int p_opt = 0;   //ÊÇ·ñÒª´òÓ¡³ö½ÓÊÕµ½µÄ×Ö·û£¿
	    // const char* p1 = NULL;
	    // const char* p2 = NULL;
	    // const char* p3 = NULL;


	create_queue(&keyCmdQueue);//´´½¨¼üÅÌÏûÏ¢»·ÐÎ¶ÓÁÐ
//	create_queue(&mouseCmdQueue);//´´½¨Êó±êÏûÏ¢»·ÐÎ¶ÓÁÐ
 


	printf("Program %s is running\n", argv[0]);
    if(argc != 1)
	{
	//	printf("usage: ./kmUtil keyboardComName mouseComName\n");		
	    while(1)
	    {
	        c = getopt(argc, argv, my_opt);
	        //printf("optind: %d\n", optind);
	        if (c < 0)
	        {
	            break;
	        }
	        //printf("option char: %x %c\n", c, c);
	        switch(c)
	        {
	        case 'p':
	        		p_opt = 1;
	                //debug_level = atoi(optarg);
	                printf("p_opt = 1\n");
	                break;
	        case 'd':
	        	//	com_port = 	
	                if(strncmp(optarg,"/dev/tty",8) == 0)
	             		com_port = optarg;
	             	else
	             		printf("select device error ,start with /dev/tty please!\n");
	                printf("com_port = %s.\n\n",com_port);
	                break;
	        case 'b':
	        		baudrate = atoi(optarg);
	        		if(baudrate < 1200)
	        			baudrate = 115200;
	                printf("set baudrate is: %d\n\n", baudrate);
	             //   p1 = optarg;
	                break;
	        case 'n':
	                printf("set com nonblock mode\n\n");
	                nonblock = 1;
	                break;
	        case ':':
	                fprintf(stderr, "miss option char in optstring.\n");
	                break;
	        case '?':
	        case 'h':
	        default:
	                usage(argv[0]);
	                break;
	                //return 0;
	        }
	    }
	    if (optind == 1)
	    {
	        usage(argv[0]);
	    }
	}
	// Return an error if device not found. 
	if (setup_uinput_device() < 0) 
	{ 
		printf("Unable to find uinput device \n"); 
		return -1; 
	} 
	
	uart_fd = PortOpen(com_port,nonblock);   //²ÎÊý2Îª0±íÊ¾Îª×èÈûÄ£Ê½£¬·Ç0Îª·Ç×èÈûÄ£Ê½
	if( uart_fd < 0 )
	{
		ioctl(uinp_fd, UI_DEV_DESTROY);
		close(uinp_fd); 
		printf("Error: open serial port(%s) error.\n", com_port);
		exit(1);
	}

	PortSet(uart_fd,baudrate,1,'N');

	//Ñ­»·´¦Àí½ÓÊÕµÄÊý¾Ý
    info_recv_proc_func(uart_fd,p_opt);	//p_opt Îª´òÓ¡Ñ¡Ïî£¬1±íÊ¾½ÓÊÕµÄÊý¾Ý½«´òÓ¡³öÀ´


    //³ÌÐòÍË³öµÄ´¦Àí
	/* Close the UINPUT device */
	ioctl(uinp_fd, UI_DEV_DESTROY);
	close(uinp_fd);       
    close(uart_fd);
	
}

#endif
