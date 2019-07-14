#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <fcntl.h>
#include <unistd.h>

#define KEY1 40
#define KEY2 50
#define LINE 512

typedef struct {
    long type;
    int end;
    char line[LINE];
} Messaggio;

typedef struct {
    long type;
    int end;
    int alfabeto[26];
} Conto;

int findIndex(char c) {
    if(((int)c) >= ((int)'A') && ((int)c) <= ((int)'Z'))
        return (((int)c)-((int)'A'));
    if(((int)c) >= ((int)'a') && ((int)c) <= ((int)'z'))
        return (((int)c)-((int)'a'));
    return -1;
}

void counter(int queue_idP, int num_readers) {
    int queue_idR;
    if((queue_idR=msgget(KEY2, IPC_CREAT|0600))<0) {
        perror("Counter:: msgget");
    }
    
    Messaggio msg;
    msg.type=1;
    Conto cnt;
    cnt.type=1;
    cnt.end=0;
    int ends=0, i, index;
    while(1) {
        if(msgrcv(queue_idR, &msg, sizeof(msg)-sizeof(long), 0, 0)<0) {
            perror("Counter:: msgrcv");
        }
        
        if(msg.end==1) ends++;
        if(ends==num_readers) break;
        
        for(int j=0; j<26; j++)
            cnt.alfabeto[j]=0;
        i=0;
        while(msg.line[i]!='\0') {
            index=findIndex(msg.line[i++]);
            if(index!=-1)
                cnt.alfabeto[index]++;
        }
        
        if(msgsnd(queue_idP, &cnt, sizeof(cnt)-sizeof(long), 0)<0) {
            perror("Counter:: msgsnd");
        }
    }
    
    msgctl(queue_idR, IPC_RMID, NULL);
    cnt.end=1;
    msgsnd(queue_idP, &cnt, sizeof(cnt)-sizeof(long), 0);
}

void reader(char* fname) {
    int queue_idR;
    if((queue_idR=msgget(KEY2, 0600))<0) {
        perror("Reader:: msgget");
    }
    
    FILE* file=fopen(fname, "r");
    
    Messaggio msg;
    msg.type=1;
    msg.end=0;
    while(1) {
        if(fgets(msg.line, LINE, file)==NULL) {
            msg.end=1;
            msgsnd(queue_idR, &msg, sizeof(msg)-sizeof(long), 0);
            break;
        }
        
        if(msgsnd(queue_idR, &msg, sizeof(msg)-sizeof(long), 0)<0) {
            perror("Reader:: msgsnd");
        }
    }
    fclose(file);
}

int main(int argc, char** argv) {
    if(argc<2) {
        printf("Uso: %s  <file-1> <file-2> ... <file-n>\n", argv[0]);
        exit(1);
    }
    
    int queue_idP;
    if((queue_idP=msgget(KEY1, IPC_CREAT|0600))<0) {
        perror("Padre:: msgget");
        exit(1);
    }
    
    if(fork()==0) {
        counter(queue_idP, argc-1);
        exit(0);
    }
    
    for(int i=1; i<argc; i++) {
        if(fork()==0) {
            reader(argv[i]);
            exit(0);
        }
    }
    
    Conto cnt;
    cnt.type=1;
    
    int alfabeto[26];
    for(int i=0; i<26; i++) 
        alfabeto[i]=0;
    while(1) {
        if(msgrcv(queue_idP, &cnt, sizeof(cnt)-sizeof(long), 0, 0)<0) {
            perror("Padre:: magrcv");
        }
        
        if(cnt.end==1) break;
        
        for(int i=0; i<26; i++) 
            alfabeto[i]+=cnt.alfabeto[i];
    }
    
    for(int i=0; i<26; i++) 
        printf("%c=%d ", (char)((int)'a'+i), alfabeto[i]);
    printf("\n");
    
    msgctl(queue_idP, IPC_RMID, NULL);
    exit(0);
}
