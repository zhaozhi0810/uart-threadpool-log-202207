/*
* @Author: dazhi
* @Date:   2022-07-27 10:47:46
* @Last Modified by:   dazhi
* @Last Modified time: 2022-10-09 11:49:50
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
#include <libgen.h>
#include <stdarg.h>

#include "my_log.h"
#include "my_ipc_msgq.h"
#include "uart_to_mcu.h"
#include "threadpool.h"
//#include "drv_22134_api.h"
#include "codec.h"
#include "debug.h"
#include "i2c_reg_rw.h"
//服务端，包含串口通信，Jc_msgq通信，线程池，日志等。

#define TYPE_SENDTO_API 234   //发    必须跟api是反的！！！！，不要随意改动！！！！
#define TYPE_RECVFROM_API 678   //收

static const char* g_build_time_str = "Buildtime :"__DATE__" "__TIME__;   //获得编译时间
//全局变量，数据应该保持实时刷新。
struct threadpool* pool;  //线程池
static pid_t api_pid = 0;
	
static int server_in_debug_mode = 0;   //服务端进入调试模式		

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
static int is_server_process_start(char * cmd_name)
{
	FILE *ptr = NULL;
	char cmd[256] = "ps -ef | grep %s | grep -v grep | wc -l";
//	int status = 0;
	char buf[64];
	int count;

//	printf("server DEBUG:cmd_name = %s\n",cmd_name);
	snprintf(cmd,sizeof cmd,"ps -ef | grep %s | grep -v grep | wc -l",cmd_name);
	if(server_in_debug_mode)
		printf("ServerDEBUG: api check serverProcess is running, cmd = %s\n",cmd);

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
		if(count < 2)//当进程数小于等于2时，说明进程不存在, 1表示有一个，是grep 进程的
		{
			pclose(ptr);
			if(server_in_debug_mode)
				printf("ServerDEBUG: check serverProcess: no server process,ready to start serverProcess!!\n");
			return 0;  //系统中没有该进程	
		}
		if(server_in_debug_mode)
				printf("ServerDEBUG: check serverProcess: server process is running!!\n");
	}
	pclose(ptr);
	return 1;
}





//检查进程是否正在运行，防止运行多个进程
//返回0表示没有该进程，-1表示存在进程了
// static int isProcessRunning(char * cmd_name)
// {
// #if 0	
//     int lock_fd = open("/tmp/drvServer22134.lock",O_CREAT|O_RDWR,0666);
//     if(lock_fd < 0)
//     {
//     	printf("ERROR: open /tmp/drvServer22134.lock\n");
//     	return -1;
//     }	
//     int rc = flock(lock_fd,LOCK_EX|LOCK_NB); //flock加锁，LOCK_EX -- 排它锁；LOCK_NB -- 非阻塞模式
//     if(rc)  //返回值非0，无法正常持锁
//     {
//         if(EWOULDBLOCK == errno)    //尝试锁住该文件的时候，发现已经被其他服务锁住,errno==EWOULDBLOCK
//         {
//         //	close(lock_fd);	
//             printf("Already Running!\n");
//             return -1; 
//         }
//     }
// //    close(lock_fd);	//不能关闭文件
// #else
// 	int ret = is_server_process_start();
// //	printf("server: Process is Running ret = %d\n",ret);
// 	return ret;   //返回0表示没有该进程，非0则表示有或者出错
    
// #endif    
//     return 0;   //成功加锁。
// }



static int check_api_running(void)
{
	char filename[256] = {0};

	if(api_pid < 1)   //不存在这样的进程
		return 0;

	snprintf(filename,sizeof filename,"/proc/%d/exe",api_pid);
	if(server_in_debug_mode)
		printf("ServerDEBUG: check_api_running filename = %s\n",filename);

	if(access(filename,F_OK ) != -1)
	{
		printf("server : check_api_running,api is running\n");
		return 1;   //进程已存在
	}
	if(server_in_debug_mode)
		printf("ServerDEBUG : check_api_running ,api is not running\n");
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
	if(server_in_debug_mode)
		printf("ServerDEBUG:answer_to_api:answer_to_api cmd = %d param1 = %d param2 = %d\n",
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
		case eAPI_HWTD_SETONOFF_CMD:  //硬件看门狗的使能与禁止							
			mcu_cmd_buf[0] = eMCU_HWTD_SETONOFF_TYPE;  //设置硬件看门狗打开或者关闭			
			mcu_cmd_buf[1] = pmsgbuf->param1;   //打开1或者关闭0
			msgbuf.ret = send_mcu_data(mcu_cmd_buf);
			break;	
		case eAPI_HWTD_FEED_CMD:      //喂狗	
			mcu_cmd_buf[0] = eMCU_HWTD_FEED_TYPE;  //喂狗			
			mcu_cmd_buf[1] = pmsgbuf->param1;   //无所谓0或者1
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
		case eAPI_CHECK_APIRUN_CMD: //判断API库是否已经在运行，防止API库被多个进程使用
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
		case eAPI_HWTD_SETTIMEOUT_CMD:    //设置看门狗喂狗时间
			mcu_cmd_buf[0] = eMCU_HWTD_SETTIMEOUT_TYPE;  //设置所有的led pwm			
			mcu_cmd_buf[1] = pmsgbuf->param1;   //喂狗时间 1-250
			msgbuf.ret = send_mcu_data(mcu_cmd_buf);
			break;
		case eAPI_HWTD_GETTIMEOUT_CMD:    //获取看门狗喂狗时间
			mcu_cmd_buf[0] = eMCU_HWTD_GETTIMEOUT_TYPE;  //led 获取单片机内部温度				
		//	mcu_cmd_buf[1] = pmsgbuf->param1;   //无意义
			if(0 == send_mcu_data(mcu_cmd_buf))  //返回值为0，表示收到了数据
			{//获得mcu的数据？？
				msgbuf.ret = 1; //表示又数据返回
				msgbuf.param1 = mcu_cmd_buf[0];
			}
		break;
		case eAPI_RESET_COREBOARD_CMD:  //复位核心板
			mcu_cmd_buf[0] = eMCU_RESET_COREBOARD_TYPE;  //设置所有的led pwm			
		//	mcu_cmd_buf[1] = pmsgbuf->param1;   //无意义
			msgbuf.ret = send_mcu_data(mcu_cmd_buf);
			break;
		case eAPI_RESET_LCD_CMD:        //复位lcd 9211（复位引脚没有连通）
			mcu_cmd_buf[0] = eMCU_RESET_LCD_TYPE;  //设置所有的led pwm			
		//	mcu_cmd_buf[1] = pmsgbuf->param1;   //无意义
			msgbuf.ret = send_mcu_data(mcu_cmd_buf);
			break;
		case eAPI_RESET_LFBOARD_CMD: //,    //复位底板，好像没有这个功能！！！	
			//nothing to do
		break;

		case eAPI_MICCTRL_SETONOFF_CMD:  //控制底板mic_ctrl引脚的电平
			mcu_cmd_buf[0] = eMCU_MICCTRL_SETONOFF_TYPE;  //设置所有的led pwm			
			mcu_cmd_buf[1] = pmsgbuf->param1;   //无意义
			msgbuf.ret = send_mcu_data(mcu_cmd_buf);
		break;

		//case eMCU_LEDS_FLASH_TYPE:      //led键灯闪烁控制
		case eAPI_LEDS_FLASH_CMD:  //led键灯闪烁控制
			mcu_cmd_buf[0] = eMCU_LEDS_FLASH_TYPE;  //设置某一个led			
			mcu_cmd_buf[1] = pmsgbuf->param1;   //设置某一个led
			msgbuf.ret = send_mcu_data(mcu_cmd_buf);
		break;

		default:
			msgbuf.ret = -1;   //不能是别的命令
		break;
#endif		
	}	

	//3.做出应答给API
	if(0!= Jc_msgq_send_ack(&msgbuf))   //应答发出后，不需要等待
		printf("error : Jc_msgq_send\n");
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
		ret = Jc_msgq_recv(TYPE_RECVFROM_API,pmsgbuf,0);  //阻塞试接收，消息类型都是同一种
		if(ret == 0)   //收到信息，打印出来
		{
			//调试打印						
			{	
				if(server_in_debug_mode)			
					printf("ServerDEBUG:msg_connect type = %ld cmd = %d b = %d c = %d rt = %d\n",pmsgbuf->types,pmsgbuf->cmd,pmsgbuf->param1,pmsgbuf->param2,pmsgbuf->ret);
				threadpool_add_job(pool,api_answer_thread,pmsgbuf);
			}
		}
		else
		{
			printf("Server ERROR: Jc_msgq_recv\n");
			if(errno != EINTR)  //捕获到信号
			{
				printf("Server error errno != EINTR!\n");
			}			
			free(pmsgbuf);   //出错的情况下由自己释放
		}	
	}
	return NULL;
}






#define I2C_ADAPTER_DEVICE	"/dev/i2c-4"
#define I2C_DEVICE_ADDR		(0x11)

//设置pcm音量为某个值，val范围0-192.值越大，声音越小
static void drvSetTuneVal(int val)
{
	if(val>192)
		val = 192;
	else if(val < 0)
		val = 0;
	CHECK(!s_write_reg(ES8388_DACCONTROL4, val), , "Error s_write_reg!");
	CHECK(!s_write_reg(ES8388_DACCONTROL5, val), , "Error s_write_reg!");
}

//48.设置扬声器音量值 参数范围为[0,100]，通道2的左声道
static void drvSetSpeakVolume(int value)
{
	CHECK(value > 0 && value <= 100, , "Error value out of range!");
	value = 0x21*value/100;
	CHECK(!s_write_reg(ES8388_DACCONTROL26, value), , "Error s_write_reg!");
}

// static const char* my_opt = "vhpwb:d:";

/* This function will open the uInput device. Please make 
sure that you have inserted the uinput.ko into kernel. */ 
int main(int argc, char *argv[]) 
{
	int t;
	
	printf("%s running,%s\n",argv[0],g_build_time_str);

	if(argc == 2)
	{
		if(strcmp(argv[1],"-D") == 0)  //加了-D选项，则进入调试模式
		{
			server_in_debug_mode = 1;    //server开启调试模式
			printf("ServerDEBUG: serverProcess enter Debug Mode!!\n");
		}
	}

	if(is_server_process_start(basename(argv[0])))   //防止服务程序被多次运行
	{
		printf("ERROR : serverProcess is Running, Do not run it again!!\n");
		return 0;
	}
	if(server_in_debug_mode)	
		printf("ServerDEBUG: serverProcess is begin to Running\n");
	//转为守护进程
	if(!server_in_debug_mode){
		if(daemon(0,0))   //daemon,调试模式不进入守护进程模式，
		{
			perror("daemon");
			return -1;
		}

#if 1	
		//日志记录
	 	if(0 !=log_init())  //调试模式不记录日志，
	 		printf("ERROR: log thread init!!");
#endif
	}
	
	//串口通信	
	if(0 != uart_init(argc, argv))
	{
		printf("error:uart_init \n");
		return -1;
	}

	if(server_in_debug_mode)	
		printf("ServerDEBUG: serverProcess uart init ok!!!\n");

		//用于通信的消息队列
	if(0 != Jc_msgq_init())
	{
		printf("error : Jc_msgq_init\n");
		return -1;
	}

	if(server_in_debug_mode)	
		printf("ServerDEBUG: serverProcess uart init ok!!!\n");


	int ret = i2c_adapter_init(I2C_ADAPTER_DEVICE, I2C_DEVICE_ADDR);
	if(ret == 0) //不为0，表示出错！！
	{
		drvSetTuneVal(1);  //设置pcm音量值，0-192，值越大声音越小
		drvSetSpeakVolume(95); //设值扬声器音量值，0-100，值越大声音越大
		i2c_adapter_exit();
		if(server_in_debug_mode)	
			printf("ServerDEBUG: serverProcess Volume  set ok!!!\n");
	}
	else
		printf("serverProcess: ERROR: i2c_adapter_init,Volume not set \n");
	

	//线程池初始化
	pool = threadpool_init(4,6);  //初始有多少线程，最多有多少任务排队

	if(server_in_debug_mode)	
		printf("ServerDEBUG: serverProcess threadpool  init ok!!!\n");
	//串口任务，独占一个线程
	threadpool_add_job(pool,mcu_recvSerial_thread,/*&g_datas*/&t);  //串口接收,线程启动后不再退出！！！
	
	if(server_in_debug_mode)	
		printf("ServerDEBUG: serverProcess ready to run msg thread!!!\n");
	//处理msg的接收信息。
	msg_connect(NULL);   //这个线程启动后不再退出。接收msg的消息
	
	while(1)
	{				
		sleep(100);
	}

}


