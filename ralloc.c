#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "ralloc.h"

// global variables
int **max;
int *available;
int **allocation;
int **need;
int **request;
int process_count = 0;
int resource_count = 0;
int deadlock_handling = 0;


// Declaration of thread condition variable
pthread_cond_t cond1 = PTHREAD_COND_INITIALIZER;

// declaring mutex
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t deletion_lock = PTHREAD_MUTEX_INITIALIZER;


int ralloc_init(int p_count, int r_count, int r_exist[], int d_handling)
{
    // pthread_mutex_lock(&lock);
    process_count = p_count;
    resource_count = r_count;
    
    // filling available array for detection and avoidance algorithms
    available = malloc( resource_count * sizeof(int) );
    for (int i = 0; i < resource_count; ++i)
    {
        available[i] = r_exist[i];
    }
    
    max = malloc( process_count * sizeof(int*) );
    allocation = malloc( process_count * sizeof(int*) );
    need = malloc( process_count * sizeof(int*) );
    request = malloc( process_count * sizeof(int*) );
    for (int i = 0; i < process_count; ++i)
    {
        max[i] = malloc(resource_count * sizeof(int));
        allocation[i] = malloc(resource_count * sizeof(int));
        need[i] = malloc(resource_count * sizeof(int));
        request[i] = malloc( resource_count * sizeof(int) );
    }
    
    // DEADLOCK HANDLING INFO
    deadlock_handling = d_handling;
    
    // pthread_mutex_init(&lock, NULL);
    // pthread_mutex_unlock(&lock);
    
    return 0;
}

int ralloc_maxdemand(int pid, int r_max[]){
    
    pthread_mutex_lock(&lock);
    
    for (int i = 0; i < resource_count; ++i)
    {
        max[pid][i] = r_max[i];
        need[pid][i] = r_max[i];
    }
    pthread_mutex_unlock(&lock);
    return (0);
}
int ralloc_request (int pid, int demand[]) {
    pthread_mutex_lock(&lock);
    // initializing request
    for (int i = 0; i < resource_count; ++i)
        request[pid][i] = demand[i];
    
    // deadlock avoidance is not used
    if( deadlock_handling != DEADLOCK_AVOIDANCE )
    {
        int isSufficient = 1; // true
        for (int i = 0; i < resource_count; ++i)
        {
            if(demand[i] > available[i])
                isSufficient = 0; // false
        }
        
        if(isSufficient == 0) // if isSufficient is false
        {
            pthread_cond_wait(&cond1, &lock);
            for (int i = 0; i < resource_count; ++i)
            {
                allocation[pid][i] += demand[i];
                available[i] -= demand[i];
                need[pid][i] = max[pid][i] - allocation[pid][i];
            }
            
            //printEverything();
            
            for (int i = 0; i < resource_count; ++i)
                request[pid][i] = 0;
            pthread_mutex_unlock(&lock);
            return(0);
        }
        else
        {
            for (int i = 0; i < resource_count; ++i)
            {
                allocation[pid][i] += demand[i];
                available[i] -= demand[i];
                need[pid][i] = max[pid][i] - allocation[pid][i];
            }
            
            //printEverything();
            for (int i = 0; i < resource_count; ++i)
                request[pid][i] = 0;
            pthread_mutex_unlock(&lock);
            return (0);
        }
    }
    else
    {
        if( ralloc_avoidance(pid, demand) == 0 ){
            for (int i = 0; i < resource_count; ++i)
            {
                allocation[pid][i] += demand[i];
                available[i] -= demand[i];
                need[pid][i] = max[pid][i] - allocation[pid][i];
            }
        }
        //printEverything();
        pthread_mutex_unlock(&lock);
        return (0);
    }
}


int ralloc_release (int pid, int demand[]) {
    pthread_mutex_lock(&lock);
    printf ("ralloc_release called\n");
    if( pid > process_count){
        pthread_mutex_unlock(&lock);
        return (-1);
    }
    
    for (int i = 0; i < resource_count; ++i)
    {
        if( demand[i] > allocation[pid][i] )
        {
            pthread_mutex_unlock(&lock);
            return(-1);
        }
    }
    
    for (int i = 0; i < resource_count; ++i)
    {
        available[i] += demand[i];
        allocation[pid][i] -= demand[i];
        need[pid][i] = max[pid][i] - allocation[pid][i];
    }
    
    // checking there is a resource a
    int * procarray = malloc( process_count * sizeof(int) );
    
    for (int i = 0; i < process_count; ++i)
        procarray[i] = -1;
    printf("before ralloc detection\n");
    if( ralloc_detection_temp(procarray) > 0 )
        pthread_cond_signal(&cond1);
    
    pthread_mutex_unlock(&lock);
    
    return (0);
}

