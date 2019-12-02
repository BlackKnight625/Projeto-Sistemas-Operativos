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
    char readBuffer[10] = {0};
    assert(tfsMount(argv[1]) == 0);

    int fd = -1;
    assert((fd = tfsOpen("d", READ)) == 0);

    printf("Test: read full file content\n");
    assert(tfsRead(fd, readBuffer, 6) == 5);
    printf("Content read: %s\n", readBuffer);

    printf("Test: write file with no permission");
    assert(tfsWrite(fd, "hello", 5) == TECNICOFS_ERROR_PERMISSION_DENIED);

    printf("Test: closing file");
    assert(tfsClose(fd) == 0);

    sleep(10);

    printf("Test: open renamed file");
    assert((fd = tfsOpen("newName", READ)) == 0);

    printf("Test: read full file content");
    assert(tfsRead(fd, readBuffer, 9) == 8);
    printf("Content read: %s\n", readBuffer);
    
    assert(tfsUnmount() == 0);

    return 0;
}