/*
* @Author: dazhi
* @Date:   2022-08-08 17:39:59
* @Last Modified by:   dazhi
* @Last Modified time: 2022-08-08 17:49:28
*/

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/sem.h>
 
/*
 * brief : 创建信号量
 * param : 
 *	sem_id : 信号量键值,  一个唯一的非零整数, 不同的进程可以通过它访问同一个信号量。
 *	num_sems : 信号量数目, 它几乎总是取值为1.
 *	sem_flags:	它低端的9个比特是该信号量的权限，其作用类似于文件的访问权限。
 *				它还可以和值IPC_CREAT做按位或操作，来创建新的信号量。
 *				可以联合使用标志IPC_CREAT和IPC_EXCL, 确保创建出的是一个新的、唯一的信号量。
 *				如果该信号量已存在，它将返回一个错误。
 * ret :
 *		-1 失败
 *		>0 文件描述符
 */ 
/*semget(sem_id, sem_ops, num_sem_ops) ;*/
 
/*
 * brief : 用于改变信号量的值。
 * param : 
 *	sem_id : 信号量的文件描述符
 *	sem_ops : 操作数据结构.
 *			struct sembuf {
 *				short sem_num ;		信号量编号，一般为 0 
 *				short sem_op ;		一次操作要改变的数值，-1 P操作 ， 1 V操作. 
 *									如果其值为正数，该值会加到现有的信号内含值中。
 *									通常用于释放所控资源的使用权；
 *									如果sem_op的值为负数，而其绝对值又大于信号的现值，
 *									操作将会阻塞，直到信号值大于或等于sem_op的绝对值。
 *
 *				short sem_flag ;	通常被设置成SEM_UNDO, 
 *									它将使操作系统跟踪当前进程对这个信号量的修改情况，
 *									如果这个进程在没有释放信号量的情况下终止，
 *									OS将自动释放该进程持有的信号量。
 *			}
 *	num_sem_ops : 表示结构数组sem_ops的个数
 * ret :
 *		-1 失败
 *		0 成功
 */ 
/*semop(int sem_id, struct sembuf *sem_ops, size_t num_sem_ops) ;*/
 
/*
 * brief : 直接控制信号量信息
 * param : 
 *	sem_id : 信号量的文件描述符 
 *	sem_num : 信号理编号，一般取值为0
 *	command : 将要采取的动作
 *		SETVAL: 用来把信号量初始化为一个已知的值
 *
 * ret :
 *		-1 失败
 *		0 成功
 */ 
/*semctl(int sem_id, int sem_num, int command, ...) ;*/
 
#define SEM_KEY_PATH "/proc/cpuinfo"
#define SEM_KEY_ID -123456
static int semId = -1; 

union semun
{
	int val;
	struct semid_ds *buf;
	unsigned short *array;
};


//初始化
int initSem(int val){
	union semun semUnit ;
	key_t key;
    
    key = ftok(SEM_KEY_PATH,SEM_KEY_ID);  //获取key值
    if(key < 0)
    {
        printf("ftok fail \n");
        fflush(stdout);
        return (-EAGAIN);
    }

	semId = semget(key, 1, 0666) ;
	if(semId < 0){
		printf("get sem fail \n");
		semId = semget(key, 1, 0666 | IPC_CREAT) ;
	//	initSem(semId) ;
	}
	else  //已经存在则返回
		return;  

	semUnit.val = val ;
	if (semctl(semId, 0, SETVAL, semUnit) < 0) {
		printf("fail to init sem\n");
		return -1 ;
	}
	return 0 ;
}
 
int semP(void){
	struct sembuf op ;
	op.sem_num = 0 ;
	op.sem_op = -1 ;
	op.sem_flg = SEM_UNDO ;
 
	if (semop(semId, &op, 1) < 0) {
		printf("p fail\n");
		return -1 ;
	}
	return 0 ;
}
 
int semV(void){
	struct sembuf op ;
	op.sem_num = 0 ;
	op.sem_op = 1 ;
	op.sem_flg = SEM_UNDO ;
 
	if (semop(semId, &op, 1) < 0) {
		printf("v fail\n");
		return -1 ;
	}
	return 0 ;
}
 
int delSem(void){
	union semun semUnit ;
	if(semctl(semId, 0, IPC_RMID, semUnit) < 0){
		printf("fail to delete\n");
		return -1 ;
	}
	return 0 ;
}
 
// int main(int argc, char** argv) {
 
// 	int semId = semget((key_t)1234, 1, 0666) ;
// 	if(semId < 0){
// 		printf("get sem fail \n");
// 		semId = semget((key_t)1234, 1, 0666 | IPC_CREAT) ;
// 		initSem(semId) ;
// 	}
// 	/*delSem(semId) ;*/
 
// 	if(argc > 1){
// 		while(1){
// 			printf("before v semId\n");
// 			semV(semId) ;
// 			printf("after v semId\n");
// 			sleep(2);
// 		}
// 	}else{
// 		while(1){
// 			printf("before p semId\n");
// 			semP(semId) ;
// 			printf("after p semId\n");
// 			sleep(2);
// 		}
// 	}
 
// 	return 0;
// }