int ralloc_detection_temp(int procarray[]) { // this method creates for only in class usage like protected in OOP languages
    printf ("ralloc_detection_temp called\n");
    
    int work[resource_count];
    int finish[process_count]; // 0 false 1 true
    int tempAllocation[process_count][resource_count];
    
    int false_count = 0;
    
    for (int i = 0; i < resource_count; ++i)
    {
        work[i] = available[i];
    }
    
    for (int i = 0; i < process_count; ++i)
    {
        for (int j = 0; j < resource_count; ++j)
        {
            tempAllocation[i][j] = allocation[i][j];
        }
    }
    
    for (int i = 0; i < process_count; ++i)
    {
        finish[i] = 1; // true
        for( int j = 0; j < resource_count; j++)
        {
            if(request[i][j] != 0)
                finish[i] = 0; // false
        }
    }
    
    for (int i = 0; i < process_count; ++i)
    {
        for(int j = 0; j < resource_count; j++)
        {
            if( finish[i] == 1 ){
                work[j] += tempAllocation[i][j];
                tempAllocation[i][j] = 0;
            }
        }
    }
    
    
    int is_all_p_satisfied = 0; // false
    int all_evaulation = 0; //false
    while( is_all_p_satisfied == 0 && all_evaulation == 0) // if all processes become true
    {
        int isTraceEnd = 0; // false, there is no
        for( int i = 0; i < process_count && isTraceEnd == 0; i++)
        {
            if(finish[i] == 0)
            {
                int isSufficient = 0; // false
                for( int j = 0; j < resource_count && isSufficient == 0; j++)
                {
                    if(request[i][j] <= work[j])
                        isSufficient = 1; // true
                }
                
                if( isSufficient == 1 ){
                    for (int j = 0; j < resource_count; ++j)
                        work[j] += tempAllocation[i][j];
                    finish[i] = 1; // true
                    isTraceEnd = 1; // true since we satisfy a demand
                }
            }
        }
        
        if( isTraceEnd == 0 )
            all_evaulation = 1; // all processes are evaluated and we cannot satisfy any of them
        
        is_all_p_satisfied = 1; // true
        for (int i = 0; i < process_count; ++i)
        {
            if(finish[i]  == 0)
                is_all_p_satisfied = 0;
        }
    }
    
    for (int i = 0; i < process_count; ++i)
    {
        if(finish[i] == 0){
            false_count++;
            procarray[i] = 1;
        }
    }
    return false_count;
}
int ralloc_detection(int procarray[]) {
    pthread_mutex_lock(&lock);
    printf ("ralloc_detection called\n");
    
    int work[resource_count];
    int finish[process_count]; // 0 false 1 true
    int tempAllocation[process_count][resource_count];
    
    int false_count = 0;
    
    for (int i = 0; i < resource_count; ++i)
    {
        work[i] = available[i];
    }
    
    for (int i = 0; i < process_count; ++i)
    {
        for (int j = 0; j < resource_count; ++j)
        {
            tempAllocation[i][j] = allocation[i][j];
        }
    }
    
    for (int i = 0; i < process_count; ++i)
    {
        finish[i] = 1; // true
        for( int j = 0; j < resource_count; j++)
        {
            if(request[i][j] != 0)
                finish[i] = 0; // false
        }
    }
    
    for (int i = 0; i < process_count; ++i)
    {
        for(int j = 0; j < resource_count; j++)
        {
            if( finish[i] == 1 ){
                work[j] += tempAllocation[i][j];
                tempAllocation[i][j] = 0;
            }
        }
    }
    
    
    int is_all_p_satisfied = 0; // false
    int all_evaulation = 0; //false
    while( is_all_p_satisfied == 0 && all_evaulation == 0) // if all processes become true
    {
        int isTraceEnd = 0; // false, there is no
        for( int i = 0; i < process_count && isTraceEnd == 0; i++)
        {
            if(finish[i] == 0)
            {
                int isSufficient = 0; // false
                for( int j = 0; j < resource_count && isSufficient == 0; j++)
                {
                    if(request[i][j] <= work[j])
                        isSufficient = 1; // true
                }
                
                if( isSufficient == 1 ){
                    for (int j = 0; j < resource_count; ++j)
                        work[j] += tempAllocation[i][j];
                    finish[i] = 1; // true
                    isTraceEnd = 1; // true since we satisfy a demand
                }
            }
        }
        
        if( isTraceEnd == 0 )
            all_evaulation = 1; // all processes are evaluated and we cannot satisfy any of them
        
        is_all_p_satisfied = 1; // true
        for (int i = 0; i < process_count; ++i)
        {
            if(finish[i]  == 0)
                is_all_p_satisfied = 0;
        }
    }
    
    for (int i = 0; i < process_count; ++i)
    {
        if(finish[i] == 0){
            false_count++;
            procarray[i] = 1;
        }
    }
    
    pthread_mutex_unlock(&lock);
    return false_count;
}

