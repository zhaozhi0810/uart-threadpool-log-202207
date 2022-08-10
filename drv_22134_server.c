/*
* @Author: dazhi
* @Date:   2022-07-27 10:47:46
* @Last Modified by:   dazhi
* @Last Modified time: 2022-08-10 10:37:04
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
static pid_t api_pid = 0;
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

// static size_t get_executable_path( char* processname, size_t len)
// {
//     char* path_end;
//     char processdir[512] = {0};
//     if(readlink("/proc/self/exe", processdir,sizeof processdir) <=0)
//             return -1;
//     path_end = strrchr(processdir,  '/');
//     if(path_end == NULL)
//             return -1;
//     ++path_end;
//     strncpy(processname, path_end,len);
//     *path_end = '\0';
//     return (size_t)(path_end - processdir);
// }

//ps -ef | grep drv_22134_server | grep -v grep | wc -l
//尝试启动server进程
//返回0表示没有该进程，非0表示存在进程了
static int is_server_process_start(void)
{
	FILE *ptr = NULL;
	char cmd[256] = "ps -ef | grep drv_22134_server | grep -v grep | wc -l";
//	int status = 0;
	char buf[64];
	int count;

//	char name[64];

	// if(get_executable_path( name, sizeof name) > 0)
	// {
	// 	snprintf(cmd,sizeof cmd,"ps -ef | grep %s | grep -v grep | wc -l",name);
	// }

	// printf("server:cmd = %s\n",cmd);

	if((ptr = popen(cmd, "r")) == NULL)
	{
		printf("popen err\n");
		return -1;
	}
	memset(buf, 0, sizeof(buf));
	if((fgets(buf, sizeof(buf),ptr))!= NULL)//获取进程和子进程的总数
	{
//		printf("server: buf = %s\n",buf);
		count = atoi(buf);
		if(count < 2)//当进程数小于等于2时，说明进程不存在, 1表示有一个，是自己
		{
			pclose(ptr);
			printf("no server process \n");
			return 0;  //系统中没有该进程	
		}
		// else
		// 	system("ps -ef | grep drv_22134_server ");

	}
	pclose(ptr);
	return 1;
}





//检查进程是否正在运行，防止运行多个进程
//返回0表示没有该进程，-1表示存在进程了
static int isProcessRunning()
{
#if 0	
    int lock_fd = open("/tmp/drvServer22134.lock",O_CREAT|O_RDWR,0666);
    if(lock_fd < 0)
    {
    	printf("ERROR: open /tmp/drvServer22134.lock\n");
    	return -1;
    }	
    int rc = flock(lock_fd,LOCK_EX|LOCK_NB); //flock加锁，LOCK_EX -- 排它锁；LOCK_NB -- 非阻塞模式
    if(rc)  //返回值非0，无法正常持锁
    {
        if(EWOULDBLOCK == errno)    //尝试锁住该文件的时候，发现已经被其他服务锁住,errno==EWOULDBLOCK
        {
        //	close(lock_fd);	
            printf("Already Running!\n");
            return -1; 
        }
    }
//    close(lock_fd);	//不能关闭文件
#else
	int ret = is_server_process_start();
//	printf("server: Process is Running ret = %d\n",ret);
	return ret;   //返回0表示没有该进程，非0则表示有或者出错
    
#endif    
    return 0;   //成功加锁。
}



static int check_api_running(void)
{
	char filename[256] = {0};

	if(api_pid < 1)   //不存在这样的进程
		return 0;

	snprintf(filename,sizeof filename,"/proc/%d/exe",api_pid);
	printf("server : check_api_running filename = %s\n",filename);

	if(access(filename,F_OK ) != -1)
	{
		printf("server : check_api_running access ok\n");
		return 1;   //进程已存在
	}
	printf("server : check_api_running access 0\n");
	return 0;  //进程不存在
}


//应答客户的cpi的查询，可以在客户端并发
//直接回复全局数据
static void answer_to_api(msgq_t *pmsgbuf)
{
	int ret;
	unsigned char mcu_cmd_buf[2];
	msgq_t msgbuf;  //用于应答
		
	msgbuf.types = TYPE_SENDTO_API+pmsgbuf->cmd;  //发送的信息类型
	msgbuf.cmd = pmsgbuf->cmd;
	msgbuf.ret = 0;  //ret 等于0表示不返回有效数据（只包含应答），等于1，表示有数据返回

	//调试信息
//	printf("debug:answer_to_api cmd = %d param1 = %d param2 = %d\n",
//			pmsgbuf->cmd,pmsgbuf->param1,pmsgbuf->param2);

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
		case eAPI_LEDSETPWM_CMD:    //设置led的亮度，要发给单片机
			mcu_cmd_buf[0] = eMCU_LEDSETPWM_TYPE;  //设置所有的led pwm			
			mcu_cmd_buf[1] = pmsgbuf->param1;   //pwm值
			msgbuf.ret = send_mcu_data(mcu_cmd_buf);
			break;
		case eAPI_BOART_TEMP_GET_CMD:  //获取单片机内部温度，只有整数部分
			mcu_cmd_buf[0] = eMCU_GET_TEMP_TYPE;  //led 获取单片机内部温度				
			mcu_cmd_buf[1] = pmsgbuf->param1;   //无意义
			if(0 == send_mcu_data(mcu_cmd_buf))  //返回值为0，表示收到了数据
			{//获得mcu的数据？？
				msgbuf.ret = 1; //表示又数据返回
				msgbuf.param1 = mcu_cmd_buf[0];
			}
			break;
		case eAPI_CHECK_APIRUN_CMD:
		//		printf("server : eAPI_CHECK_APIRUN_CMD pid = %d\n",pmsgbuf->param1);
				ret = check_api_running();  //看看是否存在
				if(ret == 0)
				{
		//			printf("server : eAPI_CHECK_APIRUN_CMD 111\n");
					api_pid = pmsgbuf->param1; //记录该pid号					
					msgbuf.param1 = 0;  //表示进程不存在
				}
				else
				{
		//			printf("server : eAPI_CHECK_APIRUN_CMD 222\n");
					msgbuf.param1 = 1;  //表示进程存在	
				}
				msgbuf.ret = 1; //表示有数据返回	
			break;
		default:
			msgbuf.ret = -1;   //不能是别的命令
		break;
#endif		
	}	

	//3.做出应答给API
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
			//	printf("type = %ld cmd = %d b = %d c = %d rt = %d\n",pmsgbuf->types,pmsgbuf->cmd,pmsgbuf->param1,pmsgbuf->param2,pmsgbuf->ret);
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
	return NULL;
}




// static const char* my_opt = "vhpwb:d:";

/* This function will open the uInput device. Please make 
sure that you have inserted the uinput.ko into kernel. */ 
int main(int argc, char *argv[]) 
{
	int t;
	
	printf("%s running,Buildtime %s\n",argv[0],g_build_time_str);

	if(isProcessRunning())   //防止服务程序被多次运行
	{
		printf("server : isProcessRunning return !0\n");
		return 0;
	}	

	//转为守护进程
	if(daemon(0,0))   //daemon 2022-08-08
	{
		perror("daemon");
		return -1;
	}

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

#if 1
		//日志记录
	 if(0 !=log_init())  //自动开了线程
	 	printf("ERROR: log thread init!!");
#endif
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


