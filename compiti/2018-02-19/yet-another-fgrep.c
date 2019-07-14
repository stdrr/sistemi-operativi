#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>

#define false 0
#define true 1

#define INVERTITO 0
#define CASE_SENSITIVE 1

void outputer(int* pip) {
    close(pip[1]);
    char buff[256];
    while(true) {
        read(pip[0], buff, 256);
        if(strcmp(buff, "")==0) break;
        printf("%s\n");
    }
}

void reader(char* fname, char* word, int* pip, short* opzioni) {
    close(pip[0]); //chiude canale in ingresso
    /*apre e mappa il file*/
    int fd;
    struct stat sb;
    if((fd=open(fname, O_RDONLY|0600))<0) {
        perror("Reader:: open");
    }
    fstat(fd, &sb);
    char* file;
    if((file=(char*)mmap(NULL, sb.st_size, 0600, MAP_PRIVATE, fd, 0))==NULL) {
        perror("Reader:: mmap");
    }
    /***************************/
   
    char* token=strtok(file, "\n"), str[1];
    int go=true;
    while(go) {
        strcpy(str, token);
        if(opzioni[INVERTITO] && opzioni[CASE_SENSITIVE]) {
            if(strcasestr(str, word)!=NULL) {
                write(pip[1], token, strlen(token)+1);
            }
        }
        else if(opzioni[INVERTITO]) {
            if(strstr(str, word)==NULL) {
                write(pip[1], token, strlen(token)+1);
            }
                
        }
        else if(opzioni[CASE_SENSITIVE]) {
            if(strcasestr(str, word)!=NULL) {
                write(pip[1], token, strlen(token)+1);
            }
        }
        else {
            if(strstr(str, word)!=NULL) {
                write(pip[1], token, strlen(token)+1);
            }
        }
        if((token=strtok(NULL, "\n"))==NULL) go=false;
    }
    write(pip[1], "", 2);
    close(pip[1]);
}


void padre(char* fname, int* pipe_in, int* pipe_out) {
    close(pipe_in[1]);
    close(pipe_out[0]);
    
    char buff[256];
    while(true) {
        read(pipe_in[0], buff, 256);
        if(strcmp(buff, "")==0) break;
        write(pipe_out[1], buff, 256);
    }
    
}

int main(int argc, char** argv) {
    if(argc<3) {
        printf("Uso: %s [-v] [-i] [word] <file-1> [file-2] [file-3] [...]", argv[0]);
        exit(1);
    }
    
    int pipe_out[2];
    if(pipe(pipe_out)<0) {
        perror("pipe-output");
    }
    
    if(fork()==0) {
        outputer(pipe_out);
        exit(0);
    }
    
    close(pipe_out[0]); //il padre chiude il canale di input della pipe con il figlio outputer
    
    int n_opzioni=0;
    short opzioni[2]={0,0};
    /*parsing comando*/
    if(strcmp(argv[1],"-v")==0 || strcmp(argv[2], "-v")==0) {
        opzioni[INVERTITO]=true;
        n_opzioni++;
    }
    if(strcmp(argv[1], "-i")==0 || strcmp(argv[2], "-i")==0) {
        opzioni[CASE_SENSITIVE]=true;
        n_opzioni++;
    }
    
    int pipe_in[2];
    if(pipe(pipe_in)<0) {
        perror("pipe-in");
    }
    
    int pid;
    for(int i=n_opzioni+2; i<argc; i++) {
        if((pid=fork())==0) {
            reader(argv[i], argv[n_opzioni+1], pipe_in, opzioni);
            exit(0);
        }
        else {
            padre(argv[n_opzioni+1], pipe_in, pipe_out);
            waitpid(pid, NULL, 0);
        }
    }
    
    write(pipe_out[1], "", 1);
    close(pipe_in[0]);
    close(pipe_out[1]);
    exit(0);
}
