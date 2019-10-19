#include <linux/init.h>           // Macros used to mark up functions e.g. __init __exit
#include <linux/module.h>         // Core header for loading LKMs into the kernel
#include <linux/moduleparam.h>
#include <linux/device.h>         // Header to support the kernel Driver Model
#include <linux/kernel.h>         // Contains types, macros, functions for the kernel
#include <linux/fs.h>             // Header for the Linux file system support
#include <linux/uaccess.h>        // Required for the copy to user function
#include <linux/mutex.h>	  // Required for the mutex functionality
#include <linux/crypto.h>
#include <linux/mm.h>
#include <linux/scatterlist.h>
#include <crypto/skcipher.h>

#define  DEVICE_NAME "ebbchar"    ///< The device will appear at /dev/ebbchar using this value
#define  CLASS_NAME  "ebb"        ///< The device class -- this is a character device driver
 
MODULE_LICENSE("GPL");           
MODULE_AUTHOR("Giovani Pinke, Leonardo Massak, Augusto Moço");
MODULE_DESCRIPTION("Versao 1 do modulo de criptografia");  
MODULE_VERSION("0.1");

struct skcipher_def{
	struct scatterlist sg;
	struct crypto_skcipher *tfm;
	struct skcipher_request *req;
};
 
static int    majorNumber;                  ///< Stores the device number -- determined automatically
static char   message[256] = {0};           ///< Memory for the string that is passed from userspace
static short  size_of_message;              ///< Used to remember the size of the string stored
static int    numberOpens = 0;              ///< Counts the number of times the device is opened
static struct class*  ebbcharClass  = NULL; ///< The device-driver class struct pointer
static struct device* ebbcharDevice = NULL; ///< The device-driver device struct pointer


static char *keydata = "0123456789ABCDEF";
static char *ivdata  = "0123456789ABCDEF";

static char *key;
static char *iv;

module_param(keydata, charp, 0000);
module_param(ivdata, charp, 0000);

 
// The prototype functions for the character driver -- must come before the struct definition
static int     dev_open(struct inode *, struct file *);
static int     dev_release(struct inode *, struct file *);
static ssize_t dev_read(struct file *, char *, size_t, loff_t *);
static ssize_t dev_write(struct file *, const char *, size_t, loff_t *);

static int modcrypto_encrypt(char *buff, int len_buff);
static int modcrypto_decrypt(char *buff, int len_buff);

static DEFINE_MUTEX(ebbchar_mutex);  /// A macro that is used to declare a new mutex that is visible in this file
                                     /// results in a semaphore variable ebbchar_mutex with value 1 (unlocked)
                                     /// DEFINE_MUTEX_LOCKED() results in a variable with value 0 (locked)
 
/** @brief Devices are represented as file structure in the kernel. The file_operations structure from
 *  /linux/fs.h lists the callback functions that you wish to associated with your file operations
 *  using a C99 syntax structure. char devices usually implement open, read, write and release calls
 */
static struct file_operations fops =
{
   .open = dev_open,
   .read = dev_read,
   .write = dev_write,
   .release = dev_release,
};

static void hexdump(unsigned char *buff, unsigned int len)
{
	 unsigned char *aux = buff;
	 printk(KERN_INFO "HEXDUMP:\n");
	 while(len--) { printk(KERN_CONT "%02x[%c] ", *aux, *aux); aux++; }
	 printk("\n");
}

