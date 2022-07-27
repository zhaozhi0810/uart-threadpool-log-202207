/*
* @Author: dazhi
* @Date:   2022-07-27 10:47:46
* @Last Modified by:   dazhi
* @Last Modified time: 2022-07-27 14:14:26
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

#define TYPE_SEND_API 100   //发    必须跟api是反的！！！！，不要随意改动！！！！
#define TYPE_RECV 500   //收


//全局变量，数据应该保持实时刷新。
struct threadpool* pool;  //线程池





//应答客户的cpi的查询，可以在客户端并发
//直接回复全局数据
static void api_answer(msgq_t *pmsgbuf)
{
	msgq_t msgbuf;  //用于应答
		
	msgbuf.types = TYPE_SEND_API+pmsgbuf->cmd;  //发送的信息类型
	msgbuf.cmd = pmsgbuf->cmd;
	msgbuf.ret = 0;


	//3.做出应答
	if(0!= msgq_send_ack(&msgbuf))   //应答发出后，不需要等待
		printf("error : msgq_send\n");
}



//多线程查询消息队列
void* api_answer_thread(void *arg)
{
	msgq_t *pmsgbuf = (msgq_t*)arg;

	api_answer(pmsgbuf);    //处理接收到的信息
		
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
		ret = msgq_recv(TYPE_RECV,pmsgbuf,0);  //阻塞试接收，消息类型都是同一种
		if(ret == 0)   //收到信息，打印出来
		{
			//调试打印						
			//判断消息来自于api还是snmp
			if(pmsgbuf->cmd < 10 /*eAPI_MAX_CMD*/ )
			{				
			//	printf(PRINT_APITOCPU"type = %ld cmd = %d b = %d c = %d rt = %d\n",pmsgbuf->types,pmsgbuf->cmd,pmsgbuf->b,pmsgbuf->c,pmsgbuf->ret);
				threadpool_add_job(pool,api_answer_thread,pmsgbuf);
			}
			// else  //使用snmp处理
			// {
			// 	printf(PRINT_SNMPDTOCPU"type = %ld cmd = %d b = %d c = %d rt = %d\n",pmsgbuf->types,pmsgbuf->cmd,pmsgbuf->b,pmsgbuf->c,pmsgbuf->ret);
			// 	threadpool_add_job(pool,snmp_answer_thread,pmsgbuf);  //snmp的函数
			// }
		}
		else
		{
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
		//串口通信	
	if(0 != uart_init(argc, argv))
	{
		printf("error:uart_init \n");
		return -1;
	}

		//用于通信的消息队列
	if(0 != msgq_init())
	{
		printf("error : drvApiOpen\n");
		return -1;
	}

		//日志记录
	if(0 !=log_init())  //自动开了线程
		printf("ERROR: log thread init!!");

	//线程池初始化
	pool = threadpool_init(4,6);  //初始有多少线程，最多有多少任务排队

	//串口任务，独占一个线程
	threadpool_add_job(pool,mcu_recvSerial_thread,/*&g_datas*/NULL);  //串口接收,线程启动后不再退出！！！
	
	//热键处理，独占一个线程
	//pthread_create(&thread, NULL, hot_key_thread, NULL);
    //pthread_detach(thread);   //设置分离模式	
	//threadpool_add_job(pool,hot_key_thread,&g_datas);  //串口接收,线程启动后不再退出！！！

	//处理msg的接收信息。
	msg_connect(NULL);   //这个线程启动后不再退出。接收msg的消息
	
	while(1)
	{				
		usleep(10000);
	}

}


