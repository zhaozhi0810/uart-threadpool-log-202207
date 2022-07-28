/*
* @Author: dazhi
* @Date:   2022-07-27 09:57:14
* @Last Modified by:   dazhi
* @Last Modified time: 2022-07-27 10:19:48
*/


#ifndef __MY_IPC_SMGQ_H__
#define __MY_IPC_SMGQ_H__


//用户创建消息队列的关键字
#define KEY_PATH "/proc/cpuinfo"
#define KEY_ID -123321

typedef struct  //消息结构体
{
    long types;    //指定消息的类型
    int cmd;       //消息的命令
    int param1;    //参数1
    int param2;    //参数2
    int ret;       //参数3，可用于返回值
}msgq_t; 



enum API_CMD_types{
	eAPI_LEDSET_CMD = 0,   //设置led
	eAPI_LEDGET_CMD,         //获取led状态
	eAPI_LEDSETALL_CMD,      //设置所有的led
	eAPI_LEDGETALL_CMD,      //获取所有的led
	eAPI_BTNGET_CMD,         //获取按键
	eAPI_BTNEVENT_CMD,       //等待按键事件
	eAPI_LCDONOFF_CMD,       //lcd开启关闭事件
};





//返回值0表示成功，其他表示失败
int msgq_init(void);




//接收消息
//参数 timeout_50ms 大于0时，表示非阻塞模式，等待的时间为timeout_50ms*50ms的值，等于0表示阻塞模式
//     types 接收消息的类型
//     msgbuf 接收消息的缓存，1次接收一条消息
//返回0表示成功，其他表示失败
int msgq_recv(long types,msgq_t *msgbuf,unsigned int timeout_50ms);



//发送消息，并且要等待应答。
////参数 timeout 大于0时，设置接收应答超时时间，等于0表示阻塞模式
//     ack_types 指定应答的类型，之前是TYPE_RECV+cmd
//     msgbuf 发送消息的缓存，1次发送一条消息
//返回0表示成功，其他表示失败
//注意，函数使用时，需要指定msgbuf->types ！！！！
int msgq_send(long ack_types,msgq_t *msgbuf,int timeout);


//发送应答消息消息，不等待应答。
int msgq_send_ack(msgq_t *msgbuf);
#endif


