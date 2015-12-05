/*
    Author: Andrew Kirfman & Margaret Baxter
            CSCE-313 Machine Problem #4
            Texas A&M University
    Due   : 11/8/2015
    
    Semaphore based protection
    Atomic Operations
    Purpose: ensure rate control bewteen request threads and worker threads
    Temporarily stops accepting requests once 'full', limits the number of worker threads working if requests are coming in too slow
    Data Structure: array?
*/

#ifndef _boundedbuffer_H_                   // include file only once
#define _boundedbuffer_H_

/* STL includes */
#include <vector>
#include <string>

/* User constructed headers */
#include "semaphore.h"

using namespace std;

/*--------------------------------------------------------------------------*/
/* CLASS   bounded buffer */
/*--------------------------------------------------------------------------*/

class boundedbuffer {
    
private:
    
    int max_buffer_size;
    vector<string> request_queue;
    Semaphore *empty, *full, *mutex;
    
public:
    
    boundedbuffer(int size);
     
    ~boundedbuffer();
    
    void request_to_buffer(string request);
     //Load a request into the buffer
    
    string request_from_buffer();
     //Pull request from buffer
     
    bool bb_empty();
     //Check if the request_queue is empty

};

#endif