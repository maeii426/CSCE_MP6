/* 
    Author: Andrew Kirfman & Margaret Baxter
            CSCE-313 Machine Problem #4
            Texas A&M University
    Due   : 11/8/2015
*/

#include "boundedbuffer.h"
#include <iostream> 

boundedbuffer::boundedbuffer(int size)
     : max_buffer_size(size) 
{ 
    empty = new Semaphore(size);
    full  = new Semaphore(0);
    mutex = new Semaphore(1);
}
     
boundedbuffer::~boundedbuffer() { }

void boundedbuffer::request_to_buffer(string request) 
{ 
    /* Check if there is an empty slot */
    empty->P();
    /* Grab the lock */
    mutex->P();
    /* Add the request to the buffer */
    request_queue.push_back(request);
    /* Release the lock */
    mutex->V();
    /* Track the number of elements in buffer */
    full->V();
}
    
string boundedbuffer::request_from_buffer() 
{ 
    /* Check if the buffer is empty */
    full->P();
    /* Grab the lock */
    mutex->P();
    /* Remove request from the buffer */
    string result = request_queue.front();
    request_queue.erase(request_queue.begin());
    /* Release the lock */
    mutex->V();
    /* Track the number of elements in buffer */
    empty->V();
    
    return result;
}

bool boundedbuffer::bb_empty()
{
    /* Check if the buffer is empty */
    return request_queue.empty();
}
