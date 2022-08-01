
CROSS_COMPILE=aarch64-linux-gnu-
OBJS= libdrvapi22134.so  drv_22134_server  test_22134_api   #kmUtil/built-in.o


all: path $(obj-o)  $(OBJS)


#CUR_DIR:=$(shell find . -maxdepth 4 -type d)
#base_Dir:=$(basename $(patsubst(./%,%,$(CUR_DIR))))

#all:$(OBJS)
#	$(CROSS_COMPILE)gcc drv_22134_api.o my_ipc_msgq.o -fPIC -shared -o libdrvapi22134.so
#	$(CROSS_COMPILE)gcc test_22134_api.o -o test_22134_api -L./ -ldrvapi22134
#	[ -d kmUtil ] && cd kmUtil && make && cd ../	
#	$(CROSS_COMPILE)gcc drv_22134_server.o threadpool.o my_ipc_msgq.o my_log.o kmUtil/built-in.o -o drv_22134_server -lpthread

# mixer_scontrols.o
libdrvapi22134.so:drv_22134_api.o gpio_export.o my_ipc_msgq.o audio-i2c/i2c_reg_rw.o    
	$(CROSS_COMPILE)gcc $^ -o $@ -fPIC -shared

test_22134_api:test_22134_api.o  libdrvapi22134.so
	$(CROSS_COMPILE)gcc $^ -o $@ -L./ -ldrvapi22134

drv_22134_server:drv_22134_server.o threadpool.o my_ipc_msgq.o my_log.o kmUtil/queue.o  kmUtil/ComFunc.o  kmUtil/uart_to_mcu.o
	$(CROSS_COMPILE)gcc $^ -o $@ -lpthread

#kmUtil/built-in.o:
#	[ -d kmUtil ] && make -C kmUtil
#	[ -d kmUtil ] && cd kmUtil && make

#	gcc drv_21155.c ComFunc.c mcu_api.c cpu_mem_cal.c threadpool.c key_hot_thread.c log.c -o drv_21155 -lpthread
#	cp libdrvapi.so APItest-2021-11-24/build-apitest-Desktop_Qt_5_12_9_GCC_64bit-Debug/

#%.o:%.c 
#	$(CROSS_COMPILE)gcc -c  $<



#clean:
#	rm libdrvapi22134.so *.o test_22134_api drv_22134_server
#	[ -d kmUtil ] && cd kmUtil && make clean




#1.带1级子目录的工程，makefile规则先生成.o文件然后使用所有.o文件生成可执行文件。
#2.自动生成源文件对应.o文件的依赖
#3.使用静态模式生成.o文件
#4.最后链接所有第3步生成文件
#5.将.o .d文件放在out路径下



obj-c =
obj-c += $(wildcard *.c)
obj-c += $(wildcard kmUtil/*.c)
obj-c += $(wildcard audio-i2c/*.c)
obj-o = 
obj-o += $(patsubst %.c, %.o, $(obj-c))

INCLUDES = -IkmUtil -Iaudio-i2c
out-dir = out
obj-list = $(addprefix $(out-dir)/,$(obj-o))
DEPENDENCY_LIST = $(patsubst %.o,%.d,$(obj-o))



#    @gcc -o main $(obj-o)

#后缀静态模式
.SUFFIXES: .c .o
.c.o:
#	echo $(obj-c)

	@echo "building file " $@    
	@mkdir -p $(out-dir)/$(subst $(notdir $@),,$@)
	@$(CROSS_COMPILE)gcc   $(INCLUDES) -w -c $< -MMD -MT $@ -o $@
	@cp $@ $(out-dir)/$@

-include $(DEPENDENCY_LIST)

path:
	@[ ! -d kmUtil ] || mkdir -p $(out-dir)
    
clean:
	rm *.o *.d libdrvapi22134.so  test_22134_api drv_22134_server
	[ -d kmUtil ] && cd kmUtil && make clean
	[ -d audio-i2c ] && cd audio-i2c && make clean
	rm -rf $(out-dir)
    
#静态模式
#$(obj-o): %.o:%.c
#    echo $0
#    gcc $(INCLUDES) -c -w $< -MMD -MT $@ -o $@
#-include $(dependency_list)

.PHONY: all path clean






