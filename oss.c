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
#include <sys/msg.h>
#include <signal.h>
#include "sharedinfo.h"
#include <math.h>

//Prototypes
void raiseAlarm();
unsigned int randomTime();


// Global variables.
SharedInfo *sharedInfo;
int infoID;
int msqID;

// Message queue struct
struct MessageQueue {
  long mtype;
  char messBuff[10];
};

#define maxTimeBetweenNewProcsNS 500000000
#define maxTimeBetweenNewProcsSecs 1
#define baseQuantum 10000000


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

  // Allocate message queue -------------------------------------------
  struct MessageQueue messageQ;
  int msqID;
  key_t msqKey = ftok("user_proc.c", 666);
  msqID = msgget(msqKey, 0644 | IPC_CREAT);
  if (msqKey == -1)
  {
    perror("OSS: Error: ftok failure msqKey");
    exit(-1);
  }

  /*messageQ.mtype = 1; // Declare initial mtype as 1 for ./oss

  strcpy(messageQ.messBuff, "0"); // Put msg in buffer
  
  if(msgsnd(msqID, &messageQ, sizeof(messageQ.messBuff), 0) == -1)
  {
    perror("OSS: Error: msgsnd ");
    exit(-1);
  }*/
  
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
  

  sharedInfo->secs = sharedInfo->secs + (randomTimeToSchedule/1000000000);
  if (randomTimeToSchedule >= 1000000000)
  {
    sharedInfo->nanosecs = randomTimeToSchedule % 1000000000;
  }
  else
  {
    sharedInfo->nanosecs = randomTimeToSchedule;
  }

  printf("Time 1st child launched --> %u:%u \n", sharedInfo->secs, sharedInfo->nanosecs);

  // Launch first child -----------------------------------------------
  char childBuff[20];
  int getChildPid;
  pid_t childPid; 
  
  
  proc_count++;
  //int index_count;
  //index_count++;
  childPid = fork();
  
  getChildPid = childPid;
  snprintf(childBuff, 20, "%d", getChildPid);
  strcpy(messageQ.messBuff, childBuff);
  messageQ.mtype = 1;
  msgsnd(msqID, &messageQ, sizeof(messageQ.messBuff), 0);
  if (childPid == 0)
  {
    char *args[] = {"./user_proc", NULL};
    execvp(args[0], args);
    //execl("user_proc", "user_proc", index_count-1, NULL);
  }
  else if (childPid < 0)
  {
    perror("Error: Fork() failed");
  }
  else if (childPid > 0) 
  {
    FILE* fptr = (fopen("output", "a"));
    if (fptr == NULL)
    {
      perror("OSS: Error: ftpr error");
    }
    fprintf(fptr, "OSS(%d): Creating child: %d at time: %u:%u \n", getpid(), childPid, sharedInfo->secs, sharedInfo->nanosecs);
    printf("OSS(%d): Creating child: %d at time: %u:%u \n", getpid(), childPid, sharedInfo->secs, sharedInfo->nanosecs);
    fclose(fptr);
  }
  //sharedInfo->arrayPCB[index_count-1].localSimPid = index_count-1;
  //bitVector[index_count-1] = 1;  



  // Parent/Main loop
  while (1)
  {
    
    // Blocking receive of only type 2 msg
    msgrcv(msqID, &messageQ, sizeof(messageQ.messBuff), 2, 0);

    // Fix clock

   /* if (sharedInfo->nanosecs >= 1000000000)
    {
      sharedInfo->secs = sharedInfo->secs + sharedInfo->nanosecs/1000000000; // Add seconds to nanosecs after nanosecs gets integer division.
      sharedInfo->nanosecs = sharedInfo->nanosecs % 1000000000; // Modulo operation to get nanosecs.
    }*/
    // If terminated.
    if (strcmp(messageQ.messBuff, "1") == 0)
    {
      // Process terminated, wait for process, then log what was terminated and launch a new process.
      wait(NULL);
      printf("OSS:Process Terminated: Process time = %u \n", sharedInfo->arrayPCB[0].totalCPUtime);
      
      FILE* fptr = (fopen("output", "a"));
      if (fptr == NULL)
      {
        perror("OSS: Error: fptr error. \n");
      }

      fprintf(fptr, "OSS(%d): Receiving that child process with pid: %d was terminated at time: %u:%u \n", getpid(), sharedInfo->arrayPCB[0].localSimPid, sharedInfo->secs, sharedInfo->nanosecs);
      printf("OSS(%d): Receiving that child process with pid: %d was terminated at time: %u:%u.\n", getpid(), sharedInfo->arrayPCB[0].localSimPid, sharedInfo->secs, sharedInfo->nanosecs);
      fclose(fptr);
      //msgctl(msqID, IPC_RMID, NULL);
      
      //messageQ.mtype = 1;
      //msgsnd(msqID, &messageQ, sizeof(messageQ.messBuff), 0);
      
      if(proc_count == 100)
      { 
        break;
      }
      proc_count++;
      //index_count++;
      //bitVector[index_count-1] = 1;
      printf("OSS: Process Count: %d --------------------> \n", proc_count);
      childPid = fork();
      
      if (childPid == 0)
      {
        char *args[] = {"./user_proc", NULL};
        execvp(args[0], args);
        //execl("user_proc", "user_proc", index_count-1, NULL);
      }

      else if (childPid < 0)
      {
        perror("Error: Fork() failed");
      }
      else if (childPid > 0) 
      {
        FILE* fptr = (fopen("output", "a"));
        if (fptr == NULL)
        {
          perror("OSS: Error: ftpr error");
        }
        fprintf(fptr, "OSS(%d): Creating child: %d at time: %u:%u \n", getpid(), childPid, sharedInfo->secs, sharedInfo->nanosecs);
        printf("OSS(%d): Creating child: %d at time: %u:%u \n", getpid(), childPid, sharedInfo->secs, sharedInfo->nanosecs);
        fclose(fptr);
      }
      //sharedInfo->arrayPCB[index_count-1].localSimPid = index_count-1;
      //bitVector[index_count-1] = 1;
      
      //getChildPid = childPid;
      //snprintf(childBuff, 20, "%d", getChildPid);
      //strcpy(messageQ.messBuff, childBuff);
      messageQ.mtype = 1;
      msgsnd(msqID, &messageQ, sizeof(messageQ.messBuff), 0);
      
    }

    // If ran entire quantum.
    else if (strcmp(messageQ.messBuff, "2") == 0)
    {
      
      if (sharedInfo->nanosecs >= pow(2,32) - 10000000 + 1)
      {
        sharedInfo->secs = sharedInfo->secs + sharedInfo->nanosecs/1000000000; // Add seconds to nanosecs after nanosecs gets integer division.
        sharedInfo->nanosecs = sharedInfo->nanosecs % 1000000000; // Modulo operation to get nanosecs.
      }
      printf("OSS: Process Ran Entire Quantum. \n");
      sharedInfo->nanosecs = sharedInfo->nanosecs + 10000000; // Update nanosecs.
      sharedInfo->arrayPCB[0].totalCPUtime = sharedInfo->arrayPCB[0].totalCPUtime + 10000000; // Update total CPU time.
      sharedInfo->arrayPCB[0].timeInSystem = 10000000;
      sharedInfo->arrayPCB[0].timeLastBurst = 10000000;

      FILE* fptr = (fopen("output", "a"));
      if (fptr == NULL)
      {
        perror("OSS: Error: fptr error. \n");
      }

      fprintf(fptr, "OSS(%d): Receiving that child process with pid: %d was RAN ENTIRE QUANTUM at time: %u:%u \n", getpid(), sharedInfo->arrayPCB[0].localSimPid, sharedInfo->secs, sharedInfo->nanosecs);
      printf("OSS(%d): Receiving that child process with pid: %d was RAN ENTIRE QUANTUM at time: %u:%u.\n", getpid(), sharedInfo->arrayPCB[0].localSimPid, sharedInfo->secs, sharedInfo->nanosecs);
      fclose(fptr);
      
      //getChildPid = sharedInfo->arrayPCB[0].localSimPid; 
      //snprintf(childBuff, 20, "%d", getChildPid);
      //strcpy(messageQ.messBuff, childBuff);
     
      //printf("messageQ: %s childBuff: %s getChildPid: %d \n", messageQ.messBuff, childBuff, getChildPid);
      messageQ.mtype = 1;
      if (msgsnd(msqID, &messageQ, sizeof(messageQ.messBuff), 0) == -1)
      {
        perror("OSS: Error: msgsnd Entire Quantum \n");
      }
    }
     
    
    // If interrupted.
    else if (strcmp(messageQ.messBuff, "3") == 0)
    {
      // Process was interrupted, put into a blocked queue and schedule another process.

      FILE* fptr = (fopen("output", "a"));
      if (fptr == NULL)
      {
        perror("OSS: Error: fptr error. \n");
      }

      fprintf(fptr, "OSS(%d): Receiving that child process with pid: %d was interrupted at time: %u:%u \n", getpid(), sharedInfo->arrayPCB[0].localSimPid, sharedInfo->secs, sharedInfo->nanosecs);
      printf("OSS(%d): Receiving that child process with pid: %d was interrupted at time: %u:%u.\n", getpid(), sharedInfo->arrayPCB[0].localSimPid, sharedInfo->secs, sharedInfo->nanosecs);
      fclose(fptr);
      
      //getChildPid = sharedInfo->arrayPCB[0].localSimPid;
      //snprintf(childBuff, 20, "%d", getChildPid);
      //strcpy(messageQ.messBuff, childBuff);
      
      //printf("messageQ: %s childBuff: %s getChildPid: %d \n", messageQ.messBuff, childBuff, getChildPid);
      messageQ.mtype = 1;
      if (msgsnd(msqID, &messageQ, sizeof(messageQ.messBuff), 0) == -1)
      {
        perror("OSS: Error: msgsnd If interrupted \n");
      }
    }
    //messageQ.mtype = 1;
    //msgsnd(msqID, &messageQ, sizeof(messageQ.messBuff), 0);        
  }

  shmdt(sharedInfo); // Detach shared memory.
  shmctl(infoID, IPC_RMID, NULL); // Destroy shared memory.
  msgctl(msqID, IPC_RMID, NULL); // Destroy message queue.
  return 0;
}

void raiseAlarm() // Kill everything once time limit hits which is 3 real life seconds
{
  printf("\n Time limit hit, terminating all processes. \n");
  shmdt(sharedInfo);
  shmctl(infoID, IPC_RMID, NULL);
  msgctl(msqID, IPC_RMID, NULL);
  kill(0, SIGKILL);
}

unsigned int randomTime() // Random time for scheduler between 1,500,000,000 nanosecs and 1,000,000 nanosecs
{
  unsigned int randomNum = ((rand() % (1500000000 - 1000000 + 1)) +1);
  return randomNum;
}
