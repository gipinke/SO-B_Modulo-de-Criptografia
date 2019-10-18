#include <stdio_ext.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <string.h>
 
#define BUFFER_LENGTH 256               ///< The buffer length (crude but fine)
static char receive[BUFFER_LENGTH];     ///< The receive buffer from the LKM

//Metodo achado no stack overflow para converter hexa para string 
void textFromHexString(char *hex,char *result)
{
        char text[BUFFER_LENGTH];
        int tc=0;
                     
        for(int k=0;k<strlen(hex);k++)
        {
                if(k%2!=0)
                {
                        char temp[3];
                        sprintf(temp,"%c%c",hex[k-1],hex[k]);
                        int number = (int)strtol(temp, NULL, 16);
                        text[tc] = (char)number;
                        
                        tc++;   
                }
        } 
        strcpy(result,text);
        }

//converter string para hex 
void string2hexString(char* string, char* resultado)
{
    int loop;
    int i; 
    
    i=0;
    loop=0;
    
    while(string[loop] != '\0')
    {
        sprintf((char*)(resultado+i),"%02X", string[loop]);
        loop+=1;
        i+=2;
    }

    resultado[i++] = '\0';
}
 
//Passar para o Espaço de Kernel! 
int funcionalidade(char *string)
{
        if('c' == string[0])
        {
                return 1;    //criptografar
        }
        else if('d' == string[0])
        {
                return 2;    //descriptografar
        }
        else if('h' == string[0])
        {
                return 3;    //calcular hash
        }

        return -1;
}

//Passar para o Espaço de Kernel! 
void gerarPalavra(char *string, char *resultado)
{
        int tamanho = 0;
        int i;
        tamanho = strlen(string);
        for(i = 2; i < tamanho; i++)
        {
                resultado[i-2] = string[i];
        }
        resultado[i-2] = '\0';
}


int main()
{
   int ret, fd;
   int opcao  = 0, funcao = 0;
   char stringToSend[BUFFER_LENGTH];
   char enviarString[BUFFER_LENGTH];
   char enviarStringHEX[BUFFER_LENGTH];
   char teste[BUFFER_LENGTH];
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
		printf("(c) para cifrar\n(d) para decifrar\n(h) para calcular o resumo criptografico\n");
		printf("Depois disso use o espaço e começe a escrever a string onde a cada 2 numeros, um valor ASCI será representado\n");
 		__fpurge(stdin);
		scanf("%[^\n]%*c", stringToSend);                // Read in a string (with spaces)

                funcao = funcionalidade(stringToSend);
                while(funcao == -1)
                {
                        printf("Opcao inexistente! Digite novamente:\n");
 		        __fpurge(stdin);
		        scanf("%[^\n]%*c", stringToSend);                // Read in a string (with spaces)
                        funcao = funcionalidade(stringToSend);
                }

                switch (funcao)
                {
                        case 1:
                                gerarPalavra(stringToSend, enviarString);
                                textFromHexString(enviarString, teste);


                                printf("\nPalavra a ser Criptografada :\nHEX: %s\nASCII: %s\n\n",enviarString, teste);
                                printf("Aperte enter para criptografar...\n");
                                getchar();

                                ret = write(fd, stringToSend, strlen(stringToSend)); // Send the string to the LKM
  		 
		                if (ret < 0)
                                {
                                        perror("Failed to write the message to the device.");
                                        return errno;
                                }

                                break;
                        case 2:
                                gerarPalavra(stringToSend, enviarString);
                                textFromHexString(enviarString, teste);

                                 printf("\nPalavra a ser Criptografada :\nHEX: %s\nASCII: %s\n\n",enviarString, teste);
                                printf("Aperte enter para descriptografar...\n");
                                getchar();

                                ret = write(fd, stringToSend, strlen(stringToSend)); // Send the string to the LKM
  		 
		                if (ret < 0)
                                {
                                        perror("Failed to write the message to the device.");
                                        return errno;
                                }

                                break;
                        case 3:
                                gerarPalavra(stringToSend, enviarString);
                                textFromHexString(enviarString, teste);

                                 printf("\nPalavra a ser Criptografada :\nHEX: %s\nASCII: %s\n\n",enviarString, teste);
                                printf("Aperte enter para calcular hash...\n");
                                getchar();

                                ret = write(fd, stringToSend, strlen(stringToSend)); // Send the string to the LKM
  		 
		                if (ret < 0)
                                {
                                        perror("Failed to write the message to the device.");
                                        return errno;
                                }

                                break;
                }

  		/*
  		printf("Reading from the device...\n");
   		ret = read(fd, receive, BUFFER_LENGTH);        // Read the response from the LKM
   		if (ret < 0)
		{
   	   	        perror("Failed to read the message from the device.");
   	   		return errno;
   		}

   	        printf("The received message is: [%s]\n", receive);
   		printf("Aperte enter para continuar...\n");
		getchar();*/
   	}

	else if(opcao == 2)
        {
                system("clear");
                printf("OBS: Escreva o que deseja fazer primeiro respresentado pelas letras:\n");
                printf("(c) para cifrar\n(d) para decifrar\n(h) para calcular o resumo criptografico\n");
                printf("Depois disso use o espaço e começe a escrever a string em ASCI\n");
		__fpurge(stdin);
                scanf("%[^\n]%*c", stringToSend);                // Read in a string (with spaces)

                funcao = funcionalidade(stringToSend);
                while(funcao == -1)
                {
                        printf("Opcao inexistente! Digite novamente:\n");
 		        __fpurge(stdin);
		        scanf("%[^\n]%*c", stringToSend);                // Read in a string (with spaces)
                        funcao = funcionalidade(stringToSend);
                }

                switch (funcao)
                {
                        case 1:
                                gerarPalavra(stringToSend, enviarString);
                                string2hexString(enviarString, enviarStringHEX);

                                printf("Palavra a ser Criptografada : %s %s\n",enviarString, enviarStringHEX);
                                printf("Aperte enter para criptografar...\n");
                                getchar();
                                break;
                        case 2:
                                gerarPalavra(stringToSend, enviarString);
                                string2hexString(enviarString, enviarStringHEX);

                                printf("Palavra a ser Criptografada : %s %s\n",enviarString, enviarStringHEX);
                                printf("Aperte enter para criptografar...\n");
                                getchar();
                                break;
                        case 3:
                                gerarPalavra(stringToSend, enviarString);
                                string2hexString(enviarString, enviarStringHEX);

                                printf("Palavra a ser Criptografada : %s %s\n",enviarString, enviarStringHEX);
                                printf("Aperte enter para criptografar...\n");
                                getchar();
                                break;
                }
                /*printf("Writing message to the device [%s].\n", stringToSend);
                ret = write(fd, stringToSend, strlen(stringToSend)); // Send the string to the LKM
                 
                if (ret < 0)
                {
                        perror("Failed to write the message to the device.");
                        return errno;                }
 
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
                getchar();*/
        }       

   }while(opcao != 3);
   return 0;
}

