#include <main.h>
#include <signal.h>
#include <linkedlist.h>
#include <errormacros.h>

unsigned int remaining = 12;

void iterator(const void *ptr) {
	printf("%d ", *(int*)ptr);
}

int ll_comp(const void *ptr) {
	return *(int*)ptr == 24;
}

int ll_comp2(const void *ptr) {
	return *(int*)ptr == 20;
}

void sighandler(int sig) {
	printf("Hello %d\n", remaining);
}

int main(int argc, char *argv[]) {
	signal(SIGUSR1, &sighandler);
	linkedlist_elem *list = NULL;	
	for (int i = 0; i < 25; i++) {
		int *bubu = (int*)malloc(sizeof(int));
		*bubu = i;
		list = linkedlist_new(list, bubu);
	}
	linkedlist_iter(list, &iterator);
	printf("\n");
	list = linkedlist_remove(list, linkedlist_search(list, &ll_comp));
	list = linkedlist_remove(list, linkedlist_search(list, &ll_comp2));
	linkedlist_iter(list, &iterator);
	printf("\n");
	linkedlist_free(list);
	return 0;
}
