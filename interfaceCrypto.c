#include <stdio_ext.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <string.h>
 
#define BUFFER_LENGTH 256               ///< The buffer length (crude but fine)

//Metodo achado no stack overflow para converter hexa para string 
void textFromHexString(char *hex,char *result)
{
        char text[BUFFER_LENGTH] = "";
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

void gerarString(int funcao, char *string, char *resultado)
{
        strcpy(resultado, "");
        char auxHEX[256 ],aux[256] = "";
        if(funcao == 1)
        {
                aux[0] = 'c';
        }
        else if(funcao == 2)
        {
                aux[0] = 'd';
        }
        else if(funcao == 3)
        {
                aux[0] = 'h';
        }
        strcat(aux, " ");
        string2hexString(string, auxHEX);
        strcat(aux, auxHEX);
        strcpy(resultado,aux);
}


int main()
{
   int ret, fd; 
   int opcao  = 0, funcao = 0;
   char stringToSend[BUFFER_LENGTH]="";
   char stringToSend2[BUFFER_LENGTH]="";
   char enviarString[BUFFER_LENGTH]="";
   char enviarStringHEX[BUFFER_LENGTH]="";
   char teste[BUFFER_LENGTH]="";
   char teste2[BUFFER_LENGTH]="";
   char receberStringASCII[BUFFER_LENGTH]="";
   char receive[BUFFER_LENGTH]=""; 

   fd = open("/dev/modcrypto", O_RDWR);             // Open the device with read/write access

   if (fd < 0)
   {
      perror("Erro em abrir o device...");
      return errno;
   }
   do
   {
        //strcpy(stringToSend,"");
        memset(stringToSend,0,strlen(stringToSend));
        //strcpy(stringToSend2,"");
        memset(stringToSend2,0,strlen(stringToSend2));
       // strcpy(enviarString,"");
        memset(enviarString,0,strlen(enviarString));
        //strcpy(enviarStringHEX,"");
        memset(enviarStringHEX,0,strlen(enviarStringHEX));
        //strcpy(teste,"");
        memset(teste,0,strlen(teste));
       // strcpy(teste2,"");
        memset(teste2,0,strlen(teste2));
       // strcpy(receberStringASCII,"");
        memset(receberStringASCII,0,strlen(receberStringASCII));
        //strcpy(receive, "");
        memset(receive,0,strlen(receive));

	system("clear");
   	printf("Menu:\n1-Receber String em Hexa\n2-Receber String em ASCII\n3-Sair\nDigite a opção desejada:");
        scanf("%d", &opcao);
         
	if(opcao == 1)
	{	
		system("clear");
       		printf("OBS: Escreva o que deseja fazer primeiro respresentado pelas letras:\n");
		printf("(c) para cifrar\n(d) para decifrar\n(h) para calcular o resumo criptografico\n");
		printf("Depois disso use o espaço e começe a escrever a string onde a cada 2 numeros, um valor ASCII será representado\n");
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

                gerarPalavra(stringToSend, enviarString);
                textFromHexString(enviarString, teste);
                system("clear");

                switch (funcao)
                {
                        case 1:
                                printf("Palavra a ser Criptografada:\nHEX: [ %s ]\nASCII: [ %s ]\n\n",enviarString, teste);
                                printf("Aperte enter para criptografar...\n");
                                getchar();

                                ret = write(fd, stringToSend, strlen(stringToSend)); // Send the string to the LKM
  		 
		                if (ret < 0)
                                {
                                        perror("Failed to write the message to the device.");
                                        return errno;
                                }

                                printf("\nRecendo valor Criptografado...\n");
                                ret = read(fd, receive, BUFFER_LENGTH);        // Read the response from the LKM
                                if (ret < 0)
                                {
                                        perror("Failed to read the message from the device.");
                                        return errno;
                                }

                                textFromHexString(receive, receberStringASCII);

                                printf("\nValor Criptografado:\nHEX: [ %s ]\nASCII: [ %s ]\n", receive, receberStringASCII);
                                printf("Aperte enter para continuar...\n");
                                getchar();

                                break;
                        case 2:
                                printf("Palavra a ser Criptografada:\nHEX: [ %s ]\nASCII: [ %s ]\n\n",enviarString, teste);
                                printf("Aperte enter para descriptografar...\n");
                                getchar();

                                ret = write(fd, stringToSend, strlen(stringToSend)); // Send the string to the LKM
  		 
		                if (ret < 0)
                                {
                                        perror("Failed to write the message to the device.");
                                        return errno;
                                }

                                printf("\nRecendo valor Descriptografado...\n");
                                ret = read(fd, receive, BUFFER_LENGTH);        // Read the response from the LKM
                                if (ret < 0)
                                {
                                        perror("Failed to read the message from the device.");
                                        return errno;
                                }

                                textFromHexString(receive, receberStringASCII);

                                printf("\nValor Descriptografado:\nHEX: [ %s ]\nASCII: [ %s ]\n", receive, receberStringASCII);
                                printf("Aperte enter para continuar...\n");
                                getchar();

                                break;
                        case 3:
                                printf("Palavra a ter o hash calculado:\nHEX: [ %s ]\nASCII: [ %s ]\n\n",enviarString, teste);
                                printf("Aperte enter para calcular hash...\n");
                                getchar();

                                ret = write(fd, stringToSend, strlen(stringToSend)); // Send the string to the LKM
  		 
		                if (ret < 0)
                                {
                                        perror("Failed to write the message to the device.");
                                        return errno;
                                }

                                printf("\nRecendo valor do hash...\n");
                                ret = read(fd, receive, BUFFER_LENGTH);        // Read the response from the LKM
                                if (ret < 0)
                                {
                                        perror("Failed to read the message from the device.");
                                        return errno;
                                }

                                printf("\nValor do Hash:\nHEX: [ %s ]\n", receive);
                                printf("Aperte enter para continuar...\n");
                                getchar();


                                break;
                }
   	}

	else if(opcao == 2)
        {
                system("clear");
                printf("OBS: Escreva o que deseja fazer primeiro respresentado pelas letras:\n");
                printf("(c) para cifrar\n(d) para decifrar\n(h) para calcular o resumo criptografico\n");
                printf("Depois disso use o espaço e começe a escrever a string em ASCII\n");
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

		system("clear");
                gerarPalavra(stringToSend, enviarString);
                gerarString(funcao, enviarString, stringToSend2);
                string2hexString(enviarString, enviarStringHEX);

                switch (funcao)
                {
                        case 1:
                                printf("Palavra a ser Criptografada :\nHEX: [ %s ]\nASCII: [ %s ]\n\n",enviarStringHEX, enviarString);
                                printf("Aperte enter para criptografar...\n");
                                getchar();

                                ret = write(fd, stringToSend2, strlen(stringToSend2)); // Send the string to the LKM
  		 
		                if (ret < 0)
                                {
                                        perror("Failed to write the message to the device.");
                                        return errno;
                                }

                                printf("\nRecendo valor Criptografado...\n");
                                ret = read(fd, receive, BUFFER_LENGTH);        // Read the response from the LKM
                                if (ret < 0)
                                {
                                        perror("Failed to read the message from the device.");
                                        return errno;
                                }

                                textFromHexString(receive, teste2);

                                printf("\nValor Criptografado:\nHEX: [ %s ]\nASCII: [ %s ]\n", receive, teste2);
                                printf("Aperte enter para continuar...\n");
                                getchar();

                                break;
                        case 2:
                                printf("\nPalavra a ser Criptografada :\nHEX: %s\nASCII: %s\n\n",enviarStringHEX, enviarString);
                                printf("Aperte enter para criptografar...\n");
                                getchar();

                                ret = write(fd, stringToSend2, strlen(stringToSend2)); // Send the string to the LKM
  		 
		                if (ret < 0)
                                {
                                        perror("Failed to write the message to the device.");
                                        return errno;
                                }

                                printf("\nRecendo valor Descriptografado...\n");
                                ret = read(fd, receive, BUFFER_LENGTH);        // Read the response from the LKM
                                if (ret < 0)
                                {
                                        perror("Failed to read the message from the device.");
                                        return errno;
                                }

                                textFromHexString(receive, teste2);

                                printf("\nValor Descriptografado:\nHEX: [ %s ]\nASCII: [ %s ]\n", receive, teste2);
                                printf("Aperte enter para continuar...\n");
                                getchar();


                                break;
                        case 3:
                                printf("Palavra a ter o hash calculado:\nHEX: [ %s ]\nASCII: [ %s ]\n\n",enviarStringHEX, enviarString);
                                printf("Aperte enter para calcular hash...\n");
                                getchar();

                                ret = write(fd, stringToSend2, strlen(stringToSend2)); // Send the string to the LKM
  		 
		                if (ret < 0)
                                {
                                        perror("Failed to write the message to the device.");
                                        return errno;
                                }

                                printf("\nRecendo valor do hash...\n");
                                ret = read(fd, receive, BUFFER_LENGTH);        // Read the response from the LKM
                                if (ret < 0)
                                {
                                        perror("Failed to read the message from the device.");
                                        return errno;
                                }

                                printf("\nValor do Hash:\nHEX: [ %s ]\n", receive);
                                printf("Aperte enter para continuar...\n");
                                getchar();

                                break;
                }
        }       

   }while(opcao != 3);
   system("clear");
   return 0;
}

