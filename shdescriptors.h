// Paul Passiglia
// cs_4760
// 12/2/2020
// Shared struct for resource descriptors

#ifndef SHDESCRIPTORS_H
#define SHDESCRIPTORS_H

typedef struct
{
  int descShare;
  int descNum;
  int descHeld;
  int descAlloc[10];
  sem_t mutex;
} SharedDescriptors;

#endif
