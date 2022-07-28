
/*
	author : dazhi
	date : 2022-07-27
	version : 1.0.0
 */

#ifndef __DRV_22134_H__
#define __DRV_22134_H__	




//核心板初始化函数

//-1：初始化失败
//0：初始化成功
int drvCoreBoardInit(void);
//使能看门狗函数
//返回值
//0：设置成功
//1：设置失败
int drvWatchDogEnable(void);
//去使能看门狗函数
//返回值
//0：设置成功
//1：设置失败
int drvWatchDogDisable(void);
//喂狗函数
void drvWatchDogFeeding(void);
//设置看门狗超时时间
int drvWatchdogSetTimeout(int timeout);
//获取看门狗超时时间
int drvWatchdogGetTimeout(void);
//禁止扬声器放音  //3399的AD7管脚置0  //接口板上J45的5脚
void drvDisableSpeaker(void);
//允许扬声器放音  //3399的AD7管脚置1
void drvEnableSpeaker(void);
//关闭强声器  //3399的AG4管脚置1  //接口板芯片U5的2脚
void drvDisableWarning(void);
//打开强声器  //3399的AG4管脚置0
void drvEnableWarning(void);
//使能USB0  //3399的管脚AL4置0   //暂时未引到接口板，无法测量
void drvEnableUSB0(void);
//使能USB1  //3399的管脚AK4置0   //暂时未引到接口板，无法测量
void drvEnableUSB1(void);
//去使能USB0  //3399的管脚AL4置1
void drvDisableUSB0(void);
//去使能USB1  //3399的管脚AK4置1
void drvDisableUSB1(void);
//打开屏幕
void drvEnableLcdScreen(void);
//关闭屏幕
void drvDisableLcdScreen(void);
//使能touch module
void drvEnableTouchModule(void);
//去使能touch module
void drvDisableTouchModule(void);
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
int drvGetLCDType(void);
//获取键盘类型
/*
返回值

1：防爆终端键盘类型
2：壁挂Ⅲ型终端键盘类型（不关心！！）
4：嵌入式Ⅰ/Ⅱ/Ⅲ型、防风雨型、壁挂Ⅱ型终端键盘类型
6：多功能型终端键盘类型
 */
int getKeyboardType(void);
//获取CPU温度
float drvGetCPUTemp(void);
//获取核心板温度
float drvGetBoardTemp(void);
//获取当前RTC值
long drvGetRTC(void);
//设置RTC值
long drvSetRTC(long secs);
//设置LED灯亮度  参数范围为[0,0x64]
void drvSetLedBrt(int nBrtVal);
//设置屏幕亮度  参数范围为[0,0xff]
void drvSetLcdBrt(int nBrtVal);
//点亮对应的键灯
void drvLightLED(int nKeyIndex);
//熄灭对应的键灯
void drvDimLED(int nKeyIndex);
//点亮所有的键灯
void drvLightAllLED(void);
//熄灭所有的键灯
void drvDimAllLED(void);
//获取对应键灯状态
int drvGetLEDStatus(int nKeyIndex);
//获取耳机插入状态
////返回值
//-1：失败
//0：未插入
//1：插入
int drvGetMicStatus(void);
//获取手柄插入状态
//返回值
//-1：失败
//0：未插入
//1：插入
int drvGetHMicStatus(void);

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
void drvSetGpioCbk(GPIO_NOTIFY_FUNC cbk);

typedef void (GPIO_NOTIFY_KEY_FUNC)(int gpio, int val);
//int gpio：键值
//int val：状态，1为按下，0为松开
//面板按键事件上报回调
void drvSetGpioKeyCbk(GPIO_NOTIFY_KEY_FUNC cbk);
//获取电压值
float drvGetVoltage(void);
//获取电流值
float drvGetCurrent(void);
//Lcd屏重置
int drvLcdReset(void);
//键盘重置
int drvIfBrdReset(void);
//核心板重置
int drvCoreBrdReset(void);
//打开音量调节
void drvEnableTune(void);
//关闭音量调节
void drvDisableTune(void);
//分级调节音量大小
void drvAdjustTune(void);
//正向调节音量
void drvSetTuneUp(void);
//反向调节音量
void drvSetTuneDown(void);
//增加面板扬声器音量
void drvAddSpeakVolume(int value);
//减少面板扬声器音量
void drvSubSpeakVolume(int value);
//设置扬声器音量值 参数范围为[0,100]
void drvSetSpeakVolume(int value);
//增加手柄音量 参数范围为[0,100]
void drvAddHandVolume(int value);
//减少手柄音量
void drvSubHandVolume(int value);
//设置手柄音量值
void drvSetHandVolume(int value);
//增加耳机音量
void drvAddEarphVolume(int value);
//减少耳机音量
void drvSubEarphVolume(int value);
//设置耳机音量值  参数范围为[0,100]
void drvSetEarphVolume(int value);
//打开手柄音频输出
void drvEnableHandout(void);
//关闭手柄音频输出
void drvDisableHandout(void);
//打开耳机音频输出
void drvEnableEarphout(void);
//打开耳机音频输出
void drvDisableEarphout(void);
//选择面板麦克风语音输入
void drvSelectHandFreeMic(void);
//选择手柄麦克风语音输入
void drvSelectHandMic(void);
//选择耳机麦克风语音输入
void drvSelectEarphMic(void);

#endif

