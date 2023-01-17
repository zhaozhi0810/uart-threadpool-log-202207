

#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include "drv_22134_api.h"
//#include "debug.h"


int main(int argc,char *argv[])
{
	printf("键灯测试开始 - 2023-01-10 \n");

	drvCoreBoardInit();

	printf("熄灭所有灯\n");
	drvDimAllLED();
	sleep(1);
	printf("亮度20\n");
	drvSetLedBrt(20);
	printf("点亮所有灯\n");
	drvLightAllLED();
	sleep(2);
	printf("亮度40\n");
	drvSetLedBrt(40);

	sleep(2);
	printf("亮度60\n");
	drvSetLedBrt(60);
	sleep(2);
	printf("亮度80\n");
	drvSetLedBrt(80);
	sleep(2);
	printf("亮度100\n");
	drvSetLedBrt(100);
	printf("熄灭所有灯\n");
	drvDimAllLED();

	printf("main exit()\n");

	return 0;
}



