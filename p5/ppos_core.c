// GRR20197153 Arthur Henrique Canello Vilar

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>
#include "queue.h" 
#include "ppos.h"

#define STACKSIZE 64*1024   // tamanho da pilha
#define AGING_FACTOR -1     // fator de envelhecimento
#define QUANTUM 1           // ticks por quantum
//#define DEBUGP5


/*--------------------------------------------VARIÁVEIS GLOBAIS---------------------------------------------*/
int taskId = 0;     // id da tarefa
int userTasks = 0;  // incrementar quando criar uma tarefa e decrementar quando acabar uma tarefa
int temporizador;
task_t *currTask, *prevTask, *readyTasks, mainTask, dispatcherTask;


/*--------------------------------------------DECLARAÇÃO STRUCTS--------------------------------------------*/
struct sigaction action;    // estrutura que define um tratador de sinal
struct itimerval timer;     // estrutura de inicialização to timer


/*--------------------------------------------DECLARAÇÃO FUNÇÕES--------------------------------------------*/
static void dispatcher ();
static void set_timer ();
static void time_handler ();


/*-------------------------------------------------FUNÇÕES--------------------------------------------------*/

// Inicializa as estruturas internas do SO; deve ser chamada no inicio do main()
void ppos_init () {

    // Desativa o buffer da saida padrao (stdout), usado pela função printf
    setvbuf(stdout, 0, _IONBF, 0);

    getcontext(&mainTask.context);
    mainTask.next = mainTask.prev = NULL;
    mainTask.id = taskId;
    mainTask.status = 'E'; // E = executando
    mainTask.prioEst = mainTask.prioDin = 0;
    mainTask.preemptable = 'Y'; // preemptavel

    currTask = &mainTask;

    // cria a tarefa dispatcher (tarefa de gerencia das demais tarefas)
    task_create(&dispatcherTask, dispatcher, NULL);

    set_timer();
}

static void set_timer() {

    // registra a ação para o sinal de timer SIGALRM
    action.sa_handler = time_handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    if (sigaction(SIGALRM, &action, 0) < 0) {
        perror("Erro em sigaction: ");
        exit (1);
    }

    // ajusta valores do temporizador
    timer.it_value.tv_usec = 1000 ;      // primeiro disparo, em micro-segundos
    timer.it_value.tv_sec  = 0 ;      // primeiro disparo, em segundos
    timer.it_interval.tv_usec = 1000 ;   // disparos subsequentes, em micro-segundos
    timer.it_interval.tv_sec  = 0 ;   // disparos subsequentes, em segundos

    // arma o temporizador ITIMER_REAL (vide man setitimer)
    if (setitimer (ITIMER_REAL, &timer, 0) < 0) {
        perror ("Erro em setitimer: ") ;
        exit (1) ;
    }
}

static void time_handler() {
    // se a currTask nao for o dispatcher
    if (currTask->preemptable == 'Y') {
        #ifdef DEBUGP5
            printf ("HANDLER: task %d é preemptavel\n", currTask->id);
        #endif
        if (currTask->quantum > 0)
            currTask->quantum--;
        else
            task_yield();
    }

    return;
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
    task->prioEst = task->prioDin = 0;
    task->preemptable = 'Y';

    #ifdef DEBUG
        printf ("PPOS: task %d criou task %d, tamanho da fila: %d\n", currTask->id, task->id, queue_size((queue_t *) readyTasks));
    #endif
    
    // se a tarefa for o dispatcher nao coloca na fila
    if (taskId == 1) {
        task->preemptable = 'N';
        return taskId;
    }

    // coloca a tarefa na fila de prontos
    queue_append((queue_t **) &readyTasks, (queue_t *) task);
    userTasks++;

    return taskId;
}

