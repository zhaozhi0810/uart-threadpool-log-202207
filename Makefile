
ARCH = `uname -m`
#ifneq ($(strip , "$(ARCH)"),"aarch64")
CROSS_COMPILE=aarch64-linux-gnu-
#endif


#编译所有子目录
#SUBDIRS=`ls -d */ | grep -v 'out' | grep -v 'log' | grep -v 'include' | grep -v 'test'`
#debug文件夹里的makefile文件需要最后执行，所以这里需要执行的子目录要排除debug文件夹，这里使用awk排除了debug文件夹，读取剩下的文件夹
SUBDIRS=$(shell ls -l | grep ^d | awk '{if($$9 != "test") print $$9}')
#SUBDIRS删除includes libs文件夹，因为这个文件中是头文件，不需要make
#SUBDIRS:=$(patsubst $(OUTLIBSDIR),,$(SUBDIRS))
# 因为在根目录下存在一个 log 文件夹，此文件只为了放置日志文件，并不需要参与编译
SUBDIRS:=$(patsubst log,,$(SUBDIRS))
SUBDIRS:=$(patsubst mixer_scontrols_app,,$(SUBDIRS))
#SUBDIRS:=$(patsubst $(OUTBINDIR),,$(SUBDIRS))


OBJS= libdrvapi22134.so  drv_22134_server    





define make_subdir
 @for subdir in $(SUBDIRS) ; do \
 ([ -d $$subdir ] && make $1 -C $$subdir )\
done;
endef

# 获取所有的头文件路径
CFLAGS  += $(foreach dir, $(SUBDIRS), -I$(dir))
CFLAGS  += -I./ 

all: all_subdir $(OBJS) 
	@[ -d test ] && make -C test

all_subdir:
	$(call make_subdir, )
 	
install :
	$(call make_subdir,install)
 
debug:
	$(call make_subdir,debug)
clean:
	$(call make_subdir,clean) 
	rm libdrvapi22134.so *.o drv_22134_server *.d
	[ -d test ]  && make clean -C test






#all: path $(obj-o)  $(OBJS)


#CUR_DIR:=$(shell find . -maxdepth 4 -type d)
#base_Dir:=$(basename $(patsubst(./%,%,$(CUR_DIR))))

#all:$(OBJS)
#	$(CROSS_COMPILE)gcc drv_22134_api.o my_ipc_msgq.o -fPIC -shared -o libdrvapi22134.so
#	$(CROSS_COMPILE)gcc test_22134_api.o -o test_22134_api -L./ -ldrvapi22134
#	[ -d kmUtil ] && cd kmUtil && make && cd ../	
#	$(CROSS_COMPILE)gcc drv_22134_server.o threadpool.o my_ipc_msgq.o my_log.o kmUtil/built-in.o -o drv_22134_server -lpthread

# mixer_scontrols.o
libdrvapi22134.so:drv_22134_api.o linux-gpio-app/built-in.o msgq_app/built-in.o audio-i2c/built-in.o keyboard/built-in.o   
	@echo "Create target " $@	
	@$(CROSS_COMPILE)gcc $^ -o $@ -fPIC -shared  

#test_22134_api:test_22134_api.o  libdrvapi22134.so 
#	$(CROSS_COMPILE)gcc $^ -o $@ -L./ -ldrvapi22134  -lpthread

drv_22134_server:drv_22134_server.o threadpool_app/built-in.o msgq_app/built-in.o log_app/built-in.o kmUtil/built-in.o 
	@echo "Create target " $@
	@$(CROSS_COMPILE)gcc $^ -o $@ -lpthread 

#test/drv722test:test/main.o
#	[ -d test ] && make -C test


#kmUtil/built-in.o:
#	[ -d kmUtil ] && make -C kmUtil
#	[ -d kmUtil ] && cd kmUtil && make

#	gcc drv_21155.c ComFunc.c mcu_api.c cpu_mem_cal.c threadpool.c key_hot_thread.c log.c -o drv_21155 -lpthread
#	cp libdrvapi.so APItest-2021-11-24/build-apitest-Desktop_Qt_5_12_9_GCC_64bit-Debug/

%.o:%.c 
	@echo "building file " $@
	@$(CROSS_COMPILE)gcc -c  $<  $(CFLAGS)
#	@$(CROSS_COMPILE)gcc  -w -c $< $(CFLAGS) -MMD -MT $@ -o $@



obj-c =
obj-c += $(wildcard *.c)
obj-o = 
obj-o += $(patsubst %.c, %.o, $(obj-c))

DEPENDENCY_LIST = $(patsubst %.o,%.d,$(obj-o))
-include $(DEPENDENCY_LIST)


#clean:
#	rm libdrvapi22134.so *.o drv_22134_server
#	[ -d kmUtil ] && cd kmUtil && make clean




#1.带1级子目录的工程，makefile规则先生成.o文件然后使用所有.o文件生成可执行文件。
#2.自动生成源文件对应.o文件的依赖
#3.使用静态模式生成.o文件
#4.最后链接所有第3步生成文件
#5.将.o .d文件放在out路径下



#obj-c =
#obj-c += $(wildcard *.c)
#obj-c += $(wildcard kmUtil/*.c)
#obj-c += $(wildcard audio-i2c/*.c)
#obj-c += $(wildcard keyboard/*.c)
#obj-o = 
#obj-o += $(patsubst %.c, %.o, $(obj-c))

#INCLUDES = -IkmUtil -Iaudio-i2c
#out-dir = out
#obj-list = $(addprefix $(out-dir)/,$(obj-o))
#DEPENDENCY_LIST = $(patsubst %.o,%.d,$(obj-o))



#    @gcc -o main $(obj-o)

#后缀静态模式
#.SUFFIXES: .c .o
#.c.o:
#	echo $(obj-c)

#	@echo "building file " $@  
#	@echo $(ARCH)  
#	@mkdir -p $(out-dir)/$(subst $(notdir $@),,$@)
#	@$(CROSS_COMPILE)gcc   $(INCLUDES) -w -c $< -MMD -MT $@ -o $@
#	@cp $@ $(out-dir)/$@

#-include $(DEPENDENCY_LIST)

#path:
#	@[ ! -d $(out-dir) ] || mkdir -p $(out-dir)
    
#clean:
#	rm *.o *.d libdrvapi22134.so  test_22134_api drv_22134_server
#	[ -d kmUtil ]  && make clean -C kmUtil
#	[ -d audio-i2c ] && make clean  -C audio-i2c
#	[ -d test ]  && make clean -C test
#	rm -rf $(out-dir)
    
#静态模式
#$(obj-o): %.o:%.c
#    echo $0
#    gcc $(INCLUDES) -c -w $< -MMD -MT $@ -o $@
#-include $(dependency_list)

.PHONY: all  clean






