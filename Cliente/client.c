#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <termios.h>
#include <unistd.h>


char** divideMensagem(char * mensagem);
void escreveNoLog(char * mensagem);
void handlerAlarme(int signo);
void verificaPedidos();
void transferirDinheiro();
void depositarDinheiro();
void levantarDinheiro();
void menuPIN();
int menuPrincipal();

#define maxPINlen 5
int nroConta;
char PIN[5];
char myFIFO[15] = "";
char msgPID[50] = "Client (PID=";
pid_t pid;
int flagAlarme;

int main(int argc, char *argv[])
{
	char* defaultp = "/tmp/ans";
	char* myPID = malloc(10);
	char temp[50] = "";
	int sair = 0;

	pid = getpid();
	snprintf(myPID, 10, "%lld",(long long int)pid);
	strcat(myFIFO,defaultp);
	strcat(myFIFO,myPID);
	mkfifo(myFIFO, 0777);
	strcat(msgPID,myPID);
	strcat(msgPID,") ");
	strcpy(temp,msgPID);
	strcat(temp,"arrancou");
	escreveNoLog(temp);
	menuPIN();
	do
	{
		sair = menuPrincipal();
	}while(!sair);
	strcpy(temp,msgPID);
	strcat(temp,"encerrou");
	escreveNoLog(temp);
	unlink(myFIFO);
	return 0;
}


char** divideMensagem(char * mensagem)
{
	char** data = (char**) malloc(sizeof(char*)*10);
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
	char word[30] = "";
	int fd = open(myFIFO, O_RDONLY | O_NONBLOCK);
	char** data;
	time_t endTime = time(NULL) + 3;

	while(time(NULL) < endTime)
	{
		word[0] = '\0';
		read(fd, &word, sizeof(word));
		if(word[0] == 'A' && word[1] == 'S' && word[2] == 'U')
		{
			data = divideMensagem(word);
			system("clear");
			printf("\t\t\tMáquina Multibanco\n");
			printf("\t\t\t      Cliente     \n\n");
			if(data[0][0] == 'L')
			{
				printf("Levantamento efectuado\n");
			}
			else if(data[0][0] == 'T')
			{
				printf("Transferência concluida\n");
			}
			else if(data[0][0] == 'D')
			{
				printf("Depósito efectuado\n");
			}
			else if(data[0][0] == 'C')
			{
				printf("Saldo: %s\n",data[1]);
			}
			printf("\nPrima qualquer tecla para continuar...\n");
			getchar();
			getchar();
			return;
		}
		else if(word[0] == 'N' && word[1] == 'O' && word[2] == 'U')
		{
			data = divideMensagem(word);
			system("clear");
			printf("\t\t\tMáquina Multibanco\n");
			printf("\t\t\t      Cliente     \n\n");
			if(data[0][0] == 'L')
			{
				printf("O valor que quis levantar é superior ao seu saldo actual\n\n");
			}
			else if(data[0][0] == 'T')
			{
				if(data[1][0] == 'D')
				{
					printf("A conta para qual quer transferir não existe\n\n");
				}
				else if(data[1][0] == 'S')
				{
					printf("O valor que quis levantar é superior ao seu saldo actual\n\n");
				}
			}
			else if(data[0][0] == 'Z')
			{
				printf("O número de conta ou PIN que introduziu não está correcto.\n\n");
			}
			printf("Prima qualquer tecla para continuar...\n");
			getchar();
			getchar();
			return;
		}
	}
	system("clear");
	printf("\t\t\tMáquina Multibanco\n");
	printf("\t\t\t      Cliente     \n\n");
	printf("O servidor não respondeu ao pedido...\n\n");
	printf("Prima qualquer tecla para continuar...\n");
	getchar();
	getchar();
}


