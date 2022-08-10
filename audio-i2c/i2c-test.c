/*
* @Author: dazhi
* @Date:   2022-08-01 15:28:19
* @Last Modified by:   dazhi
* @Last Modified time: 2022-08-01 17:24:17
*/
#include <stdlib.h>
#include "i2c_reg_rw.h"
#include "debug.h"
#include "codec.h"

#define I2C_ADAPTER_DEVICE	"/dev/i2c-4"
#define I2C_DEVICE_ADDR		(0x11)

static int s_write_reg(unsigned char addr, unsigned char val) {
	int i2c_adapter_fd = 0;
	CHECK((i2c_adapter_fd = i2c_adapter_init(I2C_ADAPTER_DEVICE, I2C_DEVICE_ADDR)) > 0, -1, "Error i2c_adapter_init!");
	if(i2c_device_reg_write(i2c_adapter_fd, addr, val)) {
		ERR("Error i2c_device_reg_write!");
		i2c_adapter_exit(i2c_adapter_fd);
		return -1;
	}
	i2c_adapter_exit(i2c_adapter_fd);
	return 0;
}

static int s_read_reg(unsigned char addr, unsigned char *val) {
	CHECK(val, 0, "Error val is null!");
	int i2c_adapter_fd = 0;
	CHECK((i2c_adapter_fd = i2c_adapter_init(I2C_ADAPTER_DEVICE, I2C_DEVICE_ADDR)) > 0, -1, "Error i2c_adapter_init!");
	if(i2c_device_reg_read(i2c_adapter_fd, addr, val)) {
		ERR("Error i2c_device_reg_write!");
		i2c_adapter_exit(i2c_adapter_fd);
		return -1;
	}
	i2c_adapter_exit(i2c_adapter_fd);
	return 0;
}



int main(void)
{
	unsigned char val = 0;
	CHECK(!s_read_reg(ES8388_ADCCONTROL2, &val),-1 , "Error s_read_reg!");
	printf("val = %d\n",val);
	return 0;
}





