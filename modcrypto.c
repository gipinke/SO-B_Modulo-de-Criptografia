#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/fs.h> 
#include <linux/uaccess.h>
#include <linux/mutex.h>
#include <linux/crypto.h>
#include <linux/mm.h>
#include <linux/scatterlist.h>
#include <crypto/skcipher.h>
#include <crypto/hash.h>


//-------------------------||--------------||-------------------------//
//-------------------------|| MODULE INFOS ||-------------------------//
//-------------------------||--------------||-------------------------//

#define	DEVICE_NAME "modcrypto"
#define	CLASS_NAME  "modcrypto"
 
MODULE_LICENSE("GPL");           
MODULE_AUTHOR("Giovani Pinke, Leonardo Massak, Augusto Moço");
MODULE_DESCRIPTION("Cryption Module");  
MODULE_VERSION("1.0");


//-------------------------||-----------||-------------------------//
//-------------------------|| VARIABLES ||-------------------------//
//-------------------------||-----------||-------------------------//

#define MAX_MESSAGE_LENGTH	256
#define CRYPT_BLOCK_SIZE	16
 
static int    majorNumber;
static char   message[MAX_MESSAGE_LENGTH] = {0};
static short  size_of_message;
static struct class*  modcryptoClass  = NULL;
static struct device* modcryptoDevice = NULL;

static char *keydata = "0123456789ABCDEF";
static char *ivdata  = "0123456789ABCDEF";

static char *key;

module_param(keydata, charp, 0000);
module_param(ivdata, charp, 0000);

static DEFINE_MUTEX(modcrypto_mutex);


//-------------------------||------------------||-------------------------//
//-------------------------|| FUNCTION HEADERS ||-------------------------//
//-------------------------||------------------||-------------------------//

static int dev_open(struct inode *, struct file *);
static int dev_release(struct inode *, struct file *);
static ssize_t dev_read(struct file *, char *, size_t, loff_t *);
static ssize_t dev_write(struct file *, const char *, size_t, loff_t *);

static int modcrypto_encrypt(char *buff, int len_buff);
static int modcrypto_decrypt(char *buff, int len_buff);
static int modcrypto_hash(char *buff, int len);
static struct sdesc *init_sdesc(struct crypto_shash *alg);
static int calc_hash(struct crypto_shash *alg, const unsigned char *data, unsigned int datalen, unsigned char *digest);
void string2hexString(char* string, char* resultado, int len);
void textFromHexString(char *hex,char *result);
static void split_operation(const char *buffer, char *operation, char *data, int len);

#ifdef DEBUG
static void hexdump(unsigned char *buff, unsigned int len);
#endif


//-------------------------||---------||-------------------------//
//-------------------------|| STRUCTS ||-------------------------//
//-------------------------||---------||-------------------------//
 
static struct file_operations fops =
{
   .open = dev_open,
   .read = dev_read,
   .write = dev_write,
   .release = dev_release,
};

struct sdesc {
    struct shash_desc shash;
    char ctx[];
};


//-------------------------||------------------||-------------------------//
//-------------------------|| MODULE FUNCTIONS ||-------------------------//
//-------------------------||------------------||-------------------------//

