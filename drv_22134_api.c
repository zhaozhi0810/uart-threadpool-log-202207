
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
// #include <sys/types.h>
// #include <sys/stat.h>
// #include <fcntl.h>

#include "drv_22134_api.h"
#include "my_ipc_msgq.h"  //用于与串口程序通信！！
/*
	与串口通信部分，使用msgq的方式，发送id为678，接收id为234

 */
#define TYPE_API_SENDTO_SERVER 678   //发
#define TYPE_API_RECFROM_SERVER 234


#define  DEBUG_PRINTF    //用于输出一些出错信息
#define  MY_DEBUG        //用于输出一些调试信息，正常使用时不需要
static int print_debug = 0;


#ifdef DEBUG_PRINTF
  #define DBG_PRINTF(fmt, args...)  \
  do{\
    printf("<<File:%s  Line:%d  Function:%s>> ", __FILE__, __LINE__, __FUNCTION__);\
    printf(fmt, ##args);\
  }while(0)
#else
  #define DBG_PRINTF(fmt, args...)   
#endif




  
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



//val 非0表示打印一般的调试信息，0则不打印调试信息，默认不打印
void set_print_debug(int val)
{
	print_debug = val;
}



static int CoreBoardInit = 0;   //初始化了吗？初始化成功为1，失败为-1。



//api发送数据给服务器，并且等待服务器应答，超时时间1s
//第二个参数可以用于返回数据，无数据时可以为NULL
static int api_send_and_waitack(int cmd,int param1,int *param2)
{
	msgq_t msgbuf;  //用于应答
	int ret;	
	msgbuf.types = TYPE_API_SENDTO_SERVER;  //发送的信息类型
	msgbuf.cmd = cmd;      //结构体赋值
	msgbuf.param1 = param1;
	if(param2)
		msgbuf.param2 = *param2;
	else //空指针
		msgbuf.param2 = 0;
	msgbuf.ret = 0;   //一般没有使用
//	printf("DEBUG: cmd = %d\n",cmd);
	//3.做出应答
	ret = msgq_send(TYPE_API_RECFROM_SERVER+cmd,&msgbuf,20); //数据发出后，需要等待 20表示1s
	if(0!= ret)
	{		
		printf("error : msgq_send ,ret = %d\n",ret);
		return ret;
	} 
//	printf("DEBUG: msgq_send ok\n");
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


//核心板初始化函数
//-1：初始化失败
//0：初始化成功
int drvCoreBoardInit(void)
{
	int ret ;
	ret = msgq_init();

	if(ret)  //不为0，表示出错！！
	{
		DEBUG_PRINTF("ERROR: ret = %d\n",ret);
		return ret;
	}
	CoreBoardInit = 1;  //初始化成功
	return 0;
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



//使能看门狗函数
//返回值
//0：设置成功
//1：设置失败
int drvWatchDogEnable(void)
{


	//nothing todo 2022-07-27
	return 0;
}

//去使能看门狗函数
//返回值
//0：设置成功
//1：设置失败
int drvWatchDogDisable(void)
{
	//nothing todo 2022-07-27
	return 0;
}

//喂狗函数
void drvWatchDogFeeding(void)
{
	//nothing todo 2022-07-27
}

//设置看门狗超时时间
int drvWatchdogSetTimeout(int timeout)
{
	//nothing todo 2022-07-27
	return 0;
}

//获取看门狗超时时间
int drvWatchdogGetTimeout(void)
{
	//nothing todo 2022-07-27
	return 0;
}

//禁止扬声器放音  //3399的AD7管脚置0  //接口板上J45的5脚
void drvDisableSpeaker(void)
{

}

//允许扬声器放音  //3399的AD7管脚置1
void drvEnableSpeaker(void)
{

}

//关闭强声器  //3399的AG4管脚置1  //接口板芯片U5的2脚
void drvDisableWarning(void)
{

}

//打开强声器  //3399的AG4管脚置0
void drvEnableWarning(void)
{

}

//使能USB0  //3399的管脚AL4置0   //暂时未引到接口板，无法测量
void drvEnableUSB0(void)
{
	//nothing todo 2022-07-27
}

//使能USB1  //3399的管脚AK4置0   //暂时未引到接口板，无法测量
void drvEnableUSB1(void)
{
	//nothing todo 2022-07-27
}

//去使能USB0  //3399的管脚AL4置1
void drvDisableUSB0(void)
{
	//nothing todo 2022-07-27
}

//去使能USB1  //3399的管脚AK4置1
void drvDisableUSB1(void)
{
	//nothing todo 2022-07-27
}

//打开屏幕,由于单片机没法控制lcd的开启关闭，现在只能打开pwm
void drvEnableLcdScreen(void)
{
#if 1
	__setlcdbrt(200);
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

//关闭屏幕，由于单片机没法控制lcd的开启关闭，现在只能打开pwm
void drvDisableLcdScreen(void)
{
#if 1
	__setlcdbrt(0);
#else	
	int param = 0; //关闭屏幕
	if(assert_init())  //未初始化
		return;
	printf("debug : drvDisableLcdScreen \n");
	if(api_send_and_waitack(eAPI_LCDONOFF_CMD,0,&param))  //发送的第二个参数表示led号，第三个表示点亮还是熄灭
	{
		printf("error : drvDisableLcdScreen\n");
	}
	printf("debug : drvDisableLcdScreen ok\n");
#endif
}

//使能touch module
void drvEnableTouchModule(void)
{
	//nothing todo 2022-07-27
}

//去使能touch module
void drvDisableTouchModule(void)
{
	//nothing todo 2022-07-27
}

//获取屏幕类型
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
	return 3;   //5寸屏
//	return 0;
}

//获取键盘类型
/*
返回值

1：防爆终端键盘类型
2：壁挂Ⅲ型终端键盘类型（不关心！！）
4：嵌入式Ⅰ/Ⅱ/Ⅲ型、防风雨型、壁挂Ⅱ型终端键盘类型
6：多功能型终端键盘类型
 */
int getKeyboardType(void)
{
	
	return 0;
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

//获取CPU温度
float drvGetCPUTemp(void)
{
	int tmp;
	int ret; 

	ret = parse_cputemp(&tmp);
	if(!ret)  //返回0表示正确
	{
		return (float)tmp;   //函数只读到整数部分，返回时转为浮点数
	}

	DEBUG_PRINTF("ERROR: ret = %d\n",ret);
	return 0.0;
}

//获取核心板温度
float drvGetBoardTemp(void)
{
	//nothing todo 2022-07-27	
	return 0.0;
}

//获取当前RTC值
long drvGetRTC(void)
{
	//nothing todo 2022-07-27	
	return 0;
}

//设置RTC值
long drvSetRTC(long secs)
{
	//nothing todo 2022-07-27	
	return 0;
}

//设置LED灯亮度  参数范围为[0,0x64]
void drvSetLedBrt(int nBrtVal)
{
	if(!assert_init()) return;   //初始化失败，直接退出
}





//设置屏幕亮度  参数范围为[0,0xff]
void drvSetLcdBrt(int nBrtVal)
{
	if(nBrtVal < 0 || nBrtVal > 255)
	{
		return;
	}
	__setlcdbrt(nBrtVal);
}





//点亮对应的键灯，单片机命令
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

//熄灭对应的键灯
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

//点亮所有的键灯
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

//熄灭所有的键灯
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

//获取对应键灯状态
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

//获取耳机插入状态
////返回值
//-1：失败
//0：未插入
//1：插入
int drvGetMicStatus(void)
{
	
	return 0;
}

//获取手柄插入状态
//返回值
//-1：失败
//0：未插入
//1：插入
int drvGetHMicStatus(void)
{
	
	return 0;
}


typedef void (GPIO_NOTIFY_FUNC)(int gpio, int val);
//int gpio：键值
//int val：状态，1为按下或插入，0为松开或拔出
//Gpio值：
//面板ptt：25
//手柄ptt：30
//脚踩ptt：17
//耳机插入：48
//手柄插入：49
//PTT及耳机插入等事件上报回调
void drvSetGpioCbk(GPIO_NOTIFY_FUNC cbk)
{

}


typedef void (GPIO_NOTIFY_KEY_FUNC)(int gpio, int val);

//int gpio：键值
//int val：状态，1为按下，0为松开
//面板按键事件上报回调
void drvSetGpioKeyCbk(GPIO_NOTIFY_KEY_FUNC cbk)
{

}

//获取电压值
float drvGetVoltage(void)
{
	//nothing todo 2022-07-27	
	return 0.0;
}

//获取电流值
float drvGetCurrent(void)
{
	//nothing todo 2022-07-27
	return 0.0;
}

//Lcd屏重置
int drvLcdReset(void)
{

	return 0;
}

//键盘重置
int drvIfBrdReset(void)
{
	
	return 0;
}

//核心板重置
int drvCoreBrdReset(void)
{
	//nothing todo 2022-07-27	
	return 0;
}

//打开音量调节
void drvEnableTune(void)
{
	

}

//关闭音量调节
void drvDisableTune(void)
{

}

//分级调节音量大小
void drvAdjustTune(void)
{

}

//正向调节音量
void drvSetTuneUp(void)
{

}

//反向调节音量
void drvSetTuneDown(void)
{

}

//增加面板扬声器音量
void drvAddSpeakVolume(int value)
{

}

//减少面板扬声器音量
void drvSubSpeakVolume(int value)
{

}

//设置扬声器音量值 参数范围为[0,100]
void drvSetSpeakVolume(int value)
{

}

//增加手柄音量 参数范围为[0,100]
void drvAddHandVolume(int value)
{

}

//减少手柄音量
void drvSubHandVolume(int value)
{

}

//设置手柄音量值
void drvSetHandVolume(int value)
{

}

//增加耳机音量
void drvAddEarphVolume(int value)
{

}

//减少耳机音量
void drvSubEarphVolume(int value)
{

}

//设置耳机音量值  参数范围为[0,100]
void drvSetEarphVolume(int value)
{

}

//打开手柄音频输出
void drvEnableHandout(void)
{

}

//关闭手柄音频输出
void drvDisableHandout(void)
{

}

//打开耳机音频输出
void drvEnableEarphout(void)
{

}

//打开耳机音频输出
void drvDisableEarphout(void)
{

}

//选择面板麦克风语音输入
void drvSelectHandFreeMic(void)
{

}

//选择手柄麦克风语音输入
void drvSelectHandMic(void)
{

}

//选择耳机麦克风语音输入
void drvSelectEarphMic(void)
{

}












