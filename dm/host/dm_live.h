// Settings changed while the game runs (hle_live.c)
#pragma once
#include <stdatomic.h>
extern _Atomic unsigned dm_live_gen;                   // moves at every change
void dm_live_set(const char *k, const char *v);        // the front end: a new value for setting k
const char *dm_live_getenv(const char *k);             // the value changed this run, else the environment's (copied for the calling thread)
