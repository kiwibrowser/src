/*
 * Copyright 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/trusted/service_runtime/sys_clock.h"

#include <time.h>

#include "native_client/src/include/build_config.h"
#include "native_client/src/shared/platform/nacl_log.h"


/*
 * TODO(bsy): REMOVE THIS AND PROVIDE GETRUSAGE.  this is normally not
 * a syscall; instead, it is a library routine on top of getrusage,
 * with appropriate clock tick translation.
 */
int32_t NaClSysClock(struct NaClAppThread *natp) {
  NaClLog(3,
          ("Entered NaClSysClock(%08"NACL_PRIxPTR")\n"),
          (uintptr_t) natp);

  /*
   * Windows CLOCKS_PER_SEC is 1000, but XSI requires it to be
   * 1000000L and that's the ABI that we are sticking with.
   *
   * NB: 1000 \cdot n \bmod 2^{32} when n is a 32-bit counter is fine
   * -- user code has to deal with \pmod{2^{32}} wraparound anyway,
   * and time differences will work out fine:
   *
   * \begin{align*}
   * (1000 \cdot \Delta n) \bmod 2^{32}
   *  &\equiv ((1000 \bmod 2^{32}) \cdot (\Delta n \bmod 2^{32}) \bmod 2^{32}\\
   *  &\equiv (1000 \cdot (\Delta n \bmod 2^{32})) \bmod 2^{32}.
   * \end{align*}
   *
   * so when $\Delta n$ is small, the time difference is going to be a
   * small multiple of $1000$, regardless of wraparound.
   */
  if (NACL_WINDOWS) {
    return 1000 * clock();
  } else {
    return clock();
  }
}
