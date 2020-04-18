/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <string.h>

#include "native_client/src/include/nacl_macros.h"
#include "native_client/src/shared/gio/gio.h"
#include "native_client/src/trusted/service_runtime/nacl_error_gio.h"

/*
 * This is a service-runtime provided subclass of Gio that is used for
 * NaClLog's output Gio stream.  In addition to the normal logging and
 * error reporting function, it retains a copy of the last (N>=2) line
 * of logging output for use with fatal error reporting -- the abort
 * behavior in NaClLog is modified to use a functor that takes the
 * retained last output lines and sends them to the plugin prior to
 * copying it to an on-stack buffer and crashing.  The latter is for
 * inclusion in breakpad's minidump, which should capture the thread
 * stack.  The former is to allow the plugin to forward the log
 * message to the JavaScript console, so developers can get a better
 * idea of what happened -- they won't have direct access to the
 * minidumps (potential user privacy issue), so without this they'd
 * have to ask Google, which isn't exactly scalable.
 *
 * Since trying to use the reverse channel or SRPC when there's a
 * fatal error might not be kosher, we shouldn't rely on too much
 * infrastructure to be operational.  The system may be so messed up
 * that doing so will just lead to another LOG_FATAL error.  We could
 * prevent infinite regress by setting a global variable to prevent
 * recursive plugin logging, but rather than hacks like that, we
 * commandeer the bootstrap channel -- which is currently only used to
 * pass the socket address used to connect to the secure command
 * channel to the service runtime and then to connect to the PPAPI
 * proxy.  We use low-level, platform-specific I/O routines to send
 * the logging string via this channel.
 *
 * (The reason N>=2 above is because the log module will, after
 * printing the LOG_FATAL message, always append a LOG_ERROR message
 * "LOG_FATAL abort exit\n" as an easy-to-recognize output for drivers
 * of the software, and that will be the last entry in the output
 * stream.  So if N==1, that's all that we'd see.)
 */

struct GioVtbl const kNaClErrorGioVtbl;  /* fwd */

int NaClErrorGioCtor(struct NaClErrorGio *self,
                     struct Gio *pass_through) {
  memset(self->circular_buffer, 0, NACL_ARRAY_SIZE(self->circular_buffer));
  self->insert_ix = 0;
  self->num_bytes = 0;
  self->pass_through = pass_through;
  self->vtbl = &kNaClErrorGioVtbl;
  return 1;
}

void NaClErrorGioDtor(struct Gio *vself) {
  struct NaClErrorGio *self = (struct NaClErrorGio *) vself;

  /* excessive paranoia? */
  memset(self->circular_buffer, 0, NACL_ARRAY_SIZE(self->circular_buffer));
  self->insert_ix = 0;
  self->num_bytes = 0;
  self->pass_through = NULL;
  self->vtbl = NULL;
}

ssize_t NaClErrorGioRead(struct Gio *vself,
                         void *buf,
                         size_t count) {
  struct NaClErrorGio *self = (struct NaClErrorGio *) vself;
  return (*self->pass_through->vtbl->Read)(self->pass_through, buf, count);
}

ssize_t NaClErrorGioWrite(struct Gio *vself,
                          void const *buf,
                          size_t count) {
  struct NaClErrorGio *self = (struct NaClErrorGio *) vself;
  uint8_t *byte_buf = (uint8_t *) buf;
  ssize_t actual;
  size_t ix;

  actual = (*self->pass_through->vtbl->Write)(self->pass_through, buf, count);
  if (actual > 0) {
    for (ix = 0; ix < (size_t) actual; ++ix) {
      self->circular_buffer[self->insert_ix] = byte_buf[ix];
      self->insert_ix = (self->insert_ix + 1) %
          NACL_ARRAY_SIZE(self->circular_buffer);
    }
    if ((size_t) actual >
        NACL_ARRAY_SIZE(self->circular_buffer) - self->num_bytes) {
      self->num_bytes = NACL_ARRAY_SIZE(self->circular_buffer);
    } else {
      self->num_bytes += actual;
    }
  }
  return actual;
}

off_t NaClErrorGioSeek(struct Gio *vself,
                       off_t offset,
                       int whence) {
  struct NaClErrorGio *self = (struct NaClErrorGio *) vself;
  return (*self->pass_through->vtbl->Seek)(self->pass_through, offset, whence);
}

int NaClErrorGioFlush(struct Gio *vself) {
  struct NaClErrorGio *self = (struct NaClErrorGio *) vself;
  return (*self->pass_through->vtbl->Flush)(self->pass_through);
}

int NaClErrorGioClose(struct Gio *vself) {
  return (*vself->vtbl->Flush)(vself);
}

struct GioVtbl const kNaClErrorGioVtbl = {
  NaClErrorGioDtor,
  NaClErrorGioRead,
  NaClErrorGioWrite,
  NaClErrorGioSeek,
  NaClErrorGioFlush,
  NaClErrorGioClose,
};

size_t NaClErrorGioGetOutput(struct NaClErrorGio *self,
                             char *buffer,
                             size_t buffer_size) {
  size_t num_copy;
  size_t ix;
  size_t count;

  num_copy = self->num_bytes;
  /* 0 <= num_copy <= NACL_ARRAY_SIZE(self->circular_buffer) */
  if (0 == buffer_size) {
    return self->num_bytes;
  }
  /* buffer_size > 0 */
  if (num_copy > buffer_size) {
    num_copy = buffer_size;
  }
  /*
   * 0 <= num_copy <= min(NACL_ARRAY_SIZE(self->circular_buffer),
   *                                      buffer_size)
   */
  ix = (self->insert_ix +
        NACL_ARRAY_SIZE(self->circular_buffer) - self->num_bytes) %
      NACL_ARRAY_SIZE(self->circular_buffer);
  for (count = 0; count < num_copy; ++count) {
    buffer[count] = self->circular_buffer[ix];
    ix = (ix + 1) % NACL_ARRAY_SIZE(self->circular_buffer);
  }
  /*
   * count = num_copy
   *       <= min(NACL_ARRAY_SIZE(self->circular_buffer), buffer_size)
   */
  return self->num_bytes;
}