// alterna a execução para a tarefa indicada
int task_switch (task_t *task) {

    if (!task) 
        return -1;

    // contexto atual vira o prevTask, task vira currTask
    prevTask = currTask;
    currTask = task;

    if (prevTask->status != 'T')
        prevTask->status = 'P';
    currTask->status = 'E';

    /* ARRUMAR: adicionar quantum */

    #ifdef DEBUG
        printf ("SWITCH: trocando contexto task %d -> task %d\n", prevTask->id, task->id);
    #endif

    // salva o contexto atual em prevTask e restaura o contexto salvo em task
    if (swapcontext(&prevTask->context, &(task->context)) == -1) {
        fprintf(stderr, "Erro na troca de contexto\n");
        return -1;
    }

    return 0;
}

// Termina a tarefa corrente, indicando um valor de status encerramento
void task_exit (int exit_code) {

    #ifdef DEBUG
        printf("PPOS: tarefa %d sendo encerrada\n", currTask->id);
    #endif

    // seta tarefa no status terminada
    currTask->status = 'T';

    // decide se volta para o dispatcher ou para a main
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

    #ifdef DEBUG
        printf("YIELD: task %d sendo trocada\n", currTask->id);
    #endif

    task_switch(&dispatcherTask);
}

// define a prioridade estática de uma tarefa (ou a tarefa atual)
void task_setprio (task_t *task, int prio) {

    if (!task) 
        currTask->prioEst = currTask->prioDin = prio;

    /*ARRUMAR: adicionar prioridade maxima -20 e minima 20*/
    
    task->prioEst = task->prioDin = prio;
    
    #ifdef DEBUG
        printf ("PPOS: prioridade da task %d: %d\n", task->id, task->prioEst);
    #endif
}

// retorna a prioridade estática de uma tarefa (ou a tarefa atual)
int task_getprio (task_t *task) {

    #ifdef DEBUG
        if (task != NULL)
            printf ("PPOS: prioridade da task %d -> estatica %d, dinamica %d\n", task->id, task->prioEst, task->prioDin);
    #endif

    if (task != NULL)
        return task->prioEst;

    return currTask->prioEst;
}

// retorna ponterio pra proxima tarefa da fila a ser executada
static task_t *scheduler () {

    task_t *temp = readyTasks;  // temp no inicio da fila
    task_t *maiorPrio = temp;   // salva a task de maior prioridade (menor valor)

    if(!temp)
        return NULL;

    // percorre a fila procurando a task de maior prioridade
    while ((temp = temp->next) != readyTasks) {
    
        #ifdef DEBUG
            printf ("SCHEDULER: comparando task %d com %d, maiorPrio: %d, tempPrio: %d\n", maiorPrio->id, temp->id, maiorPrio->prioDin, temp->prioDin);
        #endif
    
        // faz o envelhecimento da task
        if (maiorPrio->prioDin > temp->prioDin) {
            maiorPrio->prioDin += AGING_FACTOR;
            maiorPrio = temp;
        } else
            temp->prioDin += AGING_FACTOR;
    }

    maiorPrio->prioDin = maiorPrio->prioEst;

    return maiorPrio;
}

// se fila de prontos estiver vazia o dispatcher encerra
static void dispatcher () {

    task_t *proxima;

    // enquanto existir pelo menos uma task
    while (userTasks > 0) {
        // pega task do scheduler
        proxima = scheduler();

        /* remove tarefa da fila e a executa, se estiver com status de pronta 
         * coloca de novo na fila, se estiver com status de terminada diminui
         * o contador de tasks  */
        if (proxima != NULL) {
            
            #ifdef DEBUG
                printf("DISPATCHER: tarefa %d indo pro switch\n", proxima->id);
            #endif
            
            proxima->status = 'E';
            proxima->quantum = 20;
            task_switch(proxima);

            switch (proxima->status) {

                // se terminada da free
                case 'T':
                    queue_remove((queue_t **) &readyTasks, (queue_t *) proxima);
                    free(proxima->context.uc_stack.ss_sp);
                    userTasks--;
                    break;
            }
        }
    }

    task_exit(0);
}
