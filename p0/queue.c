#include "queue.h"

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

	queue_t *temp_queue = queue;

	// Percorre a fila imprimindo os elementos até voltar para o começo
	while (temp_queue->next != queue) {
		print_elem(temp_queue);
		temp_queue = temp_queue->next;
	}
}


//------------------------------------------------------------------------------
// Insere um elemento no final da fila.
// Condicoes a verificar, gerando msgs de erro:
// - a fila deve existir
// - o elemento deve existir
// - o elemento nao deve estar em outra fila
// Retorno: 0 se sucesso, <0 se ocorreu algum erro

int queue_append (queue_t **queue, queue_t *elem) {

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