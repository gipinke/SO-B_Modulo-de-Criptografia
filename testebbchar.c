#include <stdio_ext.h>
#include<stdio.h>
#include<stdlib.h>
#include<errno.h>
#include<fcntl.h>
#include<string.h>
#include<unistd.h>
 
#define BUFFER_LENGTH 256               ///< The buffer length (crude but fine)
static char receive[BUFFER_LENGTH];     ///< The receive buffer from the LKM
 
int main()
{
   int ret, fd;
   int opcao  = 0;
   char stringToSend[BUFFER_LENGTH];
   fd = open("/dev/ebbchar", O_RDWR);             // Open the device with read/write access

   if (fd < 0)
   {
      perror("Erro em abrir o device...");
      return errno;
   }
   do
   {
	system("clear");
   	printf("Menu:\n1-Receber String em Hexa\n2-Receber String em ASCII\n3-Sair\nDigite a opção desejada:");
        scanf("%d", &opcao);
         
	if(opcao == 1)
	{	
		 system("clear");
       		 printf("OBS: Escreva o que deseja fazer primeiro respresentado pelas letras:\n");
		 printf("(a) para cifrar\n(d) para decifrar\n(h) para calcular o resumo criptografico\n");
		 printf("Depois disso use o espaço e começe a escrever a string onde a cada 2 numeros, um valor ASCI será representado\n");
 		 __fpurge(stdin);
		 scanf("%[^\n]%*c", stringToSend);                // Read in a string (with spaces)
  		 printf("Escrevendo a mensagem [%s].\n", stringToSend);
  		 ret = write(fd, stringToSend, strlen(stringToSend)); // Send the string to the LKM
  		 
		 if (ret < 0)
		 {
		     	 perror("Failed to write the message to the device.");
		       	 return errno;
	    	 }
 
  		 printf("Press ENTER to read back from the device...\n");
  		 getchar();
 
  		 printf("Reading from the device...\n");
   		 ret = read(fd, receive, BUFFER_LENGTH);        // Read the response from the LKM
   		 if (ret < 0)
		 {
   	   		perror("Failed to read the message from the device.");
   	   		return errno;
   		 }

	   	 printf("The received message is: [%s]\n", receive);
   		 printf("Aperte enter para continuar...\n");
		 getchar();
   	 }

	else if(opcao == 2)
        {
                 system("clear");
                 printf("OBS: Escreva o que deseja fazer primeiro respresentado pelas letras:\n");
                 printf("(a) para cifrar\n(d) para decifrar\n(h) para calcular o resumo criptografico\n");
                 printf("Depois disso use o espaço e começe a escrever a string em ASCI\n");
		 __fpurge(stdin);
                 scanf("%[^\n]%*c", stringToSend);                // Read in a string (with spaces)
                 printf("Writing message to the device [%s].\n", stringToSend);
                 ret = write(fd, stringToSend, strlen(stringToSend)); // Send the string to the LKM
                 
                 if (ret < 0)
                 {
                         perror("Failed to write the message to the device.");
                         return errno;
                 }
 
                 printf("Press ENTER to read back from the device...\n");
                 getchar();
 
                 printf("Reading from the device...\n");
                 ret = read(fd, receive, BUFFER_LENGTH);        // Read the response from the LKM
                 if (ret < 0)
                 {
                        perror("Failed to read the message from the device.");
                        return errno;
                 }

                 printf("The received message is: [%s]\n", receive);
                 printf("Aperte enter para continuar...\n");
                 getchar();
         }       

   }while(opcao != 3);
   return 0;
}
