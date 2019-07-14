#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>

#define SM_RP 0
#define SE_RP 1
#define SF_RP 2
#define SM_WP 3
#define SE_WP 4
#define SF_WP 5
#define KEY_R 40
#define KEY_W 50
#define KEY_S 60
#define MSIZE 128

int WAIT(int sem_des, int num_semaforo){
    struct sembuf operazioni[1] = {{num_semaforo,-1,0}};
    return semop(sem_des, operazioni, 1);
}

int SIGNAL(int sem_des, int num_semaforo){
    struct sembuf operazioni[1] = {{num_semaforo,+1,0}};
    return semop(sem_des, operazioni, 1);
}



void reader(int rp_id, int semid, char* fname) {
    int fd;
    if((fd=open(fname, O_RDONLY|0600))<0) {
        perror("Reader:: open");
    }
    
    struct stat sb;
    fstat(fd, &sb);
    
    char* file;
    if((file=mmap(NULL, sb.st_size, PROT_READ|0600, MAP_PRIVATE, fd, 0))==NULL) {
        perror("Reader:: mmap");
        exit(1);
    }
    
    char* shm;
    if((shm=(char*)shmat(rp_id, NULL, 0))==NULL) {
        perror("Reader:: shmat");
        exit(1);
    }
    
    int i=0, j, len=strlen(file);
    while(i<len) {
        j=0;
        WAIT(semid, SE_RP);
        WAIT(semid, SM_RP);
        while(file[i]!='\n' && i<len) {
            shm[j++]=file[i];
            i++;
        }
        shm[j]='\0';
        SIGNAL(semid, SM_RP);
        SIGNAL(semid, SF_RP);
        i++;
    }
    
    WAIT(semid, SE_RP);
    WAIT(semid, SM_RP);
    strcpy(shm, "");
    SIGNAL(semid, SM_RP);
    SIGNAL(semid, SF_RP);
    munmap(file, sb.st_size);
    shmdt(shm);
}

void writer(int wp_id, int semid, char* out) {
    if(out!=NULL) {
        int fd=open(out, O_WRONLY|0600);
        close(1);
        dup(fd);
    }
    
    char* shm;
    if((shm=(char*)shmat(wp_id, NULL, 0))==NULL) {
        perror("Writer:: shmat");
    }
    
    while(1) {
        WAIT(semid, SF_WP);
        WAIT(semid, SM_WP);
        if(strcmp(shm, "")==0) {
            SIGNAL(semid, SM_WP);
            SIGNAL(semid, SE_WP);
            break;
        }
        printf("%s\n", shm);
        SIGNAL(semid, SM_WP);
        SIGNAL(semid, SE_WP);
    }
    shmdt(shm);
}

int main(int argc, char** argv) {
    if(argc<2 || argc>3) {
        printf("Uso: %s <input file> [output file]\n", argv[0]);
        exit(1);
    }
    
    int rp_id, wp_id;
    if((rp_id=shmget(KEY_R, MSIZE, IPC_CREAT|0600))<0) {
        perror("Padre:: shmget:R-P");
        exit(1);
    }
    
    if((wp_id=shmget(KEY_W, MSIZE, IPC_CREAT|0600))<0) {
        perror("Padre:: shmget:W-P");
        shmctl(rp_id, IPC_RMID, NULL);
        exit(1);
    }
    
    int semid;
    if((semid=semget(KEY_S, 6, IPC_CREAT|0600))<0) {
        perror("Padre:: semget");
        shmctl(rp_id, IPC_RMID, NULL);
        shmctl(wp_id, IPC_RMID, NULL);
        exit(1);
    }
    
    short semvals[6]={1,1,0,1,1,0};
    semctl(semid, 0, SETALL, semvals);
    
    if(fork()==0) {
        reader(rp_id, semid, argv[1]);
        exit(0);
    }
    
    char* output=NULL;
    if(argc==3) output=argv[2]; 
    int pid_writer;
    if((pid_writer=fork())==0) {
        writer(wp_id, semid, output);
        exit(0);
    }
    
    char *shmr, *shmw;
    if((shmr=(char*)shmat(rp_id, NULL, 0))==NULL) {
        perror("Padre:: shmat:R-P");
    }
    
    if((shmw=(char*)shmat(wp_id, NULL, 0))==NULL) {
        perror("Padre:: shmat:W-P");
    }
    
    int len, i;
    while(1) {
        i=0;
        WAIT(semid, SF_RP);
        WAIT(semid, SM_RP);
        if(strcmp(shmr, "")==0) {
            SIGNAL(semid, SM_RP);
            SIGNAL(semid, SE_RP);
            break;
        }
        len=strlen(shmr);
        while(i<(len-1)/2 && shmr[i]==shmr[len-1-i]) {
            i++;
        }
        if(i==(len-1)/2) {
            WAIT(semid, SE_WP);
            WAIT(semid, SM_WP);
            strcpy(shmw, shmr);
            SIGNAL(semid, SM_WP);
            SIGNAL(semid, SF_WP);
        }
        SIGNAL(semid, SM_RP);
        SIGNAL(semid, SE_RP);
    }
    
    WAIT(semid, SE_WP);
    WAIT(semid, SM_WP);
    strcpy(shmw, "");
    SIGNAL(semid, SM_WP);
    SIGNAL(semid, SF_WP);
    
    waitpid(pid_writer, NULL, 0);
    
    shmdt(shmr);
    shmdt(shmw);
    shmctl(rp_id, IPC_RMID, NULL);
    shmctl(wp_id, IPC_RMID, NULL);
    semctl(semid, 0, IPC_RMID, 0);
    exit(0);
}