static int __init modcrypto_init(void){
	int keysize, ivsize, i;

	printk(KERN_INFO "modcrypto: Initializing modcrypto module\n");

	majorNumber = register_chrdev(0, DEVICE_NAME, &fops);

	if (majorNumber < 0)
	{
		printk(KERN_ALERT "modcrypto: Fail to register major number\n");
		return majorNumber;
	}
	printk(KERN_INFO "modcrypto: Registered correctly with major number %d\n", majorNumber);

	modcryptoClass = class_create(THIS_MODULE, CLASS_NAME);
	if (IS_ERR(modcryptoClass))
	{                
		unregister_chrdev(majorNumber, DEVICE_NAME);
		printk(KERN_ALERT "modcrypto: Failed to register device class\n");
		return PTR_ERR(modcryptoClass);
	}
	printk(KERN_INFO "modcrypto: Device class registered correctly\n");
 
	modcryptoDevice = device_create(modcryptoClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
   
	if (IS_ERR(modcryptoDevice))
	{               
		class_destroy(modcryptoClass);
		unregister_chrdev(majorNumber, DEVICE_NAME);
		printk(KERN_ALERT "modcrypto: Failed to create the device\n");
		return PTR_ERR(modcryptoDevice);
	}

	printk(KERN_INFO "modcrypto: Device class created correctly\n");
	mutex_init(&modcrypto_mutex);


	keysize = strlen(keydata);
	ivsize = strlen(ivdata);

	#ifdef DEBUG
	printk(KERN_INFO "modcrypto: keysize: %d - ivsize: %d\n", keysize, ivsize);
	#endif

	key = kmalloc(CRYPT_BLOCK_SIZE, GFP_KERNEL);

	if(!key){
		printk(KERN_INFO "modcrypto: Failed to allocate key\n");
		return -1;
	}

	for(i = 0; i < CRYPT_BLOCK_SIZE; i++){
		if(i < keysize) key[i] = keydata[i];
		else key[i] = 0;
	}

	key[i] = '\0';

	#ifdef DEBUG
	printk(KERN_INFO "modcrypto: key: %s\n", key);
	#endif

	return 0;
}
 

static void __exit modcrypto_exit(void)
{
	mutex_destroy(&modcrypto_mutex);
	device_destroy(modcryptoClass, MKDEV(majorNumber, 0));
	class_unregister(modcryptoClass);
	class_destroy(modcryptoClass);
	unregister_chrdev(majorNumber, DEVICE_NAME);
	kfree(key);
	printk(KERN_INFO "modcrypto: Device has been successfully removed\n");
}
 

static int dev_open(struct inode *inodep, struct file *filep)
{
	if(!mutex_trylock(&modcrypto_mutex))
	{
		printk(KERN_ALERT "modcrypto: Device is in use by another process");
		return -EBUSY;
	}  

	printk(KERN_INFO "modcrypto: Device successfully opened\n");
	return 0;
}
 

static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset)
{
	int error_count = 0;

	error_count = copy_to_user(buffer, message, size_of_message);

	if (error_count==0)
	{           
		printk(KERN_INFO "modcrypto: Sent %d characters to the user\n", size_of_message);

		return size_of_message;
	}
	else 
	{
		printk(KERN_INFO "modcrypto: Failed to send %d characters to the user\n", error_count);

		return -EFAULT;              
	}
}
 

static ssize_t dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset)
{
	char op, data[MAX_MESSAGE_LENGTH] = {0}, converted_data[MAX_MESSAGE_LENGTH] = {0};
	int len_buff, ret;

	len_buff = strlen(buffer);

	split_operation(buffer, &op, data, len_buff);
	textFromHexString(data, converted_data);
	len_buff = strlen(converted_data);

	switch(op){
		case 'c': 
			ret = modcrypto_encrypt(converted_data, len_buff);
			#ifdef DEBUG
			pr_info("modcrypto: Finished encryption - message: %s\n", message);
			#endif
		break;

		case 'd': 
			ret = modcrypto_decrypt(converted_data, len_buff);
			#ifdef DEBUG
			pr_info("modcrypto: Finished decryption - message: %s\n", message);
			#endif
		break;

		case 'h':
			ret = modcrypto_hash(converted_data, len_buff);
			#ifdef DEBUG
			pr_info("modcrypto: Finished hash - message: %s\n", message);
			#endif
		break;

		default:
			pr_info("modcrypto: Invalid operation type ['%c']\n", op);
			return 0;
		break;
	}

	message[size_of_message] = '\0';

	return len_buff;
}


static int dev_release(struct inode *inodep, struct file *filep)
{
	mutex_unlock(&modcrypto_mutex);

   printk(KERN_INFO "modcrypto: Device successfully closed\n");
   
   return 0;
}


//-------------------------||-----------------||-------------------------//
//-------------------------|| CRYPT FUNCTIONS ||-------------------------//
//-------------------------||-----------------||-------------------------//

