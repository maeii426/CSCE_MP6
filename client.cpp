/* 
    File: client.cpp

    Author: Andrew Kirfman & Margaret Baxter
            Department of Computer Science
            Texas A&M University
    Date  : 11/22/2015

    Simple client main program for MP5 in CSCE 313
*/


/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

// For bonus, use timer interrupt generator and a signal handler to print everything.  


#include <cassert>
#include <cstring>
#include <iostream>
#include <sstream>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <getopt.h>
#include <climits>
#include <stdlib.h>
#include <vector>
#include <pthread.h>
#include <map>
#include <netdb.h>

#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>

#include "reqchannel.h"
#include "boundedbuffer.h"

using namespace std;

/*--------------------------------------------------------------------------*/
/* GLOBAL VARIABLES */
/*--------------------------------------------------------------------------*/

/* Final results histogram */
vector<int> histogram(10);

/*--------------------------------------------------------------------------*/
/* FUNCTIONS */
/*--------------------------------------------------------------------------*/

string int2string(int number) 
{
    stringstream ss;
    ss << number;
    return ss.str();
}

struct stat_struct
{
    int personID;
    bool* responses_complete;
    boundedbuffer* response_buffer;
    vector<int>* histogram;
    
    stat_struct(int pID, bool* rc, boundedbuffer* rb, vector<int>* hm)
    {
        personID= pID;
        responses_complete = rc;
        response_buffer = rb;
        histogram = hm;
    }
};

struct req_struct 
{
    int personID, num_requests;
    boundedbuffer* request_buffer;

    req_struct(int pID, int nr, boundedbuffer* buff)
    {
        personID = pID;
        num_requests = nr;
        request_buffer = buff;
    }
};

struct handler_struct
{
    boundedbuffer* request_buffer;
    vector<boundedbuffer>* response_buffers;
    vector<RequestChannel*> request_channels;
    int num_requests;

    handler_struct(boundedbuffer* buff, vector<boundedbuffer>* rb, vector<RequestChannel*> rc, int nr)
    {
        request_buffer = buff;
        response_buffers = rb;
        request_channels = rc;
        num_requests = nr;
    }
};

void *event_handler_thread(void* arguments)
{  
    /* Event handler variables */
    handler_struct* args = (handler_struct*)arguments;
    map<int, int> fd2id_list;
    fd_set read_fds, temp;
    FD_ZERO(&read_fds);
    sigset_t sig_set;
    int read          = 0;
    int max_read      = 0;
    int personID      = -1;
    int requests_sent = 0;
    int results_read  = 0;
    int select_result = 0;
    string reply      = "";
    bool done         = false;

    /* Initialize list of file descriptors to monitor */
    for(int i = 0; i < args->request_channels.size(); i++)
    {
        int get_fd = args->request_channels[i]->read_fd();
        FD_SET(get_fd, &read_fds);
    
        if(get_fd > max_read)
        {
            max_read = get_fd;
        }
        fd2id_list[get_fd] = -1;
    }
    temp = read_fds;

    /* Perform up to n writes initially */
    for(int i = 0; i < args->request_channels.size(); i++)
    {
        if(requests_sent < args->num_requests)
        {
            requests_sent++;
            string current_request = args->request_buffer->request_from_buffer();
            
            /* Temporarily disable SIGALRM handling */
            args->request_channels[i]->cwrite(current_request);
            fd2id_list[args->request_channels[i]->read_fd()] = atoi(&current_request.at(current_request.length()-1));
        }
    }

    
    /* Start infinite event handler loop */
    while(true)
    {
        read_fds = temp;
        
        /* Temporarily disable SIGALRM handling */
        select_result = select(max_read + 1, &read_fds, NULL, NULL, NULL);
        
        for(int i = 0; i < args->request_channels.size(); i++)
        {
            if(FD_ISSET(args->request_channels[i]->read_fd(), &read_fds))
            {
                /* If data is available on the channel, read from it */
                results_read++;
                reply = args->request_channels[i]->cread();

                /* Identify which response buffer the reply should be sent to */
                personID = fd2id_list[args->request_channels[i]->read_fd()];
                fd2id_list[args->request_channels[i]->read_fd()] = -1;

                /* Send the reply to the appropriate response buffers */
                int ct=0;
                vector<boundedbuffer>::iterator it;
                for(it = args->response_buffers->begin(); it != args->response_buffers->end(); it++)
                {
                    if(ct == personID)
                    {
                        break;
                    }
                    ct++;
                }
                it->request_to_buffer(reply);

                /* Channel is now free.  Write to it */
                if(requests_sent < args->num_requests)
                {
                    requests_sent++;
                    string current_request = args->request_buffer->request_from_buffer();
                    
                    args->request_channels[i]->cwrite(current_request);
                    
                    fd2id_list[args->request_channels[i]->read_fd()] = atoi(&current_request.at(current_request.length()-1));
                }
            }
            if(results_read == args->num_requests)
            {
                for(int i = 0; i < args->request_channels.size(); i++)
                {
                    args->request_channels[i]->cwrite("quit");
                }
                pthread_exit(NULL);
            }
        }
    }
}

