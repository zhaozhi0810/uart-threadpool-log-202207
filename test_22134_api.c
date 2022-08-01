/*
* @Author: dazhi
* @Date:   2022-07-27 10:45:55
* @Last Modified by:   dazhi
* @Last Modified time: 2022-07-29 11:30:11
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

#include "my_ipc_msgq.h"
#include "drv_22134_api.h"


static const char* g_build_time_str = "Buildtime :"__DATE__" "__TIME__;   //获得编译时间


void print_help(void)
{
	printf("0. 退出测试\n");
	printf("a. 读取温度一次\n");
	printf("b. 设置屏幕亮度\n");
	printf("c. 设置屏幕打开\n");
	printf("d. 设置屏幕关闭\n");
	printf("e. 设置某一个led点亮\n");
	printf("f. 设置某一个led熄灭\n");
	printf("g. 设置全部led点亮\n");
	printf("h. 设置全部led熄灭\n");
	printf("i. 获得led状态\n");
	printf("j. 获得键盘类型\n");
}



static void set_lcd_bright(void)
{
	int brt;   //输入亮度值
	char buf[8];

	while(1)
	{
		printf("请输入您的亮度值[0-255](字母则退出)：");
		fgets(buf,8,stdin);
		if(buf[0]>= '0' && buf[0] <= '9')
		{
			brt = atoi(buf);
			if(brt > 255)
				brt = 255;
			if(brt < 0)
				brt = 0;
			printf("亮度值设置为 %d\n",brt);
			drvSetLcdBrt(brt);	
		}		
		else  //输入的非数字，即退出
			break;
	}
}



static void set_led_status(int status)
{
	int whichone;   //输入亮度值
	char buf[8];

	while(1)
	{
		printf("请输入需要控制%s的led编号[0-31](字母则退出)：",status?"点亮":"熄灭");
		fgets(buf,8,stdin);
		if(buf[0]>= '0' && buf[0] <= '9')
		{
			whichone = atoi(buf);
			if(whichone > 31)
				whichone = 31;
			if(whichone < 0)
				whichone = 0;
			printf("需要控制的led编号为 %d\n",whichone);
		//	drvSetLcdBrt(brt);	
			if(status)
				drvLightLED(whichone);
			else
				drvDimLED(whichone);
		}		
		else  //输入的非数字，即退出
			break;
	}
}


//获得led状态
static void get_led_status(void)
{
	int whichone;   //输入亮度值
	int status;
	char buf[8];

	while(1)
	{
		printf("请输入需要获得状态的led编号[0-31](字母则退出)：");
		fgets(buf,8,stdin);
		if(buf[0]>= '0' && buf[0] <= '9')
		{
			whichone = atoi(buf);
			if(whichone > 31)
				whichone = 31;
			if(whichone < 0)
				whichone = 0;

			status = drvGetLEDStatus(whichone);

			printf("编号为 %d 的led状态为 %d\n",whichone,status);			
		}		
		else  //输入的非数字，即退出
			break;
	}
}



int main(int argc,char*argv[])
{
	float temp;
	char cmd;
	int ret;

	printf("%s running,Buildtime %s\n",argv[0],g_build_time_str);


	if(drvCoreBoardInit())  //API接口进行初始化
	{
		printf("ERROR: drvCoreBoardInit \n");
		return -1;
	}

	print_help();

	//用于通信的消息队列
	if(0 != msgq_init())
	{
		printf("error : msgq_init\n");
		return -1;
	}


	while(1)
	{
		printf("请输入您的选择：");
		scanf("%c",&cmd);
		getchar();
		switch(cmd)
		{
			case '0': //2. 调节亮度
				exit(0);
				break;
			case 'a':
			//1 读取温度
				temp = drvGetCPUTemp();
				printf("drvGetCPUTemp = %0.2f\n",temp);
				break;
			
			case 'b': //2. 调节亮度
				set_lcd_bright();
				break;
			case 'c': //3. 设置屏幕打开
				printf("3. 设置屏幕打开\n");
				drvEnableLcdScreen();
				break;
			case 'd': //4. 设置屏幕关闭
				printf("3. 设置屏幕关闭\n");
				drvDisableLcdScreen();
				break;
			case 'e': //5. 设置某一个led点亮
				set_led_status(1);
				break;
			case 'f': //6. 设置某一个led熄灭
				set_led_status(0);
				break;	
			case 'g': //7. 设置全部led点亮
				drvLightAllLED();
				break;
			case 'h': //8. 设置全部led熄灭
				drvDimAllLED();
				break;
			case 'i': //9. 获得led状态
				get_led_status();
				break;	
			case 'j':	//10.获取键盘类型
				ret = getKeyboardType();	
				if(ret > 0)
					printf("KeyboardType = %d\n",ret);
				else
					printf("ERROR: getKeyboardType ret = %d\n",ret);
				break;

			default:
				print_help();
		}

	}

	return 0;
}


