CFLAGS= -g -std=c++11
LDFLAGS= #-lsocket -lnsl
LDFLAGS1= -pthread #-lsocket -lnsl -lpthread, must use pthread on linux
CC=g++

all: sclient multiThreadServer

# To make an executable
sclient: sclient.o 
	$(CC) $(LDFLAGS) -o bin/sclient sclient.o
 
multiThreadServer: multiThreadServer.o
	$(CC) $(LDFLAGS1) -o bin/multiThreadServer multiThreadServer.o

# To make an object from source
.c.o:
	$(CC) $(CFLAGS) -c $*.c

# clean out the dross
clean:
	-rm bin/* *.o 

