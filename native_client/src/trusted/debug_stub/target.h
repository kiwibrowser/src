/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


// This module provides interfaces for accessing the debugging state of
// the target.  The target can use either the thread that took the
// exception or run in it's own thread.  To respond to the host, the
// application must call the run function with a valid Transport
// which will then be polled for commands.  The target will return
// from Run when the host disconnects, or requests a continue.
//
// The object is protected by a mutex, so that it is legal to track or
// ignore threads as an exception takes place.
//
// The Run function expects that all threads of interest are stopped
// with the Step flag cleared before Run is called.  It is expected that
// and that all threads are updated with thier modified contexts and
// restarted when the target returns from Run.

#ifndef SRC_TRUSTED_GDB_RSP_TARGET_H_
#define SRC_TRUSTED_GDB_RSP_TARGET_H_ 1

#include <map>
#include <string>

#include "native_client/src/trusted/debug_stub/mutex.h"
#include "native_client/src/trusted/debug_stub/platform.h"
#include "native_client/src/trusted/debug_stub/thread.h"
#include "native_client/src/trusted/debug_stub/util.h"

struct NaClApp;

namespace gdb_rsp {

class Abi;
class Packet;
class Session;

class Target {
 public:
  enum ErrDef {
    NONE = 0,
    BAD_FORMAT = 1,
    BAD_ARGS = 2,
    FAILED = 3
  };

  typedef std::map<uint32_t, port::Thread*> ThreadMap_t;
  typedef std::map<std::string, std::string> PropertyMap_t;
  typedef std::map<uint32_t, uint8_t*> BreakpointMap_t;

 public:
  // Contruct a Target object.  By default use the native ABI.
  explicit Target(struct NaClApp *nap, const Abi *abi = NULL);
  ~Target();

  // Init must be the first function called to correctlty
  // build the Target internal structures.
  bool Init();

  // This function will spin on a session, until it closes.  If an
  // exception is caught, it will signal the exception thread by
  // setting sig_done_.
  void Run(Session *ses);

  // This function causes the target to track the state
  // of the specified thread and make it availible to
  // a connected host.
  void TrackThread(struct NaClAppThread *natp);

  // This function causes the target to stop tracking the
  // state of the specified thread, which will no longer
  // be visible to the host.
  void IgnoreThread(struct NaClAppThread *natp);

  // Send exit packet to gdb.
  void Exit();

 protected:
  uint64_t AdjustUserAddr(uint64_t addr);

  // This function always succeedes, since all errors
  // are reported as an error string of "E<##>" where
  // the two digit number.  The error codes are not
  // not documented, so this implementation uses
  // ErrDef as errors codes.  This function returns
  // true a request to continue (or step) is processed.
  bool ProcessPacket(Packet *pktIn, Packet *pktOut);

  void EmitFileError(Packet *pktOut, int code);
  void ProcessFilePacket(Packet *pktIn, Packet *pktOut, ErrDef *err);

  void SetStopReply(Packet *pktOut) const;

  void Destroy();
  void Detach();
  void Kill();

  bool GetFirstThreadId(uint32_t *id);
  bool GetNextThreadId(uint32_t *id);

  port::Thread *GetRegThread();
  port::Thread *GetRunThread();
  port::Thread *GetThread(uint32_t id);

  bool AddBreakpoint(uint32_t user_address);
  bool RemoveBreakpoint(uint32_t user_address);
  void CopyFaultSignalFromAppThread(port::Thread *thread);
  void RemoveInitialBreakpoint();
  bool IsInitialBreakpointActive();
  bool IsOnValidInstBoundary(uint32_t addr);
  void EraseBreakpointsFromCopyOfMemory(uint32_t user_address,
                                        uint8_t *data, uint32_t size);

  void SuspendAllThreads();
  void ResumeAllThreads();
  void UnqueueAnyFaultedThread(uint32_t *thread_id, int8_t *signal);

  void Resume();
  void ProcessCommands();
  void WaitForDebugEvent();
  void ProcessDebugEvent();

  void MaskAlwaysValidRegisters();

 private:
  struct NaClApp *nap_;
  const Abi *abi_;

  // This mutex protects debugging state (threads_, cur_signal, sig_thread_)
  struct NaClMutex mutex_;

  Session *session_;

  ThreadMap_t threads_;
  ThreadMap_t::const_iterator threadItr_;
  BreakpointMap_t breakpoint_map_;
  // If non-zero, an initial breakpoint is set at the given untrusted
  // code address.
  uint32_t initial_breakpoint_addr_;

  PropertyMap_t properties_;

  uint8_t *ctx_;         // Context Scratchpad

  // Signal being processed.
  // Set to 0 when execution was interrupted by GDB and not by a signal.
  int8_t cur_signal_;

  // Signaled thread id.
  // Set to 0 when execution was interrupted by GDB and not by a signal.
  uint32_t sig_thread_;

  // Thread for subsequent registers access operations.
  uint32_t reg_thread_;

  // Thread that is stepping over a breakpoint while other threads remain
  // suspended.
  uint32_t step_over_breakpoint_thread_;

  // Whether all threads are currently suspended.
  bool all_threads_suspended_;

  // Whether we are about to detach.
  bool detaching_;

  // Whether we are about to exit (from kill).
  bool should_exit_;
};


}  // namespace gdb_rsp

#endif  // SRC_TRUSTED_GDB_RSP_TARGET_H_