void transferirDinheiro()
{
	int nConta = nroConta;
	float dinheiro;
	int opcao;

	while(1)
	{
		system("clear");
		printf("\t\t\tMáquina Multibanco\n");
		printf("\t\t\t      Cliente     \n\n");
		printf("Qual o número da conta para qual deseja efectuar a transferência?\n");
		scanf("%d",&nConta);
		if(nConta != nroConta)
		{
			break;
		}
		while(opcao < 1 || opcao > 2);
		{
			printf("\nTem que inserir um número de conta diferente do seu para efectuar a transferência\n\nDeseja voltar ao menu?\n1-Sim\n2-Nao\nOpção: ");
			scanf("%d",&opcao);
			if(opcao == 1)
			{
				return;
			}
		}
	}
	printf("Qual a quantia que deseja transferir?\n");
	scanf("%f",&dinheiro);
	char word[30] = "US|T|";
	char numeroO[20]="";
	char numeroD[20]="";
	char montante[20]="";
	int fd = open("/tmp/requests", O_WRONLY | O_NONBLOCK);
	sprintf( numeroO, "%d", nroConta);
	sprintf( numeroD, "%d", nConta);
	sprintf(montante, "%f", dinheiro);
	strcat(word,myFIFO);
	strcat(word,"|");
	strcat(word,numeroO);
	strcat(word,"|");
	strcat(word,PIN);
	strcat(word,"|");
	strcat(word,numeroD);
	strcat(word,"|");
	strcat(word,montante);
	escreveNoLog("Client envia pedido para transferir dinheiro");
	write(fd, word, strlen(word));
	close(fd);
	verificaPedidos();
}

void depositarDinheiro()
{
	system("clear");
	float opcao;

	printf("\t\t\tMáquina Multibanco\n");
	printf("\t\t\t      Cliente     \n\n");
	printf("Qual a quantia que deseja depositar?\n");
	scanf("%f",&opcao);
	char word[30] = "US|D|";
	char numero[20]="";
	char montante[20]="";
	int fd = open("/tmp/requests", O_WRONLY | O_NONBLOCK);
	sprintf( numero, "%d", nroConta);
	sprintf( montante, "%f", opcao);
	strcat(word,myFIFO);
	strcat(word,"|");
	strcat(word,numero);
	strcat(word,"|");
	strcat(word,PIN);
	strcat(word,"|");
	strcat(word,montante);
	escreveNoLog("Client envia pedido para depositar dinheiro");
	write(fd, word, strlen(word));
	close(fd);
	verificaPedidos();
}

void levantarDinheiro()
{

	system("clear");
	float opcao;

	printf("\t\t\tMáquina Multibanco\n");
	printf("\t\t\t      Cliente     \n\n");
	printf("Qual a quantia que deseja levantar?\n");
	scanf("%f",&opcao);
	char word[30] = "US|L|";
	char numero[20]="";
	char montante[20]="";
	int fd = open("/tmp/requests", O_WRONLY | O_NONBLOCK);
	sprintf( numero, "%d", nroConta);
	sprintf( montante, "%f", opcao);
	strcat(word,myFIFO);
	strcat(word,"|");
	strcat(word,numero);
	strcat(word,"|");
	strcat(word,PIN);
	strcat(word,"|");
	strcat(word,montante);
	escreveNoLog("Client envia pedido para levantar dinheiro");
	write(fd, word, strlen(word));
	close(fd);
	verificaPedidos();
}


void  account_balance()
{
	char word[30] = "US|C|";
	char numero[7];

	int fd = open("/tmp/requests", O_WRONLY | O_NONBLOCK);
	sprintf( numero, "%d", nroConta );
	strcat(word,myFIFO);
	strcat(word,"|");
	strcat(word,numero);
	strcat(word,"|");
	strcat(word,PIN);
	escreveNoLog("Client envia pedido para consultar o saldo");
	write(fd, word, strlen(word));
	close(fd);
	verificaPedidos();
}

int menuPrincipal()
{
	system("clear");
	char opcao;

	printf("\t\t\tMáquina Multibanco\n");
	printf("\t\t\t      Cliente     \n\n");
	printf("\n (L) – Levantar dinheiro\n (D) - Depositar dinheiro\n (T) - Transferir dinheiro\n (C) - Consultar o saldo\n (S) - Sair\n");
	printf("\nOpção: \n");
	scanf("%c",&opcao);
	if(opcao == 'c' || opcao == 'C')
	{
		account_balance();
	}
	else if(opcao == 'l' || opcao == 'L')
	{
		levantarDinheiro();
	}
	else if(opcao == 't' || opcao == 'T')
	{
		transferirDinheiro();
	}

	else if(opcao == 'd' || opcao == 'D')
	{
		depositarDinheiro();
	}
	else if(opcao == 's' || opcao == 'S')
	{
		return 1;
	}
	return 0;
}

void menuPIN()
{
	struct termios term, oldterm;
	int i;
	char ch, echo = '*';

	system("clear");
	printf("\t\t\tMáquina Multibanco\n");
	printf("\t\t\t      Cliente     \n\n");
	printf("Número da conta:\n");
	scanf ("%d",&nroConta);
	write(STDOUT_FILENO, "PIN\n", 6);
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
	menuPrincipal();
}
