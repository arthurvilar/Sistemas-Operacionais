#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include "ppos_data.h"
#include "ppos_disk.h"
#include "disk.h"
#include "ppos.h"

disk_t disk;
struct sigaction action;

// inicializacao do gerente de disco
// retorna -1 em erro ou 0 em sucesso
// numBlocks: tamanho do disco, em blocos
// blockSize: tamanho de cada bloco do disco, em bytes
int disk_mgr_init (int *numBlocks, int *blockSize) {

    // inicia o disco
    if (disk_cmd(DISK_CMD_INIT, 0, 0))
        return -1;

    // registra o sinal de SIGUSR1
    action.sa_handler = sigusr_handler;
    sigemptyset (&action.sa_mask) ;
    action.sa_flags = 0 ;
    if (sigaction (SIGUSR1, &action, 0) < 0)
    {
        perror ("sigaction error!") ;
        exit (1) ;
    }

    sem_create(&disk.disk_access, 1);
    sem_down(&disk.disk_access);
    
    *numBlocks = disk_cmd(DISK_CMD_DISKSIZE, 0, 0);
    *blockSize = disk_cmd(DISK_CMD_BLOCKSIZE, 0, 0);

    if ( *numBlocks < 0 || *blockSize < 0 ) return -1;

    sem_up(&disk.disk_access);
    
    return 0;
}

// leitura de um bloco, do disco para o buffer
int disk_block_read (int block, void *buffer) {
    return 0;
}

// escrita de um bloco, do buffer para o disco
int disk_block_write (int block, void *buffer) {
    return 0;
}