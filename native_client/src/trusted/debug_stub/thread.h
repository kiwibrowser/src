/*
 * Copyright 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


// This module defines the interface for interacting with platform specific
// threads.  This API provides a mechanism to query for a thread, by using
// the acquire method with the ID of a pre-existing thread.

#ifndef NATIVE_CLIENT_PORT_THREAD_H_
#define NATIVE_CLIENT_PORT_THREAD_H_ 1

#include "native_client/src/include/build_config.h"
#include "native_client/src/include/portability.h"
#include "native_client/src/trusted/service_runtime/nacl_app_thread.h"
#include "native_client/src/trusted/service_runtime/nacl_signal.h"

struct NaClAppThread;
struct NaClSignalContext;

namespace port {

class Thread {
 public:
  Thread(uint32_t id, struct NaClAppThread *natp);
  ~Thread();

  uint32_t GetId();

  bool SetStep(bool on);

  bool GetRegisters(uint8_t *dst);
  bool SetRegisters(uint8_t *src);

#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_arm
  void MaskSPRegister();
#endif

  void CopyRegistersFromAppThread();
  void CopyRegistersToAppThread();

  void SuspendThread();
  void ResumeThread();
  bool HasThreadFaulted();
  void UnqueueFaultedThread();

  struct NaClSignalContext *GetContext();
  struct NaClAppThread *GetAppThread();

  int GetFaultSignal();
  void SetFaultSignal(int signal);

  static int ExceptionToSignal(int exception_code);

 private:
  uint32_t id_;
  struct NaClAppThread *natp_;
  struct NaClSignalContext context_;
  int fault_signal_;
};

}  // namespace port

#endif  // PORT_THREAD_H_