static int modcrypto_encrypt(char *buff, int len_buff)
{
	struct crypto_skcipher *skcipher = NULL;
	struct skcipher_request *req = NULL;
	char *_iv = NULL;
	struct scatterlist sg_scratchpad;
	char *scratchpad = NULL;
	struct scatterlist sg_encrypted;
	char *encrypted_buff = NULL;
	char *encrypteddata = NULL;
	int ret = -EFAULT;

	int num_blocks;
	int len_scratchpad, len_ivdata;
	int i;

	pr_info("modcrypto: Initialized Encryption\n");

	len_ivdata = strlen(ivdata);
	_iv = kmalloc(CRYPT_BLOCK_SIZE, GFP_KERNEL);
	for(i = 0; i < CRYPT_BLOCK_SIZE; i++){
		if(i < len_ivdata) _iv[i] = ivdata[i];
		else _iv[i] = 0;
	}
	_iv[i] = '\0';

	#ifdef DEBUG
	pr_info("modcrypto: Loaded iv: %s\n", _iv);
	#endif

	skcipher = crypto_alloc_skcipher("cbc(aes)", 0, 0);
	if (IS_ERR(skcipher)) {
		pr_info("modcrypto: Could not allocate skcipher handle\n");
		return PTR_ERR(skcipher);
	}

	req = skcipher_request_alloc(skcipher, GFP_KERNEL);
	if (!req) {
		pr_info("modcrypto: Could not allocate skcipher request\n");
		ret = -ENOMEM;
		goto out;
	}

	if (crypto_skcipher_setkey(skcipher, key, CRYPT_BLOCK_SIZE)) {
		pr_info("modcrypto: Key could not be set\n");
		ret = -EAGAIN;
		goto out;
	}

	if(len_buff % CRYPT_BLOCK_SIZE)num_blocks = 1 + (len_buff / CRYPT_BLOCK_SIZE);
	else num_blocks = len_buff / CRYPT_BLOCK_SIZE;

	#ifdef DEBUG
	pr_info("modcrypto: Loaded num_blocks: %d\n", num_blocks);
	#endif

	len_scratchpad = num_blocks * CRYPT_BLOCK_SIZE;

	#ifdef DEBUG
	pr_info("modcrypto: Loaded len_scratchpad: %d\n", len_scratchpad);
	#endif

	scratchpad = kmalloc(len_scratchpad, GFP_KERNEL);
	encrypted_buff = kmalloc(len_scratchpad, GFP_KERNEL);
	if (!scratchpad || !encrypted_buff) {
		pr_info("modcrypto: Could not allocate scratchpad or encrypted_buff\n");
		goto out;
	}

	for(i = 0; i < len_scratchpad;i++){
		if(i < len_buff) scratchpad[i] = buff[i];
		else scratchpad[i] = 0;
	}

	#ifdef DEBUG
	pr_info("\n\nmodcrypto: --| Before |--\n\n");
	pr_info("modcrypto: key: %s\n", key);
	pr_info("modcrypto: iv: %s\n", _iv);
	pr_info("modcrypto: scratchpad: %s\n", scratchpad);
	pr_info("modcrypto: encrypteddata: %s", encrypteddata);
	pr_info("\nmodcrypto: --| Before |--\n\n\n");
	#endif

	sg_init_one(&sg_scratchpad, scratchpad, len_scratchpad);
	sg_init_one(&sg_encrypted, encrypted_buff, len_scratchpad);

	skcipher_request_set_crypt(req, &sg_scratchpad, &sg_encrypted, len_scratchpad, _iv);

	ret = crypto_skcipher_encrypt(req);

	if(ret){
		pr_info("modcrypto: Failed to encrypt\n");
		goto out;
	}

	encrypteddata = sg_virt(&sg_encrypted);

	#ifdef DEBUG
	pr_info("modcrypto: ------| CRYPT RESULT |------\n");
	hexdump(encrypteddata, len_scratchpad);
	pr_info("modcrypto: ------| CRYPT RESULT |------\n");
	#endif

	string2hexString(encrypteddata, message, len_scratchpad);
	size_of_message = len_scratchpad*2;

	#ifdef DEBUG
	pr_info("\n\nmodcrypto: --| After |--\n\n");
	pr_info("modcrypto: key: %s\n", key);
	pr_info("modcrypto: iv: %s\n", _iv);
	pr_info("modcrypto: scratchpad: %s\n", scratchpad);
	pr_info("modcrypto: encrypteddata: %s", encrypteddata);
	pr_info("\nmodcrypto: --| After |--\n\n\n");
	#endif

out:
	if (skcipher)
		crypto_free_skcipher(skcipher);
	if (req)
		skcipher_request_free(req);
	if (scratchpad)
		kfree(scratchpad);
	if (_iv)
		kfree(_iv);
	if (encrypted_buff)
		kfree(encrypted_buff);

	return ret;
}


