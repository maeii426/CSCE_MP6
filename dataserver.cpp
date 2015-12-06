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
#include <stdio.h>

#include "NetworkRequestChannel.h"

using namespace std;


/*--------------------------------------------------------------------------*/
/* VARIABLES */
/*--------------------------------------------------------------------------*/

static int nthreads = 0;

/*--------------------------------------------------------------------------*/
/* LOCAL FUNCTIONS -- SUPPORT FUNCTIONS */
/*--------------------------------------------------------------------------*/

string int2string(int number) {
   stringstream ss;//create a stringstream
   ss << number;//add number to the stream
   return ss.str();//return a string with the contents of the stream
}

string process_request(string request)
{
  if(request.compare(0, 5, "hello") == 0)
	{
	  return "Hello to you";
	}
	else if(request.compare(0, 4, "data") == 0)
	{
	  return int2string(rand() % 100);
	}
	else if(request.compare("quit") == 0)
	{
	  return "bye";
	}
	else
	{
	  return "Unknown request";
	}
}

/*--------------------------------------------------------------------------*/
/* LOCAL FUNCTIONS -- THREAD FUNCTIONS */
/*--------------------------------------------------------------------------*/

void * handle_data_requests(void* arg) {

  while(true)
  {
    int fd = *(int*)arg;
    char buf [1024];
    string message;
  
  	if(recv (fd, buf, sizeof (buf), 0) >= 0)
  	{
    	printf("server: %d received msg: %s\n", fd, buf);
    	message = buf;
    	
    	char msg [1024];
      strcpy(msg, process_request(message).c_str());
      
      if(msg == "bye")
      {
        return 0;
      }

    	if (send(fd, msg, strlen(msg)+1, 0) == -1)
    	{
        perror("send from server");
      }
      printf("server: %d sent: %s\n", fd, msg);
  	}
  }
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
	
	/* Start the server */
  NetworkRequestChannel server(port, backlog, &handle_data_requests);
	
	return 0;
}