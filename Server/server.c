#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include <pthread.h>

typedef struct
{
    char nome[22];
    int  numero;
    char  pin[6];
    float saldo;
    int activa;

} conta;

int nroContas =0;
conta contas[500];
pthread_t threads[500];
int tamcontas = 0;
int tamthreads = 0;

void guardarContas()
{

    int i;
    char word[40] = "";
    char cash[15] = "";

    if(nroContas != 0)
    {
        FILE * fout;
        fout = fopen("accounts.txt","w+");
        sprintf(word, "%07d", nroContas);
        fprintf(fout,"%s\n",word);

        word[0] = '\0';

        for(i = 0; i < tamcontas; i++)
            if(contas[i].activa == 1)
            {
                sprintf(word,"%07d ",contas[i].numero);
                strcat(word,contas[i].nome);
                fprintf(fout,"%s",word);
                fprintf(fout,"%*s",25-strlen(contas[i].nome),contas[i].pin);
                sprintf(cash, "%14.2f", contas[i].saldo);
                fprintf(fout,"%s\n",cash);
            }

        fclose(fout);
    }
}

void carregarContas()
{

    FILE *in;
    char buffer[50];

    in = fopen("accounts.txt", "rb");

    if (!in)
        return;

    if(fgets (buffer , 50 , in) == NULL)
        nroContas = 0;
    else
    {
        sscanf(buffer, "%d", &nroContas);

        while(fgets (buffer , 50, in) != NULL)
        {
            char** data = (char**) malloc(sizeof(char*)*6); //array de dupla dimensão
            char * pal; //array de palavras
            char nome[21] =""; //array com o nome
            int ctam=0; // posição do array data
            int contador = 1;

            pal = strtok (buffer," "); //separa as palavras e mete-as no array
            data[ctam] = pal;
            ctam++;

            while (pal != NULL)
            {
                pal = strtok (NULL," ");
                data[ctam] = pal;
                ctam++;
            }

            strcat(nome,data[1]); //adiciona ao fim da "string"

            while(1)
            {
                if(isalpha(data[1+contador][0])) //verifica se é uma palavra
                {
                    strcat(nome," ");
                    strcat(nome,data[1+contador]);
                    contador++;
                }
                else
                    break;
            }

            strcpy(contas[tamcontas].nome,nome);
            sscanf(data[0], "%d", &contas[tamcontas].numero);
            strcpy(contas[tamcontas].pin, data[1+contador]);
            sscanf(data[2+contador], "%f", &contas[tamcontas].saldo);

            contas[tamcontas].activa = 1;
            tamcontas++;
        }
    }
}

void escreveNoLog(char * mensagem)
{

    FILE * in, * out;
    int bool = 0;

    in = fopen("logfile.txt","r");

    if(!in)
        bool= 1;

    if(bool)
    {
        out = fopen("logfile.txt","wb");
        fprintf(out,"%7s%10s%11s%10s","DATA","HORA","PROGRAMA","OPERACAO"); //escreve no ecra com os caracteres contados
    }
    else
        out = fopen("logfile.txt","a");


    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    fprintf(out,"\n%d-%02d-%d",tm.tm_year +1900, tm.tm_mon +1 , tm.tm_mday);
    fprintf(out," %02d:%02d:%02d",tm.tm_hour, tm.tm_min , tm.tm_sec);
    fprintf(out," SERVER    %s",mensagem);

    fclose(out);

}

char ** divideMensagem(char * mensagem)
{

    char** data = (char**) malloc(sizeof(char*)*20);
    char * pal;
    int ctam=0;

    char word[100] = "";

    strcpy(word,mensagem);

    pal = strtok (word,"|"); //separa a mensagem recebida

    while (pal != NULL)
    {
        pal = strtok (NULL,"|");
        data[ctam] = pal;
        ctam++;
    }

    return data;

}

