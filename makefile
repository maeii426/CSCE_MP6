# makefile

all: dataserver client

semaphore.o: semaphore.h semaphore.cpp
	g++ -c -g semaphore.cpp

boundedbuffer.o: boundedbuffer.h boundedbuffer.cpp
	g++ -c -g boundedbuffer.cpp

reqchannel.o: reqchannel.h reqchannel.cpp
	g++ -c -g reqchannel.cpp

dataserver: dataserver.cpp reqchannel.o 
	g++ -g -o dataserver dataserver.cpp reqchannel.o -lpthread

client: client.cpp reqchannel.o boundedbuffer.o semaphore.o
	g++ -g -o client client.cpp reqchannel.o boundedbuffer.o semaphore.o -lpthread

clean:
	rm -rf *.o dataserver client fifo*
