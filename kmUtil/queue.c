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
*   FUNCTION        :   create_queue �������г�ʼ��
******************************************************************************/
/**
 *  @parameter	qQueue ��������
 *  @return     ��
 *
 *  @dependency ��
 *****************************************************************************/
void create_queue(QQUEUE qQueue){  
    qQueue->front = qQueue->rear = 0;
	qQueue->length=0;
	qQueue->size = MAX_SIZE;

	return;
}

/******************************************************************************
*   FUNCTION        :   isFull	�ж϶����Ƿ���
******************************************************************************/
/**
 *  @parameter	qQueue ��������
 *  @return	1��������
 *			0������û��
 *
 *  @dependency ��
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
*   FUNCTION        :   isEmpty	�ж϶�����Ϊ��
******************************************************************************/
/**
 *  @parameter	qQueue ��������
 *  @return	1�����п�
 *			0��������������
 *
 *  @dependency ��
 *****************************************************************************/
int isEmpty(QQUEUE qQueue){  
    if(qQueue->front == qQueue->rear){  
        return 1;  
    }else{  
        return 0;  
    }  
}


/******************************************************************************
*   FUNCTION        :   add_queue	�����ݼ������
******************************************************************************/
/**
 *  @parameter	qQueue ��������
 *			val	�������
 *  @return	1����ӳɹ�
 *			0�����ʧ��
 *
 *  @dependency ��
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
*   FUNCTION        :   out_queue	���ݳ���
******************************************************************************/
/**
 *  @parameter	qQueue ��������
 *			value  ��������
 *  @return	1�����ӳɹ�
 *			0������ʧ��
 *
 *  @dependency ��
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
*   FUNCTION        :   clear_queue	�������
******************************************************************************/
/**
 *  @parameter	qQueue ��������
 *			
 *  @return	1������ɹ�
 *			
 *
 *  @dependency ��
 *****************************************************************************/
int clear_queue(QQUEUE qQueue)
{
	qQueue->front=qQueue->rear=0; 
	qQueue->length=0;
	
	return 1;
}

/******************************************************************************
*   FUNCTION        :   queue_length	��ȡ�������ݳ���
******************************************************************************/
/**
 *  @parameter	qQueue ��������
 *			
 *  @return	length�����ݳ���
 *			
 *
 *  @dependency ��
 *****************************************************************************/
u32 queue_length(QQUEUE qQueue)
{
	return qQueue->length;
}


/******************************************************************************
*   FUNCTION        :   queue_preget	Ԥȡ�����е���Ԫ��
******************************************************************************/
/**
 *  @parameter	qQueue ��������
 *			value  Ԥȡ����
 *          index  �ڼ���
 *  @return	1��Ԥȡ�ɹ�
 *			0��Ԥȡʧ��
 *
 *  @dependency ��
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


