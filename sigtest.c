#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <objstore.h>

#define ERRSTR objstore_errstr
#define test_str "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Praesent iaculis arcu eu tempor cras amet.\n"
#define test_strlen 100

int main(int argc, char *argv[]) {
    if (os_connect(argv[1])) {
        char *big = (char*)malloc(100000000);
        memset(big, '1', 100000000);
        os_store("big", (char*)big, 100000000);
        char *bigger = os_retrieve("big");
        fprintf(stderr, "a %ld\n", strlen(bigger));
        os_disconnect();
        free(big);
    }
}
