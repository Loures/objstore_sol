#ifndef OS_LINKED_LIST_H
	typedef struct linkedlist_elem {
		void *ptr;
		pthread_mutex_t *prevmtx;
		struct linkedlist_elem *prev;
		pthread_mutex_t mtx;
		pthread_mutex_t *nextmtx;
		struct linkedlist_elem *next;
	} linkedlist_elem;

	const size_t linkedlist_elem_size;

	linkedlist_elem *linkedlist_new(linkedlist_elem *list, void *data);

	linkedlist_elem *linkedlist_search(linkedlist_elem *list, int (*fun)(const void*, void*), void *arg);

	linkedlist_elem *linkedlist_delete(linkedlist_elem *list, int (*fun)(const void*, void*), void *arg);

	void linkedlist_free(linkedlist_elem *list);
	
	#define OS_LINKED_LIST_H
#endif

