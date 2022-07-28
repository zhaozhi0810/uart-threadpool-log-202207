CROSS_COMPILE=aarch64-linux-gnu-
CC=gcc
#LIBS = -L/opt/mobilygen/distributions/merlin-SDK6r3648/i686-linux/arm-merlin-linux-uclibc/usr/lib 
LIBS=-L/usr/local/lib -L/usr/lib
OBJS= queue.o ComFunc.o uart_to_mcu.o #msptr.o  Vkeyb.o
#CFLAGS= -c -DS3_MOUSE -DUSE_X11_LIB -DDEBUG_VKEY -DDEBUG_MSE
CFLAGS= -c #-DS3_MOUSE #-DUSE_X11_LIB   
	
built-in.o:$(OBJS)
#	$(CROSS_COMPILE)gcc  -rdynamic -s -o kmUtil $(OBJ) $(LIBS)  #-pthread  -lX11
	$(CROSS_COMPILE)ld -EL -r -o $@ $(OBJS) 
#make:$(OBJ)
#	$(CC)  -rdynamic -s -o kmUtil $(OBJ) $(LIBS)

%.o:%.c 
	$(CROSS_COMPILE)gcc $(CFLAGS) $<

# Vkeyb.o:Vkeyb.c 
# 	$(CC) $(CFLAGS) Vkeyb.c

# queue.o:queue.c 
# 	$(CC) $(CFLAGS) queue.c

# ComFunc.o:ComFunc.c 
# 	$(CC) $(CFLAGS) ComFunc.c

# msptr.o:msptr.c 
# 	$(CC) $(CFLAGS) msptr.c

.PHONY:clean
clean:
	rm -f *.o
