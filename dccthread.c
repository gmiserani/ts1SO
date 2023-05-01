#include "dccthread.h"
#include "dlist.h"

#include <ucontext.h>
#include <stdbool.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//Thread structure
typedef struct dccthread{
    char name[DCCTHREAD_MAX_NAME_SIZE]; //Threads's name
    ucontext_t *context;
    //bool isSleeping;
    bool isWaiting;
    dccthread_t *waintingFor;

} dccthread_t;

ucontext_t manager;
struct dlist *readyThreadList;

//Function to initialize the thread library and creat a new thread to excecute the function func with the parameter param
void dccthread_init(void (*func)(int), int param){
    readyThreadList = dlist_create();
    // 1 -- Initialize a meneger thread that will scalonate the new threads 
    // 2 -- Inicialize a main thread that will execute func THE NAME HAS TO BE MAIN
    dccthread_create("main", func, param);
    getcontext(&manager);
    // 3 -- Execute the main thread
    while(!dlist_empty(readyThreadList))
    {
        dccthread_t *nextThread = (dccthread_t*) dlist_get_index(readyThreadList, 0); // get the next in line thread

        if(nextThread->isWaiting)
        {
            dlist_pop_left(readyThreadList);
            dlist_push_right(readyThreadList, nextThread);
        }

        swapcontext(&manager, nextThread->context);
        dlist_pop_left(readyThreadList);

        if(nextThread->isWaiting){
            dlist_push_right(readyThreadList, nextThread);
        }
    }
    exit(0); 
}

//Create a new thread with name that will execute func with param
dccthread_t * dccthread_create(const char *name, void (*func)(int), int param){

    // 1 -- Inicialize a new thread 

        // Inicialize the thread
        dccthread_t *newThread = malloc(sizeof(dccthread_t));
        strcpy(newThread->name, name);
        newThread->isWaiting = false;
        newThread->waintingFor = NULL;

        // Inicialize the context
        ucontext_t *newContext = malloc(sizeof(ucontext_t));
        getcontext(newContext);// Initializes the new thread's context as the currently active context
        newContext->uc_link = &manager;
        newContext->uc_stack.ss_flags = 0;
        newContext->uc_stack.ss_size = THREAD_STACK_SIZE;
        newContext->uc_stack.ss_sp = malloc(THREAD_STACK_SIZE);

        newThread->context = newContext;
    
    // 2 -- Add it to the list of threads ready to execute
    dlist_push_right(readyThreadList, newThread);

    // 3 -- Return to the thread that originally called the function
    makecontext(newThread->context, (void(*)(void))func, 1, param); // criação do contexto da thread

    return newThread;
}

void dccthread_yield(void){
    // 1 -- Remove the current thread from the CPU
    dccthread_t *currentThread = dccthread_self();
    dlist_push_right(readyThreadList, currentThread); // Return to the end of the list
    // 2 -- Call the maneger thread to choose the next thread
    swapcontext(currentThread->context, &manager);
    // * a tread must be executed in the order they were created
}

// Return a pointer to the thread that is currently being executed
dccthread_t *dccthread_self(void){
    dccthread_t *currentThread = dlist_get_index(readyThreadList, 0);
    return currentThread;
}

// Return the name of the tid thread
const char *dccthread_name(dccthread_t *tid){
    return tid->name;
}

// Ends the current thread
void dccthread_exit(void){
    dccthread_t *currentThread = dccthread_self();
    for(int i = 0; i < readyThreadList->count; i++)
    {
        dccthread_t *thread = dlist_get_index(readyThreadList, i);
        if(thread->isWaiting && thread->waintingFor == currentThread){
            thread->waintingFor = NULL;
            thread->isWaiting = false;
        }
    }
    free(currentThread->context->uc_stack.ss_sp);
} // when the lat thread calls for exit the program must end

// Blocks the current thread untill tid is done executing
void dccthread_wait(dccthread_t *tid){
    bool found = false;

    // if we find the object Tid in the ready list its not done executing yet
    for(int i = 0; i < readyThreadList->count && !found; i++)
    {
        dccthread_t *thread = dlist_get_index(readyThreadList, i);
        if(thread == tid)
        {
            found = true;
        }
    }

    // if we found the object Tid in the ready list, then the current thread is going to stop executing and wait fot tid to end
    if(found)
    {
        dccthread_t *currentThread = dccthread_self();
        currentThread->isWaiting = true; 
        currentThread->waintingFor = tid;
        swapcontext(currentThread->context, &manager);
    }
}