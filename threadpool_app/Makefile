

CROSS_COMPILE=aarch64-linux-gnu-


obj-c =
obj-c += $(wildcard *.c)
obj-o = 
obj-o += $(patsubst %.c, %.o, $(obj-c))

DEPENDENCY_LIST = $(patsubst %.o,%.d,$(obj-o))
-include $(DEPENDENCY_LIST)


CFLAGS= -c  
	
built-in.o:$(obj-o)
	@echo "Create target " $@
	@$(CROSS_COMPILE)ld -EL -r -o $@ $^ 


%.o:%.c 
	@echo "building file " $@
	@$(CROSS_COMPILE)gcc $(CFLAGS) $<
	@$(CROSS_COMPILE)gcc  -w -c $< -MMD -MT $@ -o $@

.PHONY:clean
clean:
	@rm -f *.o *.d
