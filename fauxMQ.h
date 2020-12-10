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
  int fauxRequestBait;     // sending out for request
  int fauxReleaseBait;     // sending out for release
  int fauxRequestSig;      // signal for request
  int fauxReleaseSig;      // signal for release
  int fauxPid;             // pid associated with forked child

  int fRequest[20][2];     // children requesting for res, indices true/false; quantity
  int fWait[20][2];        // childred waiting for res
  int fHeld[20][2];        // childred holding res
  int fRelease[20][2];     // childred releasing res
  sem_t mutex;
} FauxMQ;

#endif
