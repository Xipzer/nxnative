// Settings changed while the game runs (the iOS in-game settings sheet): the front end sets a value here instead of in the environment, which another thread may be
// reading (setenv while getenv runs is not safe), and bumps dm_live_gen. Each reader of such a setting caches its value with the generation it read at and reads again,
// through dm_live_getenv, when the generation has moved: anisotropy and trilinear filtering (the samplers are made again), sharpening, the present upscaler and its detail,
// adaptive quality, the thermal cap and the frame workgroup. The table holds only the settings changed this run
#include "dm_live.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

_Atomic unsigned dm_live_gen;
enum { LIVE_N = 32 };
static struct { char k[40], v[48]; } g_live[LIVE_N];
static int g_nlive;
static pthread_mutex_t g_live_mu = PTHREAD_MUTEX_INITIALIZER;

void dm_live_set(const char *k, const char *v) {
    if (!k || !v) return;
    pthread_mutex_lock(&g_live_mu);
    int i = 0; while (i < g_nlive && strcmp(g_live[i].k, k)) i++;
    if (i == g_nlive && g_nlive < LIVE_N) { g_nlive++; snprintf(g_live[i].k, sizeof g_live[i].k, "%s", k); }
    if (i < g_nlive) snprintf(g_live[i].v, sizeof g_live[i].v, "%s", v);
    pthread_mutex_unlock(&g_live_mu);
    atomic_fetch_add(&dm_live_gen, 1);
    fprintf(stderr, "[dm] live setting %s=%s\n", k, v);
}

const char *dm_live_getenv(const char *k) {
    static __thread char buf[48];
    pthread_mutex_lock(&g_live_mu);
    for (int i = 0; i < g_nlive; i++) if (!strcmp(g_live[i].k, k)) { memcpy(buf, g_live[i].v, sizeof buf); pthread_mutex_unlock(&g_live_mu); return buf; }
    pthread_mutex_unlock(&g_live_mu);
    return getenv(k);
}
