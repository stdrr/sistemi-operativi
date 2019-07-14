#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <unistd.h>
#include <ctype.h>

#define false 0
#define true 1
#define DELIM "-"

#define KEY 50
#define LINE 512

typedef struct {
    long type;
    int end;
    char line[LINE];
} Messaggio;

void filter1(char* word, int type, int queue_id) {
    Messaggio msg;
    while(true) {
        msg.type=type;
        if(msgrcv(queue_id, &msg, sizeof(msg)-sizeof(long), type, 0)<0) {
            perror("Filter1:: msgrcv");
        }
        msg.type=type+1;
        if(msg.end==true) {
            msgsnd(queue_id, &msg, sizeof(msg)-sizeof(long), 0);
            break;
        }
        char* str=msg.line;
        while((str=strstr(str, word))!=NULL) {
            for(int i=0; i<strlen(word); i++)
                str[i]=(char)toupper((int)str[i]);
        }
        if(msgsnd(queue_id, &msg, sizeof(msg)-sizeof(long), 0)<0) {
            perror("Filter1:: msgsnd");
        }
    }
}

void filter2(char* word, int type, int queue_id) {
    Messaggio msg;
    while(true) {
        msg.type=type;
        if(msgrcv(queue_id, &msg, sizeof(msg)-sizeof(long), type, 0)<0) {
            perror("Filter2:: msgrcv");
        }
        msg.type=type+1;
        if(msg.end==true) {
            msgsnd(queue_id, &msg, sizeof(msg)-sizeof(long), 0);
            break;
        }
        char* str=msg.line;
        while((str=strstr(str, word))!=NULL) {
            int i;
            for(i=0; i<strlen(word); i++) {
                str[i]=(char)tolower((int)str[i]);
            }
        }
        if(msgsnd(queue_id, &msg, sizeof(msg)-sizeof(long), 0)<0) {
            perror("Filter2:: msgsnd");
        }
    }
}

void filter3(char* word, int type, int queue_id) {
    char* find=strtok(word, DELIM);
    char* replace=strtok(NULL, DELIM);
    int len=strlen(find);
    if(len!=strlen(replace)) len=0; 
    
    Messaggio msg;
    while(true) {
        msg.type=type;
        if(msgrcv(queue_id, &msg, sizeof(msg)-sizeof(long), type, 0)<0) {
            perror("Filter3:: msgrcv");
        }
        msg.type=type+1;
        if(msg.end==true) {
            msgsnd(queue_id, &msg, sizeof(msg)-sizeof(long), 0);
            break;
        }
        if(len) {
            char* str=msg.line;
            while((str=strstr(str, find))!=NULL) {
                memmove(str, replace, len);
            }
        }
        if(msgsnd(queue_id, &msg, sizeof(msg)-sizeof(long), 0)<0) {
            perror("Filter3:: msgsnd");
        }
    }
}

void filter(char* exp, int type, int queue_id) {
    if(exp[0]=='^') {
        filter1(exp+1, type, queue_id);
    }
    else if(exp[0]=='_') {
        filter2(exp+1, type, queue_id);
    }
    else if(exp[0]=='%') {
        filter3(exp+1, type, queue_id);
    }
}

int main(int argc, char** argv) {
    if(argc<3) {
        printf("Uso: %s <file.txt> <filter-1> [filter-2] ...\n", argv[0]);
        exit(1);
    }
    
    int queue_id;
    if((queue_id=msgget(KEY, IPC_CREAT|0600))<0) {
        perror("msgget");
        exit(1);
    }
    
    int pid;
    for(int i=2; i<argc; i++) {
        if((pid=fork())==0) {
            filter(argv[i], i-1, queue_id);
            exit(0);
        }
    }
    
    FILE* file=fopen(argv[1], "r");
    Messaggio msg;
    msg.end=false;
    while(true) {
        msg.type=1;    
        if(fgets(msg.line, LINE, file)==NULL) {
            msg.end=true;
            msgsnd(queue_id, &msg, sizeof(msg)-sizeof(long), 0);
            break;
        }
        if(msgsnd(queue_id, &msg, sizeof(msg)-sizeof(long), 0)<0) {
            perror("Padre:: msgsnd");
        }
        msg.type=argc-1;
        if(msgrcv(queue_id, &msg, sizeof(msg)-sizeof(long), argc-1, 0)<0) {
            perror("Padre:: msgrcv");
        }
        printf("%s", msg.line);
    }
    
    waitpid(pid, NULL, 0);
    msgctl(queue_id, IPC_RMID, NULL);
}
