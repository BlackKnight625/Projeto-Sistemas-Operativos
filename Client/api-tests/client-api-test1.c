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
    char readBuffer[20] = {0};
    assert(tfsMount(argv[1]) == 0);

    printf("Test: create file sucess");
    assert(tfsCreate("a", RW, READ) == 0);

    printf("Test: create file with name that already exists");
    assert(tfsCreate("a", RW, READ) == TECNICOFS_ERROR_FILE_ALREADY_EXISTS);

    printf("Test: create file sucess");
    assert(tfsCreate("annoying", RW, NONE) == 0);

    printf("Test: create file sucess");
    assert(tfsCreate("c", RW, WRITE) == 0);

    printf("Test: create file sucess");
    assert(tfsCreate("d", RW, READ) == 0);

    printf("Test: create file sucess");
    assert(tfsCreate("e", RW, READ) == 0);

    printf("Test: create file sucess");
    assert(tfsCreate("f", RW, RW) == 0);

    int fd = -1;
    assert((fd = tfsOpen("d", RW)) == 0);
    //assert(tfsWrite(fd, "12345", 5) == 0);

    int whtv;
    assert((whtv = tfsOpen("a", RW)) == 1);
    assert((whtv = tfsOpen("c", RW)) == 2);
    assert((whtv = tfsOpen("e", RW)) == 3);
    assert((whtv = tfsOpen("annoying", RW)) == 4);

    printf("Test: open file surpassing open file limit");
    assert((whtv = tfsOpen("f", RW)) == TECNICOFS_ERROR_MAXED_OPEN_FILES);


    printf("Test: read full file content");
    assert(tfsRead(fd, readBuffer, 6) == 0);
    printf("Content read: %s\n", readBuffer);

    assert(tfsWrite(fd, "12345", 5) == 0);

    sleep(10);

    printf("Test: rename open file");
    assert(tfsRename("d", "newName") == TECNICOFS_ERROR_FILE_IS_OPEN);

    printf("Test: read full file content");
    assert(tfsRead(2, readBuffer, 18) == 17);
    printf("Content read: %s\n", readBuffer);

    printf("Test: closing file");
    assert(tfsClose(fd) == 0);

    printf("Test: rename closed file");
    assert(tfsRename("d", "newName") == 0);

    assert((whtv = tfsOpen("newName", RW)) == 0);

    printf("Test: write new message in renamed file");
    assert(tfsWrite(fd, "Hi there", 9) == 0);

    assert(tfsUnmount() == 0);

    return 0;
}