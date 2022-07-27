
#CROSS_COMPILE=
OBJS= drv_22134_api.o my_ipc_msgq.o test_22134_api.o drv_22134_server.o threadpool.o my_ipc_msgq.o my_log.o 
all:$(OBJS)
	$(CROSS_COMPILE)gcc drv_22134_api.o my_ipc_msgq.c -fPIC -shared -o libdrvapi22134.so
	$(CROSS_COMPILE)gcc test_22134_api.o -o test_22134_api
	[ -d kmUtil ] && cd kmUtil && make && cd ../	
	$(CROSS_COMPILE)gcc drv_22134_server.o threadpool.o my_ipc_msgq.o my_log.o kmUtil/built-in.o -o drv_22134_server -lpthread
#	gcc drv_21155.c ComFunc.c mcu_api.c cpu_mem_cal.c threadpool.c key_hot_thread.c log.c -o drv_21155 -lpthread
#	cp libdrvapi.so APItest-2021-11-24/build-apitest-Desktop_Qt_5_12_9_GCC_64bit-Debug/

%.o:%.c 
	$(CROSS_COMPILE)gcc -c  $<



clean:
	rm libdrvapi22134.so *.o
	[ -d kmUtil ] && cd kmUtil && make clean
