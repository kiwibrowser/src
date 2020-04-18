/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


#include <vector>
#include <map>

/*
 * NaCl Functions for intereacting with debuggers
 */

#include "native_client/src/include/build_config.h"
#include "native_client/src/include/nacl_string.h"
#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/shared/platform/nacl_exit.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/shared/platform/nacl_threads.h"
#include "native_client/src/trusted/debug_stub/debug_stub.h"
#include "native_client/src/trusted/debug_stub/platform.h"
#include "native_client/src/trusted/debug_stub/session.h"
#include "native_client/src/trusted/debug_stub/target.h"
#include "native_client/src/trusted/debug_stub/thread.h"
#include "native_client/src/trusted/service_runtime/nacl_app_thread.h"
#include "native_client/src/trusted/service_runtime/nacl_debug_init.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"
#include "native_client/src/trusted/service_runtime/thread_suspension.h"

using port::IPlatform;
using port::Thread;
using port::ITransport;
using port::SocketBinding;

using gdb_rsp::Session;
using gdb_rsp::Target;

#if NACL_WINDOWS
/* Disable warning for unwind disabled when exceptions used */
#pragma warning(disable:4530)
#endif


static Target *g_target = NULL;
static SocketBinding *g_socket_binding = NULL;
static ITransport *g_transport = NULL;

int NaClDebugBindSocket() {
  if (g_transport == NULL) {
    NaClDebugStubInit();
    // Try port 4014 first for compatibility.
    g_socket_binding = SocketBinding::Bind("127.0.0.1:4014");
    // If port 4014 is not available, try any port.
    if (g_socket_binding == NULL) {
      g_socket_binding = SocketBinding::Bind("127.0.0.1:0");
    }
    if (g_socket_binding == NULL) {
      NaClLog(LOG_ERROR,
              "NaClDebugStubBindSocket: Failed to bind any TCP port\n");
      return 0;
    }
    g_transport = g_socket_binding->CreateTransport();
    NaClLog(LOG_WARNING,
            "nacl_debug(%d) : Connect GDB with 'target remote :%d\n",
            __LINE__, g_socket_binding->GetBoundPort());
  }
  return 1;
}

void NaClDebugSetBoundSocket(NaClSocketHandle bound_socket) {
  CHECK(g_socket_binding == NULL);
  g_socket_binding = new SocketBinding(bound_socket);
  g_transport = g_socket_binding->CreateTransport();
}

void NaClDebugStubSetPipe(NaClHandle handle) {
  CHECK(g_transport == NULL);
  g_transport = port::CreateTransportIPC(handle);
}

void WINAPI NaClStubThread(void *thread_arg) {
  UNREFERENCED_PARAMETER(thread_arg);
  while (1) {
    // Wait for a connection.
    if (!g_transport->AcceptConnection()) continue;

    // Create a new session for this connection
    Session ses(g_transport);
    ses.SetFlags(Session::DEBUG_MASK);

    NaClLog(LOG_WARNING, "nacl_debug(%d) : Connected, happy debugging!\n",
            __LINE__);

    // Run this session for as long as it lasts
    g_target->Run(&ses);
  }
}

static void ThreadCreateHook(struct NaClAppThread *natp) throw() {
  g_target->TrackThread(natp);
}

static void ThreadExitHook(struct NaClAppThread *natp) throw() {
  g_target->IgnoreThread(natp);
}

static void ProcessExitHook() throw() {
  g_target->Exit();
  NaClDebugStubFini();
}

static const struct NaClDebugCallbacks debug_callbacks = {
  ThreadCreateHook,
  ThreadExitHook,
  ProcessExitHook,
};

/*
 * This function is implemented for the service runtime.  The service runtime
 * declares the function so it does not need to be declared in our header.
 */
int NaClDebugInit(struct NaClApp *nap) {
  if (!NaClFaultedThreadQueueEnable(nap)) {
    NaClLog(LOG_ERROR, "NaClDebugInit: Failed to initialize fault handling\n");
    return 0;
  }
  nap->debug_stub_callbacks = &debug_callbacks;

  CHECK(g_target == NULL);
  g_target = new Target(nap);
  CHECK(g_target != NULL);
  g_target->Init();

  if (!NaClDebugBindSocket()) {
    return 0;
  }

  NaClThread *thread = new NaClThread;
  CHECK(thread != NULL);

  NaClLog(LOG_WARNING, "nacl_debug(%d) : Debugging started.\n", __LINE__);
  CHECK(NaClThreadCtor(thread, NaClStubThread, NULL, NACL_KERN_STACK_SIZE));

  return 1;
}

#if NACL_WINDOWS
// TODO(leslieb): Remove when windows doesn't need the port.
uint16_t NaClDebugGetBoundPort() {
  CHECK(g_socket_binding != NULL);
  return g_socket_binding->GetBoundPort();
}
#endif
