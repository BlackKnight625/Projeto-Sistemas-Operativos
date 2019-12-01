# Makefile, versao 1
# Sistemas Operativos, DEI/IST/ULisboa 2019-20

CC   = gcc
LD   = gcc
CFLAGS =-Wall -std=gnu99 -I../
LDFLAGS=-lm

# A phony target is one that is not really the name of a file
# https://www.gnu.org/software/make/manual/html_node/Phony-Targets.html
.PHONY: all clean run

all: tecnicofs

tecnicofs: lib/inodes.o lib/bst.o lib/hash.o fs.o main-rwlock.o lib/sockets.o
	$(LD) $(CFLAGS) $(LDFLAGS) -pthread -o tecnicofs-rwlock lib/inodes.o lib/bst.o lib/hash.o fs.o lib/sockets.o main-rwlock.o

lib/bst.o: lib/bst.c lib/bst.h
	$(CC) $(CFLAGS) -o lib/bst.o -c lib/bst.c

lib/hash.o: lib/hash.c lib/hash.h
	$(CC) $(CFLAGS) -o lib/hash.o -c lib/hash.c

fs.o: fs.c fs.h lib/bst.h lib/hash.h lib/inodes.h
	$(CC) $(CFLAGS) -o fs.o -c fs.c

lib/inodes.o: lib/inodes.c lib/inodes.h
	$(CC) $(CFLAGS) -o lib/inodes.o -c lib/inodes.c

lib/sockets.o: lib/sockets.c lib/sockets.h
	$(CC) $(CFLAGS) -o lib/sockets.o -c lib/sockets.c

# Nao ha neccessidade de separar os casos separados dos diferentes mains
main-rwlock.o: main.c fs.h lib/bst.h lib/hash.h
	$(CC) $(CFLAGS) -DRWLOCK -o main-rwlock.o -c main.c

clean:
	@echo Cleaning...
	rm -f lib/*.o *.o tecnicofs

run: tecnicofs
	./tecnicofs

../Client/tecnicofs-client-api.o: ../Client/tecnicofs-client-api.c ../Client/tecnicofs-client-api.h ../Client/tecnicofs-api-constants.h
	$(CC) $(CFLAGS) -o ../Client/tecnicofs-client-api.o -c ../Client/tecnicofs-client-api.c

../Client/api-tests/client-api-test-create.o: ../Client/api-tests/client-api-test-create.c ../Client/tecnicofs-client-api.h ../Client/tecnicofs-api-constants.h
	$(CC) $(CFLAGS) -o ../Client/api-tests/client-api-test-create.o -c ../Client/api-tests/client-api-test-create.c

../Client/api-tests/client-api-test-delete.o: ../Client/api-tests/client-api-test-delete.c ../Client/tecnicofs-client-api.h ../Client/tecnicofs-api-constants.h
	$(CC) $(CFLAGS) -o ../Client/api-tests/client-api-test-delete.o -c ../Client/api-tests/client-api-test-delete.c

../Client/api-tests/client-api-test-read.o: ../Client/api-tests/client-api-test-read.c ../Client/tecnicofs-client-api.h ../Client/tecnicofs-api-constants.h
	$(CC) $(CFLAGS) -o ../Client/api-tests/client-api-test-read.o -c ../Client/api-tests/client-api-test-read.c

../Client/api-tests/client-api-test-success.o: ../Client/api-tests/client-api-test-success.c ../Client/tecnicofs-client-api.h ../Client/tecnicofs-api-constants.h
	$(CC) $(CFLAGS) -o ../Client/api-tests/client-api-test-success.o -c ../Client/api-tests/client-api-test-success.c

clients: lib/sockets.o ../Client/tecnicofs-client-api.o ../Client/api-tests/client-api-test-create.o ../Client/api-tests/client-api-test-delete.o ../Client/api-tests/client-api-test-read.o ../Client/api-tests/client-api-test-success.o
	$(CC) $(CFLAGS) -o ../Client/client-api-test-create lib/sockets.o ../Client/tecnicofs-client-api.o ../Client/api-tests/client-api-test-create.o
	$(CC) $(CFLAGS) -o ../Client/client-api-test-delete lib/sockets.o ../Client/tecnicofs-client-api.o ../Client/api-tests/client-api-test-delete.o
	$(CC) $(CFLAGS) -o ../Client/client-api-test-read lib/sockets.o ../Client/tecnicofs-client-api.o ../Client/api-tests/client-api-test-read.o
	$(CC) $(CFLAGS) -o ../Client/client-api-test-success lib/sockets.o ../Client/tecnicofs-client-api.o ../Client/api-tests/client-api-test-success.o