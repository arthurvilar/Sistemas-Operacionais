// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 1.4 -- Janeiro de 2022

// Estruturas de dados internas do sistema operacional

#ifndef __PPOS_DATA__
#define __PPOS_DATA__

#include <ucontext.h>		// biblioteca POSIX de trocas de contexto

// Estrutura que define um Task Control Block (TCB)
typedef struct task_t
{
    struct task_t *prev, *next;    // ponteiros para usar em filas
    int id;				        // identificador da tarefa
    int prioEst;                    // prioridade estatica da tarefa
    int prioDin;                    // prioridade dinamica da tarefa
    int quantum;                    // tempo de processador da tarefa
    ucontext_t context;			// contexto armazenado da tarefa
    short status;			        // pronta, rodando, suspensa, ...
    short preemptable;			    // pode ser preemptada? 'Y' ou 'N'
    unsigned int executionTime;     // tempo total da tarefa
    unsigned int processorTime;     // tempo que a tarefa ficou em processador
    int activations;                // numero de ativações da tarefa
    int exitCode;                   // codigo de saida da tarefa
    int suspendedCount;             // numero de tarefas na fila suspendedQueue
    struct task_t *suspendedQueue;  // fila de tarefas suspensas
    int wakeTime;                   // tempo que a tarefa deve acordar
} task_t;

// estrutura que define um semáforo
typedef struct
{
    int value;
    task_t *semQueue;
    int lock;
    int valid;
} semaphore_t;

// estrutura que define um mutex
typedef struct
{
    // preencher quando necessário
} mutex_t;

// estrutura que define uma barreira
typedef struct
{
    // preencher quando necessário
} barrier_t;

// estrutura que define uma fila de mensagens
typedef struct
{
    int max_size;       // tamanho maximo da fila
    int msg_size;       // tamanho da mensagem
    int num_msgs;       // quantidade de mensagens na fila
    int valid;          // 1 = valido, 0 = não valido
    int start;          // inicio do buffer circular
    int end;            // fim do buffer circular
    void *buffer;       // buffer de mensagens
    semaphore_t s_vaga;
    semaphore_t s_item;
    semaphore_t s_buffer;
} mqueue_t;

#endif