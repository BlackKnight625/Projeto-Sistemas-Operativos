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

tecnicofs: lib/inodes.o lib/bst.o lib/hash.o fs.o main-nosync.o
	$(LD) $(CFLAGS) $(LDFLAGS) -pthread -o tecnicofs-nosync lib/inodes.o lib/bst.o lib/hash.o fs.o main-nosync.o
	$(LD) $(CFLAGS) $(LDFLAGS) -pthread -o tecnicofs-mutex lib/inodes.o lib/bst.o lib/hash.o fs.o main-mutex.o
	$(LD) $(CFLAGS) $(LDFLAGS) -pthread -o tecnicofs-rwlock lib/inodes.o lib/bst.o lib/hash.o fs.o main-rwlock.o

lib/bst.o: lib/bst.c lib/bst.h
	$(CC) $(CFLAGS) -o lib/bst.o -c lib/bst.c

lib/hash.o: lib/hash.c lib/hash.h
	$(CC) $(CFLAGS) -o lib/hash.o -c lib/hash.c

fs.o: fs.c fs.h lib/bst.h lib/hash.h lib/inodes.h
	$(CC) $(CFLAGS) -o fs.o -c fs.c

lib/inodes.o: lib/inodes.c lib/inodes.h
	$(CC) $(CFLAGS) -o lib/inodes.o -c lib/inodes.c

# Nao ha neccessidade de separar os casos separados dos diferentes mains
main-nosync.o: main.c fs.h lib/bst.h lib/hash.h
	$(CC) $(CFLAGS) -o main-nosync.o -c main.c
	$(CC) $(CFLAGS) -DMUTEX -o main-mutex.o -c main.c
	$(CC) $(CFLAGS) -DRWLOCK -o main-rwlock.o -c main.c

clean:
	@echo Cleaning...
	rm -f lib/*.o *.o tecnicofs

run: tecnicofs
	./tecnicofs
