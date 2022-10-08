

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
	printf("enter main \n");

	drvCoreBoardInit();


	printf("main exit()\n");

	return 0;
}



