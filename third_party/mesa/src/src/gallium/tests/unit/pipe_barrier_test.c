/**************************************************************************
 *
 * Copyright 2009-2010 VMware, Inc.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/


/*
 *  Test case for pipe_barrier.
 *
 *  The test succeeds if no thread exits before all the other threads reach
 *  the barrier.
 */


#include <stdio.h>

#include "os/os_thread.h"
#include "os/os_time.h"


#define NUM_THREADS 10

static pipe_thread threads[NUM_THREADS];
static pipe_barrier barrier;
static int thread_ids[NUM_THREADS];


static PIPE_THREAD_ROUTINE(thread_function, thread_data)
{
   int thread_id = *((int *) thread_data);

   printf("thread %d starting\n", thread_id);
   os_time_sleep(thread_id * 1000 * 1000);
   printf("thread %d before barrier\n", thread_id);
   pipe_barrier_wait(&barrier);
   printf("thread %d exiting\n", thread_id);

   return NULL;
}


int main()
{
   int i;

   printf("pipe_barrier_test starting\n");

   pipe_barrier_init(&barrier, NUM_THREADS);

   for (i = 0; i < NUM_THREADS; i++) {
      thread_ids[i] = i;
      threads[i] = pipe_thread_create(thread_function, (void *) &thread_ids[i]);
   }

   for (i = 0; i < NUM_THREADS; i++ ) {
      pipe_thread_wait(threads[i]);
   }

   pipe_barrier_destroy(&barrier);

   printf("pipe_barrier_test exiting\n");

   return 0;
}
