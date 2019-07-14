/*
    Homework n.3

    Usando la possibilita' di mappare file in memoria, creare un programma che
    possa manipolare un file arbitrariamente grande costituito da una sequenza
    di record lunghi N byte.
    La manipolazione consiste nel riordinare, tramite un algoritmo di ordinamento
    a scelta, i record considerando il contenuto dello stesso come chiave:
    ovvero, supponendo N=5, il record [4a a4 91 f0 01] precede [4a ff 10 01 a3].
    La sintassi da supportare e' la seguente:
     $ homework-3 <N> <pathname del file>

    E' possibile testare il programma sul file 'esempio.txt' prodotto dal seguente
    comando, utilizzando il parametro N=33:
     $ ( for I in `seq 1000`; do echo $I | md5sum | cut -d' ' -f1 ; done ) > esempio.txt

    Su tale file, l'output atteso e' il seguente:
     $ homework-3 33 esempio.txt
     $ head -n5 esempio.txt
        000b64c5d808b7ae98718d6a191325b7
        0116a06b764c420b8464f2068f2441c8
        015b269d0f41db606bd2c724fb66545a
        01b2f7c1a89cfe5fe8c89fa0771f0fde
        01cdb6561bfb2fa34e4f870c90589125
     $ tail -n5 esempio.txt
        ff7345a22bc3605271ba122677d31cae
        ff7f2c85af133d62c53b36a83edf0fd5
        ffbee273c7bb76bb2d279aa9f36a43c5
        ffbfc1313c9c855a32f98d7c4374aabd
        ffd7e3b3836978b43da5378055843c67
*/

/* Note:
 * memcmp(const char* str1, const char* str2, size_t len)
 * memmove(const char* dest, const char* src, size_t len)
 */

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

int insertionsort(char* p, int dim, int offset) {
    char p_i[offset];
    for(int i=offset; i<dim; i+=offset) {
        memcpy(p_i, p+i, offset);
        int j=i-offset;
        while(j>=0 && memcmp(p+j, p_i, offset)>0) {
            memcpy(p+j+offset, p+j, offset);
            j-=offset;
        }
        memcpy(p+j+offset, p_i, offset);
    }
}


void swap(char* str1, char* str2, int offset) {
    char tmp[offset];
    if(!(memmove(tmp, str1, offset) && memmove(str1, str2, offset) && memmove(str2, tmp, offset))) {
        perror("memmove");
    }
}

void quicksort(char* p, int l, int r, int offset) {
    if(l>=r) return;
    int i=l, j=r-offset;
    char* pivot[sizeof(char)*offset];
    int middle=(l+r)/2;
    middle=middle+middle%offset;
    memcpy(pivot, p+middle, sizeof(pivot));
    swap(p+r, p+middle, offset);
    while(i<j) {
        while(memcmp(p+i, pivot, sizeof(pivot))<=0 && i<r) i+=offset;
        while(memcmp(p+j, pivot, sizeof(pivot))>0 && j>=l) j-=offset;
        if(i<j) 
            swap(p+i, p+j, offset);
    }
    memcpy(p+r, p+i, offset);
    memcpy(p+i, pivot, offset);
    quicksort(p, l, j, offset);
    quicksort(p, j+offset, r, offset);
}

//****************************************************************************************
int main(int argc, char** argv) {

    if(argc < 3 ) {
        fprintf(stderr, "\nUso: $ %s <N> <pathname del file>\n", argv[0]);
        exit(1);
    }
    
    int fd;
    struct stat sb;
    
    if((fd=open(argv[2], O_RDWR)) == -1) {
        perror("open");
        exit(1);
    }
    
    if(fstat(fd, &sb) == -1) {
        perror("fstat");
        exit(1);
    }
    
    if(!S_ISREG(sb.st_mode)) {
        fprintf(stderr, "\nIl file %s non è un file regolare\n", argv[2]);
        exit(1);
    }
    
    if(sb.st_size%atoi(argv[1])!=0) {
        fprintf(stderr, "\nValore %d non permesso\n", argv[1]);
        exit(1);
    }
    
    char* p;
    
    if((p=mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }
    
    close(fd);

    
    quicksort(p, 0, sb.st_size-1, atoi(argv[1]));
//     insertionsort(p, sb.st_size, atoi(argv[1]));
    
    if(msync(p, sb.st_size,MS_SYNC)!=0) {
        perror("msync");
        exit(1);
    }
    
    
    return 0;
}
