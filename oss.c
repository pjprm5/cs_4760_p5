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
#define nanosecsMax 500000000   // Between 5-100 milliseconds for random fork times (5-100 million nanosecs)
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
int shClockID;
int shResID[descriptorLimit];
int shMQID[childMax];
FILE *fileOut;



int main (int argc, char *argv[])
{
  signal(SIGALRM, raiseAlarm);   // Setup alarm for time termination.
  srand(time(0));                // Seed random numbers.

  printf("oss.c begins....\n");
  
  int opIndex = 0; // getopt var
  int optH = 0;    // flag
  int optV = 0;     // flag

  struct timespec ts1, ts2;
  ts1.tv_sec = 0;
  ts1.tv_nsec = 2500L;

  int activeChildren = 0;   // # of active children.
  int proc_count = 0;       // Keep track of 40 processes until term

  int activeArray[childMax] = { 0 }; // Local array to keep track of active children
  int fireOff[childMax][3] = { 0 };  // Local array for spawning -> indices req; nanosecs; secs

  
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

  alarm(5);  // Alarm gets 5 secs until term.
  
  fileOut = fopen(OUTFILE, "w");
  fprintf(fileOut, "------------------------------\n");
  fprintf(fileOut, "------------------------------\n");
  fprintf(fileOut, "------------BEGIN-------------\n");
  fprintf(fileOut, "------------------------------\n");
  fprintf(fileOut, "------------------------------\n");
  fprintf(fileOut, "------------------------------\n");
  fprintf(fileOut, "------------------------------\n");
  fclose(fileOut);
  

  // Shared Memory
  key_t clockKey = ftok("makefile", (666));
  if (clockKey == -1)
  {
    perror("OSS: Error: ftok failure. \n");
    exit(-1);
  }

  shClockID = shmget(clockKey, sizeof(SharedClock), 0600 | IPC_CREAT);
  if (shClockID == -1)
  {
    perror("OSS: Error: shmget failure. \n");
    exit(-1);
  }

  sharedClock = (SharedClock*)shmat(shClockID, (void *)0, 0);
  if (sharedClock == (void*)-1)
  {
    perror("OSS: Error: shmat failure. \n");
    exit(-1);
  }

  sem_init(&(sharedClock->mutex), 1, 1);
  sem_wait(&(sharedClock->mutex));
  sharedClock->spawnCrit = 0;
  sharedClock->displacement = 0;
  sem_post(&(sharedClock->mutex));

  // Shared resources. 
  
  // Range of shared resources between 20-25% of 20 
  int randShareRes = ((rand() % (5-4+1)) + 4); 
  int i;
  int g;

  for (i = 0; i < descriptorLimit; i++)
  {
    key_t reskey = ftok("makefile", (667 + i));
    if (reskey == -1)
    {
      perror("OSS: Error: ftok failure. \n");
      printf(" i: %d\n", i);
      exit(-1);
    }
    
    shResID[i] = shmget(reskey, sizeof(SharedDescriptors), 0600 | IPC_CREAT);
    if (shResID[i] == -1)
    {
      perror("OSS: Error: shmget failure. \n");
      printf(" i: %d\n", i);
      exit(-1);
    }

    descArray[i] = (SharedDescriptors*)shmat(shResID[i], (void *)0, 0);
    if (descArray[i] == (void*)-1)
    {
      perror("OSS: Error: shmat failure. \n");
      printf(" i: %d\n", i);
      exit(-1);
    }

    sem_init(&(descArray[i]->mutex), 1, 1);

    // Between 1 and 10 inclusive resources populated in each descriptor
    descArray[i]->descNum = rangeRand(10);
    
    // Determine what resources should be shared
    if (i > ((descriptorLimit - randShareRes) - 1))
    {
      descArray[i]->descShare = 1;
    }

    else
    {
      descArray[i]->descShare = 0;
    }
    
    // Inititalize allocation array to zero
    for (g = 0; g < 10; g++)
    {
      descArray[i]->descAlloc[g] = 0;
    }

  }

  // Populate local arrays with resources, quantity/shared
  int resQuantMax[descriptorLimit];    // Max quant of res
  int localSharedRes[descriptorLimit]; // What's shared
  for(i = 0; i < descriptorLimit; i++)
  {
    resQuantMax[i] = descArray[i]->descNum;
    if (descArray[i]->descShare == 1)
    {
      localSharedRes[i] = 1;
    }
    else
    {
      localSharedRes[i] = 0;
    }
  }
  
  // Print resources available.
  printf("Resources available: ");
  for (i = 0; i < descriptorLimit; i++)
  {
    printf("%d ", resQuantMax[i]);
  }
  printf("\n");
  
  // Variables for stats
  int lineCount = 0; // Line count
  int printReq = 0;  // Print request
  int numReq = 0;    // # of requests
  int numBlock = 0;
  
  int heldFixed[childMax][descriptorLimit] = { 0 };    // number of iterations fHeld did not change
  int heldPrevious[childMax][descriptorLimit] = { 0 }; // compares current array against the next
  int heldNow[childMax][descriptorLimit] = { 0 };      // copy new current here
  
  // Main Loop
  
  int mainLoop = 0;
  while (mainLoop == 0)
  {
    //Term crit, 40 processes, quit.
    if (proc_count == 40)
    {
      break;
    }
    // Line count tracker for verbose option
    if (lineCount >= 100000)
    {
      optV = 0;
    }
    
    // Smooth out iterations
    while(nanosleep(&ts1, &ts2)); 

    // Fix clock
    sem_wait(&(sharedClock->mutex));
    if (sharedClock->nanosecs >= 1000000000)
    {
      sharedClock->secs = sharedClock->secs + sharedClock->nanosecs/1000000000;
      sharedClock->nanosecs = sharedClock->nanosecs % 1000000000;
    }
    sem_post(&(sharedClock->mutex));

    // Child Logic, handle dead children
    int b;
    if (activeChildren > 0)
    {
      sem_wait(&(sharedClock->mutex));
      if (sharedClock-> termCrit != 0)
      {
        //waitpid(sharedClock->termCrit);
        wait(NULL);
        for (b = 0; b < childMax; b++)
        {
          if (activeArray[b] == sharedClock->termCrit)
          {

            // Clear out resources held by dead child
            for (i = 0; i < descriptorLimit; i++)
            {
              int j;
              for (j = 0; j < 10; j++)
              {
                if (descArray[i]->descAlloc[j] == sharedClock->termCrit)
                {
                  descArray[i]->descAlloc[j] = 0;
                }
              }
              heldPrevious[b][i] = 0;
              heldNow[b][i] = 0;
              heldFixed[b][i] = 0;
            }
            shmdt(fauxMQ[b]);
            shmctl(shMQID[b], IPC_RMID, NULL);
            activeArray[b] = 0; // Active array false
            activeChildren--; // Decrement active child
          }
        }
        sharedClock->termCrit = 0;
      }
      sem_post(&(sharedClock->mutex));
    }

    // Search for available child slots
    for (b = 0; b < childMax; b++)
    {
      if ((activeArray[b] == 0) && (fireOff[b][0] == 0))
      {
        fireOff[b][0] = 1; // Prompt for child to fire off
        fireOff[b][1] = (sharedClock->nanosecs + nanosecsRand()); // time for fork nano
        fireOff[b][2] = (sharedClock->secs); // time for fork secs
      }
    }

    // Populate vacant children
    pid_t childPid;
    int breakOff = 0;
    for (b = 0; b < childMax; b++)
    {
      if (fireOff[b][0] == 1)
      {
        if ((sharedClock->nanosecs >= fireOff[b][1]) && (sharedClock->secs >= fireOff[b][2]) && (activeChildren <= childMax))
        {
          int checkChild = 0;
          while (checkChild == 0)
          {
            // Controls child replacement
            checkChild = 1;
            childPid = fork();
            if (childPid == 0)
            {
              char *args[] = {"./user_proc", NULL};
              execvp(args[0], args);
            }
            else if (childPid < 0) // retry loop
            {
              checkChild = 0;
              while(nanosleep(&ts1, &ts2)); // smooth iteration
            }
            else if (childPid > 0)
            {
              activeChildren++;
              printf("Child#: %d with pid: %d is born ----------> \n", proc_count, childPid);
              fileOut = fopen(OUTFILE, "a");
              fprintf(fileOut, "Child #: %d with pid: %d is born ----------->\n", proc_count, childPid); 
              fclose(fileOut);
              proc_count++;             
              activeArray[b] = childPid;
              checkChild = 1;
              int remCrit = 0;
              // Determine displacement 
              while (remCrit == 0)
              {
                sem_wait(&(sharedClock->mutex));
                if (sharedClock->spawnCrit == 0)
                {
                  sharedClock->spawnCrit = childPid;
                  sharedClock->displacement = b;
                  sem_post(&(sharedClock->mutex));
                  remCrit = 1;
                }
                else
                {
                  sem_post(&(sharedClock->mutex));
                  while(nanosleep(&ts1, &ts2));
                }
              }
            
              //Reset local spawn controls to zero
              fireOff[b][0] = 0;
              fireOff[b][1] = 0;
              fireOff[b][2] = 0;
              //Setup for fauxmq per forked child determine by displacement
              key_t fmqkey = ftok("makefile", (665 - b));
              if (fmqkey == -1)
              {
                perror("OSS: Error: ftok failure. \n");
                exit(-1);
              }
              shMQID[b] = shmget(fmqkey, sizeof(FauxMQ), 0600 | IPC_CREAT);
              if (shMQID[b] == -1)
              {
                perror("OSS: Error: shmget failure. \n");
                exit(-1);
              }
              fauxMQ[b] = (FauxMQ*)shmat(shMQID[b], (void *)0, 0);
              if (fauxMQ[b] == (void*)-1)
              {
                perror("OSS: Error: shmat failure. \n");
                exit(-1);
              }
              sem_init(&(fauxMQ[b]->mutex), 1, 1);
            }
          }
        }
      }
    }

    // Deadlock prevention

    for (b = 0; b < childMax; b++) // Recoup status
    {
      if (activeArray[b] != 0) // Go through all living children
      {
        for (i = 0; i < descriptorLimit; i++) // And each resource
        {
          heldPrevious[b][i] = heldNow[b][i];
        }
      }
    }

    // Seek updated status

    for (b = 0; b < childMax; b++)
    {
      if (activeArray[b] != 0)
      {
        for (i = 0; i < descriptorLimit; i++)
        {
          heldNow[b][i] = fauxMQ[b]->fHeld[i][1];
        }
      }
    }

    // Check for expired resources
    for (b = 0; b < childMax; b++)
    {
      if (activeArray[b] != 0)
      {
        for (i = 0; i < descriptorLimit; i++)
        {
          if (heldPrevious[b][i] == heldNow[b][i])
          {
            if (heldNow[b][i] != 0)
            {
              heldFixed[b][i]++;
            }
          }
        }
      }
    }

    // Clear expired resources
    int expired = 0;
    for (b = 0; b < childMax; b++)
    {
      if (activeArray[b] != 0)
      {
        for (i = 0; i < descriptorLimit; i++)
        {
          // If resource is held over deadlocklimit, tag as expired
          if (heldFixed[b][i] > deadLockLimit)
          {
            expired = 1;
            sem_wait(&(fauxMQ[b]->mutex));
            fauxMQ[b]->fRelease[i][1] = fauxMQ[b]->fHeld[i][1];
            fauxMQ[b]->fauxReleaseBait = 1;
            fauxMQ[b]->fauxRequestBait = 0;
            fauxMQ[b]->fauxRequestSig = 0;
            heldFixed[b][i] = 0;
            sem_post(&(fauxMQ[b]->mutex));
          }
        }
      }
    }

    if (expired == 1)
    {
      fileOut = fopen(OUTFILE, "a");
      fprintf(fileOut, "---------DEADLOCK DETECTED AND FIXED -----------------\n");
      fprintf(fileOut, "Time: %09f --- #Requests: %d --- #Blocks: %d \n", (((double)sharedClock->nanosecs/1000000000) + ((double) sharedClock->secs)), numReq, numBlock);
      fprintf(fileOut, "------------------------------------------------------\n");
      fclose(fileOut);
    }

    // Release Resources
    
    for (b = 0; b < childMax; b++)
    {
      if (activeArray[b] != 0)
      {
        sem_wait(&(fauxMQ[b]->mutex));
        // Release bait detected, release resources
        if (fauxMQ[b]->fauxReleaseBait == 1)
        {
          // For verbose setting
          if ((optV == 1) && (lineCount < lineLimit))
          {
            fileOut = fopen(OUTFILE, "a");
            fprintf(fileOut, "OSS: Child %d resources deallocation: ", fauxMQ[b]->fauxPid);
            lineCount++;
            fclose(fileOut);
          }
          
          for (i = 0; i < descriptorLimit; i++)
          {
            int freeRes = fauxMQ[b]->fRelease[i][1]; 
            // For verbose setting
            if ((optV == 1) && (lineCount < lineLimit))
            {
              fileOut = fopen(OUTFILE, "a");
              fprintf(fileOut, " %d", freeRes);
              fclose(fileOut);
            }
            int r;
            sem_wait(&(descArray[i]->mutex));
            for (r = 0; r < freeRes; r++)
            {
              int deAllocSig = 0;
              for (g = 0; g < 10; g++)
              {
                if (deAllocSig == 0)
                {
                  if (descArray[i]->descAlloc[g] == fauxMQ[b]->fauxPid)
                  { 
                    // Decrement held and zero out allocation slot
                    descArray[i]->descAlloc[g] = 0;
                    descArray[i]->descHeld--;
                    numBlock++;
                    deAllocSig = 1;
                  }
                }
              }
            }
            sem_post(&(descArray[i]->mutex));
            fauxMQ[b]->fHeld[i][1] = fauxMQ[b]->fHeld[i][1] - fauxMQ[b]->fRelease[i][1];
            fauxMQ[b]->fRelease[i][1] = 0;
            fauxMQ[b]->fauxReleaseBait = 0;
            fauxMQ[b]->fauxReleaseSig = 1;
          }
          sem_post(&(fauxMQ[b]->mutex));
          if ((optV == 1) && (lineCount < lineLimit))
          {
            fileOut = fopen(OUTFILE, "a");
            fprintf(fileOut, "\n");
            fclose(fileOut);
            lineCount++;
          }
        }
        else
        {
          sem_post(&(fauxMQ[b]->mutex));
        }
      }
    }

    // Request Resources
   
    // Update free resouces
    int freeRes[20];
    for (i = 0; i < descriptorLimit; i++)
    {
      freeRes[i] = resQuantMax[i];
      for (g = 0; g < 10; g++)
      {
        if (descArray[i]->descAlloc[g] != 0)
        {
          freeRes[i]--;
        }
      }
    }

    // Now request resources
    for (b = 0; b < childMax; b++)
    {
      if (activeArray[b] != 0)
      {
        if (fauxMQ[b]->fauxRequestBait == 1)
        {
          int stopped = 0;
          for (i = 0; i < descriptorLimit; i++)
          {
            //Request is stopped if request is greater than free resources
            if (fauxMQ[b]->fRequest[i][1] > freeRes[i])
            {
              stopped = 1;
            }
          }
          //If resources available they are granted
          if (stopped == 0)
          {
            if ((optV == 1) && (lineCount < lineLimit))
            {
              fileOut = fopen(OUTFILE, "a");
              fprintf(fileOut, "OSS: Child %d Resources Allocated: ", fauxMQ[b]->fauxPid);
              fclose(fileOut);
              lineCount++;
            }

            sem_wait(&(fauxMQ[b]->mutex));
            // If request is equal to the pid continue
            for (i = 0; i < descriptorLimit; i++)
            {
              if (fauxMQ[b]->fRequest[i][0] == fauxMQ[b]->fauxPid)
              {
                int resTake = fauxMQ[b]->fRequest[i][1];
                int r;
                if ((optV == 1) && (lineCount < lineLimit))
                {
                  fileOut = fopen(OUTFILE, "a");
                  fprintf(fileOut, " %d", resTake);
                  fclose(fileOut);
                }
                
                sem_wait(&(descArray[i]->mutex));
                // Update descriptor resources with pid and # of resouces given out
                for (r = 0; r < resTake; r++)
                {
                  int allocSig = 0;
                  for (g = 0; g < 10; g++)
                  {
                    if (allocSig == 0)
                    {
                       if (descArray[i]->descAlloc[g] == 0)
                       {
                         descArray[i]->descAlloc[g] = fauxMQ[b]->fauxPid;
                         descArray[i]->descHeld++;
                         numReq++;
                         printReq++;
                         allocSig = 1;
                       }  
                    }
                  }
                }
                sem_post(&(descArray[i]->mutex));
                fauxMQ[b]->fHeld[i][0] = fauxMQ[b]->fauxPid;
                fauxMQ[b]->fHeld[i][1] = resTake;
                fauxMQ[b]->fRequest[i][1] = 0;
                fauxMQ[b]->fauxRequestBait = 0;
                fauxMQ[b]->fauxRequestSig = 1;
              }
            }

            sem_post(&(fauxMQ[b]->mutex));
            if ((optV == 1) && (lineCount < lineLimit))
            {
              fileOut = fopen(OUTFILE, "a");
              fprintf(fileOut, "\n");
              fclose(fileOut);
              lineCount++;
              // Every 20 granted requests print out resource table
              if (printReq >= 20)
              {
                printReq = 0;
                fileOut = fopen(OUTFILE, "a");
                fprintf(fileOut, "------------- Child %d Request Approved ---------------- \n", fauxMQ[b]->fauxPid);
                fprintf(fileOut, "------------- Time: %09f --- #Request: %d --- #Blocks: %d \n", (((double)sharedClock->nanosecs/1000000000) + ((double) sharedClock->secs)), numReq, numBlock);
                fprintf(fileOut, "Resource Matrix: ");
                for (i = 0; i < descriptorLimit; i++)
                {
                  fprintf(fileOut, "%d ", resQuantMax[i]);
                }
                fprintf(fileOut, "\n");
                fprintf(fileOut, "-------------------------------------------------------\n");
                fprintf(fileOut, "|  CHILD  | R00 R01 R02 R03 R04 R05 R06 R07 R08 R09 R10 R11 R12 R13 R14 R15 R16 R17 R18 R19 |\n");

                int x;
                int y;
                // Printing out resources each iteration
                for (x = 0; x < childMax; x++)
                {
                  if (activeArray[x] != 0)
                  {
                    if (x < 10) 
                    {
                      fprintf(fileOut, "Child[0%d] | ", x);
                    }
                    if (x >= 10)
                    {
                      fprintf(fileOut, "Child[%d] | ", x);
                    }
                    for (y = 0; y < descriptorLimit; y++)
                    {
                      sem_wait(&(fauxMQ[x]->mutex));
                      if (fauxMQ[x]->fHeld[y][1] < 10)
                      {
                        if (fauxMQ[x]->fHeld[y][1] == 0)
                        {
                          fprintf(fileOut, "  - ");
                        }
                        else 
                        {
                          fprintf(fileOut, "  %d ", fauxMQ[x]->fHeld[y][1]);
                        }
                      }
                      if (fauxMQ[x]->fHeld[y][1] >= 10)
                      {
                        fprintf(fileOut, " %d ", fauxMQ[x]->fHeld[y][1]);
                      }
                      sem_post(&(fauxMQ[x]->mutex));
                    }
                    fprintf(fileOut, "|\n");
                  }
                }
                fprintf(fileOut, "--------------------------------------------------------------------\n");
                fclose(fileOut);
                lineCount = lineCount + 8 + activeChildren;
              }
            }
          }
        }
      }
    }
    // Increment clock
    sem_wait(&(sharedClock->mutex));
    sharedClock->nanosecs = sharedClock->nanosecs + (arbitraryCost);
    sem_post(&(sharedClock->mutex));
    //printf("OSS: Reached clock inc.\n");
  }

  //Below should never execute

  for (i = 0; i < descriptorLimit; i++)
  {
    shmdt(descArray[i]);
    shmctl(shResID[i], IPC_RMID, NULL);
    shmdt(fauxMQ[i]);
    shmctl(shMQID[i], IPC_RMID, NULL);
  }

  shmdt(sharedClock); // Detach shared memory.
  shmctl(shClockID, IPC_RMID, NULL); // Destroy shared memory.
  kill(0, SIGKILL);
  return 0;
}

void raiseAlarm() // Kill everything once time limit hits which is 5 real life seconds
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
//Set secs in shared clock
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
// Detertmine random # for variety of operations
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
