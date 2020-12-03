// Paul Passiglia
// cs_4760
// Project 5
// 12/2/20
// Shared faux message queue

#ifndef FAUXMQ_H
#define FAUXMQ_H

#include <semaphore.h>

typedef struct
{
  int fauxRequestBait;
  int fauxReleaseBait;
  int fauxRequestSig;
  int fauxReleaseSig;
  int fauxPid;

  int fRequest[20][2];
  int fWait[20][2];
  int fHeld[20][2];
  int fRelease[20][2];
  sem_t mutex;
} FauxMQ;

#endif
