#include "types.h"
#include "param.h"
#include "riscv.h"
#include "spinlock.h"
#include "defs.h"

#define INT_MAX 2147483647 // 2^31 - 1
#define CLOSED_SEM_VALUE (-1)
#define SUCCESS 1
#define FAILURE 0

struct semaphore {
  struct spinlock lock;
  int value;
};

static struct semaphore semaphore[NSEMAPHORE];

// Initialize the semaphore table.
void
seminit(void)
{
  for(uint i = 0; i < NSEMAPHORE; i++){
    semaphore[i].value = CLOSED_SEM_VALUE;
    initlock(&semaphore[i].lock, "semaphore");
  }
}

static int
sem_valid(int sem, char *fname)
{
  // check that sem is valid
  if(sem < 0 || sem >= NSEMAPHORE){
    printf("ERROR: %s: invalid semaphore number %d\n", fname, sem);
    return FAILURE;
  }
  return SUCCESS;
}

int
sem_open(int sem, int value)
{
  if(!sem_valid(sem, "sem_open")) return FAILURE;

  // check that value is valid
  if(value < 0){
    printf("ERROR: sem_open: semaphore %d value must be >= 0\n", sem);
    return FAILURE;
  }

  int res;

  acquire(&semaphore[sem].lock);
  if(semaphore[sem].value != CLOSED_SEM_VALUE){
    res = FAILURE;
  } else {
    semaphore[sem].value = value;
    res = SUCCESS;
  }
  release(&semaphore[sem].lock);

  return res;
}

int
sem_close(int sem)
{
  if(!sem_valid(sem, "sem_close")) return FAILURE;

  int res;

  acquire(&semaphore[sem].lock);
  if(semaphore[sem].value == CLOSED_SEM_VALUE){
    printf("ERROR: sem_close: semaphore %d already closed\n", sem);
    res = FAILURE;
  } else {
    semaphore[sem].value = CLOSED_SEM_VALUE;
    res = SUCCESS;
  }
  release(&semaphore[sem].lock);

  return res;
}

int
sem_up(int sem)
{
  if(!sem_valid(sem, "sem_up")) return FAILURE;

  int res;

  acquire(&semaphore[sem].lock);
  if(semaphore[sem].value == CLOSED_SEM_VALUE){
    printf("ERROR: sem_up: semaphore %d is closed\n", sem);
    res = FAILURE;
  } else if(semaphore[sem].value == INT_MAX){
    printf("ERROR: sem_up: semaphore %d overflow\n", sem);
    res = FAILURE;
  } else {
    semaphore[sem].value++;
    wakeup(&semaphore[sem]);
    res = SUCCESS;
  }
  release(&semaphore[sem].lock);

  return res;
}

int
sem_down(int sem)
{
  if(!sem_valid(sem, "sem_down")) return FAILURE;

  int res;

  acquire(&semaphore[sem].lock);
  if(semaphore[sem].value == CLOSED_SEM_VALUE){
    printf("ERROR: sem_down: semaphore %d is closed\n", sem);
    res = FAILURE;
  } else {
    while(semaphore[sem].value == 0){
      sleep(&semaphore[sem], &semaphore[sem].lock);
    }
    semaphore[sem].value--;
    res = SUCCESS;
  }
  release(&semaphore[sem].lock);

  return res;
}
