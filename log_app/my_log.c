
/*
 * 后台服务程序日志文件。
 * 1.服务程序与mcu的通信记录
 * 2.服务程序与api交互的数据
 * 3.服务程序与snmp服务交互的数据
 * 4.其他操作
 * 随服务程序启动。文件名由年月日日期组成，每天记录一个文件。
 * 使用一个线程来完成记录吧。
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






static int pipe_read_fd;

#define DAY10_SECS (60*60*24*10)     //10天的秒数

static void* log_thread(void* arg);
static 	int log_fd = -1;   //用于输出的文件描述符

#define LOG_PATH "/root/log"    //指定日志目录，需要绝对路径，server一般是守护进程，不指定将在根目录下。



static FILE* log_open_file_dup2_to_stdout(char *filename)
{
	int fd = open(filename,O_RDWR | O_APPEND | O_CREAT,0666);
	if(fd < 0)
	{
		perror("open log file");
		return NULL;
	}
	

	FILE* fp = fdopen(fd, "a");  //转换文件描述符
	if(NULL == fp)
	{
		perror("ERROR:fdopen ");
		return NULL;
	}

	return fp;
}


static int delete_outdate_file(void)
{
	DIR* path = NULL;
	struct dirent*ptr;
	struct stat statbuf;
//	int ret;
//	char buf[1024];
	time_t t;
//	struct tm tim;	
	char file_name[272] = {LOG_PATH};
	int filename_len = strlen(LOG_PATH);


	if(file_name[filename_len-1] != '/')
	{
		file_name[filename_len] = '/';
		filename_len += 1;
	}	

	t = time(NULL);  //获得系统时间
	//localtime_r(&t, &tim);

	path = opendir(LOG_PATH);
	if(path==NULL)
	{
		return -1;
	}

	while(NULL!=(ptr = readdir(path)))
	{
		if(ptr->d_name[0] != '.')
		{
			if(ptr->d_type == DT_REG)  //普通文件
			{
								
				//strcpy(file_name+filename_len,ptr->d_name,)	-
				snprintf(file_name+filename_len,sizeof(file_name)-filename_len,"%s",ptr->d_name);							
				if(stat(file_name, &statbuf) == 0)  //
				{
					if(((t - statbuf.st_mtime) > (DAY10_SECS)))  //如果存在呢，删除文件
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
 		if(log_fd>=0)
 			syncfs(log_fd);
 		else
 			alarm(0);  //关闭定时器
 	}
}


//打开文件，追加模式
int log_init(void)
{
	struct stat statbuf;
	pthread_t thread;
//	int path_len = 0; 
	int pipefd[2];  //创建管道
	
	//目录是否存在？
	if(stat(LOG_PATH, &statbuf))  //出错
	{
		perror("stat");
		if(2 == errno)  //文件不存在
		{
			if(-1 == mkdir(LOG_PATH,0775))
			{
				perror("mkdir");
				return -1;
			}
			goto go_on;
		}
		else
		{
			return -1;
		}
	}
	if((statbuf.st_mode & S_IFMT) != S_IFDIR)  //如果存在呢，不是目录也出错
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
		perror("dup2");
		return -1;
	}

	//标准输出无缓存
	setbuf(stdout, NULL); 
		
//	close(pipefd[1]);
	pipe_read_fd = pipefd[0];
	
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





static void* log_thread(void* arg)
{	
	char buf[1024];	
	char buf_time[32];	
	FILE * fp;
	FILE* fp_write=NULL;
	time_t t;
	struct tm tim;
//	int offset = 0;;
//	int len = 0;
//	int ret;

	int last_year=0,last_month=0,last_day=0;   //保存上次的日期
	char file_name[128] = {LOG_PATH};
	char cmd[128] = {"/bin/sh find "};

	//组成文件名
//	strcat(file_name,LOG_PATH);
	int path_len = strlen(LOG_PATH);
	
	strcat(cmd,LOG_PATH);
	strcat(cmd," -mtime +10 -type f -exec rm -rfv { } ;");

	fp = fdopen(pipe_read_fd, "r");  //转换文件描述符
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







