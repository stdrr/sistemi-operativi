 /*
    Homework n.6

    Scrivere un programma che crei un processo figlio con cui scambiera' dati
    tramite una coda di messaggi. Tale coda sara' creata dal padre e distrutta,
    a fine lavori, dal figlio.

    Il processo padre dovra' accettare comandi inseriti da tastiera (per semplicita'
    senza parametri) e questi dovranno essere inviati al figlio che li eseguira'
    di volta in volta creando dei processi nipoti: uno per ogni comando.

    Il tutto si dovra' arrestare correttamente all'inserimento del comando
    'exit' sul padre.

*/
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/msg.h>

#define CMDSIZE 32
#define true 1
#define false 0
#define KEY 35

typedef struct {
    long type;
    char cmd[CMDSIZE];
} Message;

void padre(int msqid) {
    
    int len;
    Message msg;
    while(strcmp(msg.cmd, "exit")!=0) {
        
        printf("\n$>> ");
//         memset(&msg, 0, sizeof(msg));
        
        fgets(msg.cmd, CMDSIZE, stdin);
        
        len=strlen(msg.cmd);
        if(msg.cmd[len-1]=='\n')
            msg.cmd[len-1]='\0';
        
        msg.type=1;
        if(msgsnd(msqid, &msg, sizeof(msg)-sizeof(long), 0)<0) {
            perror("msgsnd");
        }
        sleep(1);
    }
}

void figlio(int msqid) {
    Message msg;
    msg.type=1;
    int go=true;
    while(go) {
//         memset(&msg, 0, sizeof(msg));
        if(msgrcv(msqid, &msg, sizeof(msg)-sizeof(long), 0, 0)<0) {
            perror("msgrcv");
            break;
        }
        
        if(strcmp(msg.cmd, "exit")==0) 
            go=false;
        else {
            int pid=fork();
            if(pid==0) {
                if(execlp(msg.cmd, msg.cmd, NULL)<0) {
                    perror("execlp");
                    exit(1);
                }
            }
            else {
                wait(NULL);
            }
        }
    }
    
    if(msgctl(msqid, IPC_RMID, NULL)<0)
        perror("msgctl");
}
        
    

int main(int argc, char** agrv) {
    int msqid;
    if((msqid=msgget(KEY, IPC_CREAT | IPC_EXCL | 0600))<0) {
        perror("msgget");
        exit(-1);
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
        
        
        
