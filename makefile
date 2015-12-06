# makefile

all: dataserver client

semaphore.o: semaphore.h semaphore.cpp
	g++ -c -g semaphore.cpp

boundedbuffer.o: boundedbuffer.h boundedbuffer.cpp
	g++ -c -g boundedbuffer.cpp

NetworkRequestChannel.o: NetworkRequestChannel.h NetworkRequestChannel.cpp
	g++ -c -g NetworkRequestChannel.cpp

dataserver: dataserver.cpp NetworkRequestChannel.o 
	g++ -g -o dataserver dataserver.cpp NetworkRequestChannel.o -lpthread

client: client.cpp NetworkRequestChannel.o boundedbuffer.o semaphore.o
	g++ -g -o client client.cpp NetworkRequestChannel.o boundedbuffer.o semaphore.o -lpthread

clean:
	rm -rf *.o dataserver client fifo*
