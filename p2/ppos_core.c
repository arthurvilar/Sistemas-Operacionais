#include <stdio.h>
#include <stdlib.h>
#include "ppos.h"

#define STACKSIZE 64*1024

int taskId = 0;
task_t *currTask, *prevTask, mainTask;

// Inicializa as estruturas internas do SO; deve ser chamada no inicio do main()
void ppos_init () {

    // Desativa o buffer da saida padrao (stdout), usado pela função printf
    setvbuf(stdout, 0, _IONBF, 0);

    mainTask.id = taskId;
    currTask = &mainTask;
}

// Cria uma nova tarefa. Retorna um ID> 0 ou erro.
int task_create (task_t *task, void (*start_func)(void *), void *arg) {

    char *stack;

    // salva o contexto atual em task->context
    getcontext(&task->context);

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
    makecontext (&task->context, (void*)(*start_func), 1, arg);
    task->id = ++taskId;

    #ifdef DEBUG
        printf ("task_create: criou tarefa %d\n", task->id) ;
    #endif

    return taskId;
}

// alterna a execução para a tarefa indicada
int task_switch (task_t *task) {

    if (!task) {
        return -1;
    }

    #ifdef DEBUG
        printf ("task_switch: trocando contexto %d -> %d\n", currentTask->id, task->id);
    #endif

    // contxto atual vira o prevTask, task vira currTask
    prevTask = currTask;
    currTask = task;

    // salva o contexto atual em prevTask e restaura o contexto salvo em task
    if (swapcontext(&prevTask->context, &task->context) == -1) {
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

    task_switch(&mainTask);
}

// retorna o identificador da tarefa corrente (main deve ser 0)
int task_id () {

    return currTask->id;
}