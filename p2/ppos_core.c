#include <stdio.h>
#include "ppos.h"

#define STACKZISE 64*1024

int taskId = 0;
task_t *currTask, *prevTask, mainTask;

// Inicializa o sistema operacional; deve ser chamada no inicio do main()
void ppos_init () {

    setvbuf(stdout, 0, _IONBF, 0);

    mainTask.id = taskId;
    mainTask.next = mainTask.prev = NULL;

    currTask = &mainTask;
}

// Cria uma nova tarefa. Retorna um ID> 0 ou erro.
int task_create (task_t *task, void (*start_func)(void *), void *arg) {

}

// Termina a tarefa corrente, indicando um valor de status encerramento
void task_exit (int exit_code) {

}

// alterna a execução para a tarefa indicada
int task_switch (task_t *task) {

}

// retorna o identificador da tarefa corrente (main deve ser 0)
int task_id () {

}