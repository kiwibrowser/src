/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_NACL_DEBUG_INIT_H_
#define NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_NACL_DEBUG_INIT_H_ 1

#include "native_client/src/include/build_config.h"
#include "native_client/src/include/portability_sockets.h"
#include "native_client/src/shared/imc/nacl_imc_c.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"

EXTERN_C_BEGIN

/*
 * NaClDebugBindSocket() may be called to tell the debug stub to bind
 * its default TCP port to listen on.  This may need to be called
 * before enabling an outer sandbox; otherwise, NaClDebugInit() calls
 * it.  The function returns whether it was successful.
 */
int NaClDebugBindSocket(void);

/*
 * NaClDebugSetBoundSocket() takes a socket descriptor that has
 * already had bind() and listen() called on it.  This tells the debug
 * stub to use the given descriptor rather than trying to bind() the
 * default TCP port.
 */
void NaClDebugSetBoundSocket(NaClSocketHandle bound_socket);

/*
 * NaClDebugStubSetPipe() takes a named pipe on windows or a socket
 * pair descriptor on posix. This sets up the debug stub to talk over the
 * pipe with chrome.
 * TODO(leslieb): Implement for windows.
 */
void NaClDebugStubSetPipe(NaClHandle handle);

/*
 * Enables the debug stub.  If this is called, we do not guarantee
 * security to the same extent that we normally would.
 */
int NaClDebugInit(struct NaClApp *nap);

/*
 * Returns the port bound in NaClDebugBindSocket() or NaClDebugSetBoundSocket().
 * TODO(leslieb): remove when windows no longer needs the port information.
 */
#if NACL_WINDOWS
uint16_t NaClDebugGetBoundPort();
#endif

EXTERN_C_END

#endif