int ralloc_end() {
    pthread_mutex_lock(&deletion_lock);
    printf ("ralloc_end called\n");
    
    free(available);
    
    for (int i = 0; i < process_count; ++i)
    {
        free(max[i]);
        free(allocation[i]);
        free(need[i]);
        free(request[i]);
    }
    free(max);
    free(allocation);
    free(need);
    free(request);
    pthread_mutex_unlock(&deletion_lock);
    return (0);
}

//returns 1 when the system is safe
int ralloc_safety(int **need_temp){
    printf ("ralloc_safety called\n");
    
    int work[resource_count];
    int finish[process_count]; // 0 false 1 true
    
    for (int i = 0; i < resource_count; ++i)
    {
        work[i] = available[i];
    }
    
    for (int i = 0; i < process_count; ++i){
        finish[i] = 0; //false
    }
    
    int is_all_p_satisfied = 0; // false
    int all_evaulation = 0; //false
    while( is_all_p_satisfied == 0 && all_evaulation == 0) // if all processes become true
    {
        int isTraceEnd = 0; // false, there is no
        for( int i = 0; i < process_count && isTraceEnd == 0; i++)
        {
            if(finish[i] == 0)
            {
                int isSufficient = 0; // false
                for( int j = 0; j < resource_count && isSufficient == 0; j++)
                {
                    if(need_temp[i][j] <= work[j])
                        isSufficient = 1; // true
                }
                
                if( isSufficient == 1 ){
                    for (int j = 0; j < resource_count; ++j)
                        work[j] += allocation[i][j];
                    finish[i] = 1; // true
                    isTraceEnd = 1; // true since we satisfy a demand
                }
            }
        }
        
        if( isTraceEnd == 0 )
            all_evaulation = 1; // all processes are evaluated and we cannot satisfy any of them
        
        is_all_p_satisfied = 1; // true
        for (int i = 0; i < process_count; ++i)
        {
            if(finish[i]  == 0)
                is_all_p_satisfied = 0;
        }
    }
    //printEverything();
    for(int i = 0; i < process_count; i++){
        if(finish[i] == 0)
            return (-1);
    }
    
    return (0);
}


int ralloc_avoidance(int pid, int demand[]){ // returns 0 if it is successful -1 it is not
    
    for(int i = 0; i < resource_count; i++)
    {
        if(demand[i] > need[pid][i]){
            printf ("Demand is more than need\n");
            return (-1);
        }
        
        if(demand[i] > available[i])
            pthread_cond_wait(&cond1, &lock);
    }
    
    int **tempNeed;
    tempNeed = malloc( process_count * sizeof(int*) );
    
    for (int i = 0; i < process_count; ++i)
    {
        tempNeed[i] = malloc( resource_count * sizeof(int) );
        for( int j = 0; j < resource_count; j++)
        {
            tempNeed[i][j] = need[i][j] - demand[j];
        }
    }
    
    if(ralloc_safety(tempNeed) == 0 ) // if system is safe
    {
        
        for(int k=0; k < resource_count; k++){
            
            available[k] = available[k] - demand[k];
            
            allocation[pid][k] = allocation[pid][k] + demand[k];
            
            need[pid][k] = need[pid][k] - demand[k];
            
        }
        
        return(0);
        
    }
    else // if it is not safe
    {
        pthread_cond_wait(&cond1, &lock);
        for(int k=0; k < resource_count; k++){
            
            available[k] = available[k] - demand[k];
            
            allocation[pid][k] = allocation[pid][k] + demand[k];
            
            need[pid][k] = need[pid][k] - demand[k];
            
        }
        
    }
    
    for (int i = 0; i < process_count; ++i)
        free(tempNeed[i]);
    free(tempNeed);
    
    printf ("ralloc_avoidance called\n");
    
    return (0);
    
}

void printEverything()
{
    printf("Max:\n");
    for (int i = 0; i < process_count; ++i)
    {
        for (int j = 0; j < resource_count; ++j)
        {
            printf("%d \t", max[i][j]);
        }
        printf("\n");
    }
    printf("Allocation: \n");
    for (int i = 0; i < process_count; ++i)
    {
        for (int j = 0; j < resource_count; ++j)
        {
            printf("%d \t", allocation[i][j]);
        }
        printf("\n");
    }
    printf("Need: \n");
    for (int i = 0; i < process_count; ++i)
    {
        for (int j = 0; j < resource_count; ++j)
        {
            printf("%d \t", need[i][j]);
        }
        printf("\n");
    }
    printf("Request: \n");
    for (int i = 0; i < process_count; ++i)
    {
        for (int j = 0; j < resource_count; ++j)
        {
            printf("%d \t", request[i][j]);
        }
        printf("\n");
    }
    printf("Available: \n");
    for (int i = 0; i < resource_count; ++i)
    {
        printf("%d \t", available[i] );
    }
    printf("\n");
    
}
