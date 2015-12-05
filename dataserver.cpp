/* 
    File: dataserver.C

    Author: Andrew Kirfman & Margaret Baxter
            Department of Computer Science
            Texas A&M University
    Date  : 12/04/2015

    Networked dataserver main program for MP6 in CSCE 313
*/

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include <cassert>
#include <cstring>
#include <sstream>
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <netdb.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <climits>
#include <vector>

#include "reqchannel.h"

using namespace std;

/*--------------------------------------------------------------------------*/
/* VARIABLES */
/*--------------------------------------------------------------------------*/

static int nthreads = 0;

/*--------------------------------------------------------------------------*/
/* FORWARDS */
/*--------------------------------------------------------------------------*/

void handle_process_loop(RequestChannel & _channel);

/*--------------------------------------------------------------------------*/
/* LOCAL FUNCTIONS -- SUPPORT FUNCTIONS */
/*--------------------------------------------------------------------------*/

string int2string(int number) {
   stringstream ss;//create a stringstream
   ss << number;//add number to the stream
   return ss.str();//return a string with the contents of the stream
}

/*--------------------------------------------------------------------------*/
/* LOCAL FUNCTIONS -- THREAD FUNCTIONS */
/*--------------------------------------------------------------------------*/

void * handle_data_requests(void * args) {

  RequestChannel * data_channel =  (RequestChannel*)args;

  // -- Handle client requests on this channel. 
  
  //handle_process_loop(*data_channel);

  // -- Client has quit. We remove channel.
 
  delete data_channel;
}

/*--------------------------------------------------------------------------*/
/* LOCAL FUNCTIONS -- INDIVIDUAL REQUESTS */
/*--------------------------------------------------------------------------*/

void process_hello(RequestChannel & _channel, const string & _request) {
  _channel.cwrite("hello to you too");
}

void process_data(RequestChannel & _channel, const string &  _request) {
  usleep(1000 + (rand() % 5000));
  //_channel.cwrite("here comes data about " + _request.substr(4) + ": " + int2string(random() % 100));
  _channel.cwrite(int2string(rand() % 100));
}

void process_newthread(RequestChannel & _channel, const string & _request) {
  int error;
  nthreads ++;

  // -- Name new data channel

  string new_channel_name = "data" + int2string(nthreads) + "_";
  //  cout << "new channel name = " << new_channel_name << endl;

  // -- Pass new channel name back to client

  _channel.cwrite(new_channel_name);

  // -- Construct new data channel (pointer to be passed to thread function)
  
  RequestChannel * data_channel = new RequestChannel(new_channel_name, RequestChannel::SERVER_SIDE);

  // -- Create new thread to handle request channel

  pthread_t thread_id;
  //  cout << "starting new thread " << nthreads << endl;
  if (error = pthread_create(& thread_id, NULL, handle_data_requests, data_channel)) {
    fprintf(stderr, "p_create failed: %s\n", strerror(error));
  }  

}

/*--------------------------------------------------------------------------*/
/* LOCAL FUNCTIONS -- THE PROCESS REQUEST LOOP */
/*--------------------------------------------------------------------------*/

void process_request(RequestChannel & _channel, const string & _request) {

  if (_request.compare(0, 5, "hello") == 0) {
    process_hello(_channel, _request);
  }
  else if (_request.compare(0, 4, "data") == 0) {
    process_data(_channel, _request);
  }
  else if (_request.compare(0, 9, "newthread") == 0) {
    process_newthread(_channel, _request);
  }
  else {
    _channel.cwrite("unknown request");
  }

}

int handle_process_loop(char* p, int b) 
{
	/* First initialize network interface */
	
	int backlog = b;
	int sockfd;
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
        return -1;
    }
	
	/* Allocate a socket for the server */
	if ((sockfd = socket(serv->ai_family, serv->ai_socktype, serv->ai_protocol)) == -1) 
	{
        perror("server: socket");
		return -1;
    }
	
	/* Associate socket with a port on this machine */
    if (bind(sockfd, serv->ai_addr, serv->ai_addrlen) == -1) 
	{
		close(sockfd);
		perror("server: bind");
		return -1;
	}
	
    freeaddrinfo(serv); 

    if (listen(sockfd, backlog) == -1) {
        perror("listen");
        exit(1);
    }
	
	printf("server: waiting for connections...\n");
	char buf [1024];
	while(true) 
	{  
        sin_size = sizeof their_addr;
        int new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) 
		{
            perror("accept");
            continue;
        }
        printf("server: got connection\n");
		recv (new_fd, buf, sizeof (buf), 0);
		printf("server: received msg: %s\n", buf);

		
		
		
		// send
		string message = "Hello to you";
		char* msg = new char[message.length() + 1];
		strcpy(msg, message.c_str());
		
        if (send(new_fd, msg, strlen(msg)+1, 0) == -1)
		{
            perror("send");
        }
		close(new_fd);
    }

/*
    if (request.compare("quit") == 0) {
      _channel.cwrite("bye");
      usleep(10000);          // give the other end a bit of time.
      break;                  // break out of the loop;
    }

    process_request(_channel, request);
  } */
  
}

/*--------------------------------------------------------------------------*/
/* MAIN FUNCTION */
/*--------------------------------------------------------------------------*/

int main(int argc, char * argv[]) 
{	
	string port = "4995";
	int backlog = 20;
	int option_char;
	
    /* Argparse for command line options lives here */
    while(( option_char = getopt(argc, argv, "p:b:") ) != EOF)
    {
    	switch(option_char)
    	{
    	    case 'p': port = optarg; break;
    		case 'b': backlog = atoi(optarg); break;
    		case ':': cout << "Unknown option!\n"; return 1;
    	}
    }
	
    /* Check input arguments for validity */
    if(port == "" || port.find("-") != -1)
    {
    	string new_port = 0;
    	cout << "ERROR: Must enter a valid port number" << endl
    	     << "Please enter a new port: ";
        while (!(cin >> new_port) || (new_port == "" || new_port.find("-") != -1))
        {   
            cout << "Bad input - try again: ";
            cin.clear();
            cin.ignore(INT_MAX, '\n');
        }
        port = new_port;
		
    }
    if(backlog <= 0)
    {
    	int new_backlog = 0;
    	cout << "ERROR: Backlog must be positive!" << endl
    	     << "Please enter a new backlog: ";
    	while (!(cin >> new_backlog) || (new_backlog <= 0))
        {
            cout << "Bad input - try again: ";
            cin.clear();
            cin.ignore(INT_MAX, '\n');
        }
        backlog = new_backlog;
    }
	
	/* Call main handler loop */
	char* temp = new char[port.length() + 1];
	strcpy(temp, port.c_str());
	handle_process_loop(temp, backlog);
	
	return 0;
}