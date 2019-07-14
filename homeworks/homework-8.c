/*
    Homework n.8

    Modificare l'homework precedente (n.7) sostituendo il canale di comuniciazione
    offerto dalla coda di messaggi tra padre e figlio con un segmento di memoria
    condiviso e una coppia di semafori (esattamente due) opportunamente utilizzati
    per il coordinamento.

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <sys/sem.h>
#include <unistd.h>
#include <fcntl.h>

#define true 1
#define false 0

#define S1 0
#define S2 1

#define STDOUT 1
#define STDERR 2

#define KEY 30
#define MSIZE 4096


int WAIT(int sem_des, int num_semaforo){
    struct sembuf operazioni[1] = {{num_semaforo,-1,0}};
    return semop(sem_des, operazioni, 1);
}

int SIGNAL(int sem_des, int num_semaforo){
    struct sembuf operazioni[1] = {{num_semaforo,+1,0}};
    return semop(sem_des, operazioni, 1);
}


void padre(int mid, int semid) {
    int len;
    char* messaggio;
    if((messaggio=(char*) shmat(mid, NULL, 0))==NULL) {
        return;
    }
    while(true) {
        
        printf("\n$> ");
        fgets(messaggio, MSIZE, stdin);
        
        len=strlen(messaggio);
        if(messaggio[len-1]=='\n') 
            messaggio[len-1]='\0';
        
        if(strcmp(messaggio, "exit")!=0)
            SIGNAL(semid, S1);  //semaforo di scrittura
        else {
            SIGNAL(semid, S1);
            break;
        }

        WAIT(semid, S2); //semaforo di lettura
        printf("%s", messaggio);
    }
}

void figlio(int mid, int semid) {
    int go=true;
    char* messaggio;
    if((messaggio=(char*) shmat(mid, NULL, 0))==NULL) {
        return;
    }
    while(go) {
        WAIT(semid, S1); //semaforo di lettura 
        
        if(strcmp(messaggio, "exit")==0) {
            go=false;
        }
        else {
            int io_channels[2];
            if(pipe(io_channels)<0) {
                perror("pipe");
                go=false;
            }
            else if(fork()==0) {                 // *****NIPOTE*******
                close(STDOUT);
                dup(io_channels[1]); 
                close(STDERR);
                dup(io_channels[1]); //con le precedenti istruzioni redirige lo standard output e lo standard error sulla pipe
                close(io_channels[0]); 
                if(execlp(messaggio, messaggio, NULL)<0)
                    perror("execlp");
            }
            else {
                close(io_channels[1]);
                wait(NULL);
                int len;
                if((len=read(io_channels[0], messaggio, MSIZE-1))<0) {
                    perror("read");
                }
                messaggio[len]='\0';
                
                SIGNAL(semid, S2); //semaforo di scrittura 
                close(io_channels[0]);
            }
        }
    }
    if(shmctl(mid, IPC_RMID, NULL)<0) 
        perror("shmctl");
    if(semctl(semid, 0, IPC_RMID, 0)<0) 
        perror("semctl");
    
}

int main() {
    
    int mid;
    if((mid=shmget(KEY, MSIZE, IPC_CREAT | IPC_EXCL | 0600))<0) {
        perror("shmget");
        exit(1);
    }
    
    int semid;
    if((semid=semget(IPC_PRIVATE, 2, IPC_CREAT | IPC_EXCL | 0600))<0) {
        perror("semget");
        exit(1);
    }
    
    semctl(semid, S1, SETVAL, 0);
    semctl(semid, S2, SETVAL, 0);
    
    int pid=fork();
    if(pid==0) {
        figlio(mid, semid);
    }
    else {
        padre(mid, semid);
    }
    return 0;
}
