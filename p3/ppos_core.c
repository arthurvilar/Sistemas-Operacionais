#include <stdio.h>
#include <stdlib.h>
#include "queue.h" 
#include "ppos.h"

#define STACKSIZE 64*1024

int taskId = 0;
int userTasks = 0; // incrementar quando criar uma tarefa e decrementar quando acabar uma tarefa
task_t *currTask, *prevTask, *readyTasks, mainTask, dispatcherTask;

static void dispatcher ();

// Inicializa as estruturas internas do SO; deve ser chamada no inicio do main()
void ppos_init () {

    // Desativa o buffer da saida padrao (stdout), usado pela função printf
    setvbuf(stdout, 0, _IONBF, 0);

    mainTask.next = mainTask.prev = NULL;
    mainTask.id = taskId;
    mainTask.status = 'E'; // E = executando

    currTask = &mainTask;

    // cria a tarefa dispatcher (tarefa de gerencia das demais tarefas)
    task_create(&dispatcherTask, dispatcher, NULL);
}

// Cria uma nova tarefa. Retorna um ID> 0 ou erro.
int task_create (task_t *task, void (*start_func)(void *), void *arg) {

    char *stack;

    if (!task) 
        return -1;

    // salva o contexto atual em task->context
    getcontext(&(task->context));

    // aloca pilha do contexto
    stack = malloc (STACKSIZE);
    if (stack)
    {
        task->context.uc_stack.ss_sp = stack;
        task->context.uc_stack.ss_size = STACKSIZE;
        task->context.uc_stack.ss_flags = 0;
        task->context.uc_link = 0;
    }
    else
    {
        perror ("Erro na criação da pilha\n");
        return -1;
    }

    // ajusta valores internos do contexto salvo em task->context
    makecontext (&(task->context), (void*)(*start_func), 1, arg);

    task->status = 'P'; // P =pronto
    task->id = ++taskId;
    task->prev = task->next = NULL;

    #ifdef DEBUG
        printf ("PPOS: task %d criou task %d\n", currTask->id, task->id);
    #endif
    
    // se a tarefa for o dispatcher nao coloca na fila
    if (taskId == 1)
        return taskId;

    // coloca a tarefa na fila de prontos
    queue_append((queue_t **) &readyTasks, (queue_t *) task);
    userTasks++;

    #ifdef DEBUG
        printf ("PPOS: adicionou task %d na fila. Tamanho da fila: %d\n", task->id, queue_size((queue_t *) readyTasks));
        printf ("PPOS: numero de tarefas prontas: %d\n", userTasks);
    #endif

    return taskId;
}

// alterna a execução para a tarefa indicada
int task_switch (task_t *task) {

    if (!task) 
        return -1;

    #ifdef DEBUG
        printf ("PPOS: trocando contexto task %d -> task %d\n", currentTask->id, task->id);
    #endif

    // contxto atual vira o prevTask, task vira currTask
    prevTask = currTask;
    currTask = task;

    // salva o contexto atual em prevTask e restaura o contexto salvo em task
    if (swapcontext(&(prevTask->context), &(task->context)) == -1) {
        fprintf(stderr, "Erro na troca de contexto\n");
        return -1;
    }

    return 0;
}

// Termina a tarefa corrente, indicando um valor de status encerramento
void task_exit (int exit_code) {

    #ifdef DEBUG
        printf("task_exit: tarefa %d sendo encerrada\n", currTask->id);
    #endif

    currTask->status = 'T';

    if (currTask == &dispatcherTask) 
        task_switch(&mainTask);
    else
        task_switch(&dispatcherTask);
}

// retorna o identificador da tarefa corrente (main deve ser 0)
int task_id () {

    return currTask->id;
}

// libera o processador para a próxima tarefa, retornando à fila de tarefas
// prontas ("ready queue"), devolve o processador pro dispatcher task switch
void task_yield () {

    currTask->status = 'P';
    task_switch(&dispatcherTask);
}

// retorna ponterio pra proxima tarefa da fila a ser executada
static task_t *scheduler () {

    return readyTasks;
}

// se fila de prontos estiver vazia o dispatcher encerra
static void dispatcher () {

    task_t *proxima;

    while (userTasks > 0) {
        proxima = scheduler();

        if (proxima != NULL) {
            proxima->status = 'E';
            queue_remove((queue_t **) &readyTasks, (queue_t *) proxima);
            task_switch(proxima);

            switch (proxima->status) {
                // se pronta coloca no fim da fila
                case 'P':
                    queue_append((queue_t **) &readyTasks, (queue_t *) proxima);
                    break;

                // se terminada da free
                case 'T':
                    free(proxima->context.uc_stack.ss_sp);
                    userTasks--;
                    break;
            }
        }
    }

    task_exit(0);
}