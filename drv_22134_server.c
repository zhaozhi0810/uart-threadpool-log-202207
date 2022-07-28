/*
* @Author: dazhi
* @Date:   2022-07-27 10:47:46
* @Last Modified by:   dazhi
* @Last Modified time: 2022-07-28 16:43:37
*/


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

#include "my_log.h"
#include "my_ipc_msgq.h"
#include "kmUtil/uart_to_mcu.h"
#include "threadpool.h"
//服务端，包含串口通信，msgq通信，线程池，日志等。

#define TYPE_SENDTO_API 234   //发    必须跟api是反的！！！！，不要随意改动！！！！
#define TYPE_RECVFROM_API 678   //收

static const char* g_build_time_str = "Buildtime :"__DATE__" "__TIME__;   //获得编译时间
//全局变量，数据应该保持实时刷新。
struct threadpool* pool;  //线程池

//mcu 自己的协议 相当于把协议又翻译一次
//
//
//
// static int set_mcu_cmd(int cmd,int param1,int param2)
// {
// 	unsigned char mcu_cmd_buf[2];
// 	switch(cmd)
// 	{

		
// 	}	
		
	


// 	mcu_cmd_buf[1] = 1;  //命令对应的参数

// 	send_mcu_data(mcu_cmd_buf);
// }



//应答客户的cpi的查询，可以在客户端并发
//直接回复全局数据
static void answer_to_api(msgq_t *pmsgbuf)
{
	unsigned char mcu_cmd_buf[2];
	msgq_t msgbuf;  //用于应答
		
	msgbuf.types = TYPE_SENDTO_API+pmsgbuf->cmd;  //发送的信息类型
	msgbuf.cmd = pmsgbuf->cmd;
	msgbuf.ret = 0;  //ret 等于0表示不返回有效数据（只包含应答），等于1，表示有数据返回


	printf("debug:answer_to_api cmd = %d param1 = %d param2 = %d\n",
			pmsgbuf->cmd,pmsgbuf->param1,pmsgbuf->param2);

	//1.	解析数据
	switch(pmsgbuf->cmd)
	{
#if 1
		/***************************************************/  //设置需要等待完成动作，单片机通信后等待应答
		case eAPI_LEDSET_CMD: //设置led
			if(pmsgbuf->param2) //非零表示点亮				
				mcu_cmd_buf[0] = eMCU_LED_SETON_TYPE;  //设置led	 on
			else
				mcu_cmd_buf[0] = eMCU_LED_SETOFF_TYPE;  //设置led off
			mcu_cmd_buf[1] = pmsgbuf->param1;   //哪一个led
			msgbuf.ret = send_mcu_data(mcu_cmd_buf);
			break;
		case eAPI_LCDONOFF_CMD:  //lcd 打开关闭控制							
			mcu_cmd_buf[0] = eMCU_LCD_SETONOFF_TYPE;  //设置lcd 打开或者关闭			
			mcu_cmd_buf[1] = pmsgbuf->param2;   //打开1或者关闭0
			msgbuf.ret = send_mcu_data(mcu_cmd_buf);
			break;

		case eAPI_LEDSETALL_CMD:      //设置所有的led	
			mcu_cmd_buf[0] = eMCU_LEDSETALL_TYPE;  //设置所有的led 打开或者关闭			
			mcu_cmd_buf[1] = pmsgbuf->param2;   //打开1或者关闭0
			msgbuf.ret = send_mcu_data(mcu_cmd_buf);
			break;
		case eAPI_LEDGET_CMD: 		  //led 状态获取	
			mcu_cmd_buf[0] = eMCU_LED_STATUS_TYPE;  //led 状态获取				
			mcu_cmd_buf[1] = pmsgbuf->param1;   //获取哪个灯的状态
			if(0 == send_mcu_data(mcu_cmd_buf))  //返回值为0，表示收到了数据
			{//获得mcu的数据？？
				msgbuf.ret = 1; //表示又数据返回
				msgbuf.param1 = mcu_cmd_buf[0];
			}
			break;
		default:
			msgbuf.ret = -1;   //不能是别的命令
		break;
#endif		
	}	

	//3.做出应答
	if(0!= msgq_send_ack(&msgbuf))   //应答发出后，不需要等待
		printf("error : msgq_send\n");
}



//多线程查询消息队列
void* api_answer_thread(void *arg)
{
	msgq_t *pmsgbuf = (msgq_t*)arg;

	answer_to_api(pmsgbuf);    //处理接收到的信息
		
	//内存记得释放
	free(pmsgbuf);
	return NULL;	
}


//线程函数，处理snmp和API的请求
static void* msg_connect(void * data)
{
	int ret;
	msgq_t *pmsgbuf;
//	data = data;
	
	while(1)
	{
		pmsgbuf = malloc(sizeof (msgq_t));   //由线程函数释放
		if(pmsgbuf == NULL)	
		{
			printf("error malloc msg_connect!\n");
			sleep(1);  //等待1s继续
			continue;
		}
		ret = msgq_recv(TYPE_RECVFROM_API,pmsgbuf,0);  //阻塞试接收，消息类型都是同一种
		if(ret == 0)   //收到信息，打印出来
		{
			//调试打印						
			{				
				printf("type = %ld cmd = %d b = %d c = %d rt = %d\n",pmsgbuf->types,pmsgbuf->cmd,pmsgbuf->param1,pmsgbuf->param2,pmsgbuf->ret);
				threadpool_add_job(pool,api_answer_thread,pmsgbuf);
			}
		}
		else
		{
			printf("ERROR: msgq_recv\n");
			if(errno != EINTR)  //捕获到信号
			{
				printf("error msgq error!\n");
			}			
			free(pmsgbuf);   //出错的情况下由自己释放
		}	
	}
}




// static const char* my_opt = "vhpwb:d:";

/* This function will open the uInput device. Please make 
sure that you have inserted the uinput.ko into kernel. */ 
int main(int argc, char *argv[]) 
{
	int t;
	
	printf("%s running,Buildtime %s\n",argv[0],g_build_time_str);

	//串口通信	
	if(0 != uart_init(argc, argv))
	{
		printf("error:uart_init \n");
		return -1;
	}

		//用于通信的消息队列
	if(0 != msgq_init())
	{
		printf("error : msgq_init\n");
		return -1;
	}

		//日志记录
	// if(0 !=log_init())  //自动开了线程
	// 	printf("ERROR: log thread init!!");

	//线程池初始化
	pool = threadpool_init(4,6);  //初始有多少线程，最多有多少任务排队

	//串口任务，独占一个线程
	threadpool_add_job(pool,mcu_recvSerial_thread,/*&g_datas*/&t);  //串口接收,线程启动后不再退出！！！
	
	//热键处理，独占一个线程
	//pthread_create(&thread, NULL, hot_key_thread, NULL);
    //pthread_detach(thread);   //设置分离模式	
	//threadpool_add_job(pool,hot_key_thread,&g_datas);  //串口接收,线程启动后不再退出！！！

	//处理msg的接收信息。
	msg_connect(NULL);   //这个线程启动后不再退出。接收msg的消息
	
	while(1)
	{				
		sleep(100);
	}

}


