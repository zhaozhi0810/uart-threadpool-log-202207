
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <utmp.h>
#include <time.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
//#include <linux/input.h>
#include <linux/rtc.h>
//#include <linux/ioctl.h>
#include <sys/ioctl.h>
// #include <sys/types.h>
// #include <sys/stat.h>
// #include <fcntl.h>

#include "drv_22134_api.h"
#include "my_ipc_msgq.h"  //用于与串口程序通信！！
#include "gpio_export.h"
//#include "mixer_scontrols.h"   //音频控制
#include "codec.h"
#include "debug.h"
#include "i2c_reg_rw.h"
#include "drv_22134_server.h"
#include "keyboard.h"
//#include "drv_gpio_input_event.h"   //ptt事件的处理
/*
	与串口通信部分，使用Jc_msgq的方式，发送id为678，接收id为234

 */
#define TYPE_API_SENDTO_SERVER 678   //发 改到Jc_msgq.c中去了
#define TYPE_API_RECFROM_SERVER 234


#define  DEBUG_PRINTF    //用于输出一些出错信息
#define  MY_DEBUG        //用于输出一些调试信息，正常使用时不需要
static int print_debug = 0;
//static int gs_LcdType = -1;   //0 表示5inch，1表示7inch，-1屏类型未知，2022-12-13

//#define I2C_ADAPTER_DEVICE	"/dev/i2c-4"  //改到i2c_reg_rw.h中
//#define I2C_DEVICE_ADDR		(0x10)      //尝试联系0x10或者0x11
#define RTC_DEV				"/dev/rtc0"   //2022-09-22 修改为rtc0

//static int es8388_iic_addr  = -1;  //2022-12-16
static int es8388i2c_adapter_fd = -1;  //文件描述符,2022-12-19


