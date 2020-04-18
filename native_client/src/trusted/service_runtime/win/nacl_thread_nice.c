/*
 * Copyright 2009 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
/*
 * Windows thread priority support.
 */

#include <windows.h>
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/trusted/service_runtime/include/sys/nacl_nice.h"

void NaClThreadNiceInit() { }

int nacl_thread_nice(int nacl_nice) {
  BOOL rc;
  HANDLE mThreadHandle = GetCurrentThread();

  switch (nacl_nice) {
    case NICE_REALTIME:
      /* It appears as though you can lock up a machine if you use
       * THREAD_PRIORITY_TIME_CRITICAL or THREAD_PRIORITY_ABOVE_NORMAL.
       * So Windows does not get real-time threads for now.
       */
      rc = SetThreadPriority(mThreadHandle, THREAD_PRIORITY_NORMAL);
                             /* THREAD_PRIORITY_ABOVE_NORMAL); */
                             /* THREAD_PRIORITY_TIME_CRITICAL); */
      break;
    case NICE_NORMAL:
      rc = SetThreadPriority(mThreadHandle, THREAD_PRIORITY_NORMAL);
      break;
    case NICE_BACKGROUND:
      rc = SetThreadPriority(mThreadHandle, THREAD_PRIORITY_BELOW_NORMAL);
      break;
    default:
      NaClLog(LOG_WARNING, "nacl_thread_nice() failed (bad nice value).\n");
      return -1;
      break;
  }
  if (!rc) {
    NaClLog(LOG_WARNING, "nacl_thread_nice() failed.\n");
    return -1;
  }
  return 0;
}
