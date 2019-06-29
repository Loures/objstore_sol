#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <objstore.h>
#include <readline/readline.h>

void parsequery(char *msg) {
    char saveptr[212992];
    char copy[212992];
    memset(copy, 0, 212992);
    memset(saveptr, 0, 212992);
    strcpy(copy, msg);
    char *cmd = strtok_r(msg, " ", (char**)&saveptr);
    if (!cmd) return;
    if (strcmp(cmd, "REGISTER") == 0) {
        char *name = strtok_r(NULL, " ", (char**)&saveptr);
        if (!os_connect(name)) printf("%s", ERRSTR); else printf("OK\n");
    }
    if (strcmp(cmd, "RETRIEVE") == 0) {
        char *name = strtok_r(NULL, " ", (char**)&saveptr);
        char *data = os_retrieve(name);
        if (data) printf("%s\n", data); else printf("%s", ERRSTR);
        fflush(NULL);
        free(data);
    }
    if (strcmp(cmd, "STORE") == 0) {
        char *name = strtok_r(NULL, " ", (char**)&saveptr);
        if (!name) return;
        char *data = copy + 7 + strlen(name);
        ssize_t len = strlen(data);
        if (len <= 0) return;
        if (!os_store(name, data, len + 1)) printf("%s", ERRSTR); else printf("OK\n");
    }
    if (strcmp(cmd, "DELETE") == 0) {
        char *name = strtok_r(NULL, " ", (char**)&saveptr);
        if (os_delete(name) < 0) printf("%s", ERRSTR); else printf("OK\n");
    }
    if (strcmp(cmd, "LEAVE") == 0) {
        if (os_disconnect()) printf("OK\n");
        else printf("%s", ERRSTR);
    }
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
