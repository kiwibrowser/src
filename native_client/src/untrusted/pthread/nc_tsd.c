/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>
#include <limits.h>
#include <stdlib.h>

#include "native_client/src/untrusted/pthread/pthread.h"
#include "native_client/src/untrusted/pthread/pthread_internal.h"
#include "native_client/src/untrusted/pthread/pthread_types.h"


#define PTHREAD_DESTRUCTOR_ITERATIONS 4

struct tsd_key {
  void (*dtor)(void *value);
  volatile unsigned int generation;
};

/* A key is free when its generation is even (starting with 0).  */
#define KEY_ALLOCATED(generation)       (((generation) & 1) == 1)

struct tsd {
  struct {
    void *value;
    unsigned int generation;
  } values[PTHREAD_KEYS_MAX];
};

static struct tsd_key tsd_keys[PTHREAD_KEYS_MAX];
static __thread struct tsd *tsd;


int pthread_key_create(pthread_key_t *key, void (*dtor)(void *)) {
  for (size_t i = 0; i < PTHREAD_KEYS_MAX; ++i) {
    const unsigned int generation = tsd_keys[i].generation;
    /*
     * A key is not reusable after UINT_MAX-1 generations--the last even
     * generation.
     */
    if (!KEY_ALLOCATED(generation) && generation != UINT_MAX - 1 &&
        generation == __sync_val_compare_and_swap(&tsd_keys[i].generation,
                                                  generation, generation + 1)) {
      *key = i;
      tsd_keys[i].dtor = dtor;
      return 0;
    }
  }

#if defined(NC_TSD_NO_MORE_KEYS)
  NC_TSD_NO_MORE_KEYS;
#endif
  return EAGAIN;
}

int pthread_key_delete(pthread_key_t key) {
  if ((size_t) key >= PTHREAD_KEYS_MAX)
    return EINVAL;

  const unsigned int generation = tsd_keys[key].generation;
  if (!KEY_ALLOCATED(generation))
    return EINVAL;

  if (generation != __sync_val_compare_and_swap(&tsd_keys[key].generation,
                                                generation, generation + 1)) {
    /*
     * Somebody incremented the generation counter before we did,
     * i.e. the key has already been deleted.
     */
    return EINVAL;
  }

  tsd_keys[key].dtor = NULL;
  return 0;
}

int pthread_setspecific(pthread_key_t key, const void *value) {
  if ((size_t) key >= PTHREAD_KEYS_MAX)
    return EINVAL;

  const unsigned int generation = tsd_keys[key].generation;
  if (!KEY_ALLOCATED(generation))
    return EINVAL;

  if (tsd == NULL) {
    /*
     * We need to allocate the array for this thread.
     * But don't bother if storing the default value of NULL.
     */
    if (value == NULL)
      return 0;

    tsd = calloc(1, sizeof *tsd);
    if (tsd == NULL)
      return ENOMEM;
  }

  tsd->values[key].generation = generation;
  tsd->values[key].value = (void *) value;
  return 0;
}

void *pthread_getspecific(pthread_key_t key) {
  if ((size_t) key >= PTHREAD_KEYS_MAX ||
      tsd == NULL ||
      tsd->values[key].generation != tsd_keys[key].generation)
    return NULL;

  return tsd->values[key].value;
}

void __nc_tsd_exit(void) {
  if (tsd != NULL) {
    for (int repeat = 0; repeat < PTHREAD_DESTRUCTOR_ITERATIONS; ++repeat) {
      int did_any = 0;

      for (size_t key = 0; key < PTHREAD_KEYS_MAX; ++key) {
        void (*dtor)(void *) = tsd_keys[key].dtor;
        unsigned int generation = tsd_keys[key].generation;
        if (dtor != NULL &&
            KEY_ALLOCATED(generation) &&
            tsd->values[key].generation == generation) {
          void *value = tsd->values[key].value;
          if (value != NULL) {
            tsd->values[key].value = NULL;
            (*dtor)(value);
            did_any = 1;
          }
        }
      }

      if (!did_any)
        break;
    }

    free(tsd);
    tsd = NULL;
  }
}
