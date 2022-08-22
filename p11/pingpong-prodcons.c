// GRR20197153 Arthur Henrique Canello Vilar

#include <stdio.h>
#include <stdlib.h>
#include "ppos.h"
#include "queue.h"

#define BUFFER_SIZE 5

int buffer_start = 0;
int buffer_end = 0;
int buffer[BUFFER_SIZE];

task_t p1, p2, p3, c1, c2;
semaphore_t s_vaga, s_item, s_buffer;

void prodBody(void *arg) {

    int valor ;

    while (1) {

        task_sleep(1000);
        valor = random () % 100 ;

        sem_down(&s_vaga);

        sem_down(&s_buffer);
        // insere item no buffer
        buffer[buffer_end] = valor;
        buffer_end = (buffer_end + 1) % BUFFER_SIZE;
        sem_up(&s_buffer);

        sem_up(&s_item);

        printf("%s produziu %d\n", (char *) arg, valor);
    }
}

void consBody(void *arg) {

    int valor ;

    while (1) {

        sem_down(&s_item);

        sem_down(&s_buffer);
        // retira item no buffer
        valor = buffer[buffer_start];
        buffer_start = (buffer_start + 1) % BUFFER_SIZE;
        sem_up(&s_buffer);

        sem_up(&s_vaga);

        printf("%s consumiu %d\n", (char *) arg, valor);
        task_sleep(1000);
    }
}

int main (int argc, char *argv[]) {

    printf("main: inicio\n");

    ppos_init();

    // cria semaforos
    sem_create(&s_buffer, 1);
    sem_create(&s_item, 0);
    sem_create(&s_vaga, 5);

    // cria tarefas
    task_create(&p1, prodBody, "p1");
    task_create(&p2, prodBody, "    p2") ;
    task_create(&p3, prodBody, "        p3") ;
    task_create(&c1, consBody, "                         c1");
    task_create(&c2, consBody, "                             c2");

    // aguarda a1 encerrar
    task_join (&p1) ;
}