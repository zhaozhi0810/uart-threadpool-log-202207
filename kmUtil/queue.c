/**
 *                
 * 
 *----------------------------------------------------------------------------
 *  MODULES     queue 
 *----------------------------------------------------------------------------
 *  @file   queue.c
 *
 *  $Date$	2018-05-30
 *  $Author$	zhangguibin
 *  $versions$	v1.0
 *----------------------------------------------------------------------------*/
/*
 * queue.c
 *
 */
#include "queue.h"
#include <stdio.h>

/******************************************************************************
*   FUNCTION        :   create_queue 创建队列初始化
******************************************************************************/
/**
 *  @parameter	qQueue 队列名称
 *  @return     无
 *
 *  @dependency 无
 *****************************************************************************/
void create_queue(QQUEUE qQueue){  
    qQueue->front = qQueue->rear = 0;
	qQueue->length=0;
	qQueue->size = MAX_SIZE;

	return;
}

/******************************************************************************
*   FUNCTION        :   isFull	判断队列是否满
******************************************************************************/
/**
 *  @parameter	qQueue 队列名称
 *  @return	1：队列满
 *			0：队列没满
 *
 *  @dependency 无
 *****************************************************************************/
int isFull(QQUEUE qQueue){
#if 0
    if((qQueue->rear+1)%qQueue->size == qQueue->front){ 
        return 1;  
    } else{  
        return 0;  
    }
#else
    if((qQueue->front+1)%qQueue->size == qQueue->rear){
            printf("front=%d rear=%d\n", (int)qQueue->front, (int)qQueue->rear);
            return 1;
        } else{
            return 0;
        }
#endif
}

/******************************************************************************
*   FUNCTION        :   isEmpty	判断队列是为空
******************************************************************************/
/**
 *  @parameter	qQueue 队列名称
 *  @return	1：队列空
 *			0：队列中有数据
 *
 *  @dependency 无
 *****************************************************************************/
int isEmpty(QQUEUE qQueue){  
    if(qQueue->front == qQueue->rear){  
        return 1;  
    }else{  
        return 0;  
    }  
}


/******************************************************************************
*   FUNCTION        :   add_queue	将数据加入队列
******************************************************************************/
/**
 *  @parameter	qQueue 队列名称
 *			val	入队数据
 *  @return	1：入队成功
 *			0：入队失败
 *
 *  @dependency 无
 *****************************************************************************/
int add_queue(QQUEUE qQueue, unsigned char val){

	if(isFull(qQueue))
	{
		return 0;
	}
	qQueue->data[qQueue->front]=val;
	
	qQueue->front = (qQueue->front+1) % qQueue->size;
	
	qQueue->length ++;
	
	return 1;
}

/******************************************************************************
*   FUNCTION        :   out_queue	数据出队
******************************************************************************/
/**
 *  @parameter	qQueue 队列名称
 *			value  出队数据
 *  @return	1：出队成功
 *			0：出队失败
 *
 *  @dependency 无
 *****************************************************************************/
int out_queue(QQUEUE qQueue,unsigned char *value)
{  
	if(isEmpty(qQueue))
	{
		*value =0;
		return 0;
	}

	*value =  qQueue->data[qQueue->rear]; 
	
	qQueue->rear= (qQueue->rear +1) % qQueue->size;
	
	qQueue->length --;
	

	return 1;
}

/******************************************************************************
*   FUNCTION        :   clear_queue	队列清空
******************************************************************************/
/**
 *  @parameter	qQueue 队列名称
 *			
 *  @return	1：清除成功
 *			
 *
 *  @dependency 无
 *****************************************************************************/
int clear_queue(QQUEUE qQueue)
{
	qQueue->front=qQueue->rear=0; 
	qQueue->length=0;
	
	return 1;
}

/******************************************************************************
*   FUNCTION        :   queue_length	获取队列数据长度
******************************************************************************/
/**
 *  @parameter	qQueue 队列名称
 *			
 *  @return	length：数据长度
 *			
 *
 *  @dependency 无
 *****************************************************************************/
u32 queue_length(QQUEUE qQueue)
{
	return qQueue->length;
}


/******************************************************************************
*   FUNCTION        :   queue_preget	预取队列中的首元素
******************************************************************************/
/**
 *  @parameter	qQueue 队列名称
 *			value  预取数据
 *          index  第几个
 *  @return	1：预取成功
 *			0：预取失败
 *
 *  @dependency 无
 *****************************************************************************/
u32 queue_preget(QQUEUE qQueue, unsigned char *value, u32 index)
{
    int lindex = (qQueue->rear +index) % qQueue->size;
	
	if(isEmpty(qQueue) || (index >= qQueue->length))
	{
		printf("queue_preget illegal qQueue->length=%d isEmpty=%d index=%d\n", (int)qQueue->length, (int)isEmpty(qQueue), (int)index);
		*value =0;
		return 0;
	}

	*value =  qQueue->data[lindex];

	return 1;
}


