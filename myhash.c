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

int myhash_insert(ht_t *ht[], ssize_t len, char *key, void *data) {
    ssize_t position = hash(key, len);
    if (ht[position] && ht[position]->head) {
        pthread_rwlock_wrlock(&ht[position]->lock);
        ht[position]->head = ll_insert(ht[position]->head, data);
        pthread_rwlock_unlock(&ht[position]->lock);
    } else {
        ht[position] = (ht_t*)calloc(1, sizeof(ht_t));
        ht[position]->head = ll_insert(NULL, data);
        ht[position]->lock = (pthread_rwlock_t)PTHREAD_RWLOCK_INITIALIZER;
    }
    return 1;
}

void *myhash_search(ht_t *ht[], ssize_t len, char *key, int (*fun)(const void *data, void *a), void *arg) {
    ssize_t position = hash(key, len);
    if (ht[position] && ht[position]->head) {
        pthread_rwlock_rdlock(&ht[position]->lock);
        void *result = ll_search(ht[position]->head, fun, arg);
        pthread_rwlock_unlock(&ht[position]->lock);
        return result;
    }
    return NULL;
}

void myhash_delete(ht_t *ht[], ssize_t len, char *key, int (*fun)(const void *data, void *a), void *arg) {
    ssize_t position = hash(key, len);
    if (ht[position] && ht[position]->head) {
        pthread_rwlock_wrlock(&ht[position]->lock);
        ht[position]->head = ll_delete(ht[position]->head, fun, arg);
        pthread_rwlock_unlock(&ht[position]->lock);
        
        if (ht[position]->head == NULL) {
            free(ht[position]);
            ht[position] = NULL;
        }
    }
}

//void myhash_free(ht_t *ht[], ssize_t len) {
//    for (int i = 0; i < len; i++) {
//        if (ht[i]) {
//            printf("dr doom\n");
//            free(ht[i]);
//            ht[i] = NULL;
//        }
//    }
//}
