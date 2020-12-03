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
#include "shclock.h"
#include "fauxMQ.h"
#include "shdescriptors.h"
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
SharedDescriptors *descArray[descriptorLimit];    // Shared Resource structure
SharedClock *sharedClock;                         // Shared Clock structure
FauxMQ *fauxMQ[childMax];                         // Shared Faux Message Queue structure

// IDs for shared memory structs
int shclockID;
int shResID[descriptorLimit];
int shMQID[childMax];
FILE *fptrOut;



int main (int argc, char *argv[])
{
  signal(SIGALRM, raiseAlarm);   // Setup alarm for time termination.
  srand(time(0));                // Seed random numbers.

  printf("oss.c begins....\n");
  
  int opIndex = 0; // getopt var
  int optH = 0;    // flag
  int optV = 0;     // flag
  
  // Getopt loop.
  while ((opIndex = getopt(argc, argv, "hv")) != -1)
  {
    switch (opIndex)
    {
      case 'h':
        printf("For verbose logging of data choose -v \n");
        printf("The default is just Deadlock Detection data. \n");
        return 0;
        break;
      
      case 'v':
        printf(" Verbose logging format has been chosen. \n");
        optV = 1;
        break;

      case '?': // Filter bad input
        if (isprint (optopt))
        {
          perror("OSS: Error: Unknown input.");
          return -1;
        }
        else
        {
          perror("OSS: Error: Unknown error occured.");
          return -1;
        }
    }
  }

  alarm(5);
  
  





  

  //Below should never execute

  for (i = 0; i < desciptorLimit; i++)
  {
    shmdt(descArray[i]);
    shmctl(shResID[i], IPC_RMID, NULL);
    shmdt(fauxMQ[i]);
    shmctl(shMQID[i], IPC_RMID, NULL);
  }

  shmdt(sharedClock); // Detach shared memory.
  shmctl(shClockID, IPC_RMID, NULL); // Destroy shared memory.
  return 0;
}

void raiseAlarm() // Kill everything once time limit hits which is 3 real life seconds
{
  printf("\n Time limit hit, terminating all processes. \n");
  int i;
  for (i = 0; i < descriptorLimit; i++) 
  {
    shmdt(descArray[i]);
    shmctl(shResID[i], IPC_RMID, NULL);
    shmdt(fauxMQ[i]);
    shmctl(shMQID[i], IPC_RMID, NULL);
  }
  shmdt(sharedClock);
  shmctl(shClockID, IPC_RMID, NULL);
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




