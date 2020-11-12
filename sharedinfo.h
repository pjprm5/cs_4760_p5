// Paul Passiglia
// cs_4760
// Project 5
// 11/11/2020
// Struct for shared memory


#ifndef SHAREDINFO_H
#define SHAREDINFO_H

typedef struct {
  unsigned int totalCPUtime;
  unsigned int timeInSystem;
  unsigned int timeLastBurst;
  int localSimPid;
  int procPriority;
  int blocked;             // If blocked.
  int timesRanFullQ;       // Number of times ran full quantum.
} ProcessControlBlock;

typedef struct {
  unsigned int secs;
  unsigned int nanosecs;
  ProcessControlBlock arrayPCB[18];
} SharedInfo;




#endif
