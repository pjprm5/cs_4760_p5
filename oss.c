// Paul Passiglia
// cs_4760
// Project 5
// 11/11/20
// oss.c



#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <signal.h>
#include "sharedinfo.h"
#include <math.h>
#include <getopt.h>
#include <semaphore.h>


#define deadLockLimit 10        // Tune deadlock
#define lineLimit 100000        // Max lines on output file
#define OUTFILE "output.txt" 
#define arbitraryCost 250000000 // Arbitrary cost per loop
#define nanosecsMax 50000000 
#define descriptorLimit 20      // Types of resources
#define progTTL 5               // Number of real life secs until termination
#define childMax 18             // Max number of children alive
#define SLOTQ 10                // Max quant of slots per resource type


//Prototypes
void raiseAlarm();
int setSecs(int nanosecs);
int rangeRand(int range);
int nanosecsRand();


// Global variables.
SharedInfo *sharedInfo;
int infoID;
int msqID;



int main (int argc, char *argv[])
{
  signal(SIGALRM, raiseAlarm);

  printf("oss.c begins....\n");

  int proc_count = 0;
  int bitVector[18];
  
  // Timespec struct for nanosleep.
  struct timespec tim1, tim2;
  tim1.tv_sec = 0;
  tim1.tv_nsec = 10000L;

 
  
  // Allocate shared memory information -------------------------------
  key_t infoKey = ftok("makefile", 123); 
  if (infoKey == -1)
  {
    perror("OSS: Error: ftok failure");
    exit(-1);
  }
  
  // Create shared memory ID.
  infoID = shmget(infoKey, sizeof(SharedInfo), 0600 | IPC_CREAT);
  if (infoID == -1)
  {
    perror("OSS: Error: shmget failure");
    exit(-1);
  }

  // Attach to shared memory.
  sharedInfo = (SharedInfo*)shmat(infoID, (void *)0, 0);
  if (sharedInfo == (void*)-1)
  {
    perror("OSS: Error: shmat failure");
    exit(-1);
  }

  alarm(3);

  // Begin launching child processes -----------------------------------  
  // Set shared clock to zero
  sharedInfo->secs = 0;
  sharedInfo->nanosecs = 0;

  // Seed shared clock to a random time for scheduling first process
  srand(time(0)); // seed random time
  unsigned int randomTimeToSchedule = randomTime();
  printf("Random time: %u \n", randomTimeToSchedule);
  





  

  //Below should never execute

  for (i = 0; i < desciptorLimit; i++)
  {
    shmdt(rsrcArr[i]);
    shmctl(shrID[i], IPC_RMID, NULL);
    shmdt(shInterface[i]);
    shmctl(shiID[i], IPC_RMID, NULL);
  }

  shmdt(shcont); // Detach shared memory.
  shmctl(shcID, IPC_RMID, NULL); // Destroy shared memory.
  return 0;
}

void raiseAlarm() // Kill everything once time limit hits which is 3 real life seconds
{
  printf("\n Time limit hit, terminating all processes. \n");
  int i;
  for (i = 0; i < descriptorLimit; i++) 
  {
    shmdt(rsrcArr[i]);
    shmctl(shrID[i], IPC_RMID, NULL);
    shmdt(shInterface[i]);
    shmctl(shiID[i], IPC_RMID, NULL);
  }
  shmdt(shcont);
  shmctl(shcID, IPC_RMID, NULL);
  kill(0, SIGKILL);
}

int setSecs(int nanosecs) 
{
  int accumSecs = 0;
  if (nanosecs >= 10000000000)
  {
    nanosecs = nanosecs - 1000000000;
    accumSecs = 1 + setSecs(nanosecs);
  }
  return accumSecs;
}

int rangeRand (int range)
{
  int randNum = ((rand() % (range)) + 1);
  return randNum;
}

int nanosecsRand()
{
  int randNum = ((rand() % (nanosecsMax)) + 1);
  return randNum;
}




