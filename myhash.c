/************************************
 __  __       _   _           _     
|  \/  |_   _| | | | __ _ ___| |__  
| |\/| | | | | |_| |/ _` / __| '_ \ 
| |  | | |_| |  _  | (_| \__ \ | | |
|_|  |_|\__, |_| |_|\__,_|___/_| |_|
        |___/                       
****By loures************************/

#include <os_server.h>
#include <errormacros.h>
#include <limits.h>

//ElfHash
static unsigned int hash(char* key, ssize_t len) {
    unsigned long h = 0, high;
    while (*key) {
        h = ( h << 4 ) + *key++;
        if ( (high = h & 0xF0000000) ) h ^= high >> 24;
        h &= ~high;
    }
    return h % len;
}

ll_elem_t *ll_insert(ll_elem_t *head, void *data) {
    ll_elem_t *new = (ll_elem_t*)calloc(1, sizeof(ll_elem_t));
    new->data = data;
    new->prev = NULL;
    new->next = head;
    if (head) head->prev = new;
    return new;
}

void *ll_search(ll_elem_t *head, int (*fun)(const void *data, void *a), void *arg) {
    ll_elem_t *curr = head;
    while (curr != NULL) {
        int result = fun((const void*)curr->data, arg);
        if (result) return curr->data;
        curr = curr->next;
    }
    return NULL;
}

ll_elem_t *ll_delete(ll_elem_t *head, int (*fun)(const void *data, void *a), void *arg) {
    ll_elem_t *curr = head;
    while (curr != NULL) {
        ll_elem_t *next = curr->next;
        int result = fun((const void*)curr->data, arg);
        if (result) {
            if (curr->prev) (curr->prev)->next = curr->next;
            else head = curr->next;


            if (curr->next) (curr->next)->prev = curr->prev;


            free(curr);
            curr = NULL;
            return head;
        }
        curr = next;
    }
    return head;
}

void myhash_init(ht_t *ht, ssize_t len, ssize_t granularity) {
    ht->table = (ll_elem_t**)calloc(len, sizeof(ll_elem_t*));
    ht->locks = (pthread_rwlock_t*)calloc(granularity, sizeof(pthread_rwlock_t));
    ht->granularity = len / granularity;
    for (int i = 0; i < granularity; i++) ht->locks[i] = (pthread_rwlock_t)PTHREAD_RWLOCK_INITIALIZER;
}

int myhash_insert(ht_t *ht, ssize_t len, char *key, void *data) {
    ssize_t position = hash(key, len);

    int err = pthread_rwlock_wrlock(&ht->locks[position / ht->granularity]);
    if (err != 0) err_pthread("pthread_rwlock_wrlock");
    ht->table[position] = ll_insert(ht->table[position], data);
    err = pthread_rwlock_unlock(&ht->locks[position / ht->granularity]);
    if (err != 0) err_pthread("pthread_rwlock_unlock");

    return 1;
}

void *myhash_search(ht_t *ht, ssize_t len, char *key, int (*fun)(const void *data, void *a), void *arg) {
    ssize_t position = hash(key, len);
    if (ht->table[position]) {
        int err = pthread_rwlock_rdlock(&ht->locks[position / ht->granularity]);
        if (err != 0) err_pthread("pthread_rdlock_wrlock");
        void *result = ll_search(ht->table[position], fun, arg);
        err = pthread_rwlock_unlock(&ht->locks[position / ht->granularity]);
        if (err != 0) err_pthread("pthread_rwlock_unlock");
        return result;
    }
    return NULL;
}

void myhash_delete(ht_t *ht, ssize_t len, char *key, int (*fun)(const void *data, void *a), void *arg) {
    ssize_t position = hash(key, len);
    if (ht->table[position]) {
        int err = pthread_rwlock_wrlock(&ht->locks[position / ht->granularity]);
        if (err != 0) err_pthread("pthread_rwlock_wrlock");
        ht->table[position] = ll_delete(ht->table[position], fun, arg);
        err = pthread_rwlock_unlock(&ht->locks[position / ht->granularity]);
        if (err != 0) err_pthread("pthread_rwlock_unlock");
    }
}

void myhash_free(ht_t *ht, ssize_t len) {
    free(ht->table);
    free(ht->locks);
    free(ht);
}
