/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <algorithm>

#include "native_client/src/include/build_config.h"
#include "native_client/src/include/nacl_scoped_ptr.h"
#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/shared/platform/nacl_exit.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/trusted/debug_stub/abi.h"
#include "native_client/src/trusted/debug_stub/packet.h"
#include "native_client/src/trusted/debug_stub/platform.h"
#include "native_client/src/trusted/debug_stub/session.h"
#include "native_client/src/trusted/debug_stub/target.h"
#include "native_client/src/trusted/debug_stub/thread.h"
#include "native_client/src/trusted/debug_stub/util.h"
#include "native_client/src/trusted/desc/nacl_desc_base.h"
#include "native_client/src/trusted/service_runtime/nacl_app_thread.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"
#include "native_client/src/trusted/service_runtime/thread_suspension.h"
#include "native_client/src/trusted/validator/ncvalidate.h"
#include "native_client/src/trusted/validator_arm/model.h"

#if NACL_WINDOWS
#define snprintf sprintf_s
#endif

using std::string;

using port::IPlatform;
using port::Thread;
using port::MutexLock;

namespace gdb_rsp {


// Arbitrary descriptor to return when the main nexe is opened.
// This can be shared as the file connections are stateless.
static const char kMainNexeFilename[] = "nexe";
static const char kIrtNexeFilename[] = "irt";
static const uint64_t kMainNexeFd = 123;
static const uint64_t kIrtNexeFd = 234;

// The GDB debug protocol specifies particular values for return values,
// errno values, and mode flags. Explicitly defining the subset used herein.
static const uint64_t kGdbErrorResult = static_cast<uint64_t>(-1);
static const uint64_t kGdbO_RDONLY = 0;
static const uint64_t kGdbEPERM = 1;
static const uint64_t kGdbENOENT = 2;
static const uint64_t kGdbEBADF = 9;

// Assume a buffer size that matches GDB's actual current request size.
static const size_t kGdbPreadChunkSize = 4096;


Target::Target(struct NaClApp *nap, const Abi *abi)
  : nap_(nap),
    abi_(abi),
    session_(NULL),
    initial_breakpoint_addr_(0),
    ctx_(NULL),
    cur_signal_(0),
    sig_thread_(0),
    reg_thread_(0),
    step_over_breakpoint_thread_(0),
    all_threads_suspended_(false),
    detaching_(false),
    should_exit_(false) {
  if (NULL == abi_) abi_ = Abi::Get();
}

Target::~Target() {
  Destroy();
}

bool Target::Init() {
  string targ_xml = "l<target><architecture>";

  targ_xml += abi_->GetName();
  targ_xml += "</architecture><osabi>NaCl</osabi>";
  targ_xml += abi_->GetTargetXml();
  targ_xml += "</target>";

  // Set a more specific result which won't change.
  properties_["target.xml"] = targ_xml;
  properties_["Supported"] =
    "PacketSize=1000;qXfer:features:read+";

  NaClXMutexCtor(&mutex_);
  ctx_ = new uint8_t[abi_->GetContextSize()];

  initial_breakpoint_addr_ = (uint32_t) nap_->initial_entry_pt;
  if (!AddBreakpoint(initial_breakpoint_addr_))
    return false;
  return true;
}

void Target::Destroy() {
  NaClMutexDtor(&mutex_);

  delete[] ctx_;
}

bool Target::AddBreakpoint(uint32_t user_address) {
  const Abi::BPDef *bp = abi_->GetBreakpointDef();

  // Don't allow breakpoints within instructions / pseudo-instructions.
  // Except on ARM since single step uses breakpoints we need to allow
  // breakpoints mid super instruction. However we still need to ensure
  // breakpoints are on instr boundaries and cannot be set in constant pools.
#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_arm
  // 0x3 is a 4 byte mask to check the address falls on a instr boundary.
  if ((user_address & ~0x3) != user_address) {
    NaClLog(LOG_ERROR,
            "Failed to set breakpoint at 0x%x, not on instr boundary\n",
            user_address);
    return false;
  }

  uint32_t bundle_addr = user_address & ~(NACL_INSTR_BLOCK_SIZE - 1);
  uint32_t bundle_head;

  uintptr_t sys_bundle_addr =
      NaClUserToSysAddrRange(nap_, bundle_addr, sizeof(uint32_t));
  if (sys_bundle_addr == kNaClBadAddress)
    return false;

  // Get first instr of bundle.
  if (!IPlatform::GetMemory(sys_bundle_addr, sizeof(uint32_t), &bundle_head))
    return false;

  if (nacl_arm_dec::IsBreakPointAndConstantPoolHead(bundle_head)) {
    NaClLog(LOG_ERROR,
            "Failed to set breakpoint at 0x%x, within constant pool\n",
            user_address);
    return false;
  }
#else
  if (!IsOnValidInstBoundary(user_address)) {
    NaClLog(LOG_ERROR, "Failed to set breakpoint at 0x%x\n", user_address);
    return false;
  }
#endif

  // If we already have a breakpoint here then don't add it
  if (breakpoint_map_.find(user_address) != breakpoint_map_.end())
    return false;

  uintptr_t sysaddr = NaClUserToSysAddrRange(nap_, user_address, bp->size_);
  if (sysaddr == kNaClBadAddress)
    return false;
  // We allow setting breakpoints in the code area but not the data area.
  if (user_address + bp->size_ > nap_->dynamic_text_end)
    return false;

  // We add the breakpoint by overwriting the start of an instruction
  // with a breakpoint instruction.  (At least, we assume that we have
  // been given the address of the start of an instruction.)  In order
  // to be able to remove the breakpoint later, we save a copy of the
  // locations we are overwriting into breakpoint_map_.
  uint8_t *data = new uint8_t[bp->size_];

  // Copy the old code from here
  if (!IPlatform::GetMemory(sysaddr, bp->size_, data)) {
    delete[] data;
    return false;
  }
  if (!IPlatform::SetMemory(nap_, sysaddr, bp->size_, bp->code_)) {
    delete[] data;
    return false;
  }

  breakpoint_map_[user_address] = data;
  return true;
}

bool Target::RemoveBreakpoint(uint32_t user_address) {
  const Abi::BPDef *bp_def = abi_->GetBreakpointDef();

  BreakpointMap_t::iterator iter = breakpoint_map_.find(user_address);
  if (iter == breakpoint_map_.end())
    return false;

  uintptr_t sysaddr = NaClUserToSys(nap_, user_address);
  uint8_t *data = iter->second;
  // Copy back the old code, and free the data
  if (!IPlatform::SetMemory(nap_, sysaddr, bp_def->size_, data)) {
    NaClLog(LOG_ERROR, "Failed to undo breakpoint.\n");
    return false;
  }
  delete[] data;
  breakpoint_map_.erase(iter);
  return true;
}

void Target::CopyFaultSignalFromAppThread(Thread *thread) {
  if (thread->GetFaultSignal() == 0 && thread->HasThreadFaulted()) {
    int signal =
        Thread::ExceptionToSignal(thread->GetAppThread()->fault_signal);
    // If a thread hits a breakpoint, we want to ensure that it is
    // reported as SIGTRAP rather than SIGSEGV.  This is necessary
    // because we use HLT (which produces SIGSEGV) rather than the
    // more usual INT3 (which produces SIGTRAP) on x86, in order to
    // work around a Mac OS X bug.  Similarly, on ARM we use an
    // illegal instruction (which produces SIGILL) rather than the
    // more usual BKPT (which produces SIGTRAP).
    //
    // We need to check each thread to see whether it hit a
    // breakpoint.  We record this on the thread object because:
    //  * We need to check the threads before accepting any commands
    //    from GDB which might remove breakpoints from
    //    breakpoint_map_, which would remove our ability to tell
    //    whether a thread hit a breakpoint.
    //  * Although we deliver fault events to GDB one by one, we might
    //    have multiple threads that have hit breakpoints.
    if ((NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86 &&
         signal == NACL_ABI_SIGSEGV) ||
        (NACL_ARCH(NACL_BUILD_ARCH) == NACL_arm &&
         signal == NACL_ABI_SIGILL)) {
      // Casting to uint32_t is necessary to drop the top 32 bits of
      // %rip on x86-64.
      uint32_t prog_ctr = (uint32_t) thread->GetContext()->prog_ctr;
      if (breakpoint_map_.find(prog_ctr) != breakpoint_map_.end()) {
        signal = NACL_ABI_SIGTRAP;
      }
    }
    thread->SetFaultSignal(signal);
  }
}

void Target::RemoveInitialBreakpoint() {
  if (initial_breakpoint_addr_ != 0) {
    if (!RemoveBreakpoint(initial_breakpoint_addr_)) {
      NaClLog(LOG_FATAL,
              "RemoveInitialBreakpoint: Failed to remove breakpoint\n");
    }
    initial_breakpoint_addr_ = 0;
  }
}

// When the debugger reads memory, we want to report the original
// memory contents without the modifications we made to add
// breakpoints.  This function undoes the modifications from a copy of
// memory.
void Target::EraseBreakpointsFromCopyOfMemory(uint32_t user_address,
                                              uint8_t *data, uint32_t size) {
  uint32_t user_end = user_address + size;
  const Abi::BPDef *bp = abi_->GetBreakpointDef();
  for (BreakpointMap_t::iterator iter = breakpoint_map_.begin();
       iter != breakpoint_map_.end();
       ++iter) {
    uint32_t breakpoint_address = iter->first;
    uint32_t breakpoint_end = breakpoint_address + bp->size_;
    uint8_t *original_data = iter->second;

    uint32_t overlap_start = std::max(user_address, breakpoint_address);
    uint32_t overlap_end = std::min(user_end, breakpoint_end);
    if (overlap_start < overlap_end) {
      uint8_t *dest = data + (overlap_start - user_address);
      uint8_t *src = original_data + (overlap_start - breakpoint_address);
      size_t copy_size = overlap_end - overlap_start;
      // Sanity check: do some bounds checks.
      CHECK(data <= dest && dest + copy_size <= data + size);
      CHECK(original_data <= src
            && src + copy_size <= original_data + bp->size_);
      memcpy(dest, src, copy_size);
    }
  }
}

void Target::Run(Session *ses) {
  NaClXMutexLock(&mutex_);
  session_ = ses;
  NaClXMutexUnlock(&mutex_);

  do {
    WaitForDebugEvent();

    // Lock to prevent anyone else from modifying threads
    // or updating the signal information.
    MutexLock lock(&mutex_);

    ProcessDebugEvent();
    ProcessCommands();
  } while (session_->Connected());

  NaClXMutexLock(&mutex_);
  session_ = NULL;
  NaClXMutexUnlock(&mutex_);
}

bool Target::IsInitialBreakpointActive() {
  return initial_breakpoint_addr_ != 0;
}

bool Target::IsOnValidInstBoundary(uint32_t addr) {
  // No implementation exists for mips, and this would break breakpoints.
#if NACL_ARCH(NACL_BUILD_ARCH) != NACL_mips
  uint8_t code_buf[NACL_INSTR_BLOCK_SIZE];
  // Calculate nearest bundle address.
  uint32_t bundle_addr = addr & ~(NACL_INSTR_BLOCK_SIZE - 1);
  uintptr_t code_addr = NaClUserToSysAddrRange(
    nap_, bundle_addr, NACL_INSTR_BLOCK_SIZE);

  if (code_addr == kNaClBadAddress)
    return false;

  // Read memory.  This checks whether the memory is mapped.
  if (!IPlatform::GetMemory(code_addr, NACL_INSTR_BLOCK_SIZE, code_buf))
    return false;

  // We want the validator validating the original code without breakpoints.
  EraseBreakpointsFromCopyOfMemory(bundle_addr, code_buf,
                                   NACL_INSTR_BLOCK_SIZE);

  // The IsOnInstBoundary() validator function hasn't been
  // implemented for mips. Therefore this will criple debugging
  // on mips, including breakpoints, until it is implemented in
  // the validator.
  return nap_->validator->IsOnInstBoundary(
                  bundle_addr, addr,
                  code_buf,
                  NACL_INSTR_BLOCK_SIZE,
                  nap_->cpu_features) == NaClValidationSucceeded;
#else
  UNREFERENCED_PARAMETER(addr);
  return true;
#endif
}

void Target::WaitForDebugEvent() {
  if (all_threads_suspended_) {
    // If all threads are suspended (which may be left over from a previous
    // connection), we are already ready to handle commands from GDB.
    return;
  }
  // Wait for either:
  //   * an untrusted thread to fault (or single-step)
  //   * an interrupt from GDB
  bool ignore_input_from_gdb = step_over_breakpoint_thread_ != 0 ||
    IsInitialBreakpointActive();
  session_->WaitForDebugStubEvent(nap_, ignore_input_from_gdb);
}

void Target::ProcessDebugEvent() {
  if (all_threads_suspended_) {
    // We are already in a suspended state.
    return;
  } else if (step_over_breakpoint_thread_ != 0) {
    // We are waiting for a specific thread to fault while all other
    // threads are suspended.  Note that faulted_thread_count might
    // be >1, because multiple threads can fault simultaneously
    // before the debug stub gets a chance to suspend all threads.
    // This is why we must check the status of a specific thread --
    // we cannot call UnqueueAnyFaultedThread() and expect it to
    // return step_over_breakpoint_thread_.
    Thread *thread = threads_[step_over_breakpoint_thread_];
    if (!thread->HasThreadFaulted()) {
      // The thread has not faulted.  Nothing to do, so try again.
      // Note that we do not respond to input from GDB while in this
      // state.
      // TODO(mseaborn): We should allow GDB to interrupt execution.
      return;
    }
    // All threads but one are already suspended.  We only need to
    // suspend the single thread that we allowed to run.
    thread->SuspendThread();
    CopyFaultSignalFromAppThread(thread);
    cur_signal_ = thread->GetFaultSignal();
    thread->UnqueueFaultedThread();
    sig_thread_ = step_over_breakpoint_thread_;
    reg_thread_ = step_over_breakpoint_thread_;
    step_over_breakpoint_thread_ = 0;
  } else if (nap_->faulted_thread_count != 0) {
    // At least one untrusted thread has got an exception.  First we
    // need to ensure that all threads are suspended.  Then we can
    // retrieve a thread from the set of faulted threads.
    SuspendAllThreads();
    UnqueueAnyFaultedThread(&sig_thread_, &cur_signal_);
    reg_thread_ = sig_thread_;
  } else {
    // Otherwise look for messages from GDB.  To fix a potential
    // race condition, we don't do this on the first run, because in
    // that case we are waiting for the initial breakpoint to be
    // reached.  We don't want GDB to observe states where the
    // (internal) initial breakpoint is still registered or where
    // the initial thread is suspended in NaClStartThreadInApp()
    // before executing its first untrusted instruction.
    if (IsInitialBreakpointActive() || !session_->IsDataAvailable()) {
      // No input from GDB.  Nothing to do, so try again.
      return;
    }
    // GDB should have tried to interrupt the target.
    // See http://sourceware.org/gdb/current/onlinedocs/gdb/Interrupts.html
    // TODO(eaeltsin): should we verify the interrupt sequence?

    // Indicate we have no current thread.
    // TODO(eaeltsin): or pick any thread? Add a test.
    // See http://code.google.com/p/nativeclient/issues/detail?id=2743
    sig_thread_ = 0;
    SuspendAllThreads();
  }

  bool initial_breakpoint_was_active = IsInitialBreakpointActive();

  if (sig_thread_ != 0) {
    // Reset single stepping.
    threads_[sig_thread_]->SetStep(false);
    RemoveInitialBreakpoint();
  }

  // Next update the current thread info
  char tmp[16];
  snprintf(tmp, sizeof(tmp), "QC%x", sig_thread_);
  properties_["C"] = tmp;

  if (!initial_breakpoint_was_active) {
    // First time on a connection, we don't send the signal.
    // All other times, send the signal that triggered us.
    Packet pktOut;
    SetStopReply(&pktOut);
    session_->SendPacketOnly(&pktOut);
  }

  all_threads_suspended_ = true;
}

void Target::ProcessCommands() {
  if (!all_threads_suspended_) {
    // Don't process commands if we haven't stopped all threads.
    return;
  }

  // Ensure sp register is always masked on ARM. This is used to
  // defensively ensure the invariant whenever we have all untrusted
  // threads suspended and start processing commands.
  MaskAlwaysValidRegisters();

  // Now we are ready to process commands.
  // Loop through packets until we process a continue packet or a detach.
  Packet recv, reply;
  do {
    if (!session_->GetPacket(&recv))
      continue;
    reply.Clear();
    if (ProcessPacket(&recv, &reply)) {
      // If this is a continue type command, break out of this loop.
      break;
    }
    // Otherwise send the response.
    session_->SendPacket(&reply);

    if (detaching_) {
      detaching_ = false;
      session_->Disconnect();
      Resume();
      return;
    }

    if (should_exit_) {
      NaClExit(-9);
    }
  } while (session_->Connected());

  if (session_->Connected()) {
    // Continue if we're still connected.
    Resume();
  }
}

void Target::Resume() {
  // Reset the signal value
  cur_signal_ = 0;

  // TODO(eaeltsin): it might make sense to resume signaled thread before
  // others, though it is not required by GDB docs.
  if (step_over_breakpoint_thread_ == 0) {
    ResumeAllThreads();
  } else {
    // Resume one thread while leaving all others suspended.
    threads_[step_over_breakpoint_thread_]->ResumeThread();
  }

  all_threads_suspended_ = false;
}

void Target::SetStopReply(Packet *pktOut) const {
  pktOut->AddRawChar('T');
  pktOut->AddWord8(cur_signal_);

  // gdbserver handles GDB interrupt by sending SIGINT to the debuggee, thus
  // GDB interrupt is also a case of a signalled thread.
  // At the moment we handle GDB interrupt differently, without using a signal,
  // so in this case sig_thread_ is 0.
  // This might seem weird to GDB, so at least avoid reporting tid 0.
  // TODO(eaeltsin): http://code.google.com/p/nativeclient/issues/detail?id=2743
  if (sig_thread_ != 0) {
    // Add 'thread:<tid>;' pair. Note terminating ';' is required.
    pktOut->AddString("thread:");
    pktOut->AddNumberSep(sig_thread_, ';');
  }
}


bool Target::GetFirstThreadId(uint32_t *id) {
  threadItr_ = threads_.begin();
  return GetNextThreadId(id);
}

bool Target::GetNextThreadId(uint32_t *id) {
  if (threadItr_ == threads_.end()) return false;

  *id = (*threadItr_).first;
  threadItr_++;

  return true;
}


uint64_t Target::AdjustUserAddr(uint64_t addr) {
  // On x86-64, GDB sometimes uses memory addresses with the %r15
  // sandbox base included, so we must accept these addresses.
  // TODO(eaeltsin): Fix GDB to not use addresses with %r15 added.
  if (NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86 && NACL_BUILD_SUBARCH == 64 &&
      NaClIsUserAddr(nap_, (uintptr_t) addr)) {
    return NaClSysToUser(nap_, (uintptr_t) addr);
  }
  // Otherwise, we expect an untrusted address.
  return addr;
}

void Target::EmitFileError(Packet *pktOut, int code) {
  pktOut->AddString("F");
  pktOut->AddNumberSep(kGdbErrorResult, ',');
  pktOut->AddNumberSep(code, 0);
}

void Target::ProcessFilePacket(Packet *pktIn, Packet *pktOut, ErrDef *err) {
  std::string cmd;
  if (!pktIn->GetStringSep(&cmd, ':')) {
    *err = BAD_FORMAT;
    return;
  }
  CHECK(cmd == "File");
  std::string subcmd;
  if (!pktIn->GetStringSep(&subcmd, ':')) {
    *err = BAD_FORMAT;
    return;
  }
  if (subcmd == "open") {
    std::string filename;
    char sep;
    uint64_t flags;
    uint64_t mode;
    if (!pktIn->GetHexString(&filename) ||
        !pktIn->GetRawChar(&sep) ||
        sep != ',' ||
        !pktIn->GetNumberSep(&flags, NULL) ||
        !pktIn->GetNumberSep(&mode, NULL)) {
      *err = BAD_ARGS;
      return;
    }
    if (filename == kMainNexeFilename) {
      if (flags == kGdbO_RDONLY) {
        pktOut->AddString("F");
        pktOut->AddNumberSep(kMainNexeFd, 0);
      } else {
        EmitFileError(pktOut, kGdbEPERM);
      }
    } else if (filename == kIrtNexeFilename) {
      if (flags == kGdbO_RDONLY) {
        pktOut->AddString("F");
        pktOut->AddNumberSep(kIrtNexeFd, 0);
      } else {
        EmitFileError(pktOut, kGdbEPERM);
      }
    } else {
      EmitFileError(pktOut, kGdbENOENT);
    }
    return;
  } else if (subcmd == "close") {
    uint64_t fd;
    if (!pktIn->GetNumberSep(&fd, NULL)) {
      *err = BAD_ARGS;
      return;
    }
    if (fd == kMainNexeFd) {
      pktOut->AddString("F");
      pktOut->AddNumberSep(0, 0);
    } else if (fd == kIrtNexeFd) {
      pktOut->AddString("F");
      pktOut->AddNumberSep(0, 0);
    } else {
      EmitFileError(pktOut, kGdbEBADF);
    }
    return;
  } else if (subcmd == "pread") {
    uint64_t fd;
    uint64_t count;
    uint64_t offset;
    std::string data;
    if (!pktIn->GetNumberSep(&fd, NULL) ||
        !pktIn->GetNumberSep(&count, NULL) ||
        !pktIn->GetNumberSep(&offset, NULL)) {
      *err = BAD_ARGS;
      return;
    }
    NaClDesc *desc;
    if (fd == kMainNexeFd) {
      desc = nap_->main_nexe_desc;
    } else if (fd == kIrtNexeFd) {
      desc = nap_->irt_nexe_desc;
    } else {
      EmitFileError(pktOut, kGdbEBADF);
      return;
    }
    CHECK(NULL != desc);
    if (count > kGdbPreadChunkSize) {
      count = kGdbPreadChunkSize;
    }
    nacl::scoped_array<char> buffer(new char[kGdbPreadChunkSize]);
    ssize_t result = (*NACL_VTBL(NaClDesc, desc)->PRead)(
        desc, buffer.get(),
        static_cast<size_t>(count),
        static_cast<nacl_off64_t>(offset));
    pktOut->AddString("F");
    if (result < 0) {
      pktOut->AddNumberSep(kGdbErrorResult, ',');
      pktOut->AddNumberSep(static_cast<uint64_t>(-result), 0);
    } else {
      pktOut->AddNumberSep(static_cast<uint64_t>(result), ';');
      pktOut->AddEscapedData(buffer.get(), static_cast<size_t>(result));
    }
    return;
  }
  NaClLog(LOG_ERROR, "Unknown vFile command: %s\n", pktIn->GetPayload());
  *err = BAD_FORMAT;
}

bool Target::ProcessPacket(Packet *pktIn, Packet *pktOut) {
  char cmd;
  int32_t seq = -1;
  ErrDef  err = NONE;

  // Clear the outbound message
  pktOut->Clear();

  // Pull out the sequence.
  pktIn->GetSequence(&seq);
  if (seq != -1) pktOut->SetSequence(seq);

  // Find the command
  pktIn->GetRawChar(&cmd);

  switch (cmd) {
    // IN : $?
    // OUT: $Sxx
    case '?':
      SetStopReply(pktOut);
      break;

    case 'c':
      return true;

    // IN : $D
    // OUT: $OK
    case 'D':
      Detach();
      pktOut->AddString("OK");
      return false;

    // IN : $k
    // OUT: $OK
    case 'k':
      Kill();
      pktOut->AddString("OK");
      return false;

    // IN : $g
    // OUT: $xx...xx
    case 'g': {
      Thread *thread = GetRegThread();
      if (NULL == thread) {
        err = BAD_ARGS;
        break;
      }

      thread->GetRegisters(ctx_);

      pktOut->AddBlock(ctx_, abi_->GetContextSize());
      break;
    }

    // IN : $Gxx..xx
    // OUT: $OK
    case 'G': {
      Thread *thread = GetRegThread();
      if (NULL == thread) {
        err = BAD_ARGS;
        break;
      }

      pktIn->GetBlock(ctx_, abi_->GetContextSize());

      uint32_t new_pc = static_cast<uint32_t>(
          reinterpret_cast<NaClSignalContext *>(ctx_)->prog_ctr);

      if (!IsOnValidInstBoundary(new_pc) || !thread->SetRegisters(ctx_)) {
        NaClLog(LOG_ERROR, "Invalid register change\n");
        err = FAILED;
      } else {
        pktOut->AddString("OK");
      }

      break;
    }

    // IN : $H(c/g)(-1,0,xxxx)
    // OUT: $OK
    case 'H': {
        char type;
        uint64_t id;

        if (!pktIn->GetRawChar(&type)) {
          err = BAD_FORMAT;
          break;
        }
        if (!pktIn->GetNumberSep(&id, 0)) {
          err = BAD_FORMAT;
          break;
        }

        if (threads_.begin() == threads_.end()) {
            err = BAD_ARGS;
            break;
        }

        // If we are using "any" get the first thread
        if (id == static_cast<uint64_t>(-1)) id = threads_.begin()->first;

        // Verify that we have the thread
        if (threads_.find(static_cast<uint32_t>(id)) == threads_.end()) {
          err = BAD_ARGS;
          break;
        }

        pktOut->AddString("OK");
        switch (type) {
          case 'g':
            reg_thread_ = static_cast<uint32_t>(id);
            break;

          case 'c':
            // 'c' is deprecated in favor of vCont.
          default:
            err = BAD_ARGS;
            break;
        }
        break;
      }

    // IN : $maaaa,llll
    // OUT: $xx..xx
    case 'm': {
        uint64_t user_addr;
        uint64_t wlen;
        uint32_t len;
        if (!pktIn->GetNumberSep(&user_addr, 0)) {
          err = BAD_FORMAT;
          break;
        }
        if (!pktIn->GetNumberSep(&wlen, 0)) {
          err = BAD_FORMAT;
          break;
        }
        user_addr = AdjustUserAddr(user_addr);
        uint64_t sys_addr = NaClUserToSysAddrRange(nap_, (uintptr_t) user_addr,
                                                   (size_t) wlen);
        if (sys_addr == kNaClBadAddress) {
          err = FAILED;
          break;
        }

        len = static_cast<uint32_t>(wlen);
        nacl::scoped_array<uint8_t> block(new uint8_t[len]);
        if (!port::IPlatform::GetMemory(sys_addr, len, block.get())) {
          err = FAILED;
          break;
        }
        EraseBreakpointsFromCopyOfMemory((uint32_t) user_addr,
                                         block.get(), len);

        pktOut->AddBlock(block.get(), len);
        break;
      }

    // IN : $Maaaa,llll:xx..xx
    // OUT: $OK
    case 'M':  {
        uint64_t user_addr;
        uint64_t wlen;
        uint32_t len;

        if (!pktIn->GetNumberSep(&user_addr, 0)) {
          err = BAD_FORMAT;
          break;
        }
        if (!pktIn->GetNumberSep(&wlen, 0)) {
          err = BAD_FORMAT;
          break;
        }
        user_addr = AdjustUserAddr(user_addr);
        uint64_t sys_addr = NaClUserToSysAddrRange(nap_, (uintptr_t) user_addr,
                                                   (size_t) wlen);
        if (sys_addr == kNaClBadAddress) {
          err = FAILED;
          break;
        }
        len = static_cast<uint32_t>(wlen);
        // We disallow the debugger from modifying code.
        if (user_addr < nap_->dynamic_text_end) {
          err = FAILED;
          break;
        }

        nacl::scoped_array<uint8_t> block(new uint8_t[len]);
        pktIn->GetBlock(block.get(), len);

        if (!port::IPlatform::SetMemory(nap_, sys_addr, len, block.get())) {
          err = FAILED;
          break;
        }

        pktOut->AddString("OK");
        break;
      }

    case 'q': {
      string tmp;
      const char *str = &pktIn->GetPayload()[1];
      stringvec toks = StringSplit(str, ":;");
      PropertyMap_t::const_iterator itr = properties_.find(toks[0]);

      // If this is a thread query
      if (!strcmp(str, "fThreadInfo") || !strcmp(str, "sThreadInfo")) {
        uint32_t curr;
        bool more = false;
        if (str[0] == 'f') {
          more = GetFirstThreadId(&curr);
        } else {
          more = GetNextThreadId(&curr);
        }

        if (!more) {
          pktOut->AddString("l");
        } else {
          pktOut->AddString("m");
          pktOut->AddNumberSep(curr, 0);
        }
        break;
      }

      // Check for architecture query
      tmp = "Xfer:features:read:target.xml";
      if (!strncmp(str, tmp.data(), tmp.length())) {
        stringvec args = StringSplit(&str[tmp.length()+1], ",");
        if (args.size() != 2) break;

        const char *out = properties_["target.xml"].data();
        int offs = strtol(args[0].data(), NULL, 16);
        int max  = strtol(args[1].data(), NULL, 16) + offs;
        int len  = static_cast<int>(strlen(out));

        if (max >= len) max = len;

        while (offs < max) {
          pktOut->AddRawChar(out[offs]);
          offs++;
        }
        break;
      }

      // Check the property cache
      if (itr != properties_.end()) {
        pktOut->AddString(itr->second.data());
      }
      break;
    }

    case 's': {
      Thread *thread = GetRunThread();
      if (thread) thread->SetStep(true);
      return true;
    }

    case 'T': {
      uint64_t id;
      if (!pktIn->GetNumberSep(&id, 0)) {
        err = BAD_FORMAT;
        break;
      }

      if (GetThread(static_cast<uint32_t>(id)) == NULL) {
        err = BAD_ARGS;
        break;
      }

      pktOut->AddString("OK");
      break;
    }

    case 'v': {
      const char *str = pktIn->GetPayload() + 1;

      if (strncmp(str, "Cont", 4) == 0) {
        // vCont
        const char *subcommand = str + 4;

        if (strcmp(subcommand, "?") == 0) {
          // Report supported vCont actions. These 4 are required.
          pktOut->AddString("vCont;s;S;c;C");
          break;
        }

        if (strcmp(subcommand, ";c") == 0) {
          // Continue all threads.
          return true;
        }

        if (strncmp(subcommand, ";s:", 3) == 0) {
          // Single step one thread and optionally continue all other threads.
          char *end;
          uint32_t thread_id = static_cast<uint32_t>(
              strtol(subcommand + 3, &end, 16));
          if (end == subcommand + 3) {
            err = BAD_ARGS;
            break;
          }

          ThreadMap_t::iterator it = threads_.find(thread_id);
          if (it == threads_.end()) {
            err = BAD_ARGS;
            break;
          }

          if (*end == 0) {
            // Single step one thread and keep other threads stopped.
            // GDB uses this to continue from a breakpoint, which works by:
            // - replacing trap instruction with the original instruction;
            // - single-stepping through the original instruction. Other threads
            //   must remain stopped, otherwise they might execute the code at
            //   the same address and thus miss the breakpoint;
            // - replacing the original instruction with trap instruction;
            // - continuing all threads;
            if (thread_id != sig_thread_) {
              err = BAD_ARGS;
              break;
            }
            step_over_breakpoint_thread_ = sig_thread_;
          } else if (strcmp(end, ";c") == 0) {
            // Single step one thread and continue all other threads.
          } else {
            // Unsupported combination of single step and other args.
            err = BAD_ARGS;
            break;
          }

          it->second->SetStep(true);
          return true;
        }

        // Continue one thread and keep other threads stopped.
        //
        // GDB sends this for software single step, which is used:
        // - on Win64 to step over rsp modification and subsequent rsp
        //   sandboxing at once. For details, see:
        //     http://code.google.com/p/nativeclient/issues/detail?id=2903
        // - TODO: on ARM, which has no hardware support for single step
        // - TODO: to step over syscalls
        //
        // Unfortunately, we can't make this just Win-specific. We might
        // use Linux GDB to connect to Win debug stub, so even Linux GDB
        // should send software single step. Vice versa, software single
        // step-enabled Win GDB might be connected to Linux debug stub,
        // so even Linux debug stub should accept software single step.
        if (strncmp(subcommand, ";c:", 3) == 0) {
          char *end;
          uint32_t thread_id = static_cast<uint32_t>(
              strtol(subcommand + 3, &end, 16));
          if (end != subcommand + 3 && *end == 0) {
            if (thread_id == sig_thread_) {
              step_over_breakpoint_thread_ = sig_thread_;
              return true;
            }
          }

          err = BAD_ARGS;
          break;
        }

        // Unsupported form of vCont.
        err = BAD_FORMAT;
        break;
      } else if (strncmp(str, "File:", 5) == 0) {
        ProcessFilePacket(pktIn, pktOut, &err);
        break;
      }

      NaClLog(LOG_ERROR, "Unknown command: %s\n", pktIn->GetPayload());
      return false;
    }

    case 'Z': {
      uint64_t breakpoint_type;
      uint64_t breakpoint_address;
      uint64_t breakpoint_kind;
      if (!pktIn->GetNumberSep(&breakpoint_type, 0) ||
          breakpoint_type != 0 ||
          !pktIn->GetNumberSep(&breakpoint_address, 0) ||
          !pktIn->GetNumberSep(&breakpoint_kind, 0)) {
        err = BAD_FORMAT;
        break;
      }
      if (breakpoint_address != (uint32_t) breakpoint_address ||
          !AddBreakpoint((uint32_t) breakpoint_address)) {
        err = FAILED;
        break;
      }
      pktOut->AddString("OK");
      break;
    }

    case 'z': {
      uint64_t breakpoint_type;
      uint64_t breakpoint_address;
      uint64_t breakpoint_kind;
      if (!pktIn->GetNumberSep(&breakpoint_type, 0) ||
          breakpoint_type != 0 ||
          !pktIn->GetNumberSep(&breakpoint_address, 0) ||
          !pktIn->GetNumberSep(&breakpoint_kind, 0)) {
        err = BAD_FORMAT;
        break;
      }
      if (breakpoint_address != (uint32_t) breakpoint_address ||
          !RemoveBreakpoint((uint32_t) breakpoint_address)) {
        err = FAILED;
        break;
      }
      pktOut->AddString("OK");
      break;
    }

    default: {
      // If the command is not recognzied, ignore it by sending an
      // empty reply.
      string str;
      pktIn->GetString(&str);
      NaClLog(LOG_ERROR, "Unknown command: %s\n", pktIn->GetPayload());
      return false;
    }
  }

  // If there is an error, return the error code instead of a payload
  if (err) {
    pktOut->Clear();
    pktOut->AddRawChar('E');
    pktOut->AddWord8(err);
  }
  return false;
}


void Target::TrackThread(struct NaClAppThread *natp) {
  // natp->thread_num values are 0-based indexes, but we treat 0 as
  // "not a thread ID", so we add 1.
  uint32_t id = natp->thread_num + 1;
  MutexLock lock(&mutex_);
  CHECK(threads_[id] == 0);
  threads_[id] = new Thread(id, natp);
}

void Target::IgnoreThread(struct NaClAppThread *natp) {
  uint32_t id = natp->thread_num + 1;
  MutexLock lock(&mutex_);
  ThreadMap_t::iterator iter = threads_.find(id);
  CHECK(iter != threads_.end());
  delete iter->second;
  threads_.erase(iter);
}

void Target::Exit() {
  MutexLock lock(&mutex_);
  if (session_ != NULL) {
    Packet exit_packet;
    if (NACL_ABI_WIFSIGNALED(nap_->exit_status)) {
      exit_packet.AddRawChar('X');
      exit_packet.AddWord8(NACL_ABI_WTERMSIG(nap_->exit_status));
    } else {
      exit_packet.AddRawChar('W');
      exit_packet.AddWord8(NACL_ABI_WEXITSTATUS(nap_->exit_status));
    }
    session_->SendPacket(&exit_packet);
  }
}

void Target::Detach() {
  NaClLog(LOG_INFO, "Requested Detach.\n");
  detaching_ = true;
}

void Target::Kill() {
  NaClLog(LOG_INFO, "Requested Kill.\n");
  should_exit_ = true;
}

Thread *Target::GetRegThread() {
  ThreadMap_t::const_iterator itr;

  switch (reg_thread_) {
    // If we want "any" then try the signal'd thread first
    case 0:
    case 0xFFFFFFFF:
      itr = threads_.begin();
      break;

    default:
      itr = threads_.find(reg_thread_);
      break;
  }

  if (itr == threads_.end()) return 0;

  return itr->second;
}

Thread *Target::GetRunThread() {
  // This is used to select a thread for "s" (step) command only.
  // For multi-threaded targets, "s" is deprecated in favor of "vCont", which
  // always specifies the thread explicitly when needed. However, we want
  // to keep backward compatibility here, as using "s" when debugging
  // a single-threaded program might be a popular use case.
  if (threads_.size() == 1) {
    return threads_.begin()->second;
  }
  return NULL;
}

Thread *Target::GetThread(uint32_t id) {
  ThreadMap_t::const_iterator itr;
  itr = threads_.find(id);
  if (itr != threads_.end()) return itr->second;

  return NULL;
}

void Target::SuspendAllThreads() {
  NaClUntrustedThreadsSuspendAll(nap_, /* save_registers= */ 1);
  for (ThreadMap_t::const_iterator iter = threads_.begin();
       iter != threads_.end();
       ++iter) {
    Thread *thread = iter->second;
    thread->CopyRegistersFromAppThread();
    CopyFaultSignalFromAppThread(thread);
  }
}

void Target::ResumeAllThreads() {
  for (ThreadMap_t::const_iterator iter = threads_.begin();
       iter != threads_.end();
       ++iter) {
    iter->second->CopyRegistersToAppThread();
  }
  NaClUntrustedThreadsResumeAll(nap_);
}

// UnqueueAnyFaultedThread() picks a thread that has been blocked as a
// result of faulting and unblocks it.  It returns the thread's ID via
// |thread_id| and the type of fault via |signal|.  As a precondition,
// all threads must be currently suspended.
void Target::UnqueueAnyFaultedThread(uint32_t *thread_id, int8_t *signal) {
  for (ThreadMap_t::const_iterator iter = threads_.begin();
       iter != threads_.end();
       ++iter) {
    Thread *thread = iter->second;
    if (thread->GetFaultSignal() != 0) {
      *signal = thread->GetFaultSignal();
      *thread_id = thread->GetId();
      thread->UnqueueFaultedThread();
      return;
    }
  }
  NaClLog(LOG_FATAL, "UnqueueAnyFaultedThread: No threads queued\n");
}

  // On ARM its important to make sure the sp register is always masked,
  // this isn't a problem on x86 since registers are always masked first
  // in super instructions. This is not the case for sp in ARM.
  //
  // https://developer.chrome.com/native-client/reference/sandbox_internals/
  // arm-32-bit-sandbox#the-stack-pointer-thread-pointer-and-program-counter
  void Target::MaskAlwaysValidRegisters() {
#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_arm
    for (ThreadMap_t::const_iterator iter = threads_.begin();
         iter != threads_.end();
         ++iter) {
      iter->second->MaskSPRegister();
    }
#endif
  }

}  // namespace gdb_rsp
