#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

#define MAX_WORD_LEN 64
#define KEY 50
#define true 1
#define false 0

typedef struct {
    long type;
    char str1[MAX_WORD_LEN];
    char str2[MAX_WORD_LEN];
    int stop;
} Messaggio;

typedef struct {
    long type;
    int result;
} Risposta;

int comparer(int queue_id) {
    Messaggio msg;
    msg.type=1;
    Risposta rsp;
    rsp.type=2;
    /*si pone in attesa del messaggio*/
    while(true) {
        if(msgrcv(queue_id, &msg, sizeof(msg)-sizeof(long), 1, 0)<0) {
            perror("Comparer: msgrcv");
        }
        /******************************/
        
        if(msg.stop==true) break;
        
        /*esegue il confronto e lo salva nella variabile apposita*/
        rsp.result=strcasecmp(msg.str1, msg.str2);
        
        /*manda il riscontro*/
        if(msgsnd(queue_id, &rsp, sizeof(rsp)-sizeof(long), 0)<0) {
            perror("Comparer: msgsnd");
        }
        /**************************/
    }
}

void sorter(char* fpath, int queue_id, int* pipe_id) {
    /*chiude il canale di input della pipe*/
    close(pipe_id[0]);
    /***********************************/
    FILE* file;
    if((file=fopen(fpath, "r"))==NULL) {
        perror("fopen");
        exit(1);
    }
    
    
    /*crea la coda di messaggi*
    int queue_id;
    if((queue_id=msgget(KEY, IPC_CREAT|0660))<0) {
        perror("msgget");
        exit(1);
    }
    ***********************************/
    
    /*controlla il numero di parole*/
    int n_words=0;
    char* buff=(char*)malloc(sizeof(char)*MAX_WORD_LEN);
    while(fgets(buff, MAX_WORD_LEN, file)!=NULL) {
        ++n_words;
    }
    rewind(file);
    free(buff);
    /******************/
    
    /*alloca un array per le parole*/
    char** words=(char**)malloc(sizeof(char*)*n_words);
    for(int i=0; i<n_words; i++) 
        words[i]=(char*)malloc(sizeof(char)* MAX_WORD_LEN);
    /*****************/
    
    /*legge e conserva le parole del file*/
    for(int i=0; i<n_words; i++) {
        fgets(words[i], MAX_WORD_LEN, file);
    }
    /************************************/
    
    /*chiude il file*/
    fclose(file);
    /******************************/
    
    /*crea il messaggio*/
    Messaggio msg;
    msg.type=1;
    msg.stop=false;
    Risposta rsp;
    rsp.type=2;
    /*************************/
    
    /*ordinamento delle stringhe*/
    buff=(char*)malloc(sizeof(char)*MAX_WORD_LEN);
    for(int i=1; i<n_words; i++) {
        strcpy(buff, words[i]);
        int j=i-1;
        int not_in_order=true;
        while(j>=0 && not_in_order) {
            /*prepara il messaggio*/
            strcpy(msg.str1, words[j]);
            strcpy(msg.str2, buff);
            /***************************/
            
            /*invia il confronto a Comparer*/
            if(msgsnd(queue_id, &msg, sizeof(msg)-sizeof(long), 0)<0) {
                perror("Sorter: msgsnd");
            }
            /*********************************/
            
            /*ottiene la risposta*/
            if(msgrcv(queue_id, &rsp, sizeof(rsp)-sizeof(long), 2, 0)<0) {
                perror("Sorter: msgrcv");
            }
            /**********************************/
            
            /*controlla esito del confronto e agisce di conseguenza*/
            if(rsp.result>0) {
                strcpy(words[j+1], words[j]);
                j--;
            }
            else 
                not_in_order=false;
            /**********************************************/
        }
        strcpy(words[j+1], buff);
    }
    /*******************************/
    
    /*manda il segnale di terminazione a Comparer*/
    msg.stop=true;
    msgsnd(queue_id, &msg, sizeof(msg)-sizeof(long), 0);
    /*********************************************/
    
    /*manda le stringhe al padre*/
    for(int i=0; i<n_words; i++) {
        write(pipe_id[1], words[i], MAX_WORD_LEN);
    }
    buff="";
    write(pipe_id[1], buff, MAX_WORD_LEN);
    /********************************/
    
    /*chiude la pipe*/
    close(pipe_id[1]);
    /******************/
    
    /*dealloca l'array di parole*/
    for(int i=0; i<n_words; i++) 
        free(words[i]);
    free(words);
    /*****************************/
    
}


int main(int argc, char** argv) {
    if(argc!=2) {
        printf("Uso: %s <file>", argv[0]);
        exit(1);
    }
    
     /*apre la coda*/
    int queue_id;
    if((queue_id=msgget(KEY, IPC_CREAT|0660))<0) {
        perror("msgget");
    }
    /*************************************/
    
    /*apre la pipe*/
    int pipe_id[2];
    if(pipe(pipe_id)<0) {
        perror("pipe");
    }
    /****************/
    
    if(fork()==0) {
        sorter(argv[1], queue_id, pipe_id);
    }
    else {
        if(fork()==0) {
            comparer(queue_id);
        }
        else {
            /*chiude il canale di output della pipe*/
            close(pipe_id[1]);
            /*************************************/
            
            char* buff=(char*)malloc(sizeof(char)*MAX_WORD_LEN);
            while(true) {
                read(pipe_id[0], buff, MAX_WORD_LEN);
                if(strcmp(buff, "")==0) break;
                printf("%s", buff);
            }
            free(buff);
            
            /*chiude la coda*/
            msgctl(queue_id, IPC_RMID, NULL);
            /********************************/
            
            /*chiude la pipe*/
            close(pipe_id[0]);
        }
    }
    return 0;
}
