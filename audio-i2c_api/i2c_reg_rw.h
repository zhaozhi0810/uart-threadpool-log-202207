/*
 * i2c_reg_rw.h
 *
 *  Created on: Nov 30, 2021
 *      Author: zlf
 */

#ifndef LIB_COMMON_I2C_REG_RW_H_
#define LIB_COMMON_I2C_REG_RW_H_

#define I2C_ADAPTER_ES8388	"/dev/i2c-4"
extern int i2c_device_es8388_addr;     //这里作为一个全局变量，由外部函数修改！！

//返回文件描述符
extern int i2c_adapter_init(char *i2c_adapter_file, int i2c_device_addr);
extern int i2c_adapter_exit(int i2c_adapter_fd);

//extern int i2c_device_reg_read(unsigned char addr, unsigned char *value);
//extern int i2c_device_reg_write( unsigned char addr, unsigned char value);


extern int s_read_reg(int i2c_adapter_fd, unsigned char addr, unsigned char *val);
extern int s_write_reg(int i2c_adapter_fd, unsigned char addr, unsigned char val);


//找到系统中8388的地址，
//第一种呢就是0x10和0x11都试一下，看看哪个是正常的
//第二种就是去找sys目录下的设备，这里选了第二种
extern int es8388_find_iic_devaddr(void);

#endif /* LIB_COMMON_I2C_REG_RW_H_ */
