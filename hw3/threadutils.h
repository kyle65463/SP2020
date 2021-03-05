#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <signal.h>
#include <unistd.h>

extern int timeslice;
extern int switchmode;

#define YIELD 1
#define EXIT 2
#define ENDINIT 3

typedef struct TCB_NODE *TCB_ptr;
typedef struct TCB_NODE
{
    jmp_buf Environment; // The jmp_buf of each function
    int Thread_id;       // Not necessary for any tasks, but one may found it useful while debuging
    TCB_ptr Next;        // TCB_ptr points to next TCB_NODE in the circular linked-list
    TCB_ptr Prev;        // TCB_ptr points to previous TCB_NODE in the circular linked-list
    int i, N;            // See threefunctions.c for their usage
    int x, y, z;         // Three integers for you to store data you want.

} TCB;

extern jmp_buf MAIN;      // The jmp_buf for main()
extern jmp_buf SCHEDULER; // The jmp_buf for scheduler()

extern TCB_ptr Head;    // Three TCB_ptr for you to initialize your circular linked-list
extern TCB_ptr Current; // The Current TCB_ptr will move during "context switch"
extern TCB_ptr Work;

extern sigset_t base_mask, waiting_mask; // See main for meanings of each sigset_t
extern sigset_t tstp_mask, alrm_mask;

void sighandler(int signo);
void scheduler();
void printCir();

/*
Call function in the argument that is passed in
*/
#define ThreadCreate(function, thread_id, init, maxiter) \
    {                                                    \
        if (setjmp(MAIN) == 0)                           \
            function(thread_id, init, maxiter);          \
    }

/*
Build up TCB_NODE for each function, insert it into circular linked-list
*/
#define ThreadInit(thread_id, init, maxiter)    \
    {                                           \
        Current = (TCB *)(malloc(sizeof(TCB))); \
        Current->Thread_id = thread_id;         \
        Current->x = init;                      \
        Current->i = 0;                         \
        Current->N = maxiter;                   \
        if (Work == NULL)                       \
        {                                       \
            Work = Current;                     \
        }                                       \
        else if (Head == NULL)                  \
        {                                       \
            Head = Current;                     \
            Head->Prev = Work;                  \
            Work->Next = Head;                  \
        }                                       \
        else                                    \
        {                                       \
            Head->Next = Current;               \
            Current->Prev = Head;               \
            Current->Next = Work;               \
            Work->Prev = Current;               \
        }                                       \
        if (setjmp(Current->Environment) == 0)  \
            longjmp(MAIN, ENDINIT);             \
    }
/*
Call this while a thread is terminated
*/
#define ThreadExit()              \
    {                             \
        longjmp(SCHEDULER, EXIT); \
    }
/*
Decided whether to "context switch" based on the switchmode argument passed in main.c
*/
#define ThreadYield()                                    \
    {                                                    \
                                                         \
        if (setjmp(Current->Environment) == 0)           \
        {                                                \
            if (switchmode == 0)                         \
                longjmp(SCHEDULER, YIELD);               \
            if (switchmode == 1)                         \
            {                                            \
                sigpending(&waiting_mask);               \
                if (sigismember(&waiting_mask, SIGTSTP)) \
                    sigsuspend(&alrm_mask);              \
                if (sigismember(&waiting_mask, SIGALRM)) \
                    sigsuspend(&tstp_mask);              \
            }                                            \
        }                                                \
    }
