/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

int main(void) {
 int i;

 int avail_fd = dup(2);
 close(avail_fd);

 for (i = 0; i < 10; i++) {
   pthread_mutex_t mutex;
   int error = pthread_mutex_init(&mutex, NULL);
   if (error != 0) {
     fprintf(stderr, "Unable to create mutex, error %d", error);
     return 1;
   }
   error = pthread_mutex_destroy(&mutex);
   if (error != 0) {
     fprintf(stderr, "Unable to destroy mutex, error %d", error);
     return 1;
   }
 }

 int next_fd = dup(2);
 if (next_fd != avail_fd) {
   fprintf(stderr, "Leaked %d descriptors!\n", next_fd - avail_fd);
   return 1;
 }

 close(next_fd);

 return 0;
}
