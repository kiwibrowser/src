/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Linux thread priority support.
 */
#include <errno.h>
#include <sched.h>
#include <stdio.h>
#include <string.h>
#include <sys/resource.h>

#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/trusted/service_runtime/include/sys/nacl_nice.h"

void NaClThreadNiceInit(void) {
}

/* Linux version - threads are processes, so use setpriority in lieu
 * of RLIMIT_RTPRIO.
 * This appears to be lockup-free. To enable the privileged realtime
 * threads, /etc/security/limits.conf typically needs to include a
 * line like this:
 *     @audio    -      nice      -10
 * Ubuntu systems (and others?) automatically grant audio group permission
 * to user who login on the console.
 */
/* Linux realtime scheduling is pretty broken at this time. See
 * http://lwn.net/Articles/339316/ for a useful overview.
 */
int nacl_thread_nice(int nacl_nice) {
  const int kRealTimePriority = -10;
  const int kBackgroundPriority = 10;
  const int kNormalPriority = 0;

  switch (nacl_nice) {
    case NICE_REALTIME:
      if (0 == setpriority(PRIO_PROCESS, 0, kRealTimePriority)) {
        return 0;  /* success */
      }
      /* Sorry; no RT priviledges. Fall through to NICE_NORMAL */
    case NICE_NORMAL:
      if (0 == setpriority(PRIO_PROCESS, 0, kNormalPriority)) {
        return 0;  /* success */
      }
      break;
    case NICE_BACKGROUND:
      if (0 == setpriority(PRIO_PROCESS, 0, kBackgroundPriority)) {
        return 0;  /* success */
      }
      break;
    default:
      NaClLog(LOG_WARNING, "nacl_thread_nice failed (bad nice value)\n");
      return -1;
      break;
  }
  return -1;
}