#ifdef DEBUG_PRINTF
  #define DBG_PRINTF(fmt, args...)  \
  do{\
    printf("<<File:%s  Line:%d  Function:%s>> ", __FILE__, __LINE__, __FUNCTION__);\
    printf(fmt, ##args);\
  }while(0)
#else
  #define DBG_PRINTF(fmt, args...)   
#endif



// struct rtc_time t;
  
#ifdef MY_DEBUG
#define MY_PRINTF(fmt, args...)  \
  if(print_debug) \
	  do{\
		printf("<<MY_DEBUG>> ");\
	    printf(fmt, ##args);\
	  }while(0)
#else
  #define MY_PRINTF(fmt, args...)   
#endif

static const char* g_build_time_str = "Buildtime :"__DATE__" "__TIME__;   //获得编译时间


//val 非0表示打印一般的调试信息，0则不打印调试信息，默认不打印
void set_print_debug(int val)
{
	print_debug = val;
}



static int CoreBoardInit = 0;   //初始化了吗？初始化成功为1，失败为-1。
//static int KeyboardTypepins[3]={-1,-1,-1};  //用于键盘识别的
static int MicCtrl_pin = -1;   //2022-12-13  mic_ctrl 引脚改由3399控制 GPIO2D0

//static int lock_fd = -1;
//int atexit(void (*function)(void));
//进程正常结束时关闭文件
static void at_exit_close_file(void)
{ 	
//	printf("at_exit_close_file\n");
	drvCoreBoardExit();  //函数中处理了防止多次执行
}




//需要单片机通信的，就需要初始化，所以不是所有的api都需要这么做
static int assert_init(void)
{	
	if(CoreBoardInit!=1) //没有初始化
	{
		drvCoreBoardInit();  //进行初始化
		if(CoreBoardInit!=1)  //还是失败？？？
			return -1;
	}
	return 0;
}




//api发送数据给服务器，并且等待服务器应答，超时时间1s
//第二个参数可以用于返回数据，无数据时可以为NULL
int api_send_and_waitack(int cmd,int param1,int *param2)
{
	msgq_t msgbuf;  //用于应答
	int ret;	

	msgbuf.types = TYPE_API_SENDTO_SERVER;  //发送的信息类型
	msgbuf.cmd = cmd;      //结构体赋值
	msgbuf.param1 = param1;
//	printf("API DEBUG: param1 = %d msgbuf.param1 = %d\n",param1,msgbuf.param1);
	if(param2)
		msgbuf.param2 = *param2;
	else //空指针
		msgbuf.param2 = 0;
	msgbuf.ret = 0;   //一般没有使用
//	printf("DEBUG: cmd = %d\n",cmd);
	//3.做出应答
	ret = Jc_msgq_send(TYPE_API_RECFROM_SERVER+cmd,&msgbuf,20); //数据发出后，需要等待 20表示1s
	if(0!= ret)
	{		
		printf("error : Jc_msgq_send ,ret = %d\n",ret);
		return ret;
	} 
//	printf("DEBUG: Jc_msgq_send ok\n");
	//判断是否有返回数据
	if(param2 && (msgbuf.ret>0))  //ret >0 表示有数据回来,小于0表示出错，等于0表示应答
		*param2 = msgbuf.param1;   //用param1返回数据

	return 0;	
}


static int __setlcdbrt(int nBrtVal)
{
	char data[8];

	if(nBrtVal>= 0 && nBrtVal <= 255)
	{
		snprintf(data,sizeof data,"%d",nBrtVal);  //数字转字符串
		FILE *fp = fopen("/sys/class/backlight/backlight/brightness","r+");
		if(fp == NULL)
		{
			printf("ERROR : open /sys/class/backlight/backlight/brightness\n");
			return -1;
		}
		fputs(data,fp);

		fclose(fp);	
		return 0;	
	}

	return -1;  //数值超标
}


static int MicCtrl_PinInit(void)
{
	static int inited = 0;

	if(inited)
		return 0;
	//获取引脚号
	MicCtrl_pin = get_pin(RK_GPIO2,RK_PD0);
	  //设置为输出引脚
	if(gpio_direction_set(MicCtrl_pin, GPIO_DIR_OUT) == false)
	{
		printf("ERROR: gpio_direction_set MicCtrl_pin = %d\n",MicCtrl_pin);
	//	return -1;
	}
	inited = 1;   //初始化了！！！
	return 0;
}

static int my_system(char* cmd)
{
	pid_t fpid; //fpid表示fork函数返回的值

	if(cmd == NULL)  //空指针
 		return -1;

 	int ret = access(cmd, F_OK | X_OK);  //文件存在并且可执行吗？
 	if(ret)  //返回0成功，-1失败
 	{
 		printf("access %s return -1\n",cmd);
 		return -1;
 	}

    fpid = fork();
    if (fpid < 0)
    {    
    	printf("error in fork!");
    	return -1;
	}
    else if (fpid == 0) {
       execl(cmd, cmd, NULL);
       return -1;
    } 
    //else {   //父进程
    return 0;
}


//ps -ef | grep drv_22134_server | grep -v grep | wc -l
//尝试启动server进程
//返回0表示server进程已经存在，-1表示出错
static int start_server_process(void)
{
	FILE *ptr = NULL;
	char cmd[] = "ps -ef | grep drv722_22134_server | grep -v grep | wc -l";
//	int status = 0;
	char buf[64];
	int count;

	if((ptr = popen(cmd, "r")) == NULL)
	{
		printf("popen err\n");
		return -1;
	}
	memset(buf, 0, sizeof(buf));
	if((fgets(buf, sizeof(buf),ptr))!= NULL)//获取进程和子进程的总数
	{
		count = atoi(buf);
		if(count <= 0)//当进程数小于等于0时，说明进程不存在
		{
			my_system("/root/drv722_22134_server");    //system 会启动两个进程，会导致判断出现一点问题
			printf("api start drv722_22134_server \n");
			usleep(500000);  //等待一下
		}
	}
	pclose(ptr);
	return 0;
}



//检查进程是否正在运行，防止运行多个进程
//返回0，表示第一次启动API，否则为多次，则应该不允许启动
static int isProcessRunning()
{	
    //2022-08-09改为Jc_msgq通信确认进程是否存在，发送pid过去
    pid_t pid;
    int ret = 0;   //参数需要一个指针，返回函数返回值
    pid = getpid();  //获得自己的pid
    
	//查询是否存在api进程，让服务进程查询
	if(api_send_and_waitack(eAPI_CHECK_APIRUN_CMD,pid,&ret))  //发送的第二个参数表示pid，参数3用于返回结果，0表示没有启动API，1表示已经存在，-1表示出错
	{
		printf("error : isProcessRunning pid = %d\n",pid);
		return -1;
	}
	return ret;
   
}


//1. 核心板初始化函数
//-1：初始化失败
//0：初始化成功
int drvCoreBoardInit(void)
{
	int ret ;
	int es8388_iic_addr  = -1;  //2022-12-16

	if(CoreBoardInit == 1)  //已经初始化了
		return 0;

	printf("libdrv722_22134.so is running, %s\n",g_build_time_str);

	start_server_process();
	atexit(at_exit_close_file); //注册一个退出函数
	
	ret = Jc_msgq_init();
	if(ret)  //不为0，表示出错！！
	{
		DBG_PRINTF("ERROR: Jc_msgq_init ret = %d\n",ret);
		CoreBoardInit = -1;   //记录初始化失败
		return ret;
	}
	
	if(isProcessRunning())   //防止程序被多次运行
	{
		printf("API ERROR:drvCoreBoardInit Process is Running by someone,Do not allow this program to continue!!!!\n");
		exit(-1);   //结束进程
	}

	ret = MicCtrl_PinInit();  //用于mic_ctr引脚初始化
	if(ret) //不为0，表示出错！！
	{
		DBG_PRINTF("ERROR: MicCtrl_PinInit（2022-12-13） ret = %d\n",ret);
		CoreBoardInit = -1;   //记录初始化失败
		return ret;
	}

	es8388_iic_addr = es8388_find_iic_devaddr();  //函数在i2c_reg_rw.c
	if(es8388_iic_addr > 0)
	{
		es8388i2c_adapter_fd = i2c_adapter_init(I2C_ADAPTER_ES8388, es8388_iic_addr);
		if(es8388i2c_adapter_fd < 0) //不为0，表示出错！！
		{
			DBG_PRINTF("ERROR: i2c_adapter_init ret = %d\n",ret);
			CoreBoardInit = -1;   //记录初始化失败
			return ret;
		}
	}

	ret = keyboard_init();  //按键事件处理线程初始化 
	if(ret) //不为0，表示出错！！
	{
		DBG_PRINTF("ERROR: keyboard_init ret = %d\n",ret);
		CoreBoardInit = -1;   //记录初始化失败
		return ret;
	}

	//2023-01-12 默认手柄mic输入，音量调整，内部回环取消（0x26）
	drvSelectHandMic();
	s_write_reg(es8388i2c_adapter_fd, 0x1a, 0);
	s_write_reg(es8388i2c_adapter_fd, 0x1b, 0);
	s_write_reg(es8388i2c_adapter_fd, 0x26, 0x12);
	s_write_reg(es8388i2c_adapter_fd, 0x12, 0x3a);

	CoreBoardInit = 1;  //初始化成功
	//drvSetV12CrlOnOff(1);
		
	return 0;
}


//68. api退出函数
void drvCoreBoardExit(void)
{
	static int exited = 0;

	if(exited)
		return;

	keyboard_exit();   //键盘处理线程退出
	i2c_adapter_exit(es8388i2c_adapter_fd);  //iic的文件关闭
	
	gpio_direction_unset(MicCtrl_pin);

	Jc_msgq_exit();  //清除消息队列中的消息
	CoreBoardInit = 0;   //未初始化了！！！
	
	exited = 1;   //已经执行过该函数
	return;
}







//2. 使能看门狗函数
//返回值
//0：设置成功
//1：设置失败
int drvWatchDogEnable(void)
{
	int param = 1;
	if(assert_init())  //未初始化
		return -1;

	if(api_send_and_waitack(eAPI_HWTD_SETONOFF_CMD,param,&param))  //发送的第2个参数1 表示使能
	{
		printf("error : drvWatchDogEnable\n");
		return 1;
	}
	return 0;
}

//3. 去使能看门狗函数
//返回值
//0：设置成功
//1：设置失败
int drvWatchDogDisable(void)
{
	int param = 0;
	if(assert_init())  //未初始化
		return -1;

	if(api_send_and_waitack(eAPI_HWTD_SETONOFF_CMD,param,&param))  //发送的第2个参数0 表示禁止
	{
		printf("error : drvWatchDogDisable\n");
		return 1;
	}

	return 0;
}

//4. 喂狗函数
void drvWatchDogFeeding(void)
{
	int param = 0;
	if(assert_init())  //未初始化
		return;

	if(api_send_and_waitack(eAPI_HWTD_FEED_CMD,param,&param))  //发送的第2，3个参数无意义
	{
		printf("error : drvWatchDogFeeding\n");
		return;
	}
}

//5. 设置看门狗超时时间
//（参数timeout 只支持1-250，表示100ms-25S的范围）
int drvWatchdogSetTimeout(int timeout)
{
	int param = 0;
	if(assert_init())  //未初始化
		return -1;

	if(timeout < 1)
		timeout = 1;
	else if(timeout > 250)
		timeout = 250;


	if(api_send_and_waitack(eAPI_HWTD_SETTIMEOUT_CMD,timeout,&param))  //发送的第2，3个参数无意义
	{
		printf("error : drvWatchdogSetTimeout timeout = %d\n",timeout);
		return -1;
	}
	return 0;
}

//6. 获取看门狗超时时间
//返回值 1-250表示合法，表示超时时间为100ms-25s
int drvWatchdogGetTimeout(void)
{
	int param = 0;   //参数需要一个指针，返回函数返回值
	if(assert_init())  //未初始化
		return 0;
//	MY_PRINTF("nothing todo 2022-07-27\n");
	if(api_send_and_waitack(eAPI_HWTD_GETTIMEOUT_CMD,0,&param))  //发送的第二个参数无意义，第三个用于返回值
	{
		printf("error : drvWatchdogGetTimeout \n");
		return 0;
	}
	printf("drvWatchdogGetTimeout Timeout（1-250） = %d\n",param);
	return param;   //返回的温度只有整数部分！！
//	return 0;
}

//7. 禁止扬声器放音  //3399的AD7管脚置0  //接口板上J45的5脚 ,通道2的左声道
void drvDisableSpeaker(void)
{
	//通道2左声道 是否是音量调为0？
	unsigned char val = 0;
	CHECK(!s_read_reg(es8388i2c_adapter_fd,ES8388_DACPOWER, &val), , "Error s_read_reg!");
	val &= ~(0x1 << 3);
	CHECK(!s_write_reg(es8388i2c_adapter_fd,ES8388_DACPOWER, val), , "Error s_write_reg!");

}

//8. 允许扬声器放音  //3399的AD7管脚置1,通道2的左声道
void drvEnableSpeaker(void)
{
	//通道2左声道 是否是音量调为80？
	unsigned char val = 0;
	CHECK(!s_read_reg(es8388i2c_adapter_fd,ES8388_DACPOWER, &val), , "Error s_read_reg!");
	val |= (0x1 << 3);
	CHECK(!s_write_reg(es8388i2c_adapter_fd,ES8388_DACPOWER, val), , "Error s_write_reg!");
}

//9. 关闭强声器  //3399的AG4管脚置1  //接口板芯片U5的2脚
void drvDisableWarning(void)
{
	drvSetLSPKOnOff(0);
}

//10. 打开强声器  //3399的AG4管脚置0
void drvEnableWarning(void)
{
	drvSetLSPKOnOff(1);
}

//11. 使能USB0  //3399的管脚AL4置0   //暂时未引到接口板，无法测量
void drvEnableUSB0(void)
{
	MY_PRINTF("nothing todo 2022-07-27\n");
	//nothing todo 2022-07-27
}

//12. 使能USB1  //3399的管脚AK4置0   //暂时未引到接口板，无法测量
void drvEnableUSB1(void)
{
	MY_PRINTF("nothing todo 2022-07-27\n");
	//nothing todo 2022-07-27
}

//13. 去使能USB0  //3399的管脚AL4置1
void drvDisableUSB0(void)
{
	MY_PRINTF("nothing todo 2022-07-27\n");
	//nothing todo 2022-07-27
}

//14 . 去使能USB1  //3399的管脚AK4置1
void drvDisableUSB1(void)
{
	MY_PRINTF("nothing todo 2022-07-27\n");
	//nothing todo 2022-07-27
}

//15. 打开屏幕,由于单片机没法控制lcd的开启关闭，现在只能打开pwm
//2022-12-13 5寸屏可以控制背光使能了。单片机上控制
void drvEnableLcdScreen(void)
{
#if 0  //2022-12-13  --> 0
//	__setlcdbrt(200);
#else	
	int param = 1; //打开屏幕
	if(assert_init())  //未初始化
		return;

	if(api_send_and_waitack(eAPI_LCDONOFF_CMD,1,&param))  //发送的第二个参数表示led号，第三个表示点亮还是熄灭
	{
		printf("error : drvEnableLcdScreen\n");
	}
#endif
}

//16.关闭屏幕，由于单片机没法控制lcd的开启关闭，现在只能打开pwm
//2022-12-13 5寸屏可以控制背光使能了。单片机上控制
void drvDisableLcdScreen(void)
{
#if 0  //2022-12-13  --> 0
	__setlcdbrt(0);
#else	
	int param = 0; //关闭屏幕
	if(assert_init())  //未初始化
		return;
//	printf("debug : drvDisableLcdScreen \n");
	if(api_send_and_waitack(eAPI_LCDONOFF_CMD,0,&param))  //发送的第二个参数表示led号，第三个表示点亮还是熄灭
	{
		printf("error : drvDisableLcdScreen\n");
	}
//	printf("debug : drvDisableLcdScreen ok\n");
#endif
}

//17.使能touch module
void drvEnableTouchModule(void)
{
	MY_PRINTF("nothing todo 2022-07-27\n");
	//nothing todo 2022-07-27
}

//18. 去使能touch module
void drvDisableTouchModule(void)
{
	MY_PRINTF("nothing todo 2022-07-27\n");
	//nothing todo 2022-07-27
}

//19.获取屏幕类型
/*
返回值
1：7寸屏
3：5寸屏
2：捷诚屏
4：55所屏
5：捷诚5寸触摸屏
6：捷诚7寸触摸屏
 */
int drvGetLCDType(void)
{
	int ret;
	ret = getKeyboardType();
	// if(ret >= 0)
	// 	gs_LcdType = (ret == 6);   //设置全局变量，确认只有多功能屏（6）是7寸，其他都是5寸
	return ret;


	// if(ret > 0)  //2022-08-15 确认只有多功能屏是7寸，其他都是5寸
	// {
	// 	return ret;
	// }
	// return -1;   //5寸屏
//	return 0;
}





//20.获取键盘类型
/*
返回值
1：防爆终端键盘类型
2：壁挂Ⅲ型终端键盘类型（不关心！！）
4：嵌入式Ⅰ/Ⅱ/Ⅲ型、防风雨型、壁挂Ⅱ型终端键盘类型
6：多功能型终端键盘类型
 */
int getKeyboardType(void)
{
	//使用3399的gpio读取
	int result = 0;

	if(assert_init())  //未初始化
	return -1;

	if(api_send_and_waitack(eAPI_GET_LCDTYPE_CMD,0,&result))  //发送的第二个参数表示led号，第三个无意义
	{
		result = -1;  //没有获得状态
		printf("error : getKeyboardType \n");
	}

	return result;
}



/*
	读取cpu的温度
	 //2021-12-03 增加
	//2022-07-27,ubuntu文件系统 的路径 /sys/class/hwmon/hwmon0/device/temp0_input 
	 通过参数返回值，读取的温度只有整数部分

	 返回值：0 表示成功，其他表示失败
*/
static  int parse_cputemp(int *temp)
{
	char buf[60]; /* actual lines we expect are ~30 chars or less */
	FILE *fp;
	
	if(!temp)
		return -EPERM;

	//fp = fopen("/sys/class/hwmon/hwmon0/temp1_input","r");   //只读的方式打开 ，原中标麒麟的路径
	fp = fopen("/sys/class/hwmon/hwmon0/device/temp0_input","r");   //只读的方式打开,2022-07-27,ubuntu文件系统
	if(NULL == fp)
		return -ENOENT;
	
	if(fgets(buf, sizeof(buf), fp))
	{
		*temp = atoi(buf) / 1000;
		MY_PRINTF("parse_cputemp = %d\n",*temp);

	}
	/* Have to close because of NOFORK */
	fclose(fp);

	return 0;
}

//21. 获取CPU温度
float drvGetCPUTemp(void)
{
	int tmp = 0;
	int ret; 

	ret = parse_cputemp(&tmp);
	if(!ret)  //返回0表示正确
	{
		return (float)tmp;   //函数只读到整数部分，返回时转为浮点数
	}

	DBG_PRINTF("ERROR: ret = %d\n",ret);
	return 0.0;
}

//22. 获取核心板温度
float drvGetBoardTemp(void)
{
	int param = 0;   //参数需要一个指针，返回函数返回值
	if(assert_init())  //未初始化
		return 0.0;

	//改为从单片机获得温度
	if(api_send_and_waitack(eAPI_BOART_TEMP_GET_CMD,0,&param))  //发送的第二个参数表示led号，第三个表示点亮还是熄灭
	{
		printf("error : drvGetBoardTemp \n");
		return 0.0;
	}
	printf("drvGetBoardTemp param = %d\n",param);
	return param;   //返回的温度只有整数部分！！
}

//23. 获取当前RTC值
long drvGetRTC(void)
{
	int fd = 0;
	struct rtc_time rtc_time;
	struct tm tp;

	CHECK((fd = open(RTC_DEV, O_RDONLY)) > 0, -1, "Error open with %d: %s", errno, strerror(errno));
	bzero(&rtc_time, sizeof(struct rtc_time));
	if(ioctl(fd, RTC_RD_TIME, &rtc_time)) {
		ERR("Error ioctl with %d: %s", errno, strerror(errno));
		close(fd);
		return -1;
	}
	CHECK(!close(fd), -1, "Error close with %d: %s", errno, strerror(errno));
	fd = 0;

	bzero(&tp, sizeof(struct tm));
	tp.tm_sec = rtc_time.tm_sec;
	tp.tm_min = rtc_time.tm_min;
	tp.tm_hour = rtc_time.tm_hour;
	tp.tm_mday = rtc_time.tm_mday;
	tp.tm_mon = rtc_time.tm_mon;
	tp.tm_year = rtc_time.tm_year;
	tp.tm_wday = rtc_time.tm_wday;
	tp.tm_yday = rtc_time.tm_yday;
	tp.tm_isdst = rtc_time.tm_isdst;
	return mktime(&tp);	
//	return 0;
}

//24. 设置RTC值
long drvSetRTC(long secs)
{
	int fd = 0;
	struct rtc_time rtc_time;
	struct tm *tp = NULL;

	CHECK(tp = localtime(&secs), -1, "Error localtime with %d: %s", errno, strerror(errno));

	CHECK((fd = open(RTC_DEV, O_RDONLY)) > 0, -1, "Error open with %d: %s", errno, strerror(errno));
	bzero(&rtc_time, sizeof(struct rtc_time));
	rtc_time.tm_sec = tp->tm_sec;
	rtc_time.tm_min = tp->tm_min;
	rtc_time.tm_hour = tp->tm_hour;
	rtc_time.tm_mday = tp->tm_mday;
	rtc_time.tm_mon = tp->tm_mon;
	rtc_time.tm_year = tp->tm_year;
	rtc_time.tm_wday = tp->tm_wday;
	rtc_time.tm_yday = tp->tm_yday;
	rtc_time.tm_isdst = tp->tm_isdst;
	if(ioctl(fd, RTC_SET_TIME, &rtc_time)) {
		ERR("Error ioctl with %d: %s", errno, strerror(errno));
		close(fd);
		return -1;
	}
	CHECK(!close(fd), -1, "Error close with %d: %s", errno, strerror(errno));
	return 0;
}

//25. 设置LED灯亮度  参数范围为[0,0x64]
void drvSetLedBrt(int nBrtVal)
{
	int param = 1;
	if(assert_init())  //未初始化
		return;

	if(api_send_and_waitack(eAPI_LEDSETPWM_CMD,nBrtVal,&param))  //发送的第二个参数表示亮度，第三个无意义
	{
		printf("error : drvSetLedBrt ,nBrtVal = %d\n",nBrtVal);
	}
}





//26. 设置屏幕亮度  参数范围为[0,0xff]
void drvSetLcdBrt(int nBrtVal)
{
	if(nBrtVal < 0 || nBrtVal > 255)
	{
		return;
	}
	__setlcdbrt(nBrtVal);
}





//27. 点亮对应的键灯，单片机命令
void drvLightLED(int nKeyIndex)
{
	int param = 1;
	if(assert_init())  //未初始化
		return;

	if(api_send_and_waitack(eAPI_LEDSET_CMD,nKeyIndex,&param))  //发送的第二个参数表示led号，第三个表示点亮还是熄灭
	{
		printf("error : drvLightLED ,nKeyIndex = %d\n",nKeyIndex);
	}

}

//28. 熄灭对应的键灯
void drvDimLED(int nKeyIndex)
{
	int param = 0;
	if(assert_init())  //未初始化
		return;

	if(api_send_and_waitack(eAPI_LEDSET_CMD,nKeyIndex,&param))  //发送的第二个参数表示led号，第三个表示点亮还是熄灭
	{
		printf("error : drvDimLED ,nKeyIndex = %d\n",nKeyIndex);
	}

}

//29. 点亮所有的键灯
void drvLightAllLED(void)
{
	int param = 1;
	if(assert_init())  //未初始化
		return;

	if(api_send_and_waitack(eAPI_LEDSETALL_CMD,1,&param))  //发送的第二个参数无意义，第三个表示点亮还是熄灭
	{
		printf("error : drvLightAllLED \n");
	}
}

//30. 熄灭所有的键灯
void drvDimAllLED(void)
{
	int param = 0;
	if(assert_init())  //未初始化
		return;

	if(api_send_and_waitack(eAPI_LEDSETALL_CMD,0,&param))  //发送的第二个参数无意义，第三个表示点亮还是熄灭
	{
		printf("error : drvDimAllLED \n");
	}
}

//31. 获取对应键灯状态
int drvGetLEDStatus(int nKeyIndex)
{
	int status = 0;
	if(assert_init())  //未初始化
		return -1;

	if(api_send_and_waitack(eAPI_LEDGET_CMD,nKeyIndex,&status))  //发送的第二个参数表示led号，第三个无意义
	{
		status = -1;  //没有获得状态
		printf("error : drvGetLEDStatus ,nKeyIndex = %d\n",nKeyIndex);
	}

	return status;
}

//32. 获取耳机插入状态
////返回值
//-1：失败
//0：未插入
//1：插入
int drvGetMicStatus(void)
{
//	return get_headset_insert_status()? 1:0;
	return 0;
}

//33. 获取手柄插入状态
//返回值
//-1：失败
//0：未插入
//1：插入
int drvGetHMicStatus(void)
{
//	return get_handle_insert_status()? 1:0;
	return 0;
}


//typedef void (GPIO_NOTIFY_FUNC)(int gpio, int val);
//int gpio：键值
//int val：状态，1为按下或插入，0为松开或拔出
//Gpio值：
//面板ptt：25
//手柄ptt：30
//脚踩ptt：17
//耳机插入：48
//手柄插入：49
//34. PTT及耳机插入等事件上报回调
void drvSetGpioCbk(GPIO_NOTIFY_FUNC cbk)
{
	__drvSetGpioCbk(cbk);  //keyboard.c 中给变量赋值
}


//typedef void (GPIO_NOTIFY_KEY_FUNC)(int gpio, int val);

//int gpio：键值
//int val：状态，1为按下，0为松开
//35. 面板按键事件上报回调
void drvSetGpioKeyCbk(GPIO_NOTIFY_KEY_FUNC cbk)
{
//	printf("drvSetGpioKeyCbk\n");
	__drvSetGpioKeyCbk(cbk);  //keyboard.c 中给变量赋值
//	static GPIO_NOTIFY_KEY_FUNC s_cbk;
//	cbk = cbk;
//	threadpool_add_job(pool, s_recv_event_thread, s_cbk);   //把任务添加到线程池
}

//36. 获取电压值
float drvGetVoltage(void)
{
	MY_PRINTF("nothing todo 2022-07-27\n");
	//nothing todo 2022-07-27	
	return 0.0;
}

//37. 获取电流值
float drvGetCurrent(void)
{
	MY_PRINTF("nothing todo 2022-07-27\n");
	//nothing todo 2022-07-27
	return 0.0;
}

//38. Lcd屏重置，返回0表示正确
int drvLcdReset(void)
{
	int param = 1;
	if(assert_init())  //未初始化
		return -1;

	if(api_send_and_waitack(eAPI_RESET_LCD_CMD,1,&param))  //发送的第二个参数无意义，第三个无意义
	{
		printf("error : drvLcdReset \n");
		return -1;
	}
	return 0;
}

//39.键盘重置
int drvIfBrdReset(void)
{
//	MY_PRINTF("nothing todo 2022-07-27\n");
	//nothing todo 2022-07-27	
	int param = 1;
	if(assert_init())  //未初始化
		return -1;

	if(api_send_and_waitack(eAPI_RESET_LFBOARD_CMD,1,&param))  //发送的第二个参数无意义，第三个无意义
	{
		printf("error : drvIfBrdReset \n");
		return -1;
	}

	return 0;
}

//40.核心板重置，返回0表示正确
int drvCoreBrdReset(void)
{
	int param = 1;
	if(assert_init())  //未初始化
		return -1;

	if(api_send_and_waitack(eAPI_RESET_COREBOARD_CMD,1,&param))  //发送的第二个参数无意义，第三个无意义
	{
		printf("error : drvCoreBrdReset \n");
		return -1;
	}
	return 0;
}

//41.打开音量调节
void drvEnableTune(void)
{
	MY_PRINTF("nothing todo 2022-07-27\n");
	//nothing todo 2022-07-27
}

//42.关闭音量调节
void drvDisableTune(void)
{
	MY_PRINTF("nothing todo 2022-07-27\n");
	//nothing todo 2022-07-27
}

//43.分级调节音量大小
void drvAdjustTune(void)
{
	MY_PRINTF("nothing todo 2022-07-27\n");
	//nothing todo 2022-07-27
}

//44.正向调节音量，pcm调节
void drvSetTuneUp(void)
{
//	amixer_vol_control(PCM,10,'+');
//	CHECK(value > 0 && value <= 100, , "Error value out of range!");
	int val = 0;
	// unsigned char val_max = 0x21;
	// unsigned char val_step = val_max*value/100;

	CHECK(!s_read_reg(es8388i2c_adapter_fd,ES8388_DACCONTROL4, (void*)&val), , "Error s_read_reg!");
//	printf("drvSetTuneUp val = %d\n",val);
	val -= 10;
	if(val>192)
		val = 192;
	else if(val < 0)
		val = 0;
	
	//val = (val <= 0)? 0:val;
//	printf("2 drvSetTuneUp val = %d\n",val);
	CHECK(!s_write_reg(es8388i2c_adapter_fd,ES8388_DACCONTROL4, val), , "Error s_write_reg!");
	CHECK(!s_write_reg(es8388i2c_adapter_fd,ES8388_DACCONTROL5, val), , "Error s_write_reg!");		
}




//45.反向调节音量，pcm调节
void drvSetTuneDown(void)
{
//	amixer_vol_control(PCM,10,'-');
	unsigned char val = 0;

	CHECK(!s_read_reg(es8388i2c_adapter_fd,ES8388_DACCONTROL4, &val), , "Error s_read_reg!");
	val += 10;
	if(val>192)
		val = 192;
	else if(val < 0)
		val = 0;
	CHECK(!s_write_reg(es8388i2c_adapter_fd,ES8388_DACCONTROL4, val), , "Error s_write_reg!");
	CHECK(!s_write_reg(es8388i2c_adapter_fd,ES8388_DACCONTROL5, val), , "Error s_write_reg!");
}

//46.增加面板扬声器音量，输出2的左声道
void drvAddSpeakVolume(int value)
{
//	amixer_vol_left_control(Output_2,value,'+');
	CHECK(value > 0 && value <= 100, , "Error value out of range!");
	unsigned char val = 0;
	unsigned char val_max = 0x21;
	unsigned char val_step = val_max*value/100;

	CHECK(!s_read_reg(es8388i2c_adapter_fd,ES8388_DACCONTROL26, &val), , "Error s_read_reg!");
	val += val_step;
	val = (val > val_max)? val_max:val;
	CHECK(!s_write_reg(es8388i2c_adapter_fd,ES8388_DACCONTROL26, val), , "Error s_write_reg!");	
}

//47.减少面板扬声器音量，通道2的左声道
void drvSubSpeakVolume(int value)
{
//	amixer_vol_left_control(Output_2,value,'-');
	CHECK(value > 0 && value <= 100, , "Error value out of range!");
	unsigned char val = 0;
	unsigned char val_max = 0x21;
	unsigned char val_step = val_max*value/100;

	CHECK(!s_read_reg(es8388i2c_adapter_fd,ES8388_DACCONTROL26, &val), , "Error s_read_reg!");
	val -= val_step;
	val = (val < 0)? 0:val;
	CHECK(!s_write_reg(es8388i2c_adapter_fd,ES8388_DACCONTROL26, val), , "Error s_write_reg!");
}

//48.设置扬声器音量值 参数范围为[0,100]，通道2的左声道
void drvSetSpeakVolume(int value)
{
//	amixer_vol_left_control(Output_2,value,' ');
	CHECK(value > 0 && value <= 100, , "Error value out of range!");
	// unsigned char val = 0;
	// unsigned char val_max = 0x21;
	value = 0x21*value/100;

	//CHECK(!s_read_reg(ES8388_DACCONTROL26, &val), , "Error s_read_reg!");
	//val -= val_step;
	//val = (val < 0)? 0:val;
	CHECK(!s_write_reg(es8388i2c_adapter_fd,ES8388_DACCONTROL26, value), , "Error s_write_reg!");
}

//49.增加手柄音量 参数范围为[0,100]，通道2的右声道
void drvAddHandVolume(int value)
{
	CHECK(value > 0 && value <= 100, , "Error value out of range!");
	unsigned char val = 0;
	unsigned char val_max = 0x21;
	unsigned char val_step = val_max*value/100;

	CHECK(!s_read_reg(es8388i2c_adapter_fd,ES8388_DACCONTROL25, &val), , "Error s_read_reg!");
	val += val_step;
	val = (val > val_max)? val_max:val;
	CHECK(!s_write_reg(es8388i2c_adapter_fd,ES8388_DACCONTROL25, val), , "Error s_write_reg!");
}

//50.减少手柄音量，通道2的右声道
void drvSubHandVolume(int value)
{
	CHECK(value > 0 && value <= 100, , "Error value out of range!");
	unsigned char val = 0;
	unsigned char val_max = 0x21;
	unsigned char val_step = val_max*value/100;

	CHECK(!s_read_reg(es8388i2c_adapter_fd,ES8388_DACCONTROL25, &val), , "Error s_read_reg!");
	val -= val_step;
	val = (val < 0)? 0:val;
	CHECK(!s_write_reg(es8388i2c_adapter_fd,ES8388_DACCONTROL25, val), , "Error s_write_reg!");
}

//51.设置手柄音量值，通道2的右声道
void drvSetHandVolume(int value)
{
	CHECK(value >= 0 && value <= 100, , "Error value out of range!");
	value = 0x21*value/100;
	// unsigned char val = 0;
	// unsigned char val_max = 0x21;
	// unsigned char val_step = val_max*value/100;

	// CHECK(!s_read_reg(ES8388_DACCONTROL25, &val), , "Error s_read_reg!");
	// val -= val_step;
	// val = (val < 0)? 0:val;
	CHECK(!s_write_reg(es8388i2c_adapter_fd,ES8388_DACCONTROL25, value), , "Error s_write_reg!");
}

//52.增加耳机音量  通道1 的左右声道
void drvAddEarphVolume(int value)
{
//	amixer_vol_control(Output_1,value,'+');
	CHECK(value > 0 && value <= 100, , "Error value out of range!");
	unsigned char val = 0;
	unsigned char val_max = 0x21;
	unsigned char val_step = val_max*value/100;

//	CHECK(!s_read_reg(ES8388_DACCONTROL27, &val), , "Error s_read_reg!");
	CHECK(!s_read_reg(es8388i2c_adapter_fd,ES8388_DACCONTROL24, &val), , "Error s_read_reg!");

	val += val_step;
	val = (val > val_max)? val_max:val;
	CHECK(!s_write_reg(es8388i2c_adapter_fd,ES8388_DACCONTROL24, val), , "Error s_write_reg!");
	CHECK(!s_write_reg(es8388i2c_adapter_fd,ES8388_DACCONTROL25, val), , "Error s_write_reg!");
}

//53.减少耳机音量，通道1左右声道
void drvSubEarphVolume(int value)
{
//	amixer_vol_control(Output_1,value,'-');
	CHECK(value > 0 && value <= 100, , "Error value out of range!");
	unsigned char val = 0;
	unsigned char val_max = 0x21;
	unsigned char val_step = val_max*value/100;

	//CHECK(!s_read_reg(ES8388_DACCONTROL27, &val), , "Error s_read_reg!");
	CHECK(!s_read_reg(es8388i2c_adapter_fd,ES8388_DACCONTROL24, &val), , "Error s_read_reg!");
	val -= val_step;
	val = (val < 0)? 0:val;
	CHECK(!s_write_reg(es8388i2c_adapter_fd,ES8388_DACCONTROL24, val), , "Error s_write_reg!");
	CHECK(!s_write_reg(es8388i2c_adapter_fd,ES8388_DACCONTROL25, val), , "Error s_write_reg!");
}

//54.设置耳机音量值  参数范围为[0,100]，通道1左右声道
void drvSetEarphVolume(int value)
{
//	amixer_vol_control(Output_1,value,' ');
	CHECK(value >= 0 && value <= 100, , "Error value out of range!");
	value = 0x21*value/100;
	CHECK(!s_write_reg(es8388i2c_adapter_fd,ES8388_DACCONTROL24, value), , "Error s_write_reg!");
	CHECK(!s_write_reg(es8388i2c_adapter_fd,ES8388_DACCONTROL25, value), , "Error s_write_reg!");
}

//55.打开手柄音频输出 通道2的右声道
void drvEnableHandout(void)
{
	unsigned char val = 0;
	CHECK(!s_read_reg(es8388i2c_adapter_fd,ES8388_DACPOWER, &val), , "Error s_read_reg!");
	val |= (0x1 << 2);
	CHECK(!s_write_reg(es8388i2c_adapter_fd,ES8388_DACPOWER, val), , "Error s_write_reg!");
}

//56.关闭手柄音频输出 通道2的右声道
void drvDisableHandout(void)
{
	unsigned char val = 0;
	CHECK(!s_read_reg(es8388i2c_adapter_fd,ES8388_DACPOWER, &val), , "Error s_read_reg!");
	val &= ~((unsigned char)0x1 << 2);
	CHECK(!s_write_reg(es8388i2c_adapter_fd,ES8388_DACPOWER, val), , "Error s_write_reg!");
}

//57.打开耳机音频输出，通道1的左右声道
void drvEnableEarphout(void)
{
	unsigned char val = 0;
	CHECK(!s_read_reg(es8388i2c_adapter_fd,ES8388_DACPOWER, &val), , "Error s_read_reg!");
	val |= (0x3 << 4);
	CHECK(!s_write_reg(es8388i2c_adapter_fd,ES8388_DACPOWER, val), , "Error s_write_reg!");
}

//58.关闭耳机音频输出， 通道1的左右声道
void drvDisableEarphout(void)
{
	unsigned char val = 0;
	CHECK(!s_read_reg(es8388i2c_adapter_fd,ES8388_DACPOWER, &val), , "Error s_read_reg!");
	val &= ~((unsigned char)0x3 << 4);
	CHECK(!s_write_reg(es8388i2c_adapter_fd,ES8388_DACPOWER, val), , "Error s_write_reg!");
}

//59.选择面板麦克风语音输入，输入通道1
void drvSelectHandFreeMic(void)
{
	unsigned char val = 0;
	CHECK(!s_read_reg(es8388i2c_adapter_fd,ES8388_ADCCONTROL2, &val), , "Error s_read_reg!");
	val &= 0x0f;
	CHECK(!s_write_reg(es8388i2c_adapter_fd,ES8388_ADCCONTROL2, val), , "Error s_write_reg!");
//	CHECK(!s_write_reg(ES8388_ADCCONTROL1, 0x33), , "Error s_write_reg!");
}

//60.选择手柄麦克风语音输入，输入通道2
void drvSelectHandMic(void)
{
	unsigned char val = 0;
	CHECK(!s_read_reg(es8388i2c_adapter_fd,ES8388_ADCCONTROL2, &val), , "Error s_read_reg!");
	val &= 0x0f;
	val |= 0x50;
	CHECK(!s_write_reg(es8388i2c_adapter_fd,ES8388_ADCCONTROL2, val), , "Error s_write_reg!");
//	CHECK(!s_write_reg(ES8388_ADCCONTROL1, 0x33), , "Error s_write_reg!");
}

//61.选择耳机麦克风语音输入，输入通道2
void drvSelectEarphMic(void)
{
	drvSelectHandMic();
}



//62.打印编译时间
void drvShowVersion(void)
{
	printf("lib_drv_so version printf,Buildtime %s\n",g_build_time_str);;
}




//63.MicCtrl引脚状态控制改变
//status 非0表示MicCtrl输出高，0表示MicCtrl输出低
//2022-12-13 modify by dazhi
void drvSetMicCtrlStatus(int status)
{
	if(status)   //根据情况调整
		status = GPIO_LEVEL_HIGH;
	else 
		status = GPIO_LEVEL_LOW;


	if(MicCtrl_pin < 0 || gpio_level_set(MicCtrl_pin, status)==false)
		return ;
}


//64.键灯led闪烁接口 (nKeyIndex：1-40)
//闪烁类型（0：500ms,1:800ms,2:1s:3:2s）
void drvFlashLEDs(int nKeyIndex,unsigned char flash_type)
{
	int param = 1;
	if(assert_init())  //未初始化
		return;

	if(nKeyIndex > 40 || nKeyIndex < 1)
	{
		printf("error : drvFlashLEDs ,nKeyIndex = %d,out of range\n",nKeyIndex);
		return;
	}	

	if(flash_type > 3)
	{
		flash_type = 0;
	}

	nKeyIndex = (flash_type << 6) | (nKeyIndex & 0x3f);

	if(api_send_and_waitack(eAPI_LEDS_FLASH_CMD,nKeyIndex,&param))  //发送的第二个参数表示led号，第三个表示点亮还是熄灭
	{
		printf("error : drvFlashLEDs ,nKeyIndex = %d\n",nKeyIndex);
	}

}




//65. 设置LSPK : OnOff，Val：非0 --》on ，0--> off
//2022-12-13 added by dazhi
void drvSetLSPKOnOff(int Val)
{
	int param = 1;
	if(assert_init())  //未初始化
		return;

	if(api_send_and_waitack(eAPI_LSPK_SETONOFF_CMD,Val,&param))  //发送的第二个参数表示亮度，第三个无意义
	{
		printf("error : drvSetLSPKOnOff ,Val = %d\n",Val);
	}
}

//66. 设置V12Crl : OnOff，Val：非0 --》on ，0--> off
//2022-12-13 added by dazhi
void drvSetV12CrlOnOff(int Val)
{
	int param = 1;
	if(assert_init())  //未初始化
		return;

	if(api_send_and_waitack(eAPI_V12_CTL_SETONOFF_CMD,Val,&param))  //发送的第二个参数表示亮度，第三个无意义
	{
		printf("error : drvSetV12CrlOnOff ,Val = %d\n",Val);
	}
}


//67.设置pcm音量为某个值，val范围0-192.值越大，声音越小
void drvSetTuneVal(int val)
{
	if(val>192)
		val = 192;
	else if(val < 0)
		val = 0;
	CHECK(!s_write_reg(es8388i2c_adapter_fd,ES8388_DACCONTROL4, val), , "Error s_write_reg!");
	CHECK(!s_write_reg(es8388i2c_adapter_fd,ES8388_DACCONTROL5, val), , "Error s_write_reg!");
}



//69. 显示单片机的版本【0-255】
int drvShowMcuVersion(void)//显示单片机的版本
{
	int param = 0;   //参数需要一个指针，返回函数返回值
	if(assert_init())  //未初始化
		return -1;

	if(api_send_and_waitack(eAPI_GET_MCUVERSION_CMD,0,&param))  //发送的第二个参数表示led号，第三个表示点亮还是熄灭
	{
		printf("error : drvGetBoardTemp \n");
		return -1;
	}
	printf("drvShowMcuVersion param = %d\n",param);
	return param;   //返回的温度只有整数部分！！	

}




