// GRR20197153 Arthur Henrique Canello Vilar

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
	queue_t *temp_queue = queue; // ponteiro temporario para percorrer a fila

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

	queue_t *temp_queue;
	
	// checa se fila existe
	if (queue == NULL) {
		fprintf(stderr, "Fila não existe\n");
		return -1;
	}

	// checa se elemento existe
	if (elem == NULL) {
		fprintf(stderr, "Elemento não existe\n");
		return -2;
	}

	// checa se elemento pertence à uma fila
	if (elem->next || elem->prev) {

		temp_queue = *queue;

		// checa se elemento esta na fila
		while (temp_queue->next != (*queue)) {
			if (temp_queue == elem) {
				fprintf(stderr, "Elemento já está na fila\n");
				return -3;
			}
			temp_queue = temp_queue->next;
		}
		// confere último elemento (pois não passa pelo while)
		if (temp_queue == elem)	{ 
			fprintf(stderr, "Elemento já está na fila\n");
			return -3;
		}

		// se elem não está nessa fila significa que está em outra
		fprintf(stderr, "Elemento está em outra fila\n");
		return -4;
	}

	// checa se a fila esta vazia
	if (*queue == NULL) {
		*queue = elem;
		(*queue)->prev = (*queue)->next = *queue;
		return 0;
	}

	// fila tem pelo menos um elemento, então insere
	temp_queue = (*queue)->prev;

	(*queue)->prev = elem;
	temp_queue->next = elem;
	elem->next = *queue;
	elem->prev = temp_queue;

	return 0;
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

	// checa se fila existe
	if (queue == NULL) {
		fprintf(stderr, "Fila não existe\n");
		return -1;
	}

	// checa se a fila esta vazia
	if (*queue == NULL) {
		fprintf(stderr, "Fila está vazia\n");
		return -2;
	}

	// checa se elemento existe
	if (elem == NULL) {
		fprintf(stderr, "Elemento não existe\n");
		return -3;
	}

	queue_t *temp_queue = *queue;
	queue_t *flag = NULL;

	// checa se elemento esta na fila
	while (temp_queue->next != (*queue)) {
		if (temp_queue == elem)
			flag = temp_queue;
		temp_queue = temp_queue->next;
	}
	// confere último elemento (pois não passa pelo while)
	if (temp_queue == elem)	
		flag = temp_queue;

	// elemento não está na fila
	if (!flag && !elem->next && !elem->prev) {
		fprintf(stderr, "Elemento não está em nenhuma fila\n");
		return -4;
	} else if (!flag && (elem->next != NULL || elem->prev != NULL)) {
		fprintf(stderr, "Elemento está em outra fila\n");
		return -5;
	}

	// checa se o elemento removido é o primeiro
	if (*queue == elem) {
		// se a fila só tem um elemento
		if (*queue == (*queue)->next)
			*queue = NULL;

		else {
			(*queue)->next->prev = (*queue)->prev;
			(*queue)->prev->next = (*queue)->next;
			(*queue) = (*queue)->next;
		}

		elem->next = NULL;
		elem->prev = NULL;

		return 0;
	}
	
	flag->next->prev = flag->prev;
	flag->prev->next = flag->next;
	elem->next = NULL;
	elem->prev = NULL;

	return 0;
}