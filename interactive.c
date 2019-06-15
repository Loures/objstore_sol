#include <os_server.h>
#include <objstore.h>
#include <readline/readline.h>

void parsequery(char *msg) {
    char saveptr[212992];
    memset(saveptr, 0, 212992);
    char *cmd = strtok_r(msg, " ", (char**)&saveptr);
    if (!cmd) return;
    if (strcmp(cmd, "REGISTER") == 0) {
        char *name = strtok_r(NULL, " ", (char**)&saveptr);
        os_connect(name);
    }
    if (strcmp(cmd, "RETRIEVE") == 0) {
        char *name = strtok_r(NULL, " ", (char**)&saveptr);
        printf("%s", os_retrieve(name));
        fflush(NULL);
    }
    if (strcmp(cmd, "STORE") == 0) {
        char *name = strtok_r(NULL, " ", (char**)&saveptr);
        char *lenstr = strtok_r(NULL, " ", (char**)&saveptr);
        char *data = strtok_r(NULL, " ", (char**)&saveptr);
        size_t len = atol(lenstr);
        os_store(name, data, len);
    }
    if (strcmp(cmd, "DELETE") == 0) {
        char *name = strtok_r(NULL, " ", (char**)&saveptr);
        os_delete(name);  
    } 
    return;
}

int main(int argc, char *argv[]) {

    
    char *query = readline("> ");
    do {
        if (query) parsequery(query);
        free(query);
        query = readline("> ");
    } while (query && strcmp(query, "LEAVE") != 0);

    return 0;
}
