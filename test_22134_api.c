/*
* @Author: dazhi
* @Date:   2022-07-27 10:45:55
* @Last Modified by:   dazhi
* @Last Modified time: 2022-07-28 10:00:47
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

}



static void set_lcd_bright(void)
{
	int brt;   //输入亮度值
	char buf[8];

	while(1)
	{
		printf("请输入您的亮度值[0-255]：");
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



int main(int argc,char*argv[])
{
	float temp;
	char cmd;


	printf("%s running,Buildtime %s\n",argv[0],g_build_time_str);


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
			case 'a':
			//1 读取温度
				temp = drvGetCPUTemp();
				printf("drvGetCPUTemp = %0.2f\n",temp);
				break;
			
			case 'b': //2. 调节亮度
				set_lcd_bright();
				break;
			default:
				print_help();
		}

	}

	return 0;
}


