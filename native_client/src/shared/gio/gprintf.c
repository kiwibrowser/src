/*
 * Copyright (c) 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Generic I/O interface.
 */
#include "native_client/src/include/build_config.h"
#include "native_client/src/include/portability.h"

#include <stdlib.h>

#include "native_client/src/shared/gio/gio.h"

/*
 * Windows Visual Studio pre-2013 does not provide va_copy.  When
 * compiled with a pre-2013 Visual Studio, we use knowledge of MSVS's
 * implementation to poly-fill.  This is ugly, but the API for earlier
 * versions is extremely unlikely to change, since it would break
 * other existing code that depends on it.
 */
#if NACL_WINDOWS
/* check definition of _MSC_VER first, in case we switch to clang */
# if defined(_MSC_VER) && _MSC_VER < 1800
#  define va_copy(dst, src) do { (dst) = (src); } while (0)
# endif
#endif

size_t gvprintf(struct Gio *gp,
                char const *fmt,
                va_list    ap) {
  size_t    bufsz = 1024;
  char      *buf = malloc(bufsz);
  int       rv;
  va_list   ap_copy;

  if (!buf) return (size_t) -1;

  va_copy(ap_copy, ap);

  while ((rv = vsnprintf(buf, bufsz, fmt, ap_copy)) < 0 ||
         (unsigned) rv >= bufsz) {
    va_end(ap_copy);
    free(buf);
    buf = 0;

    /**
     * Since the buffer size wasn't big enough, we want to double it.
     * Stop doubling when we reach SIZE_MAX / 2, though, otherwise we
     * risk wraparound.
     *
     * On Windows, vsnprintf returns -1 if the supplied buffer is not
     * large enough; on Linux and OSX, it returns the number of actual
     * characters that would have been output (excluding the NUL
     * byte), which means a single resize would have been sufficient.
     * Since buffer size increase should be infrequent, we do doubly
     * on Linux and OSX as well.
     */
    if (bufsz < SIZE_MAX / 2) {
      bufsz *= 2;
      buf = malloc(bufsz);
    }

    if (!buf) {
      return (size_t) -1;
    }
    va_copy(ap_copy, ap);
  }
  va_end(ap_copy);
  if (rv >= 0) {
    rv = (int) (*gp->vtbl->Write)(gp, buf, rv);
  }
  free(buf);

  return rv;
}

size_t gprintf(struct Gio *gp,
               char const *fmt, ...) {
  va_list ap;
  size_t  rv;

  va_start(ap, fmt);
  rv = gvprintf(gp, fmt, ap);
  va_end(ap);

  return rv;
}
