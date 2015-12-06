/* 
    File: NetworkRequestChannel.cpp

    Author: Andrew Kirfman & Margaret Baxter
            Department of Computer Science
            Texas A&M University
    Date  : 12/5/2015

*/

#include <cassert>
#include <cstring>
#include <iostream>
#include <sstream>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <getopt.h>
#include <stdlib.h>
#include <netdb.h>
#include <errno.h>
#include <stdio.h>
#include <vector>
#include <unistd.h>
#include <pthread.h>

#include "NetworkRequestChannel.h"


const int MAX_MESSAGE = 255;

/* Client Side NetworkRequestChannel Constructor */
NetworkRequestChannel::NetworkRequestChannel(const string server_name, const string port) 
{
 	char* s_name = new char[server_name.length() + 1];
	strcpy(s_name, server_name.c_str());
	char* p = new char[port.length() + 1];
	strcpy(p, port.c_str());
	
	struct addrinfo hints, *res;

	/* Load up address structs with getaddrinfo() */
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	int status;
	if ((status = getaddrinfo(s_name, p, &hints, &res)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return;
    }

	/* Make a Socket */
	sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (sockfd < 0)
	{
		perror ("Error creating socket\n");	
		return;
	}

	/* Connect the client side to the server side */
	if (connect(sockfd, res->ai_addr, res->ai_addrlen) < 0)
	{
		perror ("Connect error\n");
		return;
	}
	printf ("Successfully connected to server %s\n", s_name); 
}

/* Server Side NetworkRequestChannel Constructor */
NetworkRequestChannel::NetworkRequestChannel(const string port, int backlog, void * (*connection_handler) (void *))
{
	accept_sockets = true;
    char* p = new char[port.length() + 1];
	strcpy(p, port.c_str());
	
	vector<pthread_t> handler_ids;
	vector<int> connection_fds;
	
	fd_set active_fds, temp;
	FD_ZERO(&active_fds);
	FD_ZERO(&temp);
	
    struct addrinfo hints, *serv;
    struct sockaddr_storage their_addr;
    socklen_t sin_size;
    char s[INET6_ADDRSTRLEN];
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

	/* Get information about server's local address info */
    if ((rv = getaddrinfo(NULL, p, &hints, &serv)) != 0) 
	{
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return;
    }
	
	/* Allocate a socket for the server */
	if ((sockfd = socket(serv->ai_family, serv->ai_socktype, serv->ai_protocol)) == -1) 
	{
        perror("server: socket");
		return;
    }
	
	/* Associate socket with a port on this machine */
    if (bind(sockfd, serv->ai_addr, serv->ai_addrlen) == -1) 
	{
		close(sockfd);
		perror("server: bind");
		return;
	}
	
    freeaddrinfo(serv); 

    if (listen(sockfd, backlog) == -1) {
        perror("listen");
        exit(1);
    }
	
	printf("server: waiting for connections...\n");
	
	/* Wait for connections to handle */
	char buf [1024];
	while(accept_sockets) 
	{  
		/* Receive a connection */
        sin_size = sizeof their_addr;
        int new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) 
		{
            perror("accept failed");
            continue;
        }
        connection_fds.push_back(new_fd);
        
        /* Show connection to dataserver */
        pthread_t new_id;
        handler_ids.push_back(new_id);
    	pthread_create(&handler_ids.back(), NULL, connection_handler, &new_fd);
       
		printf("server: connection made\n");
    }
    
    /* Wait for the server threads to complete */
    for(int i=0; i<handler_ids.size(); i++)
	{
    	pthread_join(handler_ids[i], NULL);
    	close(connection_fds[i]);
	}
}

/* Destructor */
NetworkRequestChannel::~NetworkRequestChannel()
{
    close(sockfd);
}

/*--------------------------------------------------------------------------*/
/* Reading and Writing  */
/*--------------------------------------------------------------------------*/

/* Send a request and return the response */
string NetworkRequestChannel::send_request(string _request)
{
    cwrite(_request);
    string s = cread();
    return s;
}

/* Read the response from the dataserver */
string NetworkRequestChannel::cread()
{
  char buf[MAX_MESSAGE];

  if (recv(sockfd, buf, MAX_MESSAGE, 0) < 0) 
  {
    perror(string("Request Channel: Error reading!").c_str());
  }
  
  string s = buf;

  return s;
}

/* Write a request to the dataserver */
int NetworkRequestChannel::cwrite(string _msg)
{
  if (_msg.length() >= MAX_MESSAGE) 
  {
  	cerr << "Message too long for Channel!\n";
  	return -1;
  }

  const char * s = _msg.c_str();

  if (send(sockfd, s, strlen(s)+1, 0) < 0)
  {
    perror(string("Network Request Channel: Error sending!").c_str());
  }
}

/* Returns the socket file descriptor */
int NetworkRequestChannel::read_fd()
{
    return sockfd;
}
