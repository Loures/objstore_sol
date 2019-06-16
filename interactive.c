#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <objstore.h>
#include <readline/readline.h>

#define ERR_STRING objstore_errstr


void parsequery(char *msg) {
    char saveptr[212992];
    memset(saveptr, 0, 212992);
    char *cmd = strtok_r(msg, " ", (char**)&saveptr);
    if (!cmd) return;
    if (strcmp(cmd, "REGISTER") == 0) {
        char *name = strtok_r(NULL, " ", (char**)&saveptr);
        if (!os_connect(name)) printf(ERR_STRING); else printf("OK\n");
    }
    if (strcmp(cmd, "RETRIEVE") == 0) {
        char *name = strtok_r(NULL, " ", (char**)&saveptr);
        char *data = os_retrieve(name);
        if (data) printf("%s\n", data); else printf(ERR_STRING);
        fflush(NULL);
        free(data);
    }
    if (strcmp(cmd, "STORE") == 0) {
        char *name = strtok_r(NULL, " ", (char**)&saveptr);
        char *lenstr = strtok_r(NULL, " ", (char**)&saveptr);
        char *data = strtok_r(NULL, " ", (char**)&saveptr);
        size_t len = atol(lenstr);
        char cpy[len + 1];
        cpy[len] = '\0';
        memcpy(cpy, data, len);
        if (!os_store(name, cpy, len + 1)) printf(ERR_STRING); else printf("OK\n");
    }
    if (strcmp(cmd, "DELETE") == 0) {
        char *name = strtok_r(NULL, " ", (char**)&saveptr);
        if (os_delete(name) < 0) printf(ERR_STRING); else printf("OK\n");
    }
    if (strcmp(cmd, "LEAVE") == 0) os_disconnect();
    return;
}

int main(int argc, char *argv[]) {

    
    char *query = readline("> ");
    do {
        if (query) parsequery(query);
        free(query);
        query = readline("> ");
    } while (query && strcmp(query, "quit") != 0);

    return 0;
}
