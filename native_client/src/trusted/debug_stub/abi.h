/*
 * Copyright (c) 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

// This module provides an interface for performing ABI specific
// functions needed by the Target and Host.  ABI objects are provided
// to the rest of the system as "const" to prevent accidental modification.
// None of the resources in the Abi object are actually owned by the Abi
// object, they are assumed to be static and never destroyed.
//
// This module will only throw standard errors
//    std::bad_alloc - when out of memory
//    std::out_of_range - when using an out of range regsiter index
//
// It is required that Init be called prior to calling Find to ensure
// the various built-in ABIs have been registered.

#ifndef NATIVE_CLIENT_GDB_RSP_ABI_H_
#define NATIVE_CLIENT_GDB_RSP_ABI_H_ 1

#include <map>
#include <string>

#include "native_client/src/include/portability.h"

namespace gdb_rsp {

class Abi {
 public:
  // Defines how a register is exposed to the debugger.
  enum RegType {
    GENERAL,
    READ_ONLY,
    ARM_TRUSTED_PTR,
    ARM_STATUS,
    X86_64_TRUSTED_PTR,
    READ_ONLY_ZERO,
    REG_TYPE_CNT
  };

  // Defines an individual register
  struct RegDef {
    const char *name_;
    uint32_t bytes_;
    RegType type_;
    uint32_t index_;
    uint32_t offset_;
  };

  // Defines how breakpoints work.
  // code_ points to a series of bytes which will be placed in the code to
  // create the breakpoint.  size_ is the size of that array.  We use a 32b
  // size since the memory modification API only supports a 32b size.
  struct BPDef {
    uint32_t size_;
    uint8_t *code_;
  };

  // Returns the registered name of this ABI.
  const char *GetName() const;

  // Returns a pointer to the breakpoint definition.
  const BPDef *GetBreakpointDef() const;

  // Returns the size of the thread context.
  uint32_t GetContextSize() const;

  // Returns the number of registers visbible.
  uint32_t GetRegisterCount() const;

  // Returns a definition of the register at the provided index, or
  // NULL if it does not exist.
  const RegDef *GetRegisterDef(uint32_t index) const;

  // Returns a definition of the instruction pointer.
  const RegDef *GetInstPtrDef() const;

  const char *GetTargetXml() const {
    return target_xml_;
  }

  // Called to assign a set of register definitions to an ABI.
  // This function is non-reentrant.
  static void Register(const char *name, RegDef *defs,
                       uint32_t cnt, uint32_t ip, const BPDef *bp,
                       const char *target_xml);

  // Called to search the map for a matching Abi by name.
  // This function is reentrant.
  static const Abi *Find(const char *name);

  // Get the ABI of for the running application.
  static const Abi *Get();

 protected:
  const char *name_;
  const RegDef *regDefs_;
  uint32_t regCnt_;
  uint32_t ctxSize_;
  const BPDef *bpDef_;
  uint32_t ipIndex_;
  const char *target_xml_;

 private:
  Abi();
  ~Abi();
  void operator =(const Abi&);
};

typedef std::map<std::string, Abi*> AbiMap_t;

}  // namespace gdb_rsp

#endif  // NATIVE_CLIENT_GDB_RSP_ABI_H_

