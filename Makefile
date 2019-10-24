#Remove the comment to use debug mode on module
#ccflags-y := -DDEBUG=$(DEBUG)

obj-m += modcrypto.o
 
all:
	 make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) modules
	 gcc interfaceCrypto.c -o prog
	 @echo
	 @echo
	 @echo Execute o seguinte comando para inserir o módulo com os devidos parâmetros:
	 @echo insmod modcrypto.ko ivdata=\"valor\" keydata=\"valor\"
	 @echo
clean:
	 rmmod modcrypto
	 make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) clean
	 rm prog
