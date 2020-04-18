/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Service Runtime.  Secure RNG abstraction.
 */

#include "native_client/src/include/build_config.h"
#include "native_client/src/shared/platform/nacl_global_secure_random.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/shared/platform/nacl_secure_random.h"
#include "native_client/src/shared/platform/nacl_sync.h"
#include "native_client/src/shared/platform/nacl_sync_checked.h"

static struct NaClMutex          nacl_global_rng_mu;
static struct NaClSecureRng      nacl_global_rng;

static struct NaClSecureRng      *nacl_grngp = &nacl_global_rng;

void NaClGlobalSecureRngInit(void) {
  NaClXMutexCtor(&nacl_global_rng_mu);
  if (!NaClSecureRngCtor(nacl_grngp)) {
    NaClLog(LOG_FATAL,
            "Could not construct global random number generator.\n");
  }
}

void NaClGlobalSecureRngFini(void) {
  (*nacl_grngp->base.vtbl->Dtor)(&nacl_grngp->base);
  NaClMutexDtor(&nacl_global_rng_mu);
}

void NaClGlobalSecureRngSwitchRngForTesting(struct NaClSecureRng  *rng) {
  NaClXMutexLock(&nacl_global_rng_mu);
  nacl_grngp = rng;
  NaClXMutexUnlock(&nacl_global_rng_mu);
}

int32_t NaClGlobalSecureRngUniform(int32_t range_max) {
  int32_t  rv;

  NaClXMutexLock(&nacl_global_rng_mu);
  rv = (*nacl_grngp->base.vtbl->Uniform)(&nacl_grngp->base, range_max);
  NaClXMutexUnlock(&nacl_global_rng_mu);
  return rv;
}

uint32_t NaClGlobalSecureRngUint32(void) {
  uint32_t rv;
  NaClXMutexLock(&nacl_global_rng_mu);
  rv = (*nacl_grngp->base.vtbl->GenUint32)(&nacl_grngp->base);
  NaClXMutexUnlock(&nacl_global_rng_mu);
  return rv;
}

void NaClGlobalSecureRngGenerateBytes(uint8_t *buf, size_t buf_size) {
  NaClXMutexLock(&nacl_global_rng_mu);
  (*nacl_grngp->base.vtbl->GenBytes)(&nacl_grngp->base, buf, buf_size);
  NaClXMutexUnlock(&nacl_global_rng_mu);
}

void NaClGenerateRandomPath(char *path, int length) {
  /*
   * This function is used for generating random paths and names,
   * e.g. for IMC sockets and semaphores.
   *
   * IMC sockets note: the IMC header file omits some important
   * details.  The alphabet cannot include backslash for Windows.  For
   * Linux, it uses the abstract namespace (see unix(7)), and can
   * contain arbitrary characters.  The limitations for OSX are the
   * same as for pathname components, since the IMC library uses
   * unix-domain sockets there.
   *
   * We uniformly generate from an alphabet of digits and uppercase
   * and lowercase alphabetic characters on case-sensitive platforms
   * (non-Windows), and uniformly generate from an alphabet of digits
   * and lowercase characters on case-insensitive platforms (Windows).
   * Since IMC allows 27 characters (plus a '\0' terminator), we can
   * compute the (apparent) entropy which gives the effort required by
   * an attacker who cannot enumerate the namespace but must probe
   * blindly.  ("Apparent", since the actual amount of entropy
   * available is the entropy in the cryptographically secure
   * pseudorandom number generator's seed.)
   *
   * For case sensitive names (linux, osx), we have an alphabet size
   * of 62 characters, so assuming that the underlying generator is
   * ideal, we have a total entropy of the name is
   *
   *  H_{27} = 27 * H_1 = 27 * -\log_2(1/62)
   *         \approx 160.76 bits
   *
   * For case insensitive names (windows), we have an alphabet size of
   * 36 characters, and we have
   *
   *  H_{27} = 27 * H_1 = 27 * -log_2(1/36)
   *         \approx 137.59 bits
   *
   * If we used the 62-character alphabet in a case insensitive
   * environment then the case folding means that an attacker will
   * encounter an entropy of
   *
   *  H_{27} = 27 * H_1 = 27 * (10/62 * -log_2(1/62) + 26*2/62 * -log_2(2/62))
   *         \approx 138.12 bits
   *
   * which reduces security by 1.47 bits.  So, using an equiprobable
   * alphabet rather than case folding is slightly better in a case
   * insensitive environment, and using a larger alphabet is much
   * better than always case folding in a case sensitive environment
   * (23.48 bits).
   *
   * TODO(bsy): determine if the alphabets can be enlarged, and by how
   * much.
   */
  static char const alphabet[] =
      "abcdefghijklmnopqrstuvwxyz"
#if !NACL_WINDOWS
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
#endif  /* !NACL_WINDOWS */
      "0123456789";
  int const         alphabet_size = sizeof alphabet - 1;  /* NUL termination */

  int               i;
  int               r;

  for (i = 0; i < length-1; ++i) {
    r = NaClGlobalSecureRngUniform(alphabet_size);
    path[i] = alphabet[r];
  }
  path[length-1] = '\0';
}
