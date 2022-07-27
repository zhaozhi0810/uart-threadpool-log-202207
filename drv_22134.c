
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

#include "drv_22134.h"
#include "my_ipc_msgq.h"  //用于与串口程序通信！！
/*
	与串口通信部分，使用msgq的方式，发送id为678，接收id为234

 */




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

	return 0;
}

//使能看门狗函数
//返回值
//0：设置成功
//1：设置失败
int drvWatchDogEnable(void)
{
	
	return 0;
}

//去使能看门狗函数
//返回值
//0：设置成功
//1：设置失败
int drvWatchDogDisable(void)
{
	
	return 0;
}

//喂狗函数
void drvWatchDogFeeding(void)
{

}

//设置看门狗超时时间
int drvWatchdogSetTimeout(int timeout)
{
	
	return 0;
}

//获取看门狗超时时间
int drvWatchdogGetTimeout(void)
{
	
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

}

//使能USB1  //3399的管脚AK4置0   //暂时未引到接口板，无法测量
void drvEnableUSB1(void)
{

}

//去使能USB0  //3399的管脚AL4置1
void drvDisableUSB0(void)
{

}

//去使能USB1  //3399的管脚AK4置1
void drvDisableUSB1(void)
{

}

//打开屏幕
void drvEnableLcdScreen(void)
{

}

//关闭屏幕
void drvDisableLcdScreen(void)
{

}

//使能touch module
void drvEnableTouchModule(void)
{

}

//去使能touch module
void drvDisableTouchModule(void)
{

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
	
	return 0;
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
	
	return 0.0;
}

//获取当前RTC值
long drvGetRTC(void)
{
	
	return 0;
}

//设置RTC值
long drvSetRTC(long secs)
{
	
	return 0;
}

//设置LED灯亮度  参数范围为[0,0x64]
void drvSetLedBrt(int nBrtVal)
{

}

//设置屏幕亮度  参数范围为[0,0xff]
void drvSetLcdBrt(int nBrtVal)
{

}

//点亮对应的键灯
void drvLightLED(int nKeyIndex)
{

}

//熄灭对应的键灯
void drvDimLED(int nKeyIndex)
{

}

//点亮所有的键灯
void drvLightAllLED(void)
{

}

//熄灭所有的键灯
void drvDimAllLED(void)
{

}

//获取对应键灯状态
int drvGetLEDStatus(int nKeyIndex)
{
	
	return 0;
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
	
	return 0.0;
}

//获取电流值
float drvGetCurrent(void)
{

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












