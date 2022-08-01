/*
 * i2c_reg_rw.h
 *
 *  Created on: Nov 30, 2021
 *      Author: zlf
 */

#ifndef LIB_COMMON_I2C_REG_RW_H_
#define LIB_COMMON_I2C_REG_RW_H_

extern int i2c_adapter_init(char *i2c_adapter_file, int i2c_device_addr);
extern int i2c_adapter_exit(void);

//extern int i2c_device_reg_read(unsigned char addr, unsigned char *value);
//extern int i2c_device_reg_write( unsigned char addr, unsigned char value);


int s_read_reg(unsigned char addr, unsigned char *val);
int s_write_reg(unsigned char addr, unsigned char val);

#endif /* LIB_COMMON_I2C_REG_RW_H_ */
