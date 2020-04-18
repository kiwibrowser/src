/*
 * Copyright 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include <stdio.h>
#include <pthread.h>

struct ConditionPair {
  pthread_mutex_t* mutex;
  pthread_cond_t* condition;
};

void* lockingThread(void* data) {
  ConditionPair* pair = (ConditionPair*) data;
  pthread_mutex_lock(pair->mutex);
  pthread_cond_broadcast(pair->condition);
  pthread_mutex_unlock(pair->mutex);
  pthread_exit(NULL);
  return 0;
}

int main(int argc, char** argv) {
  // http://code.google.com/p/nativeclient/issues/detail?id=1177
  // This test checks that pthread_cond_wait correctly interacts with the mutex
  // error checking state.
  pthread_mutex_t mutex;
  pthread_mutexattr_t mta;
  pthread_mutexattr_init(&mta);
  pthread_mutexattr_settype(&mta, PTHREAD_MUTEX_ERRORCHECK);
  pthread_mutex_init(&mutex, &mta);
  pthread_mutexattr_destroy(&mta);

  pthread_cond_t condition;
  pthread_cond_init(&condition, NULL);

  pthread_mutex_lock(&mutex);

  ConditionPair pair = { &mutex, &condition };
  pthread_t thread;
  pthread_create(&thread, NULL, lockingThread, (void*) &pair);

  pthread_cond_wait(&condition, &mutex); // releases mutex
  // Mutex should be held when pthread_cond_returns
  // And it turns out it is, but if we're using PTHREAD_MUTEX_ERRORCHECK
  // the pthread_mutex_unlock code gets confused and returns EPERM. :(
  int rc = pthread_mutex_unlock(&mutex);
  printf("pthread_mutex_unlock return: %i (should be 0)\n", rc);
  return rc;
}