static int modcrypto_decrypt(char *buff, int len_buff)
{
	struct crypto_skcipher *skcipher = NULL;
	struct skcipher_request *req = NULL;
	char *_iv = NULL;
	struct scatterlist sg_scratchpad;
	char *scratchpad = NULL;
	struct scatterlist sg_decrypted;
	char *decrypted_buff = NULL;
	char *decrypteddata = NULL;
	int ret = -EFAULT;

	int num_blocks;
	int len_scratchpad, len_ivdata;
	int i;

	pr_info("modcrypto: Initialized Decription");

	len_ivdata = strlen(ivdata);
	_iv = kmalloc(CRYPT_BLOCK_SIZE, GFP_KERNEL);
	for(i = 0; i < CRYPT_BLOCK_SIZE; i++){
		if(i < len_ivdata) _iv[i] = ivdata[i];
		else _iv[i] = 0;
	}
	_iv[i] = '\0';

	skcipher = crypto_alloc_skcipher("cbc(aes)", 0, 0);
    if (IS_ERR(skcipher)) {
		pr_info("modcrypto: Could not allocate skcipher handle\n");
    	return PTR_ERR(skcipher);
	}

	req = skcipher_request_alloc(skcipher, GFP_KERNEL);
	if (!req) {
		pr_info("modcrypto: Could not allocate skcipher request\n");
		ret = -ENOMEM;
		goto out;
	}

	if (crypto_skcipher_setkey(skcipher, key, CRYPT_BLOCK_SIZE)) {
		pr_info("modcrypto: Key could not be set\n");
		ret = -EAGAIN;
		goto out;
	}
	 
	if(len_buff % CRYPT_BLOCK_SIZE)num_blocks = 1 + (len_buff / CRYPT_BLOCK_SIZE);
	else num_blocks = len_buff / CRYPT_BLOCK_SIZE;

	#ifdef DEBUG
	pr_info("modcrypto: Loaded num_blocks: %d\n", num_blocks);
	#endif

	len_scratchpad = num_blocks * CRYPT_BLOCK_SIZE;

	#ifdef DEBUG
	pr_info("modcrypto: Loaded len_scratchpad: %d\n", len_scratchpad);
	#endif

	scratchpad = kmalloc(len_scratchpad, GFP_KERNEL);
	decrypted_buff = kmalloc(len_scratchpad, GFP_KERNEL);
	if (!scratchpad || !decrypted_buff) {
		pr_info("modcrypto: Could not allocate scratchpad or decrypted_buff\n");
		goto out;
	}

	for(i = 0; i < len_scratchpad;i++){
		if(i < len_buff) scratchpad[i] = buff[i];
		else scratchpad[i] = 0;
	}

	#ifdef DEBUG
	pr_info("\n\nmodcrypto: --| Before |--\n\n");
	pr_info("modcrypto: key: %s\n", key);
	pr_info("modcrypto: iv: %s\n", _iv);
	pr_info("modcrypto: scratchpad: %s\n", scratchpad);
	pr_info("modcrypto: decrypteddata: %s", decrypteddata);
	pr_info("\nmodcrypto: --| Before |--\n\n\n");
	#endif

	sg_init_one(&sg_scratchpad, scratchpad, len_scratchpad);
	sg_init_one(&sg_decrypted, decrypted_buff, len_scratchpad);

	skcipher_request_set_crypt(req, &sg_scratchpad, &sg_decrypted, len_scratchpad, _iv);

	ret = crypto_skcipher_decrypt(req);

	if(ret){
		pr_info("modcrypto: Failed to decrypt\n");
		goto out;
	}

	decrypteddata = sg_virt(&sg_decrypted);

	#ifdef DEBUG
	pr_info("modcrypto: ------| DECRYPT RESULT |------\n");
	hexdump(decrypteddata, len_scratchpad);
	pr_info("modcrypto: ------| DECRYPT RESULT |------\n");
	#endif

	string2hexString(decrypteddata, message, len_scratchpad);
	size_of_message = len_scratchpad*2;

	#ifdef DEBUG
	pr_info("\n\nmodcrypto: --| After |--\n\n");
	pr_info("modcrypto: key: %s\n", key);
	pr_info("modcrypto: iv: %s\n", _iv);
	pr_info("modcrypto: scratchpad: %s\n", scratchpad);
	pr_info("modcrypto: decrypteddata: %s", decrypteddata);
	pr_info("\nmodcrypto: --| After |--\n\n\n");
	#endif

out:
	if (skcipher)
		crypto_free_skcipher(skcipher);
	if (req)
		skcipher_request_free(req);
	if (scratchpad)
		kfree(scratchpad);
	if (_iv)
		kfree(_iv);
	if (decrypted_buff)
		kfree(decrypted_buff);

	return ret;
}


