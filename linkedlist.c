#include <linkedlist.h>

const size_t linkedlist_elem_size = sizeof(linkedlist_elem);

linkedlist_elem *linkedlist_new(linkedlist_elem *list, void *data) {
	linkedlist_elem *new = (linkedlist_elem*)malloc(linkedlist_elem_size);
	if (new == NULL) err_malloc(linkedlist_elem_size);
	new->ptr = data;
	new->prev = NULL;
	new->next = NULL;
	if(list != NULL) {
		new->next = list;
		list->prev = new;
	}
	return new;
}

linkedlist_elem *linkedlist_search(linkedlist_elem *list, int (*fun)(const void*)) {
	linkedlist_elem *curr = list;
	while (curr != NULL) {
		if (fun((const void*)(curr->ptr))) return curr;
		curr = curr->next;
	}
	return NULL;
}

void linkedlist_iter(linkedlist_elem *list, void (*fun)(const void*)) {
	linkedlist_elem *curr = list;
	while (curr != NULL) {
		fun((const void*)(curr->ptr));
		curr = curr->next;
	}
}

linkedlist_elem *linkedlist_remove(linkedlist_elem *list, linkedlist_elem *elem) {
	linkedlist_elem *curr = list;
	while (curr != NULL) {
		if (curr == elem) {
			if (curr->prev) (curr->prev)->next = elem->next;
			else list = curr->next;
			if (elem->next) (elem->next)->prev = curr;
			curr = curr->next;
			free(elem->ptr);
			free(elem);
		} else curr = curr->next;
	}
	return list;	//failure
}

void linkedlist_free(linkedlist_elem *list) {	//WARNING: USING linkedlist WITH STACK ELEMENTS FUCK THINGS UP, DON'T DO IT
	if (list->next != NULL) linkedlist_free(list->next);
	free(list->ptr);
	free(list);
}
