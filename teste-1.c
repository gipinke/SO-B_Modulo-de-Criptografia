/*
 * teste-1.c - Modulo de Kernel para teste do dispositivo de Kriptografia.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#define DEVICE_NAME "Krypto Teste"
#define CLASS_NAME "Krypto"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Augusto Zambello");
MODULE_DESCRIPTION("Dispositivo de Kriptografia.");
MODULE_VERSION("0.1");

static	int	majorNumber;
static	char	message[256] = {0};
static	short	size_of_message;
static	int	numberOpens = 0;
static	struct	class*	kryptocharClass	= NULL;
static	struct	device*	kryptocharDevice = NULL;

static	int	dev_open(struct	inode *, struct	file *);
static	int	dev_release(struct inode *, struct file *);
static ssize_t dev_read(struct file *, char *, size_t, loff_t *);
static ssize_t dev_write(struct file *, const char *, size_t, loff_t *);

static struct file_operations fops = 
{
	.open = dev_open,
	.read = dev_read,
	.write = dev_write,
	.release = dev_release,
};

static int __init device_init(void){
	printk(KERN_INFO "KRYPTO TESTE: Inicializando o dispositivo\n");

	majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
	if(majorNumber < 0){
		printk(KERN_ALERT "KRYPTO TESTE: Falha ao registrar o major number\n");
		return majorNumber;
	}
	printk(KERN_INFO " KRYPTO TESTE: Registrado corretamente com major number %d\n", majorNumber);

	kryptocharClass = class_create(THIS_MODULE, CLASS_NAME);
	if(IS_ERR(kryptocharClass)){
		unregister_chrdev(majorNumber, DEVICE_NAME);
		printk(KERN_ALERT " KRYPTO TESTE: Falha ao registrar a classe do dispositivo\n");
		return PTR_ERR(kryptocharClass);
	}
	printk(KERN_INFO " KRYPTO TESTE: Classe do dispositivo registrada\n");

	kryptocharDevice = device_create(kryptocharClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
	if(IS_ERR(kryptocharDevice)){
		class_destroy(kryptocharClass);
		unregister_chrdev(majorNumber, DEVICE_NAME);
		printk(KERN_INFO " KRYPTO TESTE: Falha ao criar o dispositivo\n");
		return PTR_ERR(kryptocharDevice);
	}
	printk(KERN_INFO " KRYPTO TESTE: Dispositivo criado\n");
	return 0;
}

static void __exit device_exit(void){
	device_destroy(kryptocharClass, MKDEV(majorNumber, 0));
	class_unregister(kryptocharClass);
	class_destroy(kryptocharClass);
	unregister_chrdev(majorNumber, DEVICE_NAME);
	printk(KERN_INFO "KRYPTO TESTE: Encerrando o dispositivo.\n");
}

static int dev_open(struct inode *inodep, struct file *filep){
	numberOpens++;
	printk(KERN_INFO "KRYPTO TESTE: O dispositivo foi aberto %d vez(es)\n", numberOpens);
	return 0;
}

static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset){
	int error_count = 0;
	error_count = copy_to_user(buffer, message, size_of_message);

	if(error_count == 0){
		printk(KERN_INFO "KRYPTO TESTE: Foram enviados %d caractere(s) para o usuario\n", size_of_message);
		return (size_of_message=0);
	}
	else{
		printk(KERN_INFO "KRYPTO TESTE: Falha ao enviar %d caractere(s) ao usuario\n", error_count);
		return -EFAULT;
	}
}

static ssize_t dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset){
	sprintf(message, "%s(%zu letters)", buffer, len);
	size_of_message = strlen(message);
	printk(KERN_INFO "KRYPTO TESTE: Foi recebido %zu caractere(s) do usuario\n", len);
	return len;
}

static int dev_release(struct inode *inodep, struct file *filep){
	printk(KERN_INFO "KRYPTO TESTE: Dispositivo fechado com sucesso\n");
	return 0;
}

module_init(device_init);
module_exit(device_exit);

