// Paul Passiglia
// cs_4760
// 12/2/2020
// Shared struct for resource descriptors

#ifndef SHDESCRIPTORS_H
#define SHDESCRIPTORS_H

typedef struct
{
  int descShare;     // Is it shared
  int descNum;       // Number of resources
  int descHeld;      // Is it held
  int descAlloc[10]; // Where each resource is allocated to
  sem_t mutex;
} SharedDescriptors;

#endif
