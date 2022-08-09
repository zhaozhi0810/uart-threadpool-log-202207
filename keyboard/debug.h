/*
 * debug.h
 *
 *  Created on: Nov 19, 2021
 *      Author: zlf
 */

#ifndef LIB_COMMON_DEBUG_H_
#define LIB_COMMON_DEBUG_H_

#include <stdio.h>

#define DBG(fmt, args...)	do {	\
	printf("[%s: %d] ", __func__, __LINE__);	\
	printf(fmt, ##args);	\
	printf("\n");	\
	} while(0)
#define ERR(fmt, args...)	DBG(fmt, ##args)
#define INFO(fmt, args...)	DBG(fmt, ##args)
#define CHECK(condition, ret, fmt, args...)	do {	\
	if(!(condition)) {	\
		ERR(fmt, ##args);	\
		return ret;	\
	}	\
} while(0)

#endif /* LIB_COMMON_DEBUG_H_ */
