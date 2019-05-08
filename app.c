#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "pthread.h"
#include "ralloc.h"

int handling_method;          // deadlock handling method

#define M 3                   // number of resource types
int exist[3] =  {17, 10, 5};  // resources existing in the system

#define N 5                   // number of processes - threads
pthread_t tids[N];           // ids of created threads
int threads[N];

void *aprocess (void *p)
{
    int req[3];
    int pid;
    
    pid =  *((int *)p);
    
    // fflush (stdout);
    /*
     req[0] = 3;
     req[1] = 3;
     req[2] = 3;
     printf("pid: %d\n",pid);*/
    // printEverything();
    if(pid == 0){
        req[0] = 8;
        req[1] = 7;
        req[2] = 5;
    }
    else if( pid == 1){
        req[0] = 4;
        req[1] = 4;
        req[2] = 4;
    }
    else if( pid == 2 ){
        
        req[0] = 9;
        req[1] = 0;
        req[2] = 2;
    }
    else if( pid == 3 ){
        
        req[0] = 2;
        req[1] = 2;
        req[2] = 2;
    }
    else{
        
        req[0] = 4;
        req[1] = 3;
        req[2] = 3;
    }
    
    ralloc_maxdemand(pid, req);
    
    
    //for (k = 0; k < 10; ++k) {
    // printf("number of k: %d\n",k );
    if(pid == 0){
        req[0] = 7;
        req[1] = 4;
        req[2] = 3;
    }
    else if( pid == 1){
        req[0] = 0;
        req[1] = 2;
        req[2] = 0;
    }
    else if( pid == 2 ){
        
        req[0] = 6;
        req[1] = 0;
        req[2] = 0;
    }
    else if( pid == 3 ){
        
        req[0] = 0;
        req[1] = 1;
        req[2] = 1;
    }
    else{
        
        req[0] = 4;
        req[1] = 3;
        req[2] = 1;
    }
    
    ralloc_request (pid, req);
    
    
    ralloc_release (pid, req);
    
    
    // call request and release as many times as you wish with
    // different parameters
    //}
    threads[pid] = 1;
    pthread_exit(NULL);
}


void *secProcess (void *p)
{
    int req[3];
    int pid;
    
    pid =  *((int *)p);
    
    req[0] = 1;
    req[1] = 2;
    req[2] = 2;
    
    ralloc_request(pid, req);
    
    ralloc_release(pid, req);
    
    pthread_exit(NULL);
    
}

int main(int argc, char **argv)
{
    int dn; // number of deadlocked processes
    int deadlocked[N]; // array indicating deadlocked processes
    int k;
    int i;
    int pids[N];
    for (int i = 0; i < N; ++i)
    {
        threads[i] = 0;
    }
    for (k = 0; k < N; ++k)
        deadlocked[k] = -1; // initialize
    // DEADLOCK_AVOIDANCE
    handling_method = DEADLOCK_AVOIDANCE;
    ralloc_init (N, M, exist, handling_method);
    
    fflush(stdout);
    
    for (i = 0; i < N; ++i) {
        pids[i] = i;
        pthread_create (&(tids[i]), NULL, (void *) &aprocess,
                        (void *)&(pids[i]));
    }
    fflush (stdout);
    int isDone = 1;
    while (isDone) {
        sleep (10); // detection period
        if (handling_method == DEADLOCK_DETECTION) {
            dn = ralloc_detection(deadlocked);
        }
        isDone = 0;
        for (int i = 0; i < N; ++i)
        {
            if(threads[i] == 0)
                isDone = 1;
        }
        printEverything();
        ralloc_end();
        exit(0);
    }
}