void * verificaUtilizador(void* arg)
{
    char** data = divideMensagem((char*) arg);
    int i;
    char nroContaChar[20] = "";
    char PINIntroduzido[5] = "";
    int nConta;

    strcpy(nroContaChar, data[2]);
    strcpy(PINIntroduzido, data[3]);
    sscanf(nroContaChar,"%d",&nConta);

    for(i = 0; i < tamcontas; i++)
    {
        if(contas[i].numero == nConta && contas[i].activa == 1)
            if(strcmp(contas[i].pin, PINIntroduzido) == 0)
            pthread_exit((void*) 1);

    }
    pthread_exit((void*) 2);

}

void * transferenciaCliente(void* arg)
{
    int i,j;
    char mensagemOK[30] = "ASU|T|";
    char mensagemSemSaldo[10] = "NOU|T|S";
    char mensagemErroDestino[10] = "NOU|T|D";
    char ** data = divideMensagem((char*) arg);
    char res[20] = "";
    char saldoChar[20] = "";
    char nContaChar[20] = "";
    char nContaChar2[20] = "";
    int nConta;
    int nConta2;
    float dinheiro;

    strcpy(res, data[1]);
    strcpy(nContaChar, data[2]);
    strcpy(nContaChar2, data[4]);
    strcpy(saldoChar, data[5]);
    sscanf(nContaChar,"%d",&nConta);
    sscanf(nContaChar2,"%d",&nConta2);
    sscanf(saldoChar,"%f",&dinheiro);

    int abriu = open(res, O_WRONLY | O_NONBLOCK); //abre o ficheiro, devolve 0 se sucesso


    for(i = 0; i < tamcontas; i++)
    {
        if(contas[i].numero == nConta)
        {

            for(j = 0; j < tamcontas; j++)
            {

                if((contas[i].saldo - dinheiro) < 0)
                {

                    write(abriu, mensagemSemSaldo, 10);

                    printf("mensagem enviada: %s\n",mensagemSemSaldo);

                    close(abriu);

                    pthread_detach(pthread_self());

                    return NULL;
                }
                else
                {

                    if(contas[j].numero == nConta2 && contas[j].activa == 1)
                    {

                        write(abriu, mensagemOK, 10);

                        printf("mensagem enviada: %s\n",mensagemOK);

                        contas[i].saldo -= dinheiro;
                        contas[j].saldo += dinheiro;

                        close(abriu);

                        pthread_detach(pthread_self());

                        return NULL;

                    }

                }
            }

        }


    }

    write(abriu, mensagemErroDestino, 10);

    printf("Mensagem Enviada: %s\n",mensagemErroDestino);

    close(abriu);

    pthread_detach(pthread_self());

    return NULL;

}


void * depositoCliente(void* arg)
{

    int i;
    char mensagemOK[30] = "ASU|D|";
    char ** data = divideMensagem((char*) arg);
    char res[20] = "";
    char saldoChar[20] = "";
    char nContaChar[20] = "";
    int nConta;
    float saldo;

    strcpy(res, data[1]);
    strcpy(nContaChar, data[2]);
    strcpy(saldoChar, data[4]);
    sscanf(nContaChar,"%d",&nConta);
    sscanf(saldoChar,"%f",&saldo);

    for(i = 0; i < tamcontas; i++)
    {
        if(contas[i].numero == nConta)
        {
            int abriu = open(res, O_WRONLY | O_NONBLOCK);
            {
                write(abriu, mensagemOK, 30);

                printf("mensagem enviada: %s\n",mensagemOK);

                contas[i].saldo += saldo;

                pthread_detach(pthread_self());

                return NULL;
            }


            close(abriu);

        }
    }

    pthread_detach(pthread_self());

    return NULL;
}

void * levantamentoCliente(void* arg)
{
    int i;
    char mensagemOK[30] = "ASU|L|";
    char mensagemSemSaldo[10] = "NOU|L|S";
    char ** data = divideMensagem((char*) arg);
    char res[20] = "";
    char saldoChar[20] = "";
    char nContaChar[20] = "";
    int nConta;
    float saldo;

    strcpy(res, data[1]);
    strcpy(nContaChar, data[2]);
    strcpy(saldoChar, data[4]);
    sscanf(nContaChar,"%d",&nConta);
    sscanf(saldoChar,"%f",&saldo);

    for(i = 0; i < tamcontas; i++)
    {

        if(contas[i].numero == nConta)
        {
            int abriu = open(res, O_WRONLY | O_NONBLOCK);

            if( (contas[i].saldo - saldo ) >= 0)
            {

                write(abriu, mensagemOK, 30);

                printf("mensagem enviada: %s\n",mensagemOK);

                contas[i].saldo -= saldo;

                pthread_detach(pthread_self());

                return NULL;
            }

            else
            {

                write(abriu, mensagemSemSaldo, 10);

                printf("mensagem enviada: %s\n",mensagemSemSaldo);


            }

            close(abriu);

        }
    }

    pthread_detach(pthread_self());

    return NULL;

}

