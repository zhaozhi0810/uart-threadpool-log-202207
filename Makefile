
CROSS_COMPILE=aarch64-linux-gnu-
OBJS= libdrvapi22134.so  drv_22134_server kmUtil/built-in.o test_22134_api

all:$(OBJS)
#	$(CROSS_COMPILE)gcc drv_22134_api.o my_ipc_msgq.o -fPIC -shared -o libdrvapi22134.so
#	$(CROSS_COMPILE)gcc test_22134_api.o -o test_22134_api -L./ -ldrvapi22134
#	[ -d kmUtil ] && cd kmUtil && make && cd ../	
#	$(CROSS_COMPILE)gcc drv_22134_server.o threadpool.o my_ipc_msgq.o my_log.o kmUtil/built-in.o -o drv_22134_server -lpthread


libdrvapi22134.so:drv_22134_api.o my_ipc_msgq.o
	$(CROSS_COMPILE)gcc $^ -o $@ -fPIC -shared

test_22134_api:test_22134_api.o libdrvapi22134.so
	$(CROSS_COMPILE)gcc $^ -o $@ -L./ -ldrvapi22134

drv_22134_server:drv_22134_server.o threadpool.o my_ipc_msgq.o my_log.o kmUtil/built-in.o
	$(CROSS_COMPILE)gcc $^ -o $@ -lpthread

kmUtil/built-in.o:
	[ -d kmUtil ] && cd kmUtil && make

#	gcc drv_21155.c ComFunc.c mcu_api.c cpu_mem_cal.c threadpool.c key_hot_thread.c log.c -o drv_21155 -lpthread
#	cp libdrvapi.so APItest-2021-11-24/build-apitest-Desktop_Qt_5_12_9_GCC_64bit-Debug/

%.o:%.c 
	$(CROSS_COMPILE)gcc -c  $<



clean:
	rm libdrvapi22134.so *.o test_22134_api drv_22134_server
	[ -d kmUtil ] && cd kmUtil && make clean
