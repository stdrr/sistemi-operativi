/*
    Homework n.4

    Estendere l'esercizio 'homework n.1' affinche' operi correttamente
    anche nel caso in cui tra le sorgenti e' indicata una directory, copiandone
    il contenuto ricorsivamente. Eventuali link simbolici incontrati dovranno
    essere replicati come tali (dovrà essere creato un link e si dovranno
    preservare tutti permessi di accesso originali dei file e directory).

    Una ipotetica invocazione potrebbe essere la seguente:
     $ homework-4 directory-di-esempio file-semplice.txt path/altra-dir/ "nome con spazi.pdf" directory-destinazione
     
    Homework n.1 :
    Scrivere un programma in linguaggio C che permetta di copiare un numero
    arbitrario di file regolari su una directory di destinazione preesistente.

    Il programma dovra' accettare una sintassi del tipo:
     $ homework-1 file1.txt path/file2.txt "nome con spazi.pdf" directory-destinazione
*/


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <libgen.h>

#define BUFFSIZE 1024



int copy(char* pathname, char* dest) {
    struct stat sb;
    
    if(lstat(pathname, &sb)==-1) {              //prendo le informazioni sul file
        perror("lstat");
        return -1;
    }
    
    
    if(S_ISDIR(sb.st_mode)) {                   //verifico se è una cartella o un file
        DIR* dir;
        struct dirent* entry;
        
        if((dir=opendir(pathname))==NULL) {     //se è una cartella, la apro
            perror("opendir");
            return -1;
        }
        
        if (chdir(pathname) ==-1) {
            perror(pathname);
            return -1;
        }
        
        while((entry=readdir(dir))!=NULL) {     //leggo il contenuto della cartella
            if(lstat(entry->d_name, &sb)==-1) { //prendo le informazioni su ogni file contenuto
                perror(entry->d_name);
                return -1;
            }
            
            if(S_ISDIR(sb.st_mode))             //se il file contenuto è una cartella, mi assicuro che non sia un riferimento alla cartella corrente (.) o a quella genitore (..), pena un loop
                if(strcmp(entry->d_name, ".")==0 || strcmp(entry->d_name, "..")==0) continue;
                
            copy(entry->d_name, dest);          //richiamo ricorsivamente la funzione copy(...) sulla cartella, con destinzione dest+nuova_sottocartella
        }
        chdir("..");
        closedir(dir);
        return 0;
    }
    else if(S_ISLNK(sb.st_mode)) {
        
        return 0;
    }
    else {
        int fd, new_fd;
        if((fd=open(pathname, O_RDONLY))==-1) {
            perror(pathname);
            return -1;
        }
        char new_pathname[strlen(pathname)+strlen(dest)];
        strcat(new_pathname,dest);
        strcat(new_pathname, "/");
        strcat(new_pathname, basename(pathname));
        if((new_fd=open(new_pathname, O_WRONLY | O_TRUNC | O_CREAT, 0660))<0) { 
            perror(new_pathname);
            return -1;
        }
        
        char* buff[BUFFSIZE];
        int size;
        do {
            if((size=read(fd, buff, BUFFSIZE))<0) {
                perror(new_pathname);
                return -1;
            }
            if(write(new_fd, buff, size)<0) {
                perror(new_pathname);
                return -1;
            }
        } while(size==BUFFSIZE);
        
        close(fd);
        close(new_fd);
        return 0;
    }
}

int main(int argc, char** argv) {
    if(argc<3) {
        printf("Uso: $ %s directory-di-esempio file-semplice.txt path/altra-dir/ \"nome con spazi.pdf\" directory-destinazione\n", argv[0]);
        exit(1);
    }
    
    int i=1;
    while(i<argc-1) {
        if(copy(argv[1], argv[argc-1])==-1) {
            fprintf(stderr, "\nErrore durante la copia di %s in %s\n", argv[i], argv[argc-1]);
        }
        i++;
    }
    return 0;
}
