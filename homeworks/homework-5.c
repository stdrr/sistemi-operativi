/*
    Homework n.5

    Estendere l'esempio 'nanoshell.c' ad una shell piu' realistica in cui
    si possono:
    - passare argomenti al comando invocato (per semplicita', assumiamo
    che questi non contengano spazi);
    - usare la redirezione dei canali di input/output/error;
    - mandare un comando in esecuzione in background (con la '&' finale).

    Esempi di invocazione che la shell dovrebbe supportare sono:
    $ cal 3 2015
    $ cp /etc/passwd /etc/hosts /tmp
    $ cat </dev/passwd >/tmp/passwd
    $ cat filechenonesiste 2>/dev/null
    $ ls -R /etc/ &
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#define true 1
#define false 0
#define BUFFSIZE 1024
#define NUMARGS 32
#define ARGSSIZE 1024

typedef struct {
    char* args[NUMARGS];
    int redirection[3];
    int background;
    int redir_index;
} Commands;

/*
void getCmd(char* buff, char* cmd) {
    int i=0;
    while(buff[i]!=' ' && buff[i]!='\0' && i<CMDSIZE) {
        cmd[i]=buff[i];
        i++;
    }
    cmd[i]='\0';
}
*/
/*
int numArgs(char* buff) {
    int i=0, args=0;
    while(buff[i]!='\0') {
        if(buff[i++]==' ')
            args++;
    }
    return ++args;
}
*/
int out_redirection(char* buff, int i) {
    int j=0;
    char* fpath=calloc(ARGSSIZE, sizeof(char));
    while(buff[i]!='\0' && buff[i]!='&') {
        i++;
        if(buff[i]==' ') continue;
        fpath[j++]=buff[i];
    }
    int fd;
    if((fd=open(fpath, O_CREAT | O_TRUNC, 0600))<0) {
        perror("open");
        i=-1;
    }
    else {
        close(1); //chiude il canale di stdout
        dup(fd);
    }
    free(fpath);
    return i;
}

int in_redirection(char* buff, int i) {
    int j=0;
    char* fpath=calloc(ARGSSIZE, sizeof(char));
    while(buff[i]!='\0' && buff[i]!='&') {
        i++;
        if(buff[i]==' ') continue;
        fpath[j++]=buff[i];
    }
    int fd;
    if((fd=open(fpath, O_RDONLY, 0600))<0) {
        perror("open");
        i=-1;
    }
    else {
        close(0); //chiude il canale di stdin
        dup(fd);
    }
    free(fpath);
    return i;
}

int err_redirection(char* buff, int i) {
    char* fpath=calloc(ARGSSIZE, sizeof(char));
    while(buff[i]!='\0' && buff[i]!='&') {
        if(buff[i]!=' ') continue;
        fpath[i]=buff[i];
        i++;
    }
    int fd;
    if((fd=open(fpath, O_WRONLY | O_CREAT | O_TRUNC, 0600))<0) {
        perror("open");
        i=-1;
    }
    else {
        close(2); //chiude il canale di stderr
        dup(fd);
    }
    free(fpath);
    return i;
}

void io_redirection(char* buff, Commands* cmd, int i) {
//     cmd->redirection={0,0,0};
    for(int j=0; j<3; j++) 
        cmd->redirection[j]=0;
    if(buff[i]=='<')
        //return in_redirection(buff, i);
        cmd->redirection[0]=1;
    if(buff[i]=='>')
        //return out_redirection(buff, i);
        cmd->redirection[1]=1;
    cmd->redir_index=i;
}

int getArgs(char* buff, Commands* cmd) {
    int i=0, j=0, k=0, go=true, more=false;
    
    cmd->args[0]=calloc(ARGSSIZE, sizeof(char));
    
    while(go) {
        if(buff[i]=='\0' || buff[i]=='&' || buff[i]=='<' || buff[i]=='>') {
            go=false;
        }
        else if(buff[i]!=' ') {
            if(more) {
                cmd->args[j]=calloc(ARGSSIZE, sizeof(char));
                more=false;
            }
            cmd->args[j][k]=buff[i];
            k++;
        }
        else {
            cmd->args[j][k]='\0';
            k=0;
            j++;
            more=true;
        }
        i++;
    }
    if(buff[i]=='\0') cmd->args[j+1]=NULL;
    else cmd->args[j]=NULL;
    return --i;
}

void background(char* buff, Commands* cmd, int i) {
    if(buff[i]=='&')
        cmd->background=true;
    else cmd->background=false;
}

int parse(char* buff, Commands* cmd) {
    int i=getArgs(buff, cmd);
    //if((i=io_redirection(buff, cmd, i))<0)
//         return -1;
    io_redirection(buff, cmd, i);
    background(buff, cmd, i);
    return 0;
}

int main(int argc, char** argv) {
    printf(">> Shell homework-5, inserire comando [e argomenti]; q per uscire\n\n");
    
    char buff[BUFFSIZE];
    int len, pid;
    while(true) {
        printf(">> ");
        fgets(buff, BUFFSIZE, stdin);
        
        len=strlen(buff);
        if(buff[len-1]=='\n') 
            buff[len-1]='\0';
        
        if(strcmp(buff, "q")==0)
            exit(0);
        
        Commands cmd;
        if(parse(buff, &cmd)<0) 
            continue;
        
        pid=fork();
        if(pid==0) {
            if(cmd.redirection[0] & 1)
                in_redirection(buff,cmd.redir_index);
            if(cmd.redirection[1] & 1)
                out_redirection(buff,cmd.redir_index);
            execvp(cmd.args[0], cmd.args);
        }
        else {
            if(cmd.background==false) 
                waitpid(pid, NULL, 0);
            else printf("[1] %d\n", pid);
            
            int i=0;
            while(cmd.args[i]!=NULL) 
                free(cmd.args[i++]);
        }
    }
    exit(0);
}

