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
//#define DEBUG


/*--------------------------------------------VARIÁVEIS GLOBAIS---------------------------------------------*/
int taskId = 0;     // id da tarefa
int userTasks = 0;  // incrementar quando criar uma tarefa e decrementar quando acabar uma tarefa
task_t *currTask, *prevTask, *readyTasks, *suspendedTasks, mainTask, dispatcherTask;
unsigned int totalTicks, processorTime;
/*----------------------------------------------------------------------------------------------------------*/


/*--------------------------------------------DECLARAÇÃO STRUCTS--------------------------------------------*/
struct sigaction action;    // estrutura que define um tratador de sinal
struct itimerval timer;     // estrutura de inicialização to timer
/*----------------------------------------------------------------------------------------------------------*/


/*--------------------------------------------DECLARAÇÃO FUNÇÕES--------------------------------------------*/
static void dispatcher ();      // função dispatcher
static void set_timer ();       // inicia o timer
static void time_handler ();    // trata os ticks do timer
/*----------------------------------------------------------------------------------------------------------*/


/*-------------------------------------------------FUNÇÕES--------------------------------------------------*/

// Inicializa as estruturas internas do SO; deve ser chamada no inicio do main()
void ppos_init () {

    // Desativa o buffer da saida padrao (stdout), usado pela função printf
    setvbuf(stdout, 0, _IONBF, 0);

    getcontext(&mainTask.context);
    mainTask.next = mainTask.prev = NULL;
    mainTask.id = taskId;
    mainTask.status = 'P'; // P = pronto
    mainTask.prioEst = mainTask.prioDin = 0;
    mainTask.preemptable = 'Y'; // preemptavel
    mainTask.suspendedQueue = NULL;
    mainTask.suspendedCount = 0;
    mainTask.executionTime = systime();
    mainTask.processorTime = 0;
    mainTask.activations = 0;
    mainTask.wakeTime = 0;
    queue_append((queue_t **) &readyTasks, (queue_t *) &mainTask);

    userTasks++;
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
    
    totalTicks++;
    currTask->processorTime++;

    // se a currTask nao for o dispatcher
    if (currTask->preemptable == 'Y') {
        if (currTask->quantum > 0)
            currTask->quantum--;
        else
            task_yield();
    }

    return;
}


// retorna tempo atual
unsigned int systime() {
    return totalTicks;
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
    task->executionTime = systime();
    task->processorTime = 0;
    task->activations = 0;
    task->suspendedQueue = NULL;
    task->suspendedCount = 0;
    task->wakeTime = 0;
    
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

    // incrementa o contador de ativações
    task->activations++;

    // contexto atual vira o prevTask, task vira currTask
    prevTask = currTask;
    currTask = task;

    if (prevTask->status != 'T')
        prevTask->status = 'P';
    currTask->status = 'E';

    // salva o contexto atual em prevTask e restaura o contexto salvo em task
    if (swapcontext(&prevTask->context, &(task->context)) == -1) {
        fprintf(stderr, "Erro na troca de contexto\n");
        return -1;
    }

    return 0;
}


// retorna o identificador da tarefa corrente (main deve ser 0)
int task_id () {

    return currTask->id;
}


// libera o processador para a próxima tarefa, retornando à fila de tarefas
// prontas ("ready queue"), devolve o processador pro dispatcher task switch
void task_yield () {

    task_switch(&dispatcherTask);
}


// define a prioridade estática de uma tarefa (ou a tarefa atual)
void task_setprio (task_t *task, int prio) {

    if (!task) 
        currTask->prioEst = currTask->prioDin = prio;
    
    task->prioEst = task->prioDin = prio;
}


// retorna a prioridade estática de uma tarefa (ou a tarefa atual)
int task_getprio (task_t *task) {

    if (task != NULL)
        return task->prioEst;

    return currTask->prioEst;
}


// suspende a tarefa atual na fila "queue"
void task_suspend (task_t **queue) {

    queue_remove((queue_t **) &readyTasks, (queue_t *) currTask);
    currTask->status = 'S'; // S = suspensa
    queue_append((queue_t **) queue, (queue_t *) currTask);

    task_yield();
}


// acorda a tarefa indicada, que está suspensa na fila indicada
void task_resume (task_t *task, task_t **queue) {

    queue_remove((queue_t **) queue, (queue_t *) task);
    task->status = 'P';
    queue_append((queue_t **) &readyTasks, (queue_t *) task);
}


// a tarefa corrente aguarda o encerramento de outra task
int task_join (task_t *task) {

    if ((task->status == 'T') || (task == NULL))
        return -1; 

    task->suspendedCount++;
    task_suspend((task_t **) &task->suspendedQueue);
    
    return task->exitCode; 
}


