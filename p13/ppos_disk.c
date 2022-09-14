#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <time.h>
#include "variaveis_globais.h"
#include "ppos_data.h"
#include "ppos_disk.h"
#include "queue.h"
#include "disk.h"
#include "ppos.h"

disk_t disk;
task_t disk_manager_task;
struct sigaction action_disk;


static void diskDriverBody() {

    request_t *req;

    while (1) {
        // obtém o semáforo de acesso ao disco
        sem_down(&disk.disk_access);
    
        // se foi acordado devido a um sinal do disco
        if (disk.sinal) {
            // acorda a tarefa cujo pedido foi atendido
            task_resume((task_t *) req->task, (task_t**) &disk.suspended_queue);
            queue_remove((queue_t **) &disk.suspended_queue, (queue_t *) req);
            disk.sinal = 0;
        }
    
        int status = disk_cmd(DISK_CMD_STATUS, 0, 0);
        // se o disco estiver livre e houver pedidos de E/S na fila
        if (status == 1 && (disk.request_queue != NULL)) {
            //printf("=-=-=-=-=-=-=-=-=-=-entrou no if status = 1\n");

            // escolhe na fila o pedido a ser atendido, usando FCFS
            req = disk.request_queue;

            // solicita ao disco a operação de E/S, usando disk_cmd()
            disk_cmd(req->op, req->block, req->buffer);
        }

        // libera o semáforo de acesso ao disco
        sem_up(&disk.disk_access);

        queue_remove((queue_t **) &readyTasks, (queue_t *) &disk_manager_task);
        disk_manager_task.status = 'S';

        // suspende a tarefa corrente (retorna ao dispatcher)
        task_yield();
   }
}

static void sigusr_handler() {
    printf("SIGUSR_HANDLER: antes do if, status do disk_manager_task = %c\n", disk_manager_task.status);
    if (disk_manager_task.status == 'S') {
        printf("SIGUSR_HANDLER: append do disk_manager_task em readyTasks\n");
        queue_append((queue_t **) &readyTasks, (queue_t *) &disk_manager_task);
        disk_manager_task.status = 'P';
    }
    printf("SIGUSR_HANDLER: depois do if\n");

    disk.sinal = 1;
}

static request_t *create_request(int block, void *buffer, int request_op) {

    //printf("CREATE_REQUEST: dentro da função\n");
    request_t *new_req = malloc(sizeof(request_t));
    if (!new_req) 
        return 0;

    new_req->prev = NULL;
    new_req->next = NULL;
    new_req->buffer = buffer;
    new_req->block = block;
    new_req->op = request_op;
    new_req->task = currTask;

    return new_req;
}

// inicializacao do gerente de disco
// retorna -1 em erro ou 0 em sucesso
// numBlocks: tamanho do disco, em blocos
// blockSize: tamanho de cada bloco do disco, em bytes
int disk_mgr_init (int *numBlocks, int *blockSize) {

    printf("DISK_MGR_INIT: entrou no disk_mgr_init\n");

    // inicia o disco
    if (disk_cmd(DISK_CMD_INIT, 0, 0) < 0)
        return -1;
    printf("DISK_MGR_INIT: iniciou o disco\n");

    *numBlocks = disk_cmd(DISK_CMD_DISKSIZE, 0, 0);
    *blockSize = disk_cmd(DISK_CMD_BLOCKSIZE, 0, 0);
    if (*numBlocks < 0 || *blockSize < 0) 
        return -1;
    printf("DISK_MGR_INIT: num de blocos e tamanho ok\n");

    sem_create(&disk.disk_access, 1);
    printf("DISK_MGR_INIT: criou disk.sem\n");

    disk.sinal = 0;
    disk.request_queue = NULL;
    disk.suspended_queue = NULL;

    task_create(&disk_manager_task, diskDriverBody, NULL);
    disk_manager_task.status = 'S';
    printf("DISK_MGR_INIT: criou disk_manager_task\n");

    // registra o sinal de SIGUSR1
    printf("DISK_MGR_INIT: chamando sigusr_handler\n");
    action_disk.sa_handler = sigusr_handler;
    printf("DISK_MGR_INIT: sigusr_handler acabou\n");

    sigemptyset(&action_disk.sa_mask);
    action_disk.sa_flags = 0;
    if (sigaction(SIGUSR1, &action_disk, 0) < 0) {
        perror("Sigaction error!");
        exit(1);
    }
    printf("DISK_MGR_INIT: sigaction OK\n");
    
    return 0;
}

// leitura de um bloco, do disco para o buffer
int disk_block_read (int block, void *buffer) {

    // obtém o semáforo de acesso ao disco
    sem_down(&disk.disk_access);    
    
    // cria nova requisição
    //printf("DISK_BLOCK_READ: criando nova request\n");
    request_t *new_req = create_request(block, buffer, DISK_CMD_READ);
    if (!new_req)
        return -1;

    // inclui o pedido na fila_disco
    queue_append((queue_t **) &disk.request_queue, (queue_t *) new_req);

    if (disk_manager_task.status == 'S') {
        // acorda o gerente de disco (põe ele na fila de prontas)
        queue_append((queue_t **) &readyTasks, (queue_t *) &disk_manager_task); 
        disk_manager_task.status = 'P';
    }
    
    // libera semáforo de acesso ao disco
    sem_up(&disk.disk_access); 
    
    // suspende a tarefa corrente (retorna ao dispatcher) erro
    printf("DISK_BLOCK_READ: removendo task %d de readyTask\n", currTask->id);
    queue_remove((queue_t **) &readyTasks, (queue_t *) currTask);
    currTask->status = 'S'; 
    printf("DISK_BLOCK_READ: colocando task %d na suspended queue\n", currTask->id);
    queue_append((queue_t **) &disk.suspended_queue, (queue_t *) currTask);

    //task_yield(); // da errado se deixar ?

    return 0;
}

// escrita de um bloco, do buffer para o disco
int disk_block_write (int block, void *buffer) {
    
    if (!buffer)
        return -1;

    // obtém o semáforo de acesso ao disco
    sem_down(&disk.disk_access);    
    
    // cria nova requisição
    request_t *new_req = create_request(block, buffer, DISK_CMD_WRITE);
    if (!new_req)
        return -1;

    // inclui o pedido na fila_disco
    queue_append((queue_t **) &disk.request_queue, (queue_t *) new_req);

    if (disk_manager_task.status == 'S') {
        // acorda o gerente de disco (põe ele na fila de prontas)
        queue_append((queue_t **) &readyTasks, (queue_t *) &disk_manager_task);
        disk_manager_task.status = 'P';
    }
    
    // libera semáforo de acesso ao disco
    sem_up(&disk.disk_access); 
    
    // suspende a tarefa corrente (retorna ao dispatcher) erro
    task_suspend((task_t **) &disk.suspended_queue);

    task_yield();

    return 0;
}