// Paul Passiglia
// cs_4760
// Project 5
// 11/11/2020
// user_proc.c



#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include "shclock.h"
#include "fauxMQ.h"
#include "shdescriptors.h"
#include <semaphore.h>
#include <signal.h>

#define childMax 18
#define descriptorLimit 20
#define NANO 10000000
#define MILLI 250

// Prototypes
int rangeRand(int blip);

// Global vars
SharedDescriptors *descArray[descriptorLimit];
SharedClock *sharedClock;
FauxMQ *fauxMQ;
int shMQID;
int shClockID;
int shResID[descriptorLimit];

int main (int argc, char *argv[])
{
  printf("user_proc.c begins... \n");

  // Seed rand off of pid
  srand(time(NULL)^(getpid()<<16));  
  // Wait struct
  struct timespec ts1, ts2;
  ts1.tv_sec = 0;
  ts1.tv_nsec = 500L;
  
  // Shared memory setup

  key_t clockKey = ftok("makefile", (666));
  if (clockKey == -1)
  {
    perror("USER: Error: ftok failure. \n");
    exit(-1);
  }

  shClockID = shmget(clockKey, sizeof(SharedClock), 0600 | IPC_CREAT);
  if (shClockID == -1)
  {
    perror("USER: Error: shmget failure. \n");
    exit(-1);
  }
  
  sharedClock = (SharedClock*)shmat(shClockID, (void *)0, 0);
  if (sharedClock == (void*)-1)
  {
    perror("USER: Error: shmat failure. \n");
    exit(-1);
  }

  // Seed resources
  int i;
  for (i = 0; i < descriptorLimit; i++)
  {
    key_t clockKey = ftok("makefile", (667 + i));
    if (clockKey == -1)
    {
      perror("USER: Error: ftok failure. \n");
      printf(" i: %d\n", i);
      exit(-1);
    }

    shResID[i] = shmget(clockKey, sizeof(SharedDescriptors), 0600 | IPC_CREAT);
    if (shResID[i] == -1)
    {
      perror("USER: Error: shmget failure. \n");
      printf(" i: %d\n", i);
      exit(-1);
    }

    descArray[i] = (sharedDescriptors*)shmat(shResID[i], (void *)0, 0);
    if (descArray[i] == (void*)-1)
    {
      perror("USER: Error: shmat failure. \n");
      printf(" i: %d\n", i);
      exit(-1);
    }
  }

  // Termination
  sem_wait(&(sharedClock->mutex));
  sharedClock->termCrit = getpid();
  sem_post(&(sharedClock->mutex));
  printf("USER: Child with PID: %d terminating. \n", getpid());
  for (i = 0; i < descriptorLimit; i++)
  {
    shmdt(descArray[i]);
  }
  shmdt(sharedClock);
  shmdt(fauxMQ);
  return 0;
}

int rangeRand(int blip)
{
  int rangeRand = ((rand() % (blip)) + 1);
  return rangeRand;
}