/* Each person has a thread which pushes queries to the request buffer */
void *request_thread_func(void* arguments)
{
	req_struct* args = (req_struct*) arguments;
	
    /* Generate request string, send it to buffer specified number of times */
    string request = "data " + int2string(args->personID);
    for(int i=0; i<args->num_requests; i++)
    {
        args->request_buffer->request_to_buffer(request);
    }
    
    pthread_exit(NULL);
}

/* Each person has a statistics thread which collects data from the response buffers and generates a histogram */
void *statistics_thread(void* arguments)
{
	   stat_struct* args = (stat_struct*) arguments;
	
    while(true)
    {
        /* Thread is finished when worker threads finish & response buffer is empty */
        if(*args->responses_complete == true)
        {
            if(args->response_buffer->bb_empty())
            {
                pthread_exit(NULL);
            }
        }
        if (!args->response_buffer->bb_empty())
        {
            /* Pull data from the response buffer */
            string string_response = args->response_buffer->request_from_buffer();;
            int int_response = atoi(string_response.c_str());

            /* Create histogram of data */
            args->histogram->at(int_response/10) = args->histogram->at(int_response/10) + 1;
        }
    }
}


/*--------------------------------------------------------------------------*/
/* MAIN FUNCTION */
/*--------------------------------------------------------------------------*/