void * saldoContaCliente(void* arg)
{
    int i;
    char mensagemOK[30] = "ASU|C|";
    char buffer[13] ="";
    char ** data = divideMensagem((char*) arg);
    char res[20] = "";
    char nContaChar[20] = "";
    int nConta;

    strcpy(res, data[1]);
    strcpy(nContaChar, data[2]);
    sscanf(nContaChar,"%d",&nConta);


    for(i = 0; i < tamcontas; i++)
    {
        if(contas[i].numero == nConta)
        {
            int abriu = open(res, O_WRONLY | O_NONBLOCK);

            sprintf(buffer,"%g",contas[i].saldo);

            strcat(mensagemOK, buffer);

            write(abriu, mensagemOK, 30);

            printf("mensagem enviada: %s\n",mensagemOK);

            close(abriu);

            pthread_detach(pthread_self());

            return NULL;

        }
    }

    return NULL;
}

void * mostraContasAdmin(void * arg)
{
    char mensagemOK[1000] = "ASA|L";
    char word[30] = "";
    int i;
    char ** data;
    char res[20] = "";

    data = divideMensagem((char*) arg);

    strcpy(res, data[1]);

    int abriu = open(res, O_WRONLY | O_NONBLOCK);

    for(i = 0; i < tamcontas; i++)
    {
        if(contas[i].activa == 1)
        {
            sprintf(word,"|%07d|",contas[i].numero);

            strcat(mensagemOK,word);
            strcat(mensagemOK,contas[i].nome);

            strcat(mensagemOK,"|");
            strcat(mensagemOK,contas[i].pin);
            strcat(mensagemOK,"|");

            sprintf(word, "%g", contas[i].saldo);

            strcat(mensagemOK,word);

        }

    }

    strcat(mensagemOK,"|F");

    write(abriu, mensagemOK, strlen(mensagemOK));

    printf("mensagem enviada: %s\n",mensagemOK);

    close(abriu);

    pthread_detach(pthread_self());

    return NULL;

}

void * removeContaAdmin(void * arg)
{
    int numero;
    int i;
    int contador = 0;
    char res[20] = "";

    char mensagemOK[10] = "ASA|R";
    char mensagemFail[10] = "NOA|R";

    char ** data = divideMensagem((char*) arg);
    strcpy(res, data[1]);

    int abriu = open(res, O_WRONLY | O_NONBLOCK);

    numero = atoi(data[2]);

    for(i = 0; i  < tamcontas; i++)
    {
        if(contas[i].numero == numero)
            if(contas[i].activa == 1)
            {
                contas[i].activa = 0;
                write(abriu, mensagemOK, strlen(mensagemOK));
                printf("Mensagem Enviada: %s\n",mensagemOK);
                contador++;
                break;
            }
    }

    if(!contador)
    {
        write(abriu, mensagemFail, strlen(mensagemFail));
        printf("Mensagem Enviada: %s\n",mensagemFail);
    }

    close(abriu);

    pthread_detach(pthread_self());

    return NULL;
}

void * criaContaAdmin(void * arg)
{
    char mensagemOK[10] = "ASA|C";
    char res[20] = "";
    char ** data = divideMensagem((char*) arg);

    strcpy(res, data[1]);

    int abriu = open(res, O_WRONLY /*| O_NONBLOCK*/);

    contas[tamcontas].numero = nroContas+ 1;

    printf("\n%d\n", contas[tamcontas].numero);

    strcpy(contas[tamcontas].nome,data[2]);
    strcpy(contas[tamcontas].pin,data[3]);
    sscanf(data[4], "%f", &contas[tamcontas].saldo);

    contas[tamcontas].activa = 1;

    tamcontas++;
    nroContas++;

    write(abriu, mensagemOK, strlen(mensagemOK));
    printf("mensagem enviada: %s\n",mensagemOK);

    close(abriu);

    return NULL;
}

