// GRR20197153 Arthur Hnerique Canello Vilar

#include "queue.h"
#include <stdio.h>

//------------------------------------------------------------------------------
// Conta o numero de elementos na fila
// Retorno: numero de elementos na fila

int queue_size (queue_t *queue) {

	// Se fila vazia retorna 0
	if (queue == NULL)
		return 0;
	
	int i = 1;
	queue_t *temp_queue = queue;

	// Percorre a fila contando os elementos até voltar para o começo
	while (temp_queue->next != queue) {
		temp_queue = temp_queue->next;
		i++;
	}
	
	return i;
}


//------------------------------------------------------------------------------
// Percorre a fila e imprime na tela seu conteúdo. A impressão de cada
// elemento é feita por uma função externa, definida pelo programa que
// usa a biblioteca. Essa função deve ter o seguinte protótipo:
//
// void print_elem (void *ptr) ; // ptr aponta para o elemento a imprimir

void queue_print (char *name, queue_t *queue, void print_elem (void*) ) {

	printf("%s: [", name);
	
	// Se a fila nao estiver vazia
	if (queue) {

		queue_t *temp_queue = queue;
		
		// Percorre a fila imprimindo os elementos até voltar para o começo
		while (temp_queue->next != queue) {
			print_elem(temp_queue);
			printf(" ");
			temp_queue = temp_queue->next;
		}

		// Imprime o ultimo elemento
		print_elem(temp_queue);
	}

	printf("]\n");
}


//------------------------------------------------------------------------------
// Insere um elemento no final da fila.
// Condicoes a verificar, gerando msgs de erro:
// - a fila deve existir
// - o elemento deve existir
// - o elemento nao deve estar em outra fila
// Retorno: 0 se sucesso, <0 se ocorreu algum erro

int queue_append (queue_t **queue, queue_t *elem) {

	// se fila não existe
	if (*queue == NULL) {
		fprintf(stderr, "Fila vazia");
		return -1;
	}

	if (elem == NULL) {
		fprint(stderr, "Elemento não existe");
		return -2;
	}

	if ((elem->next == NULL) || (elem->prev == NULL)) {
		fprint(stderr, "Elemento está em outra fila ou definido errado");
		return -3;
	}


}


//------------------------------------------------------------------------------
// Remove o elemento indicado da fila, sem o destruir.
// Condicoes a verificar, gerando msgs de erro:
// - a fila deve existir
// - a fila nao deve estar vazia
// - o elemento deve existir
// - o elemento deve pertencer a fila indicada
// Retorno: 0 se sucesso, <0 se ocorreu algum erro

int queue_remove (queue_t **queue, queue_t *elem) {

}