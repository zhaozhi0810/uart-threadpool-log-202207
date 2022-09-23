
/*
 * 后台服务程序日志文件。
 * 1. 只需要调用log_init()即可
 * 2. 注意日志写的路径的权限问题
 * */
#define _GNU_SOURCE

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <stdio.h>
#include <error.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <signal.h>

#define DAY10_SECS (60*60*24*10)     //10天的秒数
//要考虑可能会是守护进程，这里要用绝对路径
#define LOG_PATH "/root/log"    //指定日志目录，需要绝对路径，server一般是守护进程，不指定将在根目录下。


static int pipe_read_fd;  //保存管道的文件描述符
static 	int log_fd = -1;   //用于输出的文件描述符

static void* log_thread(void* arg);



//打开一个日志文件
static FILE* log_open_file_dup2_to_stdout(char *filename)
{
	log_fd = open(filename,O_RDWR | O_APPEND | O_CREAT,0666);
	if(log_fd < 0)
	{
		perror("open log file");
		return NULL;
	}
	
	FILE* fp = fdopen(log_fd, "a");  //转换文件描述符
	//FILE* fp = fopen(filename, "a+"); 
	if(NULL == fp)
	{
		close(log_fd);
		log_fd = -1;
		perror("ERROR:fdopen ");
		return NULL;
	}

	return fp;
}


//考虑到日志文件会越来越多，也会影响磁盘的空间占用，这里就会删除以前的日志文件，默认删除10天前的日志
//1.假定该目录只存放了日志文件，所以没有去判断文件名了
//2.获取目录中文件的修改时间，以修改时间和现在的时间作比较，则可以找出10天前的文件了。
static int delete_outdate_file(void)
{
	DIR* path = NULL;
	struct dirent*ptr;
	struct stat statbuf;
	time_t t;
	char file_name[272] = {LOG_PATH};  //太小有警告
	int filename_len = strlen(LOG_PATH);

	//最后一个字符是不是/
	if(file_name[filename_len-1] != '/')
	{
		file_name[filename_len] = '/';
		filename_len += 1;
		file_name[filename_len] = '\0';		
	}	

	t = time(NULL);  //获得系统时间
	//localtime_r(&t, &tim);

	path = opendir(LOG_PATH);
	if(path==NULL)
	{
		return -1;
	}

	while(NULL!=(ptr = readdir(path)))  //循环读取目录中的文件
	{
		if(ptr->d_name[0] != '.') //跳过 . 和..
		{
			if(ptr->d_type == DT_REG)  //普通文件
			{
				snprintf(file_name+filename_len,sizeof(file_name)-filename_len,"%s",ptr->d_name);	//形成文件的绝对路径						
				if(stat(file_name, &statbuf) == 0)  //如果存在呢
				{
					if(((t - statbuf.st_mtime) > (DAY10_SECS)))  //如果文件修改时间超时了，删除文件
					{
						unlink(file_name);
					}				
				}
			}
		}	
	}
	closedir(path);
	return 0;
}



/*用定时器来处理硬盘同步信息*/
void alarm_sig_handler(int signo) {
 	printf( "debug signo = %d\n " ,signo);

 	if(signo == SIGALRM)
 	{
 		alarm(30);   //30秒触发
 		if(log_fd>=0)    //文件是正常打开了，则刷新缓存
 			syncfs(log_fd);
 		else   //文件没有被打开，关闭定时器
 			alarm(0);  //关闭定时器
 	}
}


