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
  int secs;           // Holds seconds
  int nanosecs;       // Holds nanoseconds
  int termCrit;       // For termination control
  int spawnCrit;      // For spawn control
  int displacement;   // find placement within memory/array 
  sem_t mutex;
} SharedClock;

#endif
