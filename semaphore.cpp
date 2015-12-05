/*
    Author: Andrew Kirfman & Margaret Baxter
            CSCE-313 Machine Problem #4
            Texas A&M University
    Due   : 11/8/2015
*/

#include "semaphore.h"
#include <iostream>
using namespace std;

Semaphore::Semaphore(int _val)
    : value(_val) 
{
    pthread_mutex_init(&m, NULL);
    pthread_cond_init(&c, NULL);
}

Semaphore::~Semaphore()
{ 
    
}

int Semaphore::P()
{ 
    pthread_mutex_lock(&m);
    value--;
    if(value < 0)
    {
        pthread_cond_wait(&c, &m);
    }
    pthread_mutex_unlock(&m);
}

int Semaphore::V()
{ 
    pthread_mutex_lock(&m);
    value++;
    if (value <= 0)
    {
        pthread_cond_signal(&c);
    }
    pthread_mutex_unlock(&m);
}