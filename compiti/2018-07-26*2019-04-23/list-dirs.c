#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>

#define false 0
#define true 1

#define SM 0
#define SF 1
#define SD 2
#define SEXP 3

#define KEY 50
#define MEMSIZE 2048 

typedef struct {
    int processi_attivi;
    char info[MEMSIZE];
} ShMemory;

int WAIT(int sem_des, int num_semaforo){
    struct sembuf operazioni[1] = {{num_semaforo,-1,0}};
    return semop(sem_des, operazioni, 1);
}

int SIGNAL(int sem_des, int num_semaforo){
    struct sembuf operazioni[1] = {{num_semaforo,+1,0}};
    return semop(sem_des, operazioni, 1);
}

void file_consumer(int mid, int semid) {
    ShMemory* shm;
    if((shm=(ShMemory*)shmat(mid, NULL, 0))==NULL) {    
        perror("File-Consumer: shmat");
    }
    
    int go=true;
    while(go) {
        WAIT(semid, SF);
        printf("%s\n", shm->info);
        if(shm->processi_attivi==1) go=false;
        SIGNAL(semid, SM);
    }
    
    shmdt(shm);
    shmctl(mid, IPC_RMID, NULL);
    semctl(semid, 0, IPC_RMID, 0);
    exit(0);
}

void dir_consumer(int mid, int semid) {
    ShMemory* shm;
    if((shm=(ShMemory*)shmat(mid, NULL, 0))==NULL) {    
        perror("Dir-Consumer: shmat");
    }
    
    int go=true;
    while(go) {
        WAIT(semid, SD);
        printf("%s\n", shm->info);
        if(shm->processi_attivi==2) { 
            go=false;
            shm->processi_attivi--;
        }
        SIGNAL(semid, SM);
    }
    
    SIGNAL(semid, SF);
    shmdt(shm);
    exit(0);
}

void explorer(char* path, int mid, int semid) {
    chdir(path);
    ShMemory* shm;
    if((shm=(ShMemory*)shmat(mid, NULL, 0))==NULL) {    
        perror("Explorer: shmat");
    }
    
    DIR *dir;
    if((dir=opendir(path))==NULL) {
        perror("opendir");
    }
    
    struct dirent* list_dir;
    struct stat info;
    while(true) {
        if((list_dir=readdir(dir))==NULL) break;
        
        lstat(list_dir->d_name, &info);
        
        if(S_ISDIR(info.st_mode) && strcmp(list_dir->d_name, ".")!=0 && strcmp(list_dir->d_name, "..")!=0) {
            WAIT(semid, SM);
            sprintf(shm->info, "%s%s [directory]", path, list_dir->d_name);
            SIGNAL(semid, SD);
        }
        else if(S_ISREG(info.st_mode)) {
            WAIT(semid, SM);
            sprintf(shm->info, "%s%s [%d B]", path, list_dir->d_name, info.st_size);
            SIGNAL(semid, SF);
        }
    }
//     printf("Explorer\n");
    WAIT(semid, SM);
    shm->processi_attivi--;
    if(shm->processi_attivi==2) SIGNAL(semid, SD);
    SIGNAL(semid, SM);
    
    shmdt(shm);
    exit(0);
}

int main(int argc, char** argv) {
    if(argc<2) {
        printf("Uso: %s <dir1> <dir2> ...\n", argv[0]);
        exit(1);
    }
    
    int mid;
    if((mid=shmget(KEY, MEMSIZE, IPC_CREAT|0600))<0) {
        perror("shmget");
        exit(1);
    }
    
    ShMemory* shm;
    if((shm=(ShMemory*)shmat(mid, NULL, 0))==NULL) {
        perror("shmat");
    }
    
    shm->processi_attivi=argc+1; //il numero di explorers è argc-1; considero anche i due consumer; quindi: argc-1+2=argc+1
    
    int semid;
    if((semid=semget(KEY, 3, IPC_CREAT|0600))<0) {
        perror("semget");
    }
    
    semctl(semid, SM, SETVAL, 1);
    semctl(semid, SF, SETVAL, 0);
    semctl(semid, SD, SETVAL, 0);
    
    int pid_file;
    if((pid_file=fork())==0) {
        file_consumer(mid, semid);
    }
    
    if((fork())==0) {
        dir_consumer(mid, semid);
    }
    
    for(int i=1; i<argc; i++) {
        if(fork()==0) {
            explorer(argv[i], mid, semid);
            break;
        }
    }
    
    shmdt(shm);
    waitpid(pid_file, NULL, 0);
    exit(0);
} 