static int modcrypto_hash(char *buff, int len)
{
	struct crypto_shash *alg;
	unsigned char *hashval, *scratchpad;
	int ret, i;

	hashval = kmalloc(sizeof(unsigned char) * 20, GFP_KERNEL);
	scratchpad = kmalloc(sizeof(unsigned char) * 20, GFP_KERNEL);

	for(i = 0; i < len; i++) scratchpad[i] = buff[i];
	scratchpad[i] = '\0';

	alg = crypto_alloc_shash("sha1", 0, 0);
	if(IS_ERR(alg)){
		pr_info("modcrypto: Failed to set algorithm\n");
		return PTR_ERR(alg);
	}

	ret = calc_hash(alg, scratchpad, len, hashval);

	string2hexString(hashval, message, 20);
	size_of_message = 40;

	crypto_free_shash(alg);
	if(hashval)
		kfree(hashval);
	if(scratchpad)
		kfree(scratchpad);

	return ret;
}


//-------------------------||------------------||-------------------------//
//-------------------------|| HELPER FUNCTIONS ||-------------------------//
//-------------------------||------------------||-------------------------//

static struct sdesc *init_sdesc(struct crypto_shash *alg)
{
    struct sdesc *sdesc;
    int size;

    size = sizeof(struct shash_desc) + crypto_shash_descsize(alg);
    sdesc = kmalloc(size, GFP_KERNEL);
    if (!sdesc)
        return ERR_PTR(-ENOMEM);
    sdesc->shash.tfm = alg;
    return sdesc;
}


static int calc_hash(struct crypto_shash *alg, const unsigned char *data, unsigned int datalen, unsigned char *digest)
{
    struct sdesc *sdesc;
    int ret;

    sdesc = init_sdesc(alg);
    if (IS_ERR(sdesc)) {
        pr_info("modcrypto: Can't alloc sdesc\n");
        return PTR_ERR(sdesc);
    }

    ret = crypto_shash_digest(&sdesc->shash, data, datalen, digest);

    kfree(sdesc);
    return ret;
}

#ifdef DEBUG
static void hexdump(unsigned char *buff, unsigned int len)
{
	 unsigned char *aux = buff;
	 printk(KERN_INFO "modcrypto: HEXDUMP:\n");
	 while(len--) { printk(KERN_CONT "%02x[%c] ", *aux, *aux); aux++; }
	 printk("\n");
}
#endif


void textFromHexString(char *hex,char *result)
{
    char text[MAX_MESSAGE_LENGTH] = {0};
    int tc=0, k;
	char temp[3];
	long number;
                 
    for(k=0;k<strlen(hex);k++)
    {
		if(k%2!=0)
		{
			sprintf(temp,"%c%c",hex[k-1],hex[k]);
			kstrtol(temp, CRYPT_BLOCK_SIZE, &number);
			text[tc] = (char)number;
			
			tc++;   
		}
    } 
    strcpy(result,text);
}


void string2hexString(char* string, char* resultado, int len)
{
	unsigned char *aux = string;
	int i = 0;
	while(len--) { sprintf(resultado+i, "%02x", *aux); aux++; i+=2; }
	resultado[i] = 0;
}


static void split_operation(const char *buffer, char *operation, char *data, int len){
	int i = 0;	

	*operation = buffer[0];

	for(i = 2; i < len; i++){
		data[i-2] = buffer[i];
	}
}


//-------------------------||--------------------------||-------------------------//
//-------------------------|| MODULE DEFAULT FUNCTIONS ||-------------------------//
//-------------------------||--------------------------||-------------------------//

module_init(modcrypto_init);
module_exit(modcrypto_exit);