/* 
    File: semaphore.H

    Author: Andrew Kirfman & Margaret Baxter
            CSCE-313 Machine Problem #4
            Texas A&M University
    Date  : 11/04/15
*/

/* Make sure to include file only once */
#ifndef _semaphore_H_
#define _semaphore_H_

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include <pthread.h>


class Semaphore 
{

private:

  int             value;
  pthread_mutex_t m;
  pthread_cond_t  c;

public:

  /* Constructor */
  Semaphore(int _val);

  /* Destructor */
  ~Semaphore();

  /* Decrement or wait */    
  int P();
  
  /* Increment and wakeup */
  int V();
};


#endif