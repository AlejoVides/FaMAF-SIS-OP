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
  int pid;
  int count;

  if(argc != 2 || (count = atoi(argv[1])) <= 0){
    fprintf(2, "ERROR: usage: pingpong <positive_integer>\n");
    exit(1);
  }

  int ping = open_available_semaphore(1);
  int pong = open_available_semaphore(0);

  if(ping == NSEMAPHORE || pong == NSEMAPHORE){
    fprintf(2, "\nERROR: no available semaphore\n");
    if(ping != NSEMAPHORE) sem_close(ping);
    if(pong != NSEMAPHORE) sem_close(pong);
    exit(1);
  }

  pid = fork();
  if(pid == -1){
    fprintf(2, "ERROR: fork failed\n");
    exit(1);
  } else if(pid == 0){
    for(int i = 0; i < count; i++){
      sem_down(ping); 
      printf("ping\n");
      sem_up(pong);
    }
    exit(0);
  } else {
    for(int i = 0; i < count; i++){
      sem_down(pong);
      printf("\tpong\n");
      sem_up(ping);
    }
  }

  wait(0);

  sem_close(ping);
  sem_close(pong);

  exit(0);
}