void * desligaServerAdmin(void * arg)
{
    unlink("/tmp/requests");

    char mensagemOK[10] = "ASA|E";
    char res[20] = "";
    char ** data = divideMensagem((char*) arg);

    strcpy(res, data[1]);

    int abriu = open(res, O_WRONLY | O_NONBLOCK);

    write(abriu, mensagemOK, strlen(mensagemOK));
    printf("Mensagem Enviada: %s\n",mensagemOK);

    close(abriu);

    return NULL;
}

void * verificaPedidos(void* arg)
{
	int bool=1;

    char word[100] = "";
    int abriu = open("/tmp/requests", O_RDONLY | O_TRUNC | O_NONBLOCK);

    while(bool)
    {
        word[0] = '0';

        read(abriu, &word, sizeof(word));

        if(word[0] == 'U' && word[1] == 'S')
        {
            pthread_t tid;

            void * stat;

            pthread_create(&tid, NULL, verificaUtilizador, word);
            pthread_join( tid, &stat);

            if((int)(stat) == 1)
            {

                if(word[3] == 'L')
                {
                    escreveNoLog("Efectua levantamento de dinheiro");
                    pthread_create(&tid, NULL, levantamentoCliente, word);
                }
                else if(word[3] == 'D')
                {
                    escreveNoLog("Efectua depósito de dinheiro");
                    pthread_create(&tid, NULL, depositoCliente, word);
                }
                else if(word[3] == 'T')
                {
                    escreveNoLog("Procede à transferência de dinheiro");
                    pthread_create(&tid, NULL, transferenciaCliente, word);
                }
                else if(word[3] == 'C')
                {
                    escreveNoLog("Lê pedido de consulta de saldo");
                    pthread_create(&tid, NULL, saldoContaCliente, word);
                }

            }
            else
            {
                char ** data = divideMensagem(word);
                char res[20] = "";
                char mensagemFail[10] = "NOU|Z";


                strcpy(res, data[1]);


                int abriu1 = open(res, O_WRONLY | O_NONBLOCK);

                write(abriu1, mensagemFail, strlen(mensagemFail));
                printf("Mensagem Enviada: %s\n",mensagemFail);

                close(abriu1);

            }

            close(abriu);

            abriu = open("/tmp/requests", O_RDONLY | O_TRUNC | O_NONBLOCK);
        }
        else if(word[0] == 'A' && word[1] == 'D')
        {

            pthread_t tid;

            if(word[3] == 'L')
            {
                escreveNoLog("Pedido de listagem das contas");
                pthread_create(&tid, NULL, mostraContasAdmin, word);
            }

            else if(word[3] == 'C')
            {
                escreveNoLog("Pedido de criação de conta");
                pthread_create(&tid, NULL, criaContaAdmin, word);
            }
            else if(word[3] == 'R')
            {
                escreveNoLog("Pedido de eliminação de uma conta");
                pthread_create(&tid, NULL, removeContaAdmin, word);
            }
            else if(word[3] == 'E')
            {
                escreveNoLog("Pedido de encerramento do servidor");
                pthread_create(&tid, NULL, desligaServerAdmin, word);
                pthread_join(tid,NULL);
                return NULL;
            }

            close(abriu);

            abriu = open("/tmp/requests", O_RDONLY | O_TRUNC | O_NONBLOCK);
        }

    }

    return NULL;

}

int main(int argc, char *argv[])
{

    carregarContas();

    mkfifo("/tmp/requests", 0777);

    escreveNoLog("Server arrancou");

    pthread_t tid;

    pthread_create(&tid, NULL, verificaPedidos, NULL);
    pthread_join( tid, NULL);

    guardarContas();

    escreveNoLog("Server encerrou");

    return 0;
}

