

#ifndef _GPIO_BASE_H_
#define _GPIO_BASE_H_

#include <stdbool.h>

#define RK3399_PIN    //使用rk3399的gpio 引脚

#ifdef RK3399_PIN
#define RK_GPIO0	0
#define RK_GPIO1	1
#define RK_GPIO2	2
#define RK_GPIO3	3
#define RK_GPIO4	4
#define RK_GPIO6	6

#define RK_PA0		0
#define RK_PA1		1
#define RK_PA2		2
#define RK_PA3		3
#define RK_PA4		4
#define RK_PA5		5
#define RK_PA6		6
#define RK_PA7		7
#define RK_PB0		8
#define RK_PB1		9
#define RK_PB2		10
#define RK_PB3		11
#define RK_PB4		12
#define RK_PB5		13
#define RK_PB6		14
#define RK_PB7		15
#define RK_PC0		16
#define RK_PC1		17
#define RK_PC2		18
#define RK_PC3		19
#define RK_PC4		20
#define RK_PC5		21
#define RK_PC6		22
#define RK_PC7		23
#define RK_PD0		24
#define RK_PD1		25
#define RK_PD2		26
#define RK_PD3		27
#define RK_PD4		28
#define RK_PD5		29
#define RK_PD6		30
#define RK_PD7		31
#endif





typedef enum 
{
	GPIO_DIR_IN = 0, 
	GPIO_DIR_OUT = 1
} GPIO_DIR;


typedef enum 
{
	GPIO_LEVEL_LOW = 0x01, 
	GPIO_LEVEL_HIGH = 0x02
} GPIO_LEVEL;


/*
*** 设置GPIO输入/输出
*/
bool gpio_direction_set(const int pin, GPIO_DIR dir);
//还原gpio
bool gpio_direction_unset(const int pin);


/*
*** 设置IO口电平 
*** 在设置IO电平之前,需要调用 gpio_direction_set 
*/
bool gpio_level_set(const int pin, GPIO_LEVEL level);



/*
*** 读取IO电平
*** 在读取IO电平之前,需要调用 gpio_direction_set 
*/
bool gpio_level_read(const int pin, GPIO_LEVEL * level);




/*
*** 设置gpio 内置上拉使能
*/
bool gpio_pull_enable(const int pin, bool enable);


#ifdef RK3399_PIN
/*
	由goio2_B1 转为引脚号
	param1 ： port   RK_GPIOx x:0~6
	param2 ： whichpad  RK_PA0~RK_PA7 RK_PB0~RK_PB7 RK_PC0~RK_PC7  RK_PD0~RK_PD7

	返回值 -1 表示出错，其他表示正确
 */

int get_pin(unsigned int port,unsigned int whichpad);
#endif


#endif


