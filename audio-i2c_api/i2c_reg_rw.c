/*
 * i2c_reg_rw.c
 *
 *  Created on: Nov 30, 2021
 *      Author: zlf
 */

#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>



#include "debug.h"

static int inited = -1;   //初始化了么 1表示初始化了 
//去掉全局变量
//static int i2c_adapter_fd = -1;  //文件描述符

//由外部的函数修改！！！！2022-12-19增加
int i2c_device_es8388_addr = -1;   //初始化值为-1； 等待修改为大于0的值才能生效

//2022-12-19 ,改为返回文件描述符
int i2c_adapter_init(char *i2c_adapter_file, int i2c_device_addr) {
	int i2c_adapter_fd = -1;  //文件描述符
	if(inited == 1)
		return 0;

	if(i2c_device_addr<0)  //iic的设备地址不能小于0，2022-12-19增加
		return -1;

	CHECK(i2c_adapter_file, -1, "Error i2c_adapter_file is null!");
	CHECK(i2c_device_addr >= 0 && i2c_device_addr <= (0xff>>1), -1, "Error i2c_device_addr out of range!");
	
	CHECK((i2c_adapter_fd = open(i2c_adapter_file, O_RDWR)) > 0, -1, "Error open with %d: %s", errno, strerror(errno));

	if(ioctl(i2c_adapter_fd, I2C_SLAVE_FORCE, i2c_device_addr)) {
		ERR("Error ioctl with %d: %s", errno, strerror(errno));
		close(i2c_adapter_fd);
		i2c_adapter_fd = -1;
		return -1;
	}

	inited = 1; //已经初始化了

	return i2c_adapter_fd;
}

int i2c_adapter_exit(int i2c_adapter_fd) {
	// CHECK(i2c_adapter_fd > 0, -1, "Error i2c_adapter_fd is %d", i2c_adapter_fd);
	// CHECK(i2c_adapter_fd > 0, -1, "\n");
	if(i2c_adapter_fd <= 0)
	 	return -1;
	CHECK(!close(i2c_adapter_fd), -1, "Error close with %d: %s", errno, strerror(errno));
	inited = -1;
	i2c_adapter_fd = -1;
	return 0;
}

static int i2c_device_reg_read(int i2c_adapter_fd, unsigned char addr, unsigned char *value) {
	CHECK(inited == 1, -1, "Error i2c_adapter init failed!!!");
	CHECK(i2c_adapter_fd > 0, -1, "Error i2c_adapter_fd is %d", i2c_adapter_fd);
	CHECK(value, -1, "Error value is null!");
	struct i2c_smbus_ioctl_data i2c_smbus_ioctl_data;
	union i2c_smbus_data i2c_smbus_data;
	bzero(&i2c_smbus_data, sizeof(union i2c_smbus_data));
	bzero(&i2c_smbus_ioctl_data, sizeof(struct i2c_smbus_ioctl_data));
	i2c_smbus_ioctl_data.read_write = I2C_SMBUS_READ;
	i2c_smbus_ioctl_data.command = addr;
	i2c_smbus_ioctl_data.size = I2C_SMBUS_BYTE_DATA;
	i2c_smbus_ioctl_data.data = &i2c_smbus_data;
	CHECK(!ioctl(i2c_adapter_fd, I2C_SMBUS, &i2c_smbus_ioctl_data), -1, "Error ioctl with %d: %s", errno, strerror(errno));
	*value = i2c_smbus_data.byte;
	return 0;
}

static int i2c_device_reg_write(int i2c_adapter_fd, unsigned char addr, unsigned char value) {
	CHECK(inited == 1, -1, "Error i2c_adapter init failed!!!");
	CHECK(i2c_adapter_fd > 0, -1, "Error i2c_adapter_fd is %d", i2c_adapter_fd);
	struct i2c_smbus_ioctl_data i2c_smbus_ioctl_data;
	union i2c_smbus_data i2c_smbus_data;
	unsigned char value_temp = 0;
	bzero(&i2c_smbus_data, sizeof(union i2c_smbus_data));
	bzero(&i2c_smbus_ioctl_data, sizeof(struct i2c_smbus_ioctl_data));
	i2c_smbus_data.byte = value;
	i2c_smbus_ioctl_data.read_write = I2C_SMBUS_WRITE;
	i2c_smbus_ioctl_data.command = addr;
	i2c_smbus_ioctl_data.size = I2C_SMBUS_BYTE_DATA;
	i2c_smbus_ioctl_data.data = &i2c_smbus_data;
	CHECK(!ioctl(i2c_adapter_fd, I2C_SMBUS, &i2c_smbus_ioctl_data), -1, "Error ioctl with %d: %s", errno, strerror(errno));
	CHECK(!i2c_device_reg_read(i2c_adapter_fd,addr, &value_temp), -1, "Error i2c_device_reg_read!");
	CHECK(value == value_temp, -1, "Error write byte %#x is unequal read byte %#x", value, value_temp);
	return 0;
}



int s_write_reg(int i2c_adapter_fd, unsigned char addr, unsigned char val) {
	if(i2c_device_reg_write(i2c_adapter_fd,addr, val)) {
		ERR("Error i2c_device_reg_write!");
	//	i2c_adapter_exit();
		return -1;
	}
	//i2c_adapter_exit(i2c_adapter_fd);
	return 0;
}

int s_read_reg(int i2c_adapter_fd, unsigned char addr, unsigned char *val) {
	CHECK(val, 0, "Error val is null!");
	if(i2c_device_reg_read(i2c_adapter_fd,addr, val)) {
		ERR("Error i2c_device_reg_write!");
	//	i2c_adapter_exit();
		return -1;
	}
	//i2c_adapter_exit(i2c_adapter_fd);
	return 0;
}



//找到系统中8388的地址，
//第一种呢就是0x10和0x11都试一下，看看哪个是正常的
//第二种就是去找sys目录下的设备，这里选了第二种
int es8388_find_iic_devaddr(void)
{
	int addr;
	DIR *dir = NULL;
	struct dirent *file;


	if(access("/sys/bus/i2c/drivers/ES8388",F_OK))
	{
		printf("ES8388 not exist !!\n");
		return -1;
	}

	
	if((dir = opendir("/sys/bus/i2c/drivers/ES8388")) == NULL) {  
		printf("opendir failed!");
		return -1;
	}
	while((file = readdir(dir))) {
		
		if(file->d_name[0] == '.')
			continue;
		//总线号也是可以找到的，这里就没有去识别总线号了
		else if(strncmp(file->d_name,"4-00",4) == 0)
		{
			printf("name = %s\n",file->d_name);
			addr = strtol(file->d_name+4, NULL, 16);
			printf("addr = %#x\n",addr);
			closedir(dir);
			return addr;   //返回设备地址			
		}	
	
	}
	closedir(dir);
	return -1;
}


