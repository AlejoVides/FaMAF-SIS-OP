#include "kernel/param.h"
#include "kernel/types.h"
#include "user/user.h"

// PRE: value >= 0
int
open_available_semaphore(int value)
{
  int sem = 0;
  while(sem < NSEMAPHORE && !sem_open(sem, value)){
    sem++;
  }
  return sem;
}

int
main(int argc, char* argv[])
{
  int pid1;
  int pid2;
  int count;

  if(argc != 2 || (count = atoi(argv[1])) <= 0){
    fprintf(2, "ERROR: usage: tateti <positive_integer>\n");
    exit(1);
  }

  int ta = open_available_semaphore(1);
  int te = open_available_semaphore(0);
  int ti = open_available_semaphore(0);

  if(ta == NSEMAPHORE || te == NSEMAPHORE || ti == NSEMAPHORE){
    fprintf(2, "\nERROR: no available semaphore\n");
    if(ta != NSEMAPHORE) sem_close(ta);
    if(te != NSEMAPHORE) sem_close(te);
    if(ti != NSEMAPHORE) sem_close(ti);
    exit(1);
  }

  pid1 = fork();
  if(pid1 == -1){
    fprintf(2, "ERROR: fork failed\n");
    exit(1);
  } else if(pid1 == 0){
    for(int i = 0; i < count; i++){
      sem_down(ta);
      printf("TA ");
      sem_up(te);
    }
    exit(0);
  }

  pid2 = fork();
  if(pid2 == -1){
    fprintf(2, "ERROR: fork failed\n");
    exit(1);
  } else if(pid2 == 0){
    for(int i = 0; i < count; i++){
      sem_down(te);
      printf("TE ");
      sem_up(ti);
    }
    exit(0);
  }

  for(int i = 0; i < count; i++){
    sem_down(ti);
    printf("TI\n");
    sem_up(ta);
  }

  wait(0);
  wait(0);

  sem_close(ta);
  sem_close(te);
  sem_close(ti);

  exit(0);
}