void textFromHexString(char *hex,char *result)
{
    char text[256] = {0};
    int tc=0, k;
		char temp[3];
		long number;
                 
    for(k=0;k<strlen(hex);k++)
    {
            if(k%2!=0)
            {
                    sprintf(temp,"%c%c",hex[k-1],hex[k]);
                    kstrtol(temp, 16, &number);
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
 
/** @brief The LKM initialization function
 *  The static keyword restricts the visibility of the function to within this C file. The __init
 *  macro means that for a built-in driver (not a LKM) the function is only used at initialization
 *  time and that it can be discarded and its memory freed up after that point.
 *  @return returns 0 if successful
 */
static int __init ebbchar_init(void){
	 int keysize, ivsize, i;

   printk(KERN_INFO "EBBChar: Initializing the EBBChar LKM\n");
 
   // Try to dynamically allocate a major number for the device -- more difficult but worth it
   majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
   
   if (majorNumber < 0)
   {
      printk(KERN_ALERT "EBBChar failed to register a major number\n");
      return majorNumber;
   }
   printk(KERN_INFO "EBBChar: registered correctly with major number %d\n", majorNumber);
 
   // Register the device class
   ebbcharClass = class_create(THIS_MODULE, CLASS_NAME);
   if (IS_ERR(ebbcharClass))
   {                
      unregister_chrdev(majorNumber, DEVICE_NAME);
      printk(KERN_ALERT "Failed to register device class\n");
      return PTR_ERR(ebbcharClass);          // Correct way to return an error on a pointer
   }
   printk(KERN_INFO "EBBChar: device class registered correctly\n");
 
   // Register the device driver
   ebbcharDevice = device_create(ebbcharClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
   
   if (IS_ERR(ebbcharDevice))	// Clean up if there is an error
   {               
      class_destroy(ebbcharClass);           // Repeated code but the alternative is goto statements
      unregister_chrdev(majorNumber, DEVICE_NAME);
      printk(KERN_ALERT "Failed to create the device\n");
      return PTR_ERR(ebbcharDevice);
   }

   printk(KERN_INFO "EBBChar: device class created correctly\n"); // Made it! device was initialized
   mutex_init(&ebbchar_mutex);       /// Initialize the mutex lock dynamically at runtime


	 //Get the keydata and the ivdata

	 keysize = strlen(keydata);
	 ivsize = strlen(ivdata);

	 printk(KERN_INFO "EBBChar: keysize: %d - ivsize: %d\n", keysize, ivsize);

	 key = kmalloc(16, GFP_KERNEL);
	 iv = kmalloc(16, GFP_KERNEL);

	 if(!key || !iv){
		 printk(KERN_INFO "EBBChar: Não foi possível alocar a key ou o iv\n");
		 return -1;
	 }

	 for(i = 0; i < 16; i++){
		 if(i < keysize) key[i] = keydata[i];
		 else key[i] = '0';

		 if(i < ivsize) iv[i] = ivdata[i];
		 else iv[i] = 0;

	 }
	 key[i] = '\0';
	 iv[i] = '\0';

	 printk(KERN_INFO "EBBChar: key: %s - iv: %s\n", key, iv);

   return 0;
}
 
/** @brief The LKM cleanup function
 *  Similar to the initialization function, it is static. The __exit macro notifies that if this
 *  code is used for a built-in driver (not a LKM) that this function is not required.
 */
static void __exit ebbchar_exit(void)
{
   mutex_destroy(&ebbchar_mutex);        		    //destroy the dynamically-allocated mutex
   device_destroy(ebbcharClass, MKDEV(majorNumber, 0));     // remove the device
   class_unregister(ebbcharClass);                          // unregister the device class
   class_destroy(ebbcharClass);                             // remove the device class
   unregister_chrdev(majorNumber, DEVICE_NAME);             // unregister the major number
	 kfree(key);
	 kfree(iv);
   printk(KERN_INFO "EBBChar: Goodbye from the LKM!\n");
}
 
/** @brief The device open function that is called each time the device is opened
 *  This will only increment the numberOpens counter in this case.
 *  @param inodep A pointer to an inode object (defined in linux/fs.h)
 *  @param filep A pointer to a file object (defined in linux/fs.h)
 */
static int dev_open(struct inode *inodep, struct file *filep)
{
   if(!mutex_trylock(&ebbchar_mutex))	// Try to acquire the mutex (i.e., put the lock on/down)
   {					// return 1 if sucessful and 0 if there is contention
      printk(KERN_ALERT "EBBChar: Device in use by another process");
      return -EBUSY;
   }  
   numberOpens++;
   printk(KERN_INFO "EBBChar: Device has been opened %d time(s)\n", numberOpens);
   return 0;
}
 
/** @brief This function is called whenever device is being read from user space i.e. data is
 *  being sent from the device to the user. In this case is uses the copy_to_user() function to
 *  send the buffer string to the user and captures any errors.
 *  @param filep A pointer to a file object (defined in linux/fs.h)
 *  @param buffer The pointer to the buffer to which this function writes the data
 *  @param len The length of the b
 *  @param offset The offset if required
 */
static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset)
{
   int error_count = 0;
   // copy_to_user has the format ( * to, *from, size) and returns 0 on success
   error_count = copy_to_user(buffer, message, size_of_message);
 
   if (error_count==0)	// if true then have sucess
   {           
      printk(KERN_INFO "EBBChar: Sent %d characters to the user\n", size_of_message);
      return (size_of_message=0);  // clear the position to the start and return 0
   }
   else 
   {
      printk(KERN_INFO "EBBChar: Failed to send %d characters to the user\n", error_count);
      return -EFAULT;              // Failed -- return a bad address message (i.e. -14)
   }
}
 
/** @brief This function is called whenever the device is being written to from user space i.e.
 *  data is sent to the device from the user. The data is copied to the message[] array in this
 *  LKM using the sprintf() function along with the length of the string.
 *  @param filep A pointer to a file object
 *  @param buffer The buffer to that contains the string to write to the device
 *  @param len The length of the array of data that is being passed in the const char buffer
 *  @param offset The offset if required
 */
static ssize_t dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset)
{
   char op, buff[256] = {0}, buff_test[256] = {0};
	 int len_buff, ret;

 
   len_buff = strlen(buffer);

	 split_operation(buffer, &op, buff, len_buff);
	 textFromHexString(buff, buff_test);
	 len_buff = strlen(buff_test);

		pr_info("EBBChar: ------| BUFF RESULT |------\n");
 		hexdump(buff_test, len_buff);
 		pr_info("EBBChar: ------| BUFF RESULT |------\n");

	 switch(op){
      case 'c': //Cripografia

		 	ret = modcrypto_encrypt(buff_test, len_buff);

			pr_info("EBBChar: Criptografia concluida - message: %s\n", message);


			//ret = modcrypto_decrypt(message, size_of_message);

			//pr_info("EBBChar: Descriptografia concluida - message: %s\n", message);

      break;


      case 'd': //Descriptografia

		 	ret = modcrypto_decrypt(buff_test, len_buff);

			pr_info("EBBChar: Descriptografia concluida - message: %s\n", message);

      break;


      case 'h': //Resumo Criptográfico -- Não entendi direito

      break;


      default: //Erro - nenhuma das opções possíveis
         return 0;
      break;
   }

   return len_buff;
}

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

	 len_ivdata = strlen(ivdata);
	 _iv = kmalloc(16, GFP_KERNEL);
	 for(i = 0; i < 16; i++){
		 if(i < len_ivdata) _iv[i] = ivdata[i];
		 else _iv[i] = 0;

	 }
	 _iv[i] = '\0';

	 skcipher = crypto_alloc_skcipher("cbc(aes)", 0, 0);
   if (IS_ERR(skcipher)) {
		 pr_info("could not allocate skcipher handle\n");
     return PTR_ERR(skcipher);
   }

   req = skcipher_request_alloc(skcipher, GFP_KERNEL);
   	 if (!req) {
			 pr_info("could not allocate skcipher request\n");
       ret = -ENOMEM;
       goto out;
	 }

	 if (crypto_skcipher_setkey(skcipher, key, 16)) {
       pr_info("key could not be set\n");
       ret = -EAGAIN;
       goto out;
   }
	 
	 if(len_buff % 16)num_blocks = 1 + (len_buff / 16);
	 else num_blocks = len_buff / 16;

	 len_scratchpad = num_blocks * 16;

	 scratchpad = kmalloc(len_scratchpad, GFP_KERNEL);
	 encrypted_buff = kmalloc(len_scratchpad, GFP_KERNEL);
   if (!scratchpad || !encrypted_buff) {
   	 pr_info("could not allocate scratchpad\n");
     goto out;
   }

	 for(i = 0; i < len_scratchpad;i++){
		 if(i < len_buff) scratchpad[i] = buff[i];
		 else scratchpad[i] = 0;
	 }

	 pr_info("\n\nEBBChar: --| Before |--\n\n");
	 pr_info("EBBChar: key: %s\n", key);
	 pr_info("EBBChar: iv: %s\n", iv);
	 pr_info("EBBChar: scratchpad: %s\n", scratchpad);
	 pr_info("EBBChar: encrypteddata: %s", encrypteddata);
	 pr_info("\nEBBChar: --| Before |--\n\n\n");

	
	 sg_init_one(&sg_scratchpad, scratchpad, len_scratchpad);
	 sg_init_one(&sg_encrypted, encrypted_buff, len_scratchpad);

	 skcipher_request_set_crypt(req, &sg_scratchpad, &sg_encrypted, len_scratchpad, _iv);

	 ret = crypto_skcipher_encrypt(req);

	 if(ret){
		 pr_info("EBBChar: Falha na criptografia\n");
		 goto out;
	 }

	 encrypteddata = sg_virt(&sg_encrypted);

	 pr_info("EBBChar: ------| CRYPTO RESULT |------\n");
	 hexdump(encrypteddata, len_scratchpad);
	 pr_info("EBBChar: ------| CRYPTO RESULT |------\n");
	 
	 //for(i = 0; i < len_scratchpad; i++) message[i] = encrypteddata[i];
	 //message[i] = 0;

	 string2hexString(encrypteddata, message, len_scratchpad);

	 size_of_message = len_scratchpad*2;

	 pr_info("\n\nEBBChar: --| After |--\n\n");
	 pr_info("EBBChar: key: %s\n", key);
	 pr_info("EBBChar: iv: %s\n", iv);
	 pr_info("EBBChar: scratchpad: %s\n", scratchpad);
	 pr_info("EBBChar: encrypteddata: %s", encrypteddata);
	 pr_info("\nEBBChar: --| After |--\n\n\n");

out:
	if (skcipher)
      crypto_free_skcipher(skcipher);
  if (req)
      skcipher_request_free(req);
  if (scratchpad)
      kfree(scratchpad);
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

	 len_ivdata = strlen(ivdata);
	 _iv = kmalloc(16, GFP_KERNEL);
	 for(i = 0; i < 16; i++){
		 if(i < len_ivdata) _iv[i] = ivdata[i];
		 else _iv[i] = 0;

	 }
	 _iv[i] = '\0';

	 skcipher = crypto_alloc_skcipher("cbc(aes)", 0, 0);
   if (IS_ERR(skcipher)) {
		 pr_info("could not allocate skcipher handle\n");
     return PTR_ERR(skcipher);
   }

   req = skcipher_request_alloc(skcipher, GFP_KERNEL);
   	 if (!req) {
			 pr_info("could not allocate skcipher request\n");
       ret = -ENOMEM;
       goto out;
	 }

	 if (crypto_skcipher_setkey(skcipher, key, 16)) {
       pr_info("key could not be set\n");
       ret = -EAGAIN;
       goto out;
   }
	 
	 if(len_buff % 16)num_blocks = 1 + (len_buff / 16);
	 else num_blocks = len_buff / 16;

	 len_scratchpad = num_blocks * 16;

	 scratchpad = kmalloc(len_scratchpad, GFP_KERNEL);
	 decrypted_buff = kmalloc(len_scratchpad, GFP_KERNEL);
   if (!scratchpad || !decrypted_buff) {
   	 pr_info("could not allocate scratchpad\n");
     goto out;
   }

	 for(i = 0; i < len_scratchpad;i++){
		 if(i < len_buff) scratchpad[i] = buff[i];
		 else scratchpad[i] = 0;
	 }

	 pr_info("\n\nEBBChar: --| Before |--\n\n");
	 pr_info("EBBChar: key: %s\n", key);
	 pr_info("EBBChar: iv: %s\n", iv);
	 pr_info("EBBChar: scratchpad: %s\n", scratchpad);
	 pr_info("EBBChar: decrypteddata: %s", decrypteddata);
	 pr_info("\nEBBChar: --| Before |--\n\n\n");

	
	 sg_init_one(&sg_scratchpad, scratchpad, len_scratchpad);
	 sg_init_one(&sg_decrypted, decrypted_buff, len_scratchpad);

	 skcipher_request_set_crypt(req, &sg_scratchpad, &sg_decrypted, len_scratchpad, _iv);

	 ret = crypto_skcipher_decrypt(req);

	 if(ret){
		 pr_info("EBBChar: Falha na descriptografia\n");
		 goto out;
	 }

	 decrypteddata = sg_virt(&sg_decrypted);

	 pr_info("EBBChar: ------| DECRYPTO RESULT |------\n");
	 hexdump(decrypteddata, len_scratchpad);
	 pr_info("EBBChar: ------| DECRYPTO RESULT |------\n");
	 
	 //for(i = 0; i < len_scratchpad; i++) message[i] = decrypteddata[i];
	 //message[i] = 0;

	 string2hexString(decrypteddata, message, len_scratchpad);

	 size_of_message = len_scratchpad*2;

	 pr_info("\n\nEBBChar: --| After |--\n\n");
	 pr_info("EBBChar: key: %s\n", key);
	 pr_info("EBBChar: iv: %s\n", iv);
	 pr_info("EBBChar: scratchpad: %s\n", scratchpad);
	 pr_info("EBBChar: decrypteddata: %s", decrypteddata);
	 pr_info("\nEBBChar: --| After |--\n\n\n");

out:
	if (skcipher)
      crypto_free_skcipher(skcipher);
  if (req)
      skcipher_request_free(req);
  if (scratchpad)
      kfree(scratchpad);
	if (decrypted_buff)
			kfree(decrypted_buff);

	return ret;

}
 
/** @brief The device release function that is called whenever the device is closed/released by
 *  the userspace program
 *  @param inodep A pointer to an inode object (defined in linux/fs.h)
 *  @param filep A pointer to a file object (defined in linux/fs.h)
 */
static int dev_release(struct inode *inodep, struct file *filep)
{
   mutex_unlock(&ebbchar_mutex);          /// Releases the mutex (i.e., the lock goes up)
   printk(KERN_INFO "EBBChar: Device successfully closed\n");
   return 0;
}
 
/** @brief A module must use the module_init() module_exit() macros from linux/init.h, which
 *  identify the initialization function at insertion time and the cleanup function (as
 *  listed above)
 */
module_init(ebbchar_init);
module_exit(ebbchar_exit);
