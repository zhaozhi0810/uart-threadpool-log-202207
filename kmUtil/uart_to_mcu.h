




#ifndef __UART_TO_MCU_H__
#define __UART_TO_MCU_H__

//单片机发给cpu和cpu发给单片机都是4个字节！！！  2022-07-27



//#pragma pack(1) 这个会设置全局的，注释掉
//数据总共4个字节，这里不含帧头
typedef struct
{
	unsigned char data_type;   //led的控制，状态的获取，lcd的熄灭
	unsigned char data;
//	mcu_data_t data;
	unsigned char crc;     //校验和
}__attribute__((packed))com_frame_t;    //注意对齐方式


int uart_init(int argc, char *argv[]) ;

//负责接收数据的线程
/*
 * arg 传出参数，串口收到数据，要修改全局变量，所以arg指向全局数据结构
 * 
 * 返回值：
 * 	根据线程函数定义，无意义，该函数不返回！！！！
 * 
 * */
void* mcu_recvSerial_thread(void* arg);


//发送数据，不由单独的线程处理了。data只需要包含数据类型和数据。头部和crc由该函数完成。
/*
 * data 用于发送的数据，不需要包括帧头和校验和，只要包括数据类型和数据（共2个字节）
 * 返回值
 * 	0表示成功，其他表示失败
 * */
int send_mcu_data(const void* data);  //只需要两个字节的数据就行
#endif
