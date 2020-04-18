/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_NACL_ERROR_GIO_H_
#define NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_NACL_ERROR_GIO_H_

#include "native_client/src/include/nacl_compiler_annotations.h"
#include "native_client/src/include/portability.h"
#include "native_client/src/shared/gio/gio.h"

#define NACL_ERROR_GIO_MAX_BYTES  (512)  /* should be power of 2 */

EXTERN_C_BEGIN

struct NaClErrorGio {
  struct GioVtbl const *vtbl;
  char circular_buffer[NACL_ERROR_GIO_MAX_BYTES];
  size_t insert_ix;
  size_t num_bytes;
  struct Gio *pass_through;
};

/*
 * It is the caller's responsibility to ensure that the lifetime of
 * the |pass_through| Gio object is at least that of the |self|
 * NaClErrorGio object being constructed.
 *
 * NB: Close and Dtor methods on NaClErrorGio do not invoke the
 * corresponding methods on the pass_through object.  It is the
 * responsibility of the caller to invoke those operations as
 * necessary.
 */
int NaClErrorGioCtor(struct NaClErrorGio *self,
                     struct Gio *pass_through) NACL_WUR;


/*
 * NaClErrorGioGetOutput: Copy out at most the last
 * NACL_ERROR_GIO_MAX_BYTES bytes of output data from internal buffer
 * into caller-supplied |buffer|.  If available bytes exceeds
 * |buffer_size| then only |buffer_size| bytes are copied.  No NUL
 * termination is performed.
 *
 * RETURNS: If there was enough room, the number of bytes copied is
 * returned.  If there wasn't, then the number of bytes that would
 * have been copied is returned.  (If return value is greather than
 * the supplied buffer_size, then there wasn't enough space to hold
 * all captured output.)
 *
 * NB: |buffer| content will not be NUL terminated -- if the Gio
 * object was used to output binary data, then the ASCII NUL character
 * may in fact be part of the payload data.
 *
 * This is a non-virtual member function of NaClErrorGio object.
 */
size_t NaClErrorGioGetOutput(struct NaClErrorGio *self,
                             char *buffer,
                             size_t buffer_size) NACL_WUR;

EXTERN_C_END

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_NACL_ERROR_GIO_H_ */
