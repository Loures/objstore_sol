#ifndef OS_MYHASH_H
	
	typedef struct ll_elem_t {
		void *data;
		struct ll_elem_t *next;
		struct ll_elem_t *prev;
	} ll_elem_t;

	typedef struct ht_t {
		int granularity;
		struct ll_elem_t **table;
		pthread_rwlock_t *locks;
	} ht_t;

	int myhash_insert(ht_t *ht, ssize_t len, char *key, void *data);

	void *myhash_search(ht_t *ht, ssize_t len, char *key, int (*fun)(const void *data, void *a), void *arg);

	void myhash_delete(ht_t *ht, ssize_t len, char *key, int (*fun)(const void *data, void *a), void *arg);

	void myhash_init(ht_t *ht, ssize_t len, ssize_t granularity);

	void myhash_free(ht_t *ht, ssize_t len);

	#define OS_MYHASH_H

#endif

