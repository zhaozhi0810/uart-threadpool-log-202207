// ComFunc.c 
#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/serial.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>             // exit
#include <sys/ioctl.h>          // ioctl
#include <string.h>             // bzero
#include <pthread.h>
#include "ComFunc.h"


#define bzero(a, b)             memset(a, 0, b)

static pthread_mutex_t uart_write_mutex=PTHREAD_MUTEX_INITIALIZER;

/*******************************************
 *	波特率转换函数（请确认是否正确）
********************************************/
static int convbaud(unsigned long int baudrate)
{
	switch(baudrate)
	{
		case 1200:
			return B1200;
		case 2400:
			return B2400;
		case 4800:
			return B4800;
		case 9600:
			return B9600;
		case 19200:
			return B19200;
		case 38400:
			return B38400;
		case 57600:
			return B57600;
		case 115200:
			return B115200;
		case 460800:
			return B460800;
		default:
			return B115200;
	}
}

/*******************************************
 *	Setup comm attr
 *	fdcom: 串口文件描述符，pportinfo: 待设置的端口信息（请确认）
 *
********************************************/
int PortSet(int fdcom, int 	baudrate,char stopbit,char fctl)
{
	struct termios termios_old, termios_new;
	int 	tmp;
	char	databit, parity;

	bzero(&termios_old, sizeof(termios_old));
	bzero(&termios_new, sizeof(termios_new));
	cfmakeraw(&termios_new);
	tcgetattr(fdcom, &termios_old);			//get the serial port attributions

	/*------------设置端口属性----------------*/
	//baudrates
	baudrate = convbaud(baudrate);
	cfsetispeed(&termios_new, baudrate);		//填入串口输入端的波特率
	cfsetospeed(&termios_new, baudrate);		//填入串口输出端的波特率
	termios_new.c_cflag |= CLOCAL;			//控制模式，保证程序不会成为端口的占有者
	termios_new.c_cflag |= CREAD;			//控制模式，使能端口读取输入的数据

	// 控制模式，flow control
	switch(fctl)
	{
		case 'n':
		case 'N':
		case '0':
		{
			termios_new.c_cflag &= ~CRTSCTS;		//no flow control
		}
		break;

		case '1':
		{
			termios_new.c_cflag |= CRTSCTS;			//hardware flow control
		}
		break;

		case '2':
		{
			termios_new.c_iflag |= IXON | IXOFF |IXANY;	//software flow control
		}
		break;

		default:
		{
			printf("unknown fctl mode, use no fctl control mode");
			termios_new.c_cflag &= ~CRTSCTS;
		}
	}

	//控制模式，data bits
	termios_new.c_cflag &= ~CSIZE;		//控制模式，屏蔽字符大小位
	databit = 0;
	switch(databit)
	{
		case '5':
			termios_new.c_cflag |= CS5;
		break;

		case '6':
			termios_new.c_cflag |= CS6;
		break;

		case '7':
			termios_new.c_cflag |= CS7;
		break;

		default:
			termios_new.c_cflag |= CS8;
	}

	//控制模式 parity check
	parity = '0';
	switch(parity)
	{
		case '0':
		{
			termios_new.c_cflag &= ~PARENB;		//no parity check
		}
		break;

		case '1':
		{
			termios_new.c_cflag |= PARENB;		//EVEN check
			termios_new.c_cflag &= ~PARODD;
		}
		break;

		case '2':
		{
			termios_new.c_cflag |= PARENB;		//ODD check
			termios_new.c_cflag |= PARODD;
		}
		break;

		default:
		{
			printf("unknown parity mode use no parity check mode");
			termios_new.c_cflag &= ~PARENB;		//no parity check
		}
	}

	//控制模式，stop bits
	if(stopbit == '2')
	{
		termios_new.c_cflag |= CSTOPB;	//2 stop bits
	}
	else
	{
		termios_new.c_cflag &= ~CSTOPB;	//1 stop bits
	}

	//other attributions default
	termios_new.c_oflag &= ~OPOST;			//输出模式，原始数据输出
	termios_new.c_cc[VMIN]  = 1;			//控制字符, 所要读取字符的最小数量
	termios_new.c_cc[VTIME] = 0;			//控制字符, 读取第一个字符的等待时间	unit: (1/10)second

	struct serial_struct serial;
    ioctl(fdcom, TIOCGSERIAL, &serial);
	serial.flags |= ASYNC_LOW_LATENCY;
    ioctl(fdcom, TIOCSSERIAL, &serial);


	tcflush(fdcom, TCIOFLUSH);				//溢出的数据可以接收，但不读
	tmp = tcsetattr(fdcom, TCSANOW, &termios_new);	//设置新属性，TCSANOW：所有改变立即生效	tcgetattr(fdcom, &termios_old);

	return(tmp);
}

/*******************************************
 *	Open serial port
 *	tty: 端口号 ttyS0, ttyS1, ....
 *	返回值为串口文件描述符
********************************************/
int PortOpen( char *devName,int arg_nonblock)
{
	int fdcom;	//串口文件描述符
	int args = O_RDWR | O_NOCTTY;


	if ( NULL == devName)
	{
		return -1;
	}

	if(arg_nonblock)
	{
		args |=  O_NONBLOCK;
	}

	//fdcom = open(devName, O_RDWR | O_NOCTTY | O_NONBLOCK | O_NDELAY);
	//fdcom = open(devName, O_RDWR | O_NOCTTY | O_NONBLOCK);
	fdcom = open(devName, args); //bocking mode

	return (fdcom);  //返回值要判断是否成功
}

/*******************************************
 *	Close serial port
********************************************/
void PortClose(int fdcom)
{
	close(fdcom);
}

/********************************************
 *	send data
 *	fdcom: 串口描述符，data: 待发送数据，datalen: 数据长度
 *	返回实际发送长度
*********************************************/
int PortSend(int fdcom,unsigned char *data, unsigned char datalen)
{
	int len = 0;
	int ret;
    pthread_mutex_lock(&uart_write_mutex); 
    do{    
    	ret =  write(fdcom, data+len, datalen-len);	
    	if(ret < 0)   //发送失败
		{
			printf("PortSend write error!\n");
			pthread_mutex_unlock(&uart_write_mutex);
			return -1;
		}
		len += ret;	//实际写入的长度
		
		if(len < datalen)
			usleep(5000);
	}while(len < datalen);
	pthread_mutex_unlock(&uart_write_mutex);
	return 0;
}


/*******************************************
 *	receive data
 *	返回实际读入的字节数
 *	参数：data 传出参数，用于接收的数据返回
 * 		datalen 指定接收数据的长度
 * 	返回值：
 * 		>0 表示收到的字节数，正常应该等于datalen（要求的字节数）
 *      其他: 错误.
********************************************/
int PortRecv(int fdcom, unsigned char *data, unsigned char  datalen)
{
	int ret;
	unsigned char readlen;

	if ( -1 == fdcom )
	{
		printf("error: PortRecv fdcom == -1\n");
		return -1;
	}
	
	if(datalen == 0)  //接收的长度不应该为0
		return 0;
	
	readlen = 0;
	
	do{
		ret = read(fdcom, data+readlen, datalen-readlen);
//		printf("PortRecv read ret == %d\n",ret);
		if(ret < 0)   //读取失败
		{
			printf("error: PortRecv read\n");
			break;
		}	

		readlen += ret;				
									
	}while(readlen != datalen);	//读到的字节不够
	
	return readlen;
}
