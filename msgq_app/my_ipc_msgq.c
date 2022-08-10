/*
* @Author: dazhi
* @Date:   2022-07-27 09:57:14
* @Last Modified by:   dazhi
* @Last Modified time: 2022-08-10 10:38:28
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
//#include "drv_22134.h"
#include <utmp.h>
#include <time.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>

#include "my_ipc_msgq.h"  //用于与串口程序通信！！
//用户创建消息队列的关键字
#define KEY_PATH "/proc/cpuinfo"
#define KEY_ID -123321




static int msgid = -1;  //消息队列的描述符


static int MsgClear(void)
{
	msgq_t msgbuf;

	if(msgid == -1)
		return -EPERM;

	while( msgrcv(msgid, &msgbuf, sizeof(msgbuf)-sizeof(long),0,IPC_NOWAIT) >0) ;  //关闭的时候把所有的消息都清除掉
    
	return 0;
}



//返回值0表示成功，其他表示失败
int msgq_init(void)
{
	key_t key;
    
    key = ftok(KEY_PATH,KEY_ID);  //获取key值
    if(key < 0)
    {
        printf("ftok fail \n");
        fflush(stdout);
        return (-EAGAIN);
    }
    // 创建消息队列，如果消息队列存在，errno 会提示 eexist
    // 错误，此时只需要直接打开消息队列即可
    msgid = msgget(key,IPC_CREAT|IPC_EXCL|0666);
    if(msgid < 0)
    {
        if(errno == EEXIST) //文件存在错误提示
        {
            msgid = msgget(key,0666);//打开消息队列
            MsgClear();
        }
        else //其他错误退出
        {
            printf("msgget fail \n");
            return(-EAGAIN);
        }
    }
    //printf("msgget success!! msgid = %d\n",msgid);
    return 0;
}



//返回值0表示成功，其他表示失败
int msgq_exit(void)
{
	MsgClear();   //清楚消息队列中的消息
	return 0;
}


//接收消息
//参数 timeout_50ms 大于0时，表示非阻塞模式，等待的时间为timeout_50ms*50ms的值，等于0表示阻塞模式
//     types 接收消息的类型
//     msgbuf 接收消息的缓存，1次接收一条消息
//返回0表示成功，其他表示失败
int msgq_recv(long types,msgq_t *msgbuf,unsigned int timeout_50ms)
{
	int recv_flg = 0;	
	int ret;
//	int retry = 0;

	if(msgid == -1) //未初始化
	{
		printf("recv error,msgq_init first!!!\n");
		fflush(stdout);
		return -EPERM;
	}

	if(timeout_50ms > 0)
		recv_flg = IPC_NOWAIT;

    //接收消息
    do{
	    ret = msgrcv(msgid, msgbuf, sizeof(msgq_t)-sizeof(long),types,recv_flg);  //防止死循环
	    if(ret >= 0)
	    	break;
	    else if(0 == recv_flg) //阻塞堵塞模式出问题
	    {
			if(errno == EINTR)
				continue;
			return -EINTR;
		}
	    else if(errno != ENOMSG)
	    {
	    	printf("msgrcv\n");
	    	fflush(stdout);
	    	return -ENOMSG;
	    }	

	    usleep(50000);   //休眠50ms	   
    }while(--timeout_50ms);
    
    if(recv_flg && (0 == timeout_50ms))   //超时,可能设置为等待模式
    {
    	printf("ERROR:msg recv wait timeout,recv type = %ld!!!\n",types);
		fflush(stdout);		
    	return -EAGAIN;
    }
    
//    printf("type = %ld cmd = %d b = %d c = %d rt = %d\n",msgbuf->types,msgbuf->cmd,msgbuf->b,msgbuf->c,msgbuf->ret);
//	fflush(stdout);
	return 0;
}



//发送消息，并且要等待应答。
////参数 timeout 大于0时，设置接收应答超时时间（50ms的整数倍），等于0表示阻塞模式
//     ack_types 指定应答的类型，之前是TYPE_RECV+cmd
//     msgbuf 发送消息的缓存，1次发送一条消息
//返回0表示成功，其他表示失败
//注意，函数使用时，需要指定msgbuf->types ！！！！
int msgq_send(long ack_types,msgq_t *msgbuf,int timeout)
{
	int ret = -EBUSY;  //初始值
//	int cmd = msgbuf->cmd;

	if(msgid == -1) //未初始化
	{
		printf("send error,msgq_init first!!!\n");
		fflush(stdout);
		return -EPERM;
	}

	//msgbuf->types = TYPE_SEND; //给消息结构赋值，发送的类型是固定的TYPE_SEND
	//printf("debug: send type = %ld cmd = %d b = %d c = %d rt = %d\n",msgbuf->types,msgbuf->cmd,msgbuf->param1,msgbuf->param2,msgbuf->ret);
    //发送消息
    if(msgsnd(msgid, msgbuf, sizeof(msgq_t)-sizeof(long),0) == 0)
    {
    //	printf("debug:msgsnd ok \n");
    	//等待应答，等待的类型跟命令有关！！！！！
    	ret =  msgq_recv(ack_types,msgbuf,timeout);
    	if(ret == 0)    //正常在这返回0，仍然可能不是0. 
    	{
    //		printf("msgq_send and recv ok!!msgbuf.ret = %d\n",msgbuf->ret);
    		//用param1返回数据
			return 0;
		}
    }  
    printf("msgsnd error!\n"); //这条路应该还是有问题的。打印一下错误提示
    fflush(stdout);
    return ret;   // 返回错误值
}





//发送应答消息消息，不等待应答。
int msgq_send_ack(msgq_t *msgbuf)
{
//	int ret;
//	int cmd = msgbuf->cmd;

	if(msgid == -1) //未初始化
	{
		printf("send error,msgq_init first!!!\n");
		fflush(stdout);
		return -1;
	}

	//msgbuf->types = TYPE_SEND; //给消息结构赋值，发送的类型是固定的TYPE_SEND
//	printf("send type = %ld cmd = %d b = %d c = %d rt = %d\n",msgbuf->types,msgbuf->cmd,msgbuf->b,msgbuf->c,msgbuf->ret);
    //发送消息
    if(msgsnd(msgid, msgbuf, sizeof(msgq_t)-sizeof(long),0) == 0)
    {    	
		return 0;	
    }  

    printf("msgsnd error!\n"); //这条路应该还是有问题的。打印一下错误提示
    fflush(stdout);
    return -1; 
}




