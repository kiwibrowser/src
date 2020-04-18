/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <map>
#include <string.h>

#include "native_client/src/include/build_config.h"
#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/trusted/debug_stub/abi.h"
#include "native_client/src/trusted/debug_stub/mutex.h"
#include "native_client/src/trusted/debug_stub/thread.h"
#include "native_client/src/trusted/service_runtime/thread_suspension.h"

namespace {

#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86
const int kX86TrapFlag = 1 << 8;
#endif

bool IsZero(const uint8_t *buf, uint32_t size) {
  for (uint32_t i = 0; i < size; i++) {
    if (buf[i] != 0) return false;
  }
  return true;
}

}  // namespace

namespace port {

Thread::Thread(uint32_t id, struct NaClAppThread *natp)
    : id_(id), natp_(natp), fault_signal_(0) {}

Thread::~Thread() {}

uint32_t Thread::GetId() {
  return id_;
}

bool Thread::SetStep(bool on) {
#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86
  if (on) {
    context_.flags |= kX86TrapFlag;
  } else {
    context_.flags &= ~kX86TrapFlag;
  }
  return true;
#else
  // TODO(mseaborn): Implement for ARM.
  UNREFERENCED_PARAMETER(on);
  return false;
#endif
}

bool Thread::GetRegisters(uint8_t *dst) {
  const gdb_rsp::Abi *abi = gdb_rsp::Abi::Get();
  for (uint32_t a = 0; a < abi->GetRegisterCount(); a++) {
    const gdb_rsp::Abi::RegDef *reg = abi->GetRegisterDef(a);
    if (reg->type_ == gdb_rsp::Abi::READ_ONLY_ZERO) {
      memset(dst + reg->offset_, 0, reg->bytes_);
    } else if (reg->type_ == gdb_rsp::Abi::ARM_STATUS) {
      CHECK(NACL_ARCH(NACL_BUILD_ARCH) == NACL_arm);
      CHECK(reg->bytes_ == 4);
      uint32_t reg_value = *((uint32_t*)((char *) &context_ + reg->offset_));
      // Mask all but top 4 bits (NZCV).
      reg_value &= 0xF0000000;
      memcpy(dst + reg->offset_,
             &reg_value,
             reg->bytes_);
    } else {
      memcpy(dst + reg->offset_,
             (char *) &context_ + reg->offset_,
             reg->bytes_);
    }
  }
  return false;
}

bool Thread::SetRegisters(uint8_t *src) {
  const gdb_rsp::Abi *abi = gdb_rsp::Abi::Get();
  bool valid = true;
  // Validate all registers before making any changes.
  for (uint32_t a = 0; a < abi->GetRegisterCount(); a++) {
    const gdb_rsp::Abi::RegDef *reg = abi->GetRegisterDef(a);
    if (reg->type_ == gdb_rsp::Abi::READ_ONLY) {
      // Ensure read-only values haven't changed.
      valid &= memcmp(src + reg->offset_,
                      (char *) &context_ + reg->offset_,
                      reg->bytes_) == 0;
    } else if (reg->type_ == gdb_rsp::Abi::READ_ONLY_ZERO) {
      // Ensure read-only zero values are zero or unchanged.
      valid &= memcmp(src + reg->offset_,
                      (char *) &context_ + reg->offset_,
                      reg->bytes_) == 0 ||
               IsZero(src + reg->offset_, reg->bytes_);
    } else if (reg->type_ == gdb_rsp::Abi::ARM_TRUSTED_PTR) {
      CHECK(NACL_ARCH(NACL_BUILD_ARCH) == NACL_arm);
      CHECK(reg->bytes_ == 4);
      uint32_t new_val = *((uint32_t*)(src + reg->offset_));
      // Ensure high 2 bits are zero.
      valid &= (new_val & 0x3FFFFFFF) == new_val;
    } else if (reg->type_ == gdb_rsp::Abi::ARM_STATUS) {
      CHECK(NACL_ARCH(NACL_BUILD_ARCH) == NACL_arm);
      CHECK(reg->bytes_ == 4);
      uint32_t new_val = *((uint32_t*)(src + reg->offset_));
      // Ensure only high 4 bits (NZCV) have changed or lower 28 bits are 0.
      valid &= (new_val & 0x0FFFFFFF) == 0;
    } else if (reg->type_ == gdb_rsp::Abi::X86_64_TRUSTED_PTR) {
      CHECK(NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86 &&
            NACL_BUILD_SUBARCH == 64);
      CHECK(reg->bytes_ == 8);
      // Ensure high 32 bits are zero or unchanged.
      valid &= memcmp(src + reg->offset_ + 4,
                      (char *) &context_ + reg->offset_ + 4,
                      4) == 0 || IsZero(src + reg->offset_ + 4, 4);
    }
    if (!valid) return false;
  }

  for (uint32_t a = 0; a < abi->GetRegisterCount(); a++) {
    const gdb_rsp::Abi::RegDef *reg = abi->GetRegisterDef(a);
    if (reg->type_ == gdb_rsp::Abi::READ_ONLY ||
        reg->type_ == gdb_rsp::Abi::READ_ONLY_ZERO) {
      // Do not change read-only registers.
    } else if (reg->type_ == gdb_rsp::Abi::X86_64_TRUSTED_PTR) {
      CHECK(NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86 &&
            NACL_BUILD_SUBARCH == 64);
      CHECK(reg->bytes_ == 8);
      // Do not change high 32 bits.
      // GDB should work with untrusted addresses, thus high 32 bits of new
      // value should be 0.
      memcpy((char *) &context_ + reg->offset_,
             src + reg->offset_, 4);
    } else if (reg->type_ == gdb_rsp::Abi::ARM_STATUS) {
      CHECK(NACL_ARCH(NACL_BUILD_ARCH) == NACL_arm);
      CHECK(reg->bytes_ == 4);

      uint32_t new_val = *((uint32_t*)(src + reg->offset_));
      uint32_t cur_val = *((uint32_t*)((char *) &context_ + reg->offset_));

      // Only copy 4 upper bits (NZCV) of cpsr.
      new_val &= 0xF0000000;
      cur_val &= 0x0FFFFFFF;
      new_val = new_val | cur_val;

      memcpy((char *) &context_ + reg->offset_, &new_val, 4);
    } else {
      memcpy((char *) &context_ + reg->offset_,
             src + reg->offset_, reg->bytes_);
    }
  }
  return true;
}

#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_arm
  void Thread::MaskSPRegister() {
    const gdb_rsp::Abi::RegDef *reg = gdb_rsp::Abi::Get()->GetRegisterDef(13);
    CHECK(reg->bytes_ == 4 && strcmp("sp", reg->name_) == 0);
    uint32_t *reg_val = (uint32_t *)((char *) &context_ + reg->offset_);
    *reg_val &= 0x3FFFFFFF;
  }
#endif

void Thread::CopyRegistersFromAppThread() {
  NaClAppThreadGetSuspendedRegisters(natp_, &context_);
}

void Thread::CopyRegistersToAppThread() {
  NaClAppThreadSetSuspendedRegisters(natp_, &context_);
}

void Thread::SuspendThread() {
  NaClUntrustedThreadSuspend(natp_, /* save_registers= */ 1);
  CopyRegistersFromAppThread();
}

void Thread::ResumeThread() {
  CopyRegistersToAppThread();
  NaClUntrustedThreadResume(natp_);
}

// HasThreadFaulted() returns whether the given thread has been
// blocked as a result of faulting.  The thread does not need to be
// currently suspended.
bool Thread::HasThreadFaulted() {
  return natp_->fault_signal != 0;
}

// UnqueueFaultedThread() takes a thread that has been blocked as a
// result of faulting and unblocks it.  As a precondition, the
// thread must be currently suspended.
void Thread::UnqueueFaultedThread() {
  int exception_code;
  CHECK(NaClAppThreadUnblockIfFaulted(natp_, &exception_code));
  fault_signal_ = 0;
}

int Thread::GetFaultSignal() {
  return fault_signal_;
}

void Thread::SetFaultSignal(int signal) {
  fault_signal_ = signal;
}

struct NaClSignalContext *Thread::GetContext() { return &context_; }
struct NaClAppThread *Thread::GetAppThread() { return natp_; }

}  // namespace port
