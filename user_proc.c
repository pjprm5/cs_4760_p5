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
  // Time variables
  unsigned int timeToSpawn;
  unsigned int minToTerm;
  unsigned int timeNow;
  unsigned int prevTime;
  int displacement;
  int wait;

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

    descArray[i] = (SharedDescriptors*)shmat(shResID[i], (void *)0, 0);
    if (descArray[i] == (void*)-1)
    {
      perror("USER: Error: shmat failure. \n");
      printf(" i: %d\n", i);
      exit(-1);
    }
  }
  
  // Collect offset from parent to begin parent/child communication
  while (wait == 0)
  {
    sem_wait(&(sharedClock->mutex));
    if (sharedClock->spawnCrit == getpid())
    {
      displacement = sharedClock->displacement;
      sharedClock->spawnCrit = 0;
      sharedClock->displacement = 0;
      sem_post(&(sharedClock->mutex));
      wait = 1;
      
      timeToSpawn = (sharedClock->nanosecs/1000000000) + (sharedClock->secs);
      minToTerm = (sharedClock->nanosecs/1000000000) + (sharedClock->secs) + 1;
      
      key_t fmqkey = ftok("makefile", (665 - displacement));
      if (fmqkey == -1)
      {
        perror("USER: Error: ftok failure. \n");
        exit(-1);
      }
      
      shMQID = shmget(fmqkey, sizeof(FauxMQ), 0600 | IPC_CREAT);
      if (shMQID == -1)
      {
        perror("USER: Error: shmget failure. \n");
        exit(-1);
      }

      fauxMQ = (FauxMQ*)shmat(shMQID, (void *)0, 0);
      if (fauxMQ == (void*)-1)
      {
        perror("USER: Error: shmat failure. \n");
        exit(-1);
      }

      sem_wait(&(fauxMQ->mutex));
      fauxMQ->fauxPid = getpid();
      for (i = 0; i < 20; i++)
      {
        fauxMQ->fRelease[i][0]= 0;
        fauxMQ->fRelease[i][1] = 0;
        fauxMQ->fRequest[i][0] = getpid();
        fauxMQ->fRequest[i][1] = 0;
        fauxMQ->fHeld[i][0] = getpid();
        fauxMQ->fHeld[i][1] = 0;
      }
      fauxMQ->fauxReleaseSig = 0;
      fauxMQ->fauxRequestSig = 0;
      sem_post(&(fauxMQ->mutex));
    }
    else
    {
      sem_post(&(sharedClock->mutex));
      while(nanosleep(&ts1, &ts2));
    }
  }
  
  printf("USER: Child: %d with displacement:%d \n", getpid(), displacement);
  
  // Populate array w/ quantity of each resource
  int resQuantity[descriptorLimit];
  for (i = 0; i < descriptorLimit; i++)
  {
    resQuantity[i] = descArray[i]->descNum;
  }

  // Main loop

  unsigned long checkTerm = 99999999999;
  int actionSig = 0;
  int mainLoop = 0;
  int restSig = 0;
  int skipRelease = 1;
  int termOver = 0;

  while (mainLoop == 0)
  {
    int mainRelease = 0;
    int mainRequest = 0;
    int new = 0;

    // Check for termination
    prevTime = timeNow;
    if (timeNow >= minToTerm)
    {
      if ((timeNow - prevTime) >= checkTerm)
      {
        mainLoop = 1;
        termOver = 1;
        actionSig = 0;
        mainRelease = 1;
        sem_wait(&(fauxMQ->mutex));
        for (i = 0; i < descriptorLimit; i++)
        {
          fauxMQ->fRelease[i][0] = getpid();
          fauxMQ->fRelease[i][1] = fauxMQ->fHeld[i][1];
          fauxMQ->fauxReleaseBait = 1;
        }
        sem_post(&(fauxMQ->mutex));
      }
      checkTerm = ((rangeRand(MILLI)) / 10000000);
    }

    // Determine release or requesting or none
    if (((timeNow - prevTime) >= (rangeRand(NANO) / 1000000000)) && (termOver == 0))
    {
      actionSig = 1;
      int direction = (rangeRand(100) % 2);
      if (direction == 1)
      {
        mainRequest = 1;
        skipRelease = 0;
      }
      else if ((direction == 0) && (restSig == 1) && (skipRelease == 0))
      {
        mainRelease = 1;
      }
    }

    // Main resource release
    if (actionSig == 1)
    {
      // Allocate all system resources
      for (i = 0; i < descriptorLimit; i++)
      {
        int j;
        int rCounter = 0;
        fauxMQ->fHeld[i][0] = 0;
        fauxMQ->fHeld[i][1] = 0;
        for (j = 0; j < 10; j++)
        {
          sem_wait(&(descArray[i]->mutex));
          if (descArray[i]->descAlloc[j] == getpid())
          {
            sem_wait(&(fauxMQ->mutex));
            fauxMQ->fHeld[i][0] = getpid();
            fauxMQ->fHeld[i][1] = fauxMQ->fHeld[i][1] + 1;
            sem_post(&(fauxMQ->mutex));
            rCounter++;
          }
          sem_post(&(descArray[i]->mutex));
          if (rCounter == 0)
          {
            sem_wait(&(fauxMQ->mutex));
            fauxMQ->fHeld[i][0] = getpid();
            fauxMQ->fHeld[i][1] = 0;
            sem_post(&(fauxMQ->mutex));
          }
        }
      }
    }

      // Determine which system resources to release
      if (mainRelease == 1)
      {
        new = 0;
        for (i = 0; i < descriptorLimit; i++)
        {
          if (fauxMQ->fHeld[i][1] != 0)
          {
            int tempRange = 100;
            if (fauxMQ->fHeld[i][1] >= 3)
            {
              tempRange = 250;
            }
            if (rangeRand(tempRange) > 25)
            {
              sem_wait(&(fauxMQ->mutex));
              fauxMQ->fRelease[1][0] = getpid();
              fauxMQ->fRelease[1][1] = rangeRand(fauxMQ->fHeld[i][1]);
              fauxMQ->fauxReleaseBait = 1;
              sem_post(&(fauxMQ->mutex));
            }
          }
        }
      }

      // Determine additional requests
      if (mainRequest == 1)
      {
        new = 0;
        for (i = 0; i < descriptorLimit; i++)
        {
          sem_wait(&(fauxMQ->mutex));
          int tempReq = ((descArray[i]->descNum) - (fauxMQ->fHeld[i][1]));
          if (tempReq == 0)
          {
            fauxMQ->fRequest[i][0] = getpid();
            fauxMQ->fRequest[i][1] = 0;
            sem_post(&(fauxMQ->mutex));
          }

          else
          {
            int tempAddReq = rangeRand(tempReq);
            if (tempAddReq > 4)
            {
              int decay = (rangeRand(100) % 2);
              if (decay == 0)
              {
                tempAddReq = (tempAddReq/2);
              }
            }
            
            if (tempAddReq == 1)
            {
              int chanceForZero = rangeRand(100);
              if ((chanceForZero % 2) == 0)
              {
                fauxMQ->fRequest[i][0] = getpid();
                fauxMQ->fRequest[i][1] = 0;
                sem_post(&(fauxMQ->mutex));
              }
              
              else 
              {
                fauxMQ->fRequest[i][0] = getpid();
                fauxMQ->fRequest[i][1] = tempAddReq;
                fauxMQ->fauxRequestBait = 1;
                sem_post(&(fauxMQ->mutex));
              }
            }
          }
        }
      }

      else 
      {
        while (nanosleep(&ts1, &ts2));
      }

      // Loop to mediate action between parent/child

      if (mainRelease = 1)
      {
        while (new == 0)
        {
          if ((fauxMQ->fauxReleaseSig == 1))
          {
            new = 1;
            fauxMQ->fauxReleaseSig = 0;
          }
          else
          {
            while (nanosleep(&ts1, &ts2));
          }
        }
      }

      else if (mainRequest == 1)
      {
        while (new == 0)
        {
          if ((fauxMQ->fauxRequestSig == 1))
          {
            new = 1;
            restSig = 1;
            fauxMQ->fauxRequestSig = 0;
          }
          else
          {
            while (nanosleep(&ts1, &ts2));
          }
        }
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