// acorda a tarefa que esta na hora de acordar
static void wakeup_tasks () {

    if (!suspendedTasks)
        return;
    
    task_t *task = suspendedTasks; 
    task_t *next = task->next;
    int currTime = systime();

    // cicla a fila de suspensa acordando as tasks que estao no momento de acordar
    while (suspendedTasks && next != suspendedTasks) {
        #ifdef DEBUG
            printf("WAKEUP TASKS: dentro do while\n");
            printf("WAKEUP TASKS: tamanho da suspendedTasks = %d\n", queue_size((queue_t *)suspendedTasks));
        #endif
        next = task->next;
        if (task->wakeTime <= currTime) {
            #ifdef DEBUG
                printf("WAKEUP TASKS: acora tarefa %d\n", task->id);
            #endif
            task->wakeTime = 0;
            task_resume((task_t *) task, (task_t **) &suspendedTasks);
        }
        task = next;
    }
    // confere a ultima (ou a primeira e unica) tarefa da fila
    if (task->wakeTime <= currTime) {
        #ifdef DEBUG
            printf("WAKEUP TASKS: acora tarefa %d\n", task->id);
        #endif
        task->wakeTime = 0;
        task_resume((task_t *) task, (task_t **) &suspendedTasks);
    }
}


// Termina a tarefa corrente, indicando um valor de status encerramento
void task_exit (int exit_code) {

    // seta tarefa no status terminada
    currTask->status = 'T';
    currTask->exitCode = exit_code;

    currTask->executionTime = systime() - currTask->executionTime; 
    printf("Task %d exit: execution time %d ms, processor time %d ms, %d activations\n", currTask->id, currTask->executionTime, currTask->processorTime, currTask->activations);

    while (currTask->suspendedCount > 0) {
        task_resume((task_t *) currTask->suspendedQueue, (task_t **) &currTask->suspendedQueue);
        currTask->suspendedCount--;
    }

    // decide se volta para o dispatcher ou para a main
    if (currTask == &dispatcherTask) 
        task_switch(&mainTask);
    else {
        userTasks--;
        task_switch(&dispatcherTask);
    }

}


// retorna ponterio pra proxima tarefa da fila a ser executada
static task_t *scheduler () {

    task_t *temp = readyTasks;  // temp no inicio da fila
    task_t *maiorPrio = temp;   // salva a task de maior prioridade (menor valor)

    if(!temp)
        return NULL;

    // percorre a fila procurando a task de maior prioridade
    while ((temp = temp->next) != readyTasks) {
    
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
        
        //dispatcherTask.activations++;

        wakeup_tasks();

        // pega task do scheduler
        proxima = scheduler();

        if (proxima != NULL) {
            
            proxima->status = 'E';
            proxima->quantum = 20;
            //proxima->activations++;

            processorTime = systime();
            task_switch(proxima);
            proxima->processorTime += (systime() - processorTime);

            switch (proxima->status) {

                // se terminada da free
                case 'T':
                    queue_remove((queue_t **) &readyTasks, (queue_t *) proxima);
                    free(proxima->context.uc_stack.ss_sp);
                    break;
            }
        }
    }

    task_exit(0);
}


// suspende a tarefa corrente por t milissegundos
void task_sleep (int t) {

    currTask->wakeTime = systime() + t;

    #ifdef DEBUG
        printf("TASK SLEEP: tarefa %d indo dormir\n", currTask->id);
    #endif

    task_suspend((task_t **) &suspendedTasks);
}


static void enter_cs (int *lock) {
  // atomic OR (Intel macro for GCC)
  while (__sync_fetch_and_or (lock, 1)) ;   // busy waiting
}
 
static void leave_cs (int *lock) {
  (*lock) = 0 ;
}


// cria um semáforo apontado por s com valor inicial "value"
int sem_create (semaphore_t *s, int value) {

    if (!s)
        return -1;

    s->semQueue = NULL;
    s->value = value;
    s->lock = 0;

    return 0;
}

// requisita o semáforo
int sem_down (semaphore_t *s) {
    // se o valor do semaforo for negativo coloca a currTask na fila do semaforo e retorna pro dispatcher
    // se nao, continua na tarefa e decrementa o valor do semaforo
    // retorna 0 em caso de sucesso ou -1 em caso de erro (semáforo não existe ou foi destruído)

    if (!s)
        return -1;

    enter_cs(&s->lock);
    s->value--;
    leave_cs(&s->lock);
    
    if (s->value < 0) {
        task_suspend((task_t **) &s->semQueue);
        // queue_remove((queue_t **) &readyTasks, (queue_t *) currTask);
        // queue_append((queue_t **) &s->semQueue, (queue_t *) currTask);
        task_yield();
    }

    return 0;
}

// libera o semáforo
int sem_up (semaphore_t *s) {
    // incrementa o valor do semaforo e libera a primeira task da fila se houver
    // retorna 0 em caso de sucesso ou -1 em caso de erro (semáforo não existe ou foi destruído)

    if (!s)
        return -1;
        
    enter_cs(&s->lock);
    s->value++;
    leave_cs(&s->lock);

    if (s->value <= 0) {
        task_resume((task_t *) s->semQueue, (task_t **) &s->semQueue);
        // queue_remove((queue_t **) &s->semQueue, (queue_t *) currTask);
        // queue_append((queue_t **) &readyTasks, (queue_t *) currTask);
    }

    return 0;
}

// destroi o semáforo, liberando as tarefas bloqueadas
int sem_destroy (semaphore_t *s) {
    // destrói o semáforo apontado por s, acordando todas as tarefas que aguardavam por ele
    // as tarefas que estavam suspensas aguardando o semáforo que foi destruído devem ser acordadas
    // e retornar da operação Down correspondente com um código de erro (valor de retorno -1)

    while (s->value <= 0) {
        sem_up(s);
    }

    s = NULL;

    return 0;
}


/*----------------------------------------------------------------------------------------------------------*/