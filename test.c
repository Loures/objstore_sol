#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <objstore.h>

#define ERRSTR objstore_errstr
#define test_str "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Praesent iaculis arcu eu tempor cras amet.\n"
#define test_strlen 100


char *repeat_str(int n) {
    char *repeat = (char*)malloc(sizeof(char) * test_strlen * n + 1);
    for (int i = 0; i < n; i++) strcpy(repeat + test_strlen * i, test_str);
    repeat[test_strlen * n] = '\0';
    return repeat;
}

int test1() {
    if (!os_store("Object1", test_str, test_strlen)) {
        #ifdef ERRSTR 
            printf(objstore_errstr);
        #endif
        return !os_disconnect();
    }
    for (int i = 1; i <= 18; i++) {
        char name[128];
        memset(name, 0, 128);
        sprintf(name, "Object%d", i + 1);
        char *str = repeat_str(i * 55);
        if (!os_store(name, str, i * test_strlen * 55)) {
            free(str);
            #ifdef ERRSTR 
                printf(objstore_errstr);
            #endif
            return !os_disconnect();
        }
        free(str);
    }
    char *str = repeat_str(1000);
    if (!os_store("Object20", str, 1000 * test_strlen)) {
            free(str);
            #ifdef ERRSTR 
                printf(objstore_errstr);
            #endif
            return !os_disconnect();
    }
    free(str);
    os_disconnect();
    return 1;
}

int test2() {
    char *str_first = os_retrieve("Object1");
    if (!(str_first && strlen(str_first) == 100)) {
        #ifdef ERRSTR
            printf(objstore_errstr);
        #endif
        free(str_first);
        return !os_disconnect();
    }
    free(str_first);
    for (int i = 2; i <= 19; i++) {
        char name[128];
        memset(name, 0, 128);
        sprintf(name, "Object%d", i);
        char *str = os_retrieve(name);
        if (!(str && strlen(str) == (i - 1) * test_strlen * 55)) {
            #ifdef ERRSTR
                printf(objstore_errstr);
            #endif
            free(str);
            return !os_disconnect();
        }
        free(str);
    }
    char *str_last = os_retrieve("Object20");
    if (!(str_last && strlen(str_last) == 100000)) {
        #ifdef ERRSTR
            printf(objstore_errstr);
        #endif
        free(str_last);
        return !os_disconnect();
    }
    os_disconnect();
    return 1;
}

int test3() {
    for (int i = 1; i <= 20; i++) {
        char name[128];
        memset(name, 0, 128);
        sprintf(name, "Object%d", i);
        if (!os_delete(name)) {
            #ifdef ERRSTR
                printf(objstore_errstr);
            #endif
            return !os_disconnect();
        };
    }
    os_disconnect();
    return 1;
}

int main(int argc, char *argv[]) {
    if (os_connect(argv[1]) && argv[2]) {
        switch (atoi(argv[2])) {
            case 1:
                return !test1();
                break;

            case 2:
                return !test2();
                break;

            case 3:
                return !test3();
                break;

            default:
                break;
        }
    } else return 1;
}