/*本程序符合GPL条约
ComFunc.h 
一组操作串口的函数
*/
#ifndef __COM_FUNC_H__
#define  __COM_FUNC_H__

//串口结构
typedef struct{
	char	prompt;		//prompt after reciving data
	int 	baudrate;		//baudrate
	char	databit;		//data bits, 5, 6, 7, 8
	char 	debug;		//debug mode, 0: none, 1: debug
	char 	echo;			//echo mode, 0: none, 1: echo
	char	fctl;			//flow control, 0: none, 1: hardware, 2: software
	char 	ttyName[64];			//tty: 0, 1, 2, 3, 4, 5, 6, 7
	char	parity;		//parity 0: none, 1: odd, 2: even
	char	stopbit;		//stop bits, 1, 2
	const int reserved;	//reserved, must be zero
}portinfo_t;

typedef portinfo_t *pportinfo_t;

typedef void (*km_call_back)(char *rBuf, int readn,  int chan);

/*
 *	打开串口，返回文件描述符
 *	devName: the name of device
*/
int PortOpen( char *devName,int arg_nonblock );
/*
 *	设置串口
 *	fdcom: 串口文件描述符， pportinfo： 待设置的串口信息
*/
int PortSet(int fdcom, int 	baudrate,char stopbit,char fctl);
/*
 *	关闭串口
 *	fdcom：串口文件描述符
*/
void PortClose(int fdcom);
/*
 *	发送数据
 *	fdcom：串口描述符， data：待发送数据， datalen：数据长度
 *	返回实际发送长度
*/
int PortSend(int fdcom,unsigned char *data, unsigned char   datalen);
/*
 *	接收数据
 *	fdcom：串口描述符数组， data：接收缓冲区, datalen：接收长度， chan:接收到数据的通道
 *	返回实际读入的长度
*/
int PortRecv( int fdcom ,unsigned  char *data, unsigned char   datalen);
#endif
