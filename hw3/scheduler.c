#include "threadutils.h"

/*
0. You should state the signal you received by:
   printf('TSTP signal!\n') or printf('ALRM signal!\n')
1. If you receive SIGALRM, you should reset alarm() by timeslice argument passed in ./main
2. You should longjmp(SCHEDULER,1) once you're done
*/
void sighandler(int signo)
{
   if (signo == SIGALRM)
   {
      printf("ALRM signal!\n");
      alarm(timeslice);
   }
   else if (signo == SIGTSTP)
   {
      printf("TSTP signal!\n");
   }
   sigprocmask(SIG_SETMASK, &base_mask, NULL);
   longjmp(SCHEDULER, YIELD);
}

/*
0. You are stronly adviced to make setjmp(SCHEDULER) = 1 for ThreadYield() case
                                   setjmp(SCHEDULER) = 2 for ThreadExit() case
1. Please point the Current TCB_ptr to correct TCB_NODE
2. Please maintain the circular linked-list here
*/
void scheduler()
{
   Current = Current->Next;
   int status = setjmp(SCHEDULER);
   if (status == YIELD)
   {
      Current = Current->Next;
   }
   else if (status == EXIT)
   {
      if (Current->Next != Current)
      {
         Current->Next->Prev = Current->Prev;
         Current->Prev->Next = Current->Next;
         free(Current);
         Current = Current->Next;
      }
      else
      {
         // Exit the scheduler
         longjmp(MAIN, 1);
      }
   }
   longjmp(Current->Environment, 1);
}