# 交叉编译工具头,如：arm-linux-gnueabihf-
CROSS_COMPILE = aarch64-linux-gnu-
AS      = $(CROSS_COMPILE)as # 把汇编文件生成目标文件
LD      = $(CROSS_COMPILE)ld # 链接器，为前面生成的目标代码分配地址空间，将多个目标文件链接成一个库或者一个可执行文件
CC      = $(CROSS_COMPILE)gcc # 编译器，对 C 源文件进行编译处理，生成汇编文件
CPP    = $(CC) -E
AR      = $(CROSS_COMPILE)ar # 打包器，用于库操作，可以通过该工具从一个库中删除或则增加目标代码模块
NM     = $(CROSS_COMPILE)nm # 查看静态库文件中的符号表

STRIP       = $(CROSS_COMPILE)strip # 以最终生成的可执行文件或者库文件作为输入，然后消除掉其中的源码
OBJCOPY  = $(CROSS_COMPILE)objcopy # 复制一个目标文件的内容到另一个文件中，可用于不同源文件之间的格式转换
OBJDUMP = $(CROSS_COMPILE)objdump # 查看静态库或则动态库的签名方法

# 共享到sub-Makefile
export AS LD CC CPP AR NM
export STRIP OBJCOPY OBJDUMP


SUBDIRS=$(shell ls -l | grep ^d | awk '{if($$9 != "test") print $$9}')
#SUBDIRS删除includes libs文件夹，因为这个文件中是头文件，不需要make
#SUBDIRS:=$(patsubst $(OUTLIBSDIR),,$(SUBDIRS))
# 因为在根目录下存在一个 log 文件夹，此文件只为了放置日志文件，并不需要参与编译
SUBDIRS:=$(patsubst log,,$(SUBDIRS))
SUBDIRS:=$(patsubst mixer_scontrols_app,,$(SUBDIRS))


# -Wall ： 允许发出 GCC 提供的所有有用的报警信息
# -O2 : “-On”优化等级
# -g : 在可执行程序中包含标准调试信息
# -I : 指定头文件路径（可多个）
CFLAGS := -Wall -O2 -g 
CFLAGS += -I./ $(foreach dir, $(SUBDIRS), -I$(dir)) 

# LDFLAGS是告诉链接器从哪里寻找库文件，这在本Makefile是链接最后应用程序时的链接选项。
LDFLAGS := 

# 共享到sub-Makefile
export CFLAGS LDFLAGS

# 顶层路径
TOPDIR := $(shell pwd)
export TOPDIR

# 最终目标
TARGET := libdrv722_22134.so  drv722_22134_server

# 本次整个编译需要源 文件 和 目录
# 这里的“obj-y”是自己定义的一个格式，和“STRIP”这些一样，*但是 一般内核会搜集 ”obj-”的变量*
obj-y += drv722_22134_server.o  drv722_22134_api.o# 需要把当前目录下的 main.c 编进程序里
#obj-y += sub.o # 需要把当前目录下的 sub.c 编进程序里
 # 需要进入 subdir 这个子目录去寻找文件来编进程序里，具体是哪些文件，由 subdir 目录下的 Makefile 决定。
obj-y +=  $(foreach dir, $(SUBDIRS), $(dir)/)   #audio-i2c/  keyboard/ kmUtil/  linux-gpio-app/ log_app/ msgq_app/ threadpool_app/
#obj-y += $(patsubst %.c,%.o,$(shell ls *.c)) 

# 第一个目标 # $(TARGET)
all : start_recursive_build $(TARGET)
	@[ -d test ] && make -C test
	@echo $(TARGET) has been built !
	
# 处理第一个依赖，**转到 Makefile.build 执行**
start_recursive_build:
	make -C ./ -f $(TOPDIR)/Makefile.build
	
# 处理最终目标，把前期处理得出的 built-in.o 用上
drv722_22134_server : drv722_22134_server.o kmUtil_server/built-in.o msgq_api_server/built-in.o log_server/built-in.o threadpool_server/built-in.o
	$(CC) -o $@ $^ $(LDFLAGS) -lpthread

libdrv722_22134.so:drv722_22134_api.o linux-gpio-api/built-in.o msgq_api_server/built-in.o audio-i2c_api/built-in.o keyboard_api/built-in.o   
	@echo "Create target " $@	
	@$(CC) $^ -o $@ -fPIC -shared 
	
# 清理
clean:
	rm -f $(shell find -name "*.o")
	rm -f $(TARGET)
	[ -d test ]  && make clean -C test
	
# 彻底清理
distclean:clean
#	rm -f $(shell find -name "*.o")
	rm -f $(shell find -name "*.d")
#	rm -f $(TARGET)