int main(int argc, char * argv[]) {
    
    /* Variable Declarations */
    int requests_pperson = 100000; 
	int option_char;
    int buffer_size = 10000;
    int num_reqchan = 30;
	string port = "4995";
	string server_name = "compute";
    bool *responses_complete = new bool(false);
    bool *handler_done = new bool(false);
    
    timeval start_time, end_time;

    /* Argparse for command line options lives here */
    while(( option_char = getopt(argc, argv, "n:b:w:h:p:") ) != EOF)
    {
    	switch(option_char)
    	{
    	    case 'n': requests_pperson = atoi(optarg); break;
    		case 'b': buffer_size = atoi(optarg); break;
    		case 'w': num_reqchan = atoi(optarg); break;
			case 'h': server_name = optarg; break;
			case 'p': port = optarg; break;
    		case ':': cout << "Unknown option!\n"; return 1;
    	}
    }
	
    /* Check input arguments for validity */
    if(requests_pperson < 0)
    {
    	int new_requests_pperson = 0;
    	cout << "ERROR: Number of requests can not be negative" << endl
    	     << "Please enter a new request count: ";
        while (!(cin >> new_requests_pperson) || (new_requests_pperson < 0))
        {   
            cout << "Bad input - try again: ";
            cin.clear();
            cin.ignore(INT_MAX, '\n');
        }
        requests_pperson = new_requests_pperson;
		
    }
    if(buffer_size < 0)
    {
    	int new_buffer_size = 0;
    	cout << "ERROR: Buffer size can not be negative!" << endl
    	     << "Please enter a new buffer size: ";
    	while (!(cin >> new_buffer_size) || (new_buffer_size < 0))
        {
            cout << "Bad input - try again: ";
            cin.clear();
            cin.ignore(INT_MAX, '\n');
        }
        buffer_size = new_buffer_size;
    }
    if(num_reqchan < 0)
    {
    	int new_reqchan = 0;
    	cout << "ERROR: Number of worker threads can not be negative" << endl
    	     << "Please enter a new worker thread count: ";
    	while (!(cin >> new_reqchan) || (new_reqchan < 0))
        {
            cout << "Bad input - try again: ";
            cin.clear();
            cin.ignore(INT_MAX, '\n');
        }
        num_reqchan = new_reqchan;
    }
	
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
	
	char* s_name = new char[server_name.length() + 1];
	strcpy(s_name, server_name.c_str());
	char* p = new char[port.length() + 1];
	strcpy(p, port.c_str());
	
	struct addrinfo hints, *res;
	int sockfd;

	// first, load up address structs with getaddrinfo():

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	int status;
	//getaddrinfo("www.example.com", "3490", &hints, &res);
	if ((status = getaddrinfo(s_name, p, &hints, &res)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return -1;
    }

	// make a socket:
	sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (sockfd < 0)
	{
		perror ("Error creating socket\n");	
		return -1;
	}

	// connect!
	if (connect(sockfd, res->ai_addr, res->ai_addrlen)<0)
	{
		perror ("connect error\n");
		return -1;
	}
	printf ("Successfully connected to the server %s\n", s_name);
	printf ("Now Attempting to send a message to the server\n", s_name);
	char buf [1024];
	sprintf (buf, "hello");
	send (sockfd, buf, strlen (buf)+1, 0);
	recv (sockfd, buf, 1024, 0);
	printf ("Received %s from the server\n", buf);
	return 0;
	
	
	
	
	
/*
    int pid = fork();
    if(pid == 0)
    {
        RequestChannel chan("control", RequestChannel::CLIENT_SIDE);
        
         Create new reqchannels sequentially 
        vector<RequestChannel*> reqchannels; 
        for(int i=0; i<num_reqchan; i++)
        {
            string channel_name = chan.send_request("newthread");
            RequestChannel *nc = new RequestChannel(channel_name, RequestChannel::CLIENT_SIDE);
            reqchannels.push_back(nc);
        }
        
        /* Generate request and response buffers as well as arg_structs 
        boundedbuffer bounded_buffer(buffer_size);
        vector<boundedbuffer> response_buffers;
        vector<req_struct>    request_structures;
        vector<stat_struct>   stat_structures;
        for(int i = 0; i < 3; i++)
        {
            boundedbuffer buffer(requests_pperson);
            response_buffers.push_back(buffer);
        }
        handler_struct handler_structure(&bounded_buffer, &response_buffers, reqchannels, 3*requests_pperson);
        for(int i=0; i < 3; i++)
        {
            req_struct rs(i, requests_pperson, &bounded_buffer);
            request_structures.push_back(rs);
            stat_struct ss(i, responses_complete, &response_buffers[i], &histogram);
            stat_structures.push_back(ss);
        }
      
        /* Start the worker threads 
        pthread_t event_handler_id;
        pthread_create(&event_handler_id, NULL, event_handler_thread, &handler_structure);
    
        /* Create three request threads 
        pthread_t request_threads[3];
        for(int i=0; i<3; i++)
        {
            pthread_create(&request_threads[i], NULL, request_thread_func, &request_structures[i]);
        }

        /* Create three statistic threads 
        pthread_t stat_threads[3];
        for(int i=0; i<3; i++)
        {
            pthread_create(&stat_threads[i], NULL, statistics_thread, &stat_structures[i]);
        }
        
        /* Wait for the request threads to complete 
        for(int i=0; i<3; i++)
        {
            pthread_join(request_threads[i], NULL);
        }

        /* Wait for the event handler to complete 
        pthread_join(event_handler_id, NULL);
        *responses_complete = true;


        /* Wait for the statistics threads to complete 
        for(int i=0; i<3; i++)
        {
            pthread_join(stat_threads[i], NULL);
        }
        
        /* Quit the master request channel 
        chan.send_request("quit");
        
        return 0;
    }
*/
	
}
