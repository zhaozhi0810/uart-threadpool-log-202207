CROSS_COMPILE = aarch64-linux-gnu-
CC = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)gcc

CFLAGS = -c -Wall
CFLAGS += -I./
CFLAGS += -I../

LDFLAGS = -L../
LDFLAGS += -lpthread
LDFLAGS += -ldrv722_22134

TARGET = test

all: $(TARGET)

$(TARGET): main.o
	$(LD) $< -o $@ $(LDFLAGS)

%.o:%.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf main.o test
	