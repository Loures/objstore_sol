#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <objstore.h>

int main(int argc, char *argv[]) {
    if (os_connect(argv[1])) {
        char *big = (char*)malloc(100000000);
        memset(big, '1', 100000000);
        
		os_store("big", (char*)big, 100000000);
        
		char *retrbig = os_retrieve("big");
		if (strcmp(retrbig, big) == 0) printf("I blocchi sono uguali\n");
		else {
			printf("I blocchi sono diversi\n");
			return 1;
		}
        os_disconnect();
        
		free(big);
		return 0;
    }
	return 1;
}
