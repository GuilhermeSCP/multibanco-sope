#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <termios.h>
#include <unistd.h>

#define maxPINlen 5
char myFIFO[20] = "";
char msgPID[100] = "Admin (PID=";

char** divideMensagem(char * mensagem)
{
	char** data = (char**) malloc(sizeof(char*)*500);
	char * pch;
	int comp=0;

	pch = strtok (mensagem,"|");
	do
	{
		pch = strtok (NULL,"|");
		data[comp] = pch;
		comp++;
	}while (pch != NULL);
	return data;
}

void escreveNoLog(char * mensagem)
{
	FILE * in, * out;
	int flag = 0;

	in = fopen("logfile.txt","r");
	if(!in)
	{
		flag= 1;
	}
	if(flag)
	{
		out = fopen("logfile.txt","wb");
		fprintf(out,"%7s%10s%11s%10s","DATA","HORA","PROGRAMA","OPERACAO");
	}
	else
	{
		out = fopen("logfile.txt","a");
	}
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);
	fprintf(out,"\n%d-%02d-%d",tm.tm_year +1900, tm.tm_mon +1 , tm.tm_mday);
	fprintf(out," %02d:%02d:%02d",tm.tm_hour, tm.tm_min , tm.tm_sec);
	fprintf(out," CLIENT    %s",mensagem);
	fclose(out);
}

void verificaPedidos()
{
	char word[200] = "";
	int fd = open(myFIFO, O_RDONLY | O_NONBLOCK);
	char** data;
	int cont = 1;
	int cont2 =1;
	time_t fim = time(NULL) + 3;

	while(time(NULL) < fim)
	{
		word[0] = '\0';
		read(fd, &word, sizeof(word));
		if(word[0] == 'A' && word[1] == 'S' && word[2] == 'A')
		{
			printf("%s\n",word);
			data = divideMensagem(word);
			system("clear");
			printf("\t\t\tMáquina Multibanco\n");
			printf("\t\t\t   Administrador  \n\n");
			if(data[0][0] == 'L')
			{
				if(data[1][0] != 'F')
				{
					printf("\tContas:\n\n");
					printf("%s\n","Número  Nome                PIN          Saldo");
					while(1)
					{
						if(data[cont][0] != 'F')
						{
							float saldo = 0;
							if(cont2%4 == 0)
							{
								saldo = atof(data[cont]);
								printf("%15.2f\n",saldo);
								cont++;
								cont2 = 1;
							}
							else if(cont2%3 == 0)
							{
								printf("%*s",23-strlen(data[cont-1]),data[cont]);
								cont++;
								cont2++;
							}
							else if(cont2%2 == 0)
							{
								printf("%s",data[cont]);
								cont++;
								cont2++;
							}
							else if(cont2%1 == 0)
							{
								printf("%s ",data[cont]);
								cont++;
								cont2++;
							}
						}
						else
						{
							break;
						}
					}
					return;
				}
				else
				{
					printf("Não existe qualquer conta registada\n");
				}
			}
			else if(data[0][0] == 'E')
			{
				printf("Servidor encerrado com sucesso\n");
			}
			else if(data[0][0] == 'C')
			{
				printf("Conta criada com sucesso\n");
			}
			else if(data[0][0] == 'R')
			{
				printf("Conta removida com sucesso\n");
			}
			printf("\nPrima qualquer tecla para continuar...\n");
			getchar();
			getchar();
			return;
		}
		else if(word[0] == 'N' && word[1] == 'O' && word[2] == 'A')
		{
			data = divideMensagem(word);
			system("clear");
			printf("\t\t\tMáquina Multibanco\n");
			printf("\t\t\t      Cliente     \n\n");
			if(data[0][0] == 'R')
			{
				printf("Não existem contas com os dados que inseriu\n\n");
			}
			printf("Prima qualquer tecla para continuar...\n");
			getchar();
			getchar();
			return;
		}
	}
	system("clear");
	printf("\t\t\tMáquina Multibanco\n");
	printf("\t\t\t   Administrador  \n\n");
	printf("O servidor não respondeu ao pedido...\n\n");
	printf("Prima qualquer tecla para continuar...\n");
	getchar();
	getchar();
}

void criaConta()
{
	struct termios term, oldterm;
	char nome[21] = "";
	char PIN[5] = "";
	char saldoc[7] = "";
	char final[60] = "AD|C|";
	int fd = open("/tmp/requests", O_WRONLY | O_NONBLOCK);
	int i;
	char* p;
	char ch, echo = '*';

	system("clear");
	printf("\t\t\tMáquina Multibanco\n");
	printf("\t\t\t   Administrador  \n\n");
	printf("Qual o nome do titular de conta?\n");
	read(0,&nome,21);
	p = strchr(nome, '\n');
	if(p)
	{
		*p = '\0';
	}
	write(STDOUT_FILENO, "PIN:\n", 6);
	tcgetattr(STDIN_FILENO, &oldterm);
	term = oldterm;
	term.c_lflag &= ~(ECHO | ECHOE | ECHOK | ECHONL | ICANON);
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &term);
	i=0;
	while (i < maxPINlen && read(STDIN_FILENO, &ch, 1) && ch != '\n')
	{
		PIN[i++] = ch;
		write(STDOUT_FILENO, &echo, 1);
	}
	PIN[i] = 0;
	tcsetattr(STDIN_FILENO, TCSANOW, &oldterm);
	printf("\n\nSaldo de entrada\n");
	scanf("%s",saldoc);
	strcat(final,myFIFO);
	strcat(final,"|");
	strcat(final,nome);
	strcat(final,"|");
	strcat(final,PIN);
	strcat(final,"|");
	strcat(final,saldoc);
	escreveNoLog("Admin envia pedido de criação de conta");
	write(fd, final, strlen(final));
	verificaPedidos();
}

void mostraConta()
{

	int fd = open("/tmp/requests", O_WRONLY | O_NONBLOCK);
	char word[30] = "AD|L|";

	strcat(word,myFIFO);
	escreveNoLog("Admin envia pedido de listagem das contas");
	write(fd, word, strlen(word));
	verificaPedidos();
}

void apagaConta()
{
	int fd = open("/tmp/requests", O_WRONLY | O_NONBLOCK);
	char word[30] = "AD|R|";
	int opcao;
	char opcaoC[2] = "";

	strcat(word,myFIFO);
	mostraConta();
	printf("\n\nQual o número da conta a eliminar?\n");
	scanf("%d",&opcao);
	sprintf(opcaoC,"%d",opcao);
	strcat(word,"|");
	strcat(word,opcaoC);
	escreveNoLog("Admin envia pedido de remoção de conta");
	write(fd, word, strlen(word));
	verificaPedidos();
}

void desligaServer()
{
	int fd = open("/tmp/requests", O_WRONLY | O_NONBLOCK);
	char word[30] = "AD|E|";

	strcat(word,myFIFO);
	escreveNoLog("Admin envia pedido de encerramento do servidor");
	write(fd, word, strlen(word));
	verificaPedidos();
}


int menuPrincipal()
{

	system("clear");
	fflush(stdin);
	char opcao;
	printf("\t\t\tMáquina Multibanco\n");
	printf("\t\t\t   Administrador  \n\n");
	printf("\n (C) - Criar nova conta de cliente\n (L) – Listar contas existentes\n (R) – Remover uma conta\n (E) – Encerrar o servidor\n (S) - Sair\n");
	printf("\nOpção: \n");
	scanf("%c",&opcao);
	if(opcao == 's' || opcao == 'S')
	{
		return 1;
	}
	else if(opcao == 'c' || opcao == 'C')
	{
		criaConta();
	}
	else if(opcao == 'r' || opcao == 'R')
	{
		apagaConta();
	}
	else if(opcao == 'l' || opcao == 'L')
	{
		mostraConta();
		printf("\nPrima qualquer tecla para continuar...\n");
		getchar();
		getchar();
	}
	else if(opcao == 'e' || opcao == 'E')
	{
		desligaServer();
	}
	return 0;
}


int main(int argc, char *argv[])
{

	char* defaultp = "/tmp/ans";
	char* myPID = malloc(20);
	char tmp[50] = "";
	int sair = 0;

	pid_t pid = getpid();
	snprintf(myPID, 10, "%lld",(long long int)pid);
	strcat(myFIFO,defaultp);
	strcat(myFIFO,myPID);
	mkfifo(myFIFO, 0777);
	strcat(msgPID,myPID);
	strcat(msgPID,") ");
	strcpy(tmp,msgPID);
	strcat(tmp,"arrancou");
	escreveNoLog(tmp);
	do
	{
		sair = menuPrincipal();
	}while(!sair);
	strcpy(tmp,msgPID);
	strcat(tmp,"encerrou");
	escreveNoLog(tmp);
	return 0;
}
