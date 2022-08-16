/**
 *----------------------------------------------------------------------------
 *  MODULES     queue 
 *----------------------------------------------------------------------------
 *  @file   queue.h
 *
 *  $Date$	2018-05-30
 *  $Author$	zhangguibin
 *  $versions$	v1.0
 *----------------------------------------------------------------------------*/
/*
 * queue.h
 *
 */
#ifndef _QUEUE_H
#define	_QUEUE_H

#define MAX_SIZE 128

typedef unsigned char u8;
typedef unsigned long u32;	

typedef struct Queue{  
	u8 data[MAX_SIZE];  /*�������ݵ�Ԫ*/
	u32 front;/* ��ͷԪ�ص�ǰһ��Ԫ��   */
	u32 rear;/* ��βԪ�� */
	u32 size;/*���д�С */
	u32 length; /*��¼���еĴ�С */ 
}QUEUE,* QQUEUE;

extern void create_queue(QQUEUE qQueue);

extern int isFull(QQUEUE qQueue);

extern int isEmpty(QQUEUE qQueue);

extern int add_queue(QQUEUE qQueue,u8 val);

extern int out_queue(QQUEUE qQueue,u8 *value);

extern int clear_queue(QQUEUE qQueue);

extern u32 queue_length(QQUEUE qQueue);

extern u32 queue_preget(QQUEUE qQueue, u8 *value, u32 index);


#endif /* !_QUEUE_H */