//日志的初始化函数，
//1.判断日志的目录是否存在，不存在则会尝试创建目录
//2.建立管道，标准输出重定向到管道的写端1.
//3.创建日志记录的线程，读取管道的数据，并写道对应的文件中
//4.为了减缓对磁盘的读写，增加一个30s的定时器，用于刷新缓存的数据到硬盘中。
int log_init(void)
{
	struct stat statbuf;
	pthread_t thread;
//	int path_len = 0; 
	int pipefd[2];  //创建管道
	
	//目录是否存在？
	if(stat(LOG_PATH, &statbuf))  //检测出错
	{
		perror("stat");
		if(2 == errno)  //文件不存在，可以尝试创建文件
		{
			if(-1 == mkdir(LOG_PATH,0775))  //创建目录
			{
				perror("mkdir");
				return -1;
			}
			goto go_on;  //创建成功，继续
		}
		else //是其他错误，则终止
		{
			return -1;
		}
	}
	if((statbuf.st_mode & S_IFMT) != S_IFDIR)  //目标如果存在呢，不是目录也出错
	{
		printf("log is not dir!!! please rm log file\n");
		return -1;
	}
go_on:
	//return 0;	
	//创建管道，将标准输出到日志文件。
	if(0 !=pipe(pipefd)) 
	{
		perror("pipe");	
		return -1;
	}
		
	//输出重定向到管道(管道的写端口设置为标准输出)
	if(-1 == dup2(pipefd[1],STDOUT_FILENO))   //出错，把标准输出定位过来
	{
		close(pipefd[0]);  //关闭前面打开的文件
		close(pipefd[1]);
		perror("dup2");
		return -1;
	}

	//标准输出无缓存
	setbuf(stdout, NULL); 
		
//	close(pipefd[1]);
	pipe_read_fd = pipefd[0];  //用全局变量保存文件描述符号
	
	//创建线程记录日志
	if(0 != pthread_create(&thread, NULL, log_thread, NULL))
	{
		printf("error:pthread_create\n");
		close(pipefd[0]);
		return -1;
	}
    pthread_detach(thread);   //设置分离模式
	
    signal(SIGALRM,alarm_sig_handler);  //注册了定时函数
    alarm(30);   //30秒触发

	return 0;	
}




//用于记录日志的线程
//1.打开管道的读端，用fdopen，相当于增加管道的缓存，方便我后面读取一行的数据。
//循环
//1.日志的记录以行为准则，前面转换过文件描述符后，使用fgets读取一行的数据，fgets可以阻塞线程
//2.考虑到日志文件应该每天生成一个新的，也要考虑到可能机器长时间不关机的情况。用时间来判断是否需要写新的日志文件。
//3.用变量记录这次日志文件的年月日，用系统时间的年月日去比较，如果不是同一天则关闭原来的日志文件，并且
//生成新的文件名，重新打开该文件，并往新的日志文件写入日志。
//4.考虑到日志文件太多，占据了比较多的硬盘空间，设置了10天前的日志将被删除的操作。这个部分只用每次时间不同的时候做一次即可。
//5.对写入日志文件的内容加上时间标记，更方便去排查故障出现的时间。先写入时间，再写缓存读回的内容。
static void* log_thread(void* arg)
{	
	char buf[1024];	
	char buf_time[32];	
	FILE * fp;
	FILE* fp_write=NULL;
	time_t t;
	struct tm tim;

	int last_year=0,last_month=0,last_day=0;   //保存上次的日期
	char file_name[128] = {LOG_PATH};
	int path_len = strlen(LOG_PATH);
	
	fp = fdopen(pipe_read_fd, "r");  //打开管道，读取printf输出的内容
	if(NULL == fp)
	{
		perror("ERROR:fdopen ");
		return NULL;
	}
	
	while(1)
	{
		memset(buf,0,sizeof(buf));//数据清零
		
		if(fgets(buf,sizeof buf,fp)==NULL) //返回空指针，有问题
			continue;
	//	perror("xxxxx");
		t = time(NULL);  //获得系统时间
		localtime_r(&t, &tim);
		//根据时间生成新的日志文件
		if((last_year != (tim.tm_year+1900)) || (last_month != (tim.tm_mon+1)) || (last_day != tim.tm_mday))
		{
			if(fp_write!=NULL)
			{
				fclose(fp_write);  //关闭老的文件。
				fp_write = NULL;
				log_fd = -1;
			}
				
			//形成新的文件名
			snprintf(file_name+path_len,sizeof(file_name)-path_len,"/%4d_%02d_%02d.log",tim.tm_year+1900,tim.tm_mon+1,tim.tm_mday);
			fp_write = log_open_file_dup2_to_stdout(file_name);
			if(fp_write == NULL)
			{
				perror("open log file");
				return NULL;
			}

			//删除10天前的所有文件
			//system(cmd);
			delete_outdate_file();
		}
			
		snprintf(buf_time,sizeof(buf_time),"[%4d_%02d_%02d %02d:%02d:%02d]",tim.tm_year+1900,tim.tm_mon+1,tim.tm_mday
																			,tim.tm_hour,tim.tm_min,tim.tm_sec);			
		//write(log_fd,buf_time,strlen(buf_time));
		//write(log_fd,buf,strlen(buf));
		//使用缓存写，就不用频繁写硬盘了。
		fwrite(buf_time,1,strlen(buf_time),fp_write);
		fwrite(buf,1,strlen(buf),fp_write);
		//syncfs(log_fd);
	}	
}







