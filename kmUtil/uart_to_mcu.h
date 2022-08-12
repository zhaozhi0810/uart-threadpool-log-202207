




#ifndef __UART_TO_MCU_H__
#define __UART_TO_MCU_H__

//单片机发给cpu和cpu发给单片机都是4个字节！！！  2022-07-27


//注意单片机与cpu保持一致  2022-07-28
//#pragma pack(1) 这个会设置全局的，注释掉
//数据总共4个字节，这里不含帧头
typedef struct
{
	unsigned char data_type;   //led的控制，状态的获取，lcd的熄灭
	unsigned char data;
//	mcu_data_t data;
	unsigned char crc;     //校验和
}__attribute__((packed))com_frame_t;    //注意对齐方式




typedef enum
{	
	eMCU_LED_STATUS_TYPE=50,  //获得led的状态
	eMCU_KEY_STATUS_TYPE,    //获得按键的状态
	eMCU_LED_SETON_TYPE,    //设置对应的led亮
	eMCU_LED_SETOFF_TYPE,    //设置对应的led灭
	eMCU_LCD_SETONOFF_TYPE,  //lcd打开关闭
	eMCU_KEY_CHANGE_TYPE,    //按键被修改上报
    eMCU_LEDSETALL_TYPE,     //对所有led进行控制，点亮或者熄灭
	eMCU_LEDSETPWM_TYPE,     //设置所有led的亮度 
	eMCU_GET_TEMP_TYPE,      //获得单片机内部温度	
	eMCU_HWTD_SETONOFF_TYPE,   //开门狗设置开关
	eMCU_HWTD_FEED_TYPE,       //看门狗喂狗
	eMCU_HWTD_SETTIMEOUT_TYPE,    //设置看门狗喂狗时间
	eMCU_HWTD_GETTIMEOUT_TYPE,    //获取看门狗喂狗时间
	eMCU_RESET_COREBOARD_TYPE,  //复位核心板
	eMCU_RESET_LCD_TYPE,        //复位lcd 9211（复位引脚没有连通）
	eMCU_RESET_LFBOARD_TYPE//,    //复位底板，好像没有这个功能！！！
}mcu_data_type;

/*
	注意要 处理com_message_handle(uart_to_mcu.c) 的接收部分，不然会导致应答超时

 */




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


//程序退出时，串口部分的处理
void uart_exit(void) ;

#endif
