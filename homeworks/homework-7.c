/*
    Homework n.7

    Modificare l'homework precedente (n.6) facendo in modo che il figlio che
    riceve il comando da eseguire tramite la coda, catturi lo standard output
    e lo standard error del figlio nipote usando la redirezione su pipe tra
    processi. L'output catturato dovrà essere mandato indietro al padre
    tramite un messaggio (per semplicita', assumiamo sufficiente grande).
    Tale contenuto sara' poi visualizzato sul terminale dal padre.

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>

#define true 1
#define false 0

#define STDOUT 1
#define STDERR 2

#define KEY 30
#define CMDSIZE 32
#define TEXTSIZE 4096

typedef struct {
    long type;
    char cmd[CMDSIZE];
} Message;

typedef struct {
    long type;
    char text[TEXTSIZE];
} TextMessage;


void padre(int msqid) {
    Message msg;
    msg.type=1;
    TextMessage tmsg;
    tmsg.type=2;
    int len;
    while(true) {
        
        printf("\n$> ");
        fgets(msg.cmd, CMDSIZE, stdin);
        len=strlen(msg.cmd);
        
        if(msg.cmd[len-1]=='\n') 
            msg.cmd[len-1]='\0';
        
        if(strcmp(msg.cmd, "exit")==0)
            break;
        
        if(msgsnd(msqid, &msg, sizeof(msg)-sizeof(long), 0)<0)
            perror("Padre::msgsnd");
        
        if(msgrcv(msqid, &tmsg, sizeof(tmsg)-sizeof(long), 2, 0)<0) 
            perror("Padre::msgrcv");
        
        printf("%s", tmsg.text);
    }
}

void figlio(int msqid) {
    Message msg;
    msg.type=1;
    TextMessage tmsg;
    tmsg.type=2;
    int go=true;
    while(go) {
        if(msgrcv(msqid, &msg, sizeof(msg)-sizeof(long), 1, 0)<0) {
            perror("Figlio::msgrcv");
            go=false;
        }
        else if(strcmp(msg.cmd, "exit")==0) {
            go=false;
        }
        else {
            int io_channels[2];
            if(pipe(io_channels)<0) {
                perror("pipe");
            }
            if(fork()==0) {                 // *****NIPOTE*******
                close(STDOUT);
                dup(io_channels[1]); 
                close(STDERR);
                dup(io_channels[1]); //con le precedenti istruzioni redirige lo standard output e lo standard error sulla pipe
                close(io_channels[0]); 
                if(execlp(msg.cmd, msg.cmd, NULL)<0)
                    perror("execlp");
            }
            else {
                close(io_channels[1]);
                wait(NULL);
                int len;
                if((len=read(io_channels[0], tmsg.text, TEXTSIZE))<0) {
                    perror("read");
                }
                tmsg.text[len]='\0';
                if(msgsnd(msqid, &tmsg, sizeof(tmsg)-sizeof(long), 0)<0) {
                    perror("Figlio::msgsnd");
                }
                close(io_channels[0]);
            }
        }
    }
    if(msgctl(msqid, IPC_RMID, NULL)<0) 
        perror("msgctl");
}

int main() {
    
    int msqid;
    if((msqid=msgget(KEY, IPC_CREAT | IPC_EXCL | 0600))<0) {
        perror("msgget");
        exit(1);
    }
    
    int pid=fork();
    if(pid==0) {
        figlio(msqid);
    }
    else {
        padre(msqid);
    }
    return 0;
}
