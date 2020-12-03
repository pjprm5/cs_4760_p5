// Paul Passiglia
// cs_4760
// Project 5
// 12/2/2020
// Shared Clock Struct

#ifndef SHCLOCK_H
#define SHCLOCH_H

#include <semaphore.h>

typedef struct 
{
  unsigned int secs;           // Holds seconds
  unsigned int nanosecs;       // Holds nanoseconds
  int termCrit;       // For termination control
  int spawnCrit;      // For spawn control
  int displacement; 
  sem_t mutex;
} SharedClock;

#endif
