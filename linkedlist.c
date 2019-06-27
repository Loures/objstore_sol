#include <os_server.h>
#include <errormacros.h>


const size_t linkedlist_elem_size = sizeof(linkedlist_elem);

linkedlist_elem *linkedlist_new(linkedlist_elem *list, void *data) {
	linkedlist_elem *new = (linkedlist_elem*)malloc(linkedlist_elem_size);
	if (new == NULL) {
		err_malloc(linkedlist_elem_size)
		exit(EXIT_FAILURE);
	}
	new->ptr = data;
	pthread_mutex_init(&(new->prevmtx), NULL);
	new->prev = NULL;
	pthread_mutex_init(&(new->nextmtx), NULL);
	new->next = NULL;
	if(list != NULL) {
		new->next = list;
		list->prev = new;
	}
	return new;

}

linkedlist_elem *linkedlist_search(linkedlist_elem *list, int (*fun)(const void*, void*), void *arg) {
	linkedlist_elem *curr = list;
	while (curr != NULL) {
		//if (curr) pthread_mutex_lock(&(curr->prevmtx));
		//if (curr) pthread_mutex_lock(&(curr->nextmtx));
		if (curr->ptr) {
			if (fun((const void*)(curr->ptr), (void*)arg)) return curr;
		}
		curr = curr->next;
		//if (curr) pthread_mutex_unlock(&(curr->prevmtx));
		//if (curr) pthread_mutex_unlock(&(curr->nextmtx));
	}
	return NULL;
}

void linkedlist_iter(linkedlist_elem *list, void (*fun)(const void*, void*), void *arg) {
	linkedlist_elem *curr = list;
	while (curr != NULL) {
		fun((const void*)(curr->ptr), arg);
		curr = curr->next;
	}
}

linkedlist_elem *linkedlist_remove(linkedlist_elem *list, linkedlist_elem *elem) {
	linkedlist_elem *curr = list;
	while (curr != NULL && elem != NULL) {
		linkedlist_elem *next = curr->next;
		if (curr == elem) { //0: prev ok, next = NULL
			pthread_mutex_lock(&(curr->prevmtx));
			pthread_mutex_lock(&(curr->nextmtx));
			if (elem->prev && elem->next) {
				(elem->prev)->next = elem->next;	
				(elem->next)->prev = elem->prev;
			} else if (!elem->prev) list = elem->next;
			else if (!elem->next) list = elem->prev; 
			free(elem->ptr);
			elem->ptr = NULL;
			free(elem);
			elem = NULL;
			pthread_mutex_unlock(&(curr->prevmtx));
			pthread_mutex_unlock(&(curr->nextmtx));
			return list;
		}
		curr = next;
	}
	return list;	//failure
}

linkedlist_elem *linkedlist_iterative_remove(linkedlist_elem *list, int (*fun)(const void*, void*), void *arg) {
	linkedlist_elem *curr = list;
	while (curr != NULL) {
		linkedlist_elem *next = curr->next;
		if (fun((const void*)curr, (void*)arg)) { //0: prev ok, next = NULL
			pthread_mutex_lock(&(curr->prevmtx));
			pthread_mutex_lock(&(curr->nextmtx));
			if (curr->prev && curr->next) {
				(curr->prev)->next = curr->next;	
				(curr->next)->prev = curr->prev;
			} else if (!curr->prev) list = curr->next;
			else if (!curr->next) list = curr->prev; 
			free(curr->ptr);
			curr->ptr = NULL;
			free(curr);
			curr = NULL;
			pthread_mutex_unlock(&(curr->prevmtx));
			pthread_mutex_unlock(&(curr->nextmtx));
			return list;
		}
		curr = next;
	}
	return list;	//failure
}

void linkedlist_free(linkedlist_elem *list) {	//WARNING: USING linkedlist WITH STACK ELEMENTS FUCK THINGS UP, DON'T DO IT
	if (list == NULL) return;
	if (list->next != NULL) linkedlist_free(list->next);
	free(list->ptr);
	free(list);
}
