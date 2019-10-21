#Remove the comment to use debug mode on module
#ccflags-y := -DDEBUG=$(DEBUG)

obj-m += modcrypto.o
 
all:
	 make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) modules
	 insmod modcrypto.ko
	 gcc interfaceCrypto.c -o prog
	 ./prog
clean:
	 rmmod modcrypto
	 make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) clean
	 rm prog
