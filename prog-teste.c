#include<stdio.h>
#include<stdlib.h>
#include<errno.h>
#include<fcntl.h>
#include<string.h>
#include<unistd.h>
 
#define BUFFER_LENGTH 256
static char receive[BUFFER_LENGTH];
 
int main(){
   int ret, fd;
   char stringToSend[BUFFER_LENGTH];
   printf("Iniciando o programa de teste do dispositivo...\n");
   fd = open("/dev/Krypto Teste", O_RDWR);
   if (fd < 0){
      perror("Falha ao abrir o dispositivo...");
      return errno;
   }
   printf("Digite uma frase para enviar ao modulo do kernel:\n");
   scanf("%[^\n]%*c", stringToSend);      
   printf("Escrevendo mensagem no dispositivo [%s].\n", stringToSend);
   ret = write(fd, stringToSend, strlen(stringToSend));
   if (ret < 0){
      perror("Falha ao escrever mensagem no dispositivo.");
      return errno;
   }
 
   printf("Precione ENTER para ler de volta do dispositivo...\n");
   getchar();
 
   printf("Lendo do dispositivo...\n");
   ret = read(fd, receive, BUFFER_LENGTH);
   if (ret < 0){
      perror("Falha ao ler a mensagem do dispositivo.");
      return errno;
   }
   printf("Mensagem Recebida: [%s]\n", receive);
   printf("Fim do programa de teste\n");
   return 0;
}
