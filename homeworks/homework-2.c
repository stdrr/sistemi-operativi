/*
    Homework n.2

    Estendere l'esempio 'move.c' visto a lezione per supportare i 2 casi speciali:
    - spostamento cross-filesystem: individuato tale caso, il file deve essere
     spostato utilizzando la strategia "copia & cancella";
    - spostamento di un link simbolico: individuato tale caso, il link simbolico
     deve essere ricreato a destinazione con lo stesso contenuto (ovvero il percorso
     che denota l'oggetto referenziato); notate come tale spostamento potrebbe
     rendere il nuovo link simbolico non effettivamente valido.

    La sintassi da supportare e' la seguente:
     $ homework-2 <pathname sorgente> <pathname destinazione>
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define BUFFSIZE 1024

//************************************************************************
//  Metodo per copiare un file
//************************************************************************
int copy_file(char* fpath, char* new_fpath) {
	int fd, new_fd;
	
	if((fd = open(fpath, O_RDONLY)) < 0) {
		perror(fpath);
		return -1;
	}
		
	if((new_fd = open(new_fpath, O_WRONLY | O_CREAT | O_EXCL, 0660)) < 0) {
		perror(new_fpath);
		return -1;
	}
	
	int size = 0;
	char* buff[BUFFSIZE];

	do {
		if((size = read(fd, buff, BUFFSIZE)) < 0) {
			perror(new_fpath);
			return -1;
		}
		if(write(new_fd, buff, size) < 0) {
			perror(new_fpath);
			return -1;
		}
	} while (size == BUFFSIZE);
	close(fd);
	close(new_fd);
	return 0;
}
//****************************************************************************

//****************************************************************************
//  Metodo per cancellare un link
//****************************************************************************
int delete_link(char* link) {
    if(unlink(link) == -1) {
        perror(link);
        return -1;
    }
    return 0;
}
//****************************************************************************

//****************************************************************************
//  Metodo per creare un link
//****************************************************************************
int move(char* fpath, char* new_fpath) {
    struct stat statbuf;
    if(lstat(fpath, &statbuf) == -1) {
        perror(fpath);
        return -1;
    }
    
    char* buf_fpath = malloc(statbuf.st_size + 1);
    memset(buf_fpath, 0, sizeof(buf_fpath));
    if(S_ISLNK(statbuf.st_mode)) {
        printf("\nLink\n");
        int n = readlink(fpath, buf_fpath, statbuf.st_size + 1);
        if(n < 0) {
            perror("readlink");
            return -1;
        }
        else if(n > statbuf.st_size) {
            fprintf(stderr, "not enought space\n");
            return -1;
        }
        buf_fpath[n] = '\0';
//         memset(fpath, 0, sizeof(fpath));
//         strcat(fpath, buff);
//         free(buff);
        if(copy_file(buf_fpath, new_fpath) == -1)       // Se la creazione dell'hard link fallisce (spostamento cross-filesystem), crea un soft link
            return -1;
    }
    else {
        strcat(buf_fpath, fpath);
    
    if(link(buf_fpath, new_fpath) == -1) {              // Crea un hard link
        if(copy_file(buf_fpath, new_fpath) == -1)       // Se la creazione dell'hard link fallisce (spostamento cross-filesystem), crea un soft link
            return -1;
    }
    }
    
    if(delete_link(buf_fpath) == -1)                    // Cancella (unlink) il file sorgente
        return -1;
    
    free(buf_fpath);
    return 0;
}


int main(int argc, char** argv) {
    if(argc < 3) {
        printf("\nSintassi: $%s <pathname sorgente> <pathname destinazione>\n", argv[0]);
        exit(1);
    }
    
    if(move(argv[1], argv[2]) == -1) {
        exit(1);
    }
    
    return 0;
}
