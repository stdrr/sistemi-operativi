/*
    Homework n.1

    Scrivere un programma in linguaggio C che permetta di copiare un numero
    arbitrario di file regolari su una directory di destinazione preesistente.

    Il programma dovra' accettare una sintassi del tipo:
     $ homework-1 file1.txt path/file2.txt "nome con spazi.pdf" directory-destinazione
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#define BUFFSIZE 1024

char *get_file_name(char* fpath) {
	int i=0, j=0;
	char* buff = malloc(sizeof(char)*strlen(fpath));
	while(fpath[i] != '\0') {
		if(fpath[i] == '/') {
			j=0;
		}
		else {
			buff[j] = fpath[i];
			j++;
		}
		i++;
	}
	buff[j] = '\0';
	char* fname = malloc(sizeof(char)*strlen(buff));
	strcat(fname, buff);
	free(buff);
	return fname;	
}

int copy_one_file(char* fpath, char* dest_dir) {
	int fd, new_fd;
	
	if((fd = open(fpath, O_RDONLY)) < 0) {
		perror(fpath);
		return -1;
	}
		
	char* fname = get_file_name(fpath);
	char new_fpath[strlen(dest_dir) + strlen(fname)];
	memset(new_fpath, 0, sizeof(new_fpath));
	strcat(new_fpath, dest_dir);
    strcat(new_fpath, "/");
	strcat(new_fpath, fname);
	
	if((new_fd = open(new_fpath, O_WRONLY | O_CREAT | O_EXCL, 0660)) < 0) {
		perror(fname);
		return -1;
	}
	
	int size = 0;
	char* buff[BUFFSIZE];

	do {
		if((size = read(fd, buff, BUFFSIZE)) < 0) {
			perror(fname);
			return -1;
		}
		if(write(new_fd, buff, size) < 0) {
			perror(fname);
			return -1;
		}
	} while (size == BUFFSIZE);
	close(fd);
	close(new_fd);
	free(fname);
	return 0;
}

int main(int argc, char** argv) {
	if(argc < 2) {
        	printf("\nUso: %s path/file1.txt [path/file2.txt] [\"nome con spazi.pdf\"] directory-destinazione\n", argv[0]);
        	exit(1);
   	}

    	for(int i=1; i<argc-1; i++) {
        	if(copy_one_file(argv[i], argv[argc-1]) == -1)
            		printf("\nCopia file %s fallita\n", argv[0]);
    	}

    	return 0;
}
