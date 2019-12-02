#include "../tecnicofs-api-constants.h"
#include "../tecnicofs-client-api.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>

int main(int argc, char** argv) {
     if (argc != 2) {
        printf("Usage: %s sock_path\n", argv[0]);
        exit(0);
    }
    char readBuffer[10] = {'\0'};
    assert(tfsMount(argv[1]) == 0);

    int fd;

    printf("Test: open file to write");
    assert((fd = tfsOpen("c", WRITE)) == 0);

    assert(tfsWrite(fd, "G E T F U C K E D", 17) == 0);
    
    assert(tfsUnmount() == 0);

    return 0;
}