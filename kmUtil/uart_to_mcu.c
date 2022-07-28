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

static 	int p_opt = 0;   //�Ƿ�Ҫ��ӡ�����յ����ַ���
static 	int uart_fd;


//#define FRAME_LENGHT (sizeof(com_frame_t)+1)    //����֡���ֽ���
#define FRAME_HEAD 0xa5



#define COM_DATLEN 4 //���ڵ����ݳ���Ϊ4���ֽ� 0x5f + dat1 + dat2 + checksum
#define FRAME_HEAD 0xa5
static unsigned char com_recv_data[COM_DATLEN*2];   //���ջ���


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



static void send_a_button_ievent(char VK, char VKState)  //1���£� 0����
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
	ioctl (uinp_fd, UI_SET_EVBIT, EV_KEY); //��������
	ioctl (uinp_fd, UI_SET_EVBIT, EV_REP); //�����ͷ�
	ioctl (uinp_fd, UI_SET_EVBIT, EV_SYN); //ͬ���¼�
//	ioctl (uinp_fd, UI_SET_EVBIT, EV_LED); //LED�¼�
#if 0  //����¼�
	ioctl(uinp_fd, UI_SET_EVBIT, EV_REL); 
   //ioctl(uinp_fd , UI_SET_EVBIT, EV_ABS);
	ioctl(uinp_fd, UI_SET_RELBIT, REL_X); 
	ioctl(uinp_fd, UI_SET_RELBIT, REL_Y);
	ioctl(uinp_fd, UI_SET_RELBIT, REL_WHEEL); //����
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



//������Ϣ����
static void com_message_handle(void)
{		
	if(com_recv_data[1]>0 && com_recv_data[1] < 37)   //����ֵ
	{
		printf("key = %d %s\n",com_recv_data[1],com_recv_data[2]?"press":"release");
		send_a_button_ievent(key_map[com_recv_data[1]-1], com_recv_data[2]);
	}
	else  //��������Ľӿ�����
	{
		printf("com_recv_data[1] = %d com_recv_data[2] = %d\n",com_recv_data[1],com_recv_data[2]);
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



//У������
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
	//��ȡԭ�����е�У��ֵ
	check = data[len - 1];
	
	//���¼���У��ֵ
	if(check == checksum(data,len - 1))
		ret = 0;
	
	return ret;
}





static void ProcRecvKeyCmd(void)
{
//	unsigned int err;
	unsigned char length,i,j;
	static unsigned char datalen =COM_DATLEN,offset = 0;  //��������������֡��������
	char ispt = 0;  //�յ���������Ҫ��ӡ��


	while(1)  //���ǵ����ܽ��յ���֡Ҫ��������
	{		
		length = queue_length(&keyCmdQueue);	
		if(length < datalen)   //������֡ͷ��С��4���ֽڣ�����������һ֡
		{	
			//com3_answer_3A3000(RECV_NO);   //Ӧ����յ��ĳ��Ȳ���
			return ;   //�����ȴ�
		}	
		length = datalen;   //������Ҫ�������ֽ���
		for(i=0;i<length;i++)
		{
			//���ﲻ�жϴ����ˣ�ǰ���Ѿ����ȷʵ����ô���ֽڡ�
			out_queue(&keyCmdQueue,com_recv_data+i+offset) ;  //com_data �ճ�1���ֽڣ�Ϊ�˼���֮ǰ��У����㷨�������ݽ����㷨
			if(ispt)
				printf("com_recv_data[%d] = %x \n",i,com_recv_data[i+offset]);
		}
	//	com3_data[0] = FRAME_HEAD;  //����֡ͷ����У��ͼ���
		if((com_recv_data[0] == FRAME_HEAD) && (0 == verify_data(com_recv_data,COM_DATLEN)))   //�ڶ������������ܳ��ȣ�����У��͹�7���ֽ�
		{
			//У�����ȷ��
			//com3_answer_3A3000(RECV_OK);
			com_message_handle();		
		}	
		else  //У��Ͳ���ȷ��������֡�д���
		{
			for(i=1;i<COM_DATLEN;i++)   //ǰ����жϳ����⣬������֡����Ѱ����һ��֡ͷ������
			{
				if(com_recv_data[i] == FRAME_HEAD)   //�м��ҵ�һ��֡ͷ
				{
					break;
				}
			}		
			if(i != COM_DATLEN) //�������м��ҵ�֡ͷ������
			{
				datalen = i;   //��һ����Ҫ�����ֽ���
				offset = COM_DATLEN-i;  //�洢��ƫ��λ�õļ���

				for(j=0;i<COM_DATLEN;i++,j++)   //�п���֡ͷ���ԣ����Ե�һ���ֽڻ���Ҫ����һ��
				{
					com_recv_data[j] = com_recv_data[i];   //��ʣ�µĿ�����ȥ
				}
			}
			else  //�������м�û���ҵ�֡ͷ
			{
				datalen = COM_DATLEN;  //	��һ����Ҫ�����ֽ���
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
		
			//������յ���Ϣ
			ProcRecvKeyCmd();
		}	
	}//while(1)

}



//����������ݵ��߳�
/*
 * arg ���������������յ����ݣ�Ҫ�޸�ȫ�ֱ���������argָ��ȫ�����ݽṹ
 * 
 * ����ֵ��
 * 	�����̺߳������壬�����壬�ú��������أ�������
 * 
 * */
void* mcu_recvSerial_thread(void* arg)
{
	while(1)
	{
		info_recv_proc_func();
	}
}





//�������ݣ����ɵ������̴߳����ˡ�dataֻ��Ҫ�����������ͺ����ݡ�ͷ����crc�ɸú�����ɡ�
/*
 * data ���ڷ��͵����ݣ�����Ҫ����֡ͷ��У��ͣ�ֻҪ�����������ͺ����ݣ���2���ֽڣ�
 * ����ֵ
 * 	0��ʾ�ɹ���������ʾʧ��
 * */
int send_mcu_data(const void* data)
{	
	unsigned char buf[8];  	
	int i;
	
	buf[0] = FRAME_HEAD;  //֡ͷ	
	memcpy(buf+1,data,sizeof(com_frame_t)-1);    //����

	//crc���¼���
	buf[sizeof(com_frame_t)] = checksum(buf,sizeof(com_frame_t));  //У��ͣ��洢�ڵ�7���ֽ��ϣ������±�6.

//	printf(PRINT_CPUTOMCU);
	// for(i=0;i<8;i++)
	// 	printf("%#x ",buf[i]);
	// printf("\n");
	
	if(PortSend(uart_fd, buf, sizeof(com_frame_t)+1) == 0)   //com_frame_t��û�а�������ͷ�����Լ�1���ֽ�	
	{
		//���ͳɹ����ȴ�Ӧ��
		return 0;   //��ʱû�еȴ�Ӧ��2021-11-23
	}
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


	create_queue(&keyCmdQueue);//����������Ϣ���ζ���
//	create_queue(&mouseCmdQueue);//���������Ϣ���ζ���
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
	
	uart_fd = PortOpen(com_port,nonblock);   //����2Ϊ0��ʾΪ����ģʽ����0Ϊ������ģʽ
	if( uart_fd < 0 )
	{
		ioctl(uinp_fd, UI_DEV_DESTROY);
		close(uinp_fd); 
		printf("Error: open serial port(%s) error.\n", com_port);
	
		exit(1);
	}

	return PortSet(uart_fd,baudrate,1,'N');

	//ѭ��������յ�����
    // info_recv_proc_func(uart_fd,p_opt);	//p_opt Ϊ��ӡѡ�1��ʾ���յ����ݽ���ӡ����


    //�����˳��Ĵ���
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
	int p_opt = 0;   //�Ƿ�Ҫ��ӡ�����յ����ַ���
	    // const char* p1 = NULL;
	    // const char* p2 = NULL;
	    // const char* p3 = NULL;


	create_queue(&keyCmdQueue);//����������Ϣ���ζ���
//	create_queue(&mouseCmdQueue);//���������Ϣ���ζ���
 


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
	
	uart_fd = PortOpen(com_port,nonblock);   //����2Ϊ0��ʾΪ����ģʽ����0Ϊ������ģʽ
	if( uart_fd < 0 )
	{
		ioctl(uinp_fd, UI_DEV_DESTROY);
		close(uinp_fd); 
		printf("Error: open serial port(%s) error.\n", com_port);
		exit(1);
	}

	PortSet(uart_fd,baudrate,1,'N');

	//ѭ��������յ�����
    info_recv_proc_func(uart_fd,p_opt);	//p_opt Ϊ��ӡѡ�1��ʾ���յ����ݽ���ӡ����


    //�����˳��Ĵ���
	/* Close the UINPUT device */
	ioctl(uinp_fd, UI_DEV_DESTROY);
	close(uinp_fd);       
    close(uart_fd);
	
}

#endif
