//===- AMDGPUMCInstLower.h MachineInstr Lowering Interface ------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef AMDGPU_MCINSTLOWER_H
#define AMDGPU_MCINSTLOWER_H

namespace llvm {

class MCInst;
class MachineInstr;

class AMDGPUMCInstLower {

public:
  AMDGPUMCInstLower();

  /// lower - Lower a MachineInstr to an MCInst
  void lower(const MachineInstr *MI, MCInst &OutMI) const;

};

} // End namespace llvm

#endif //AMDGPU_MCINSTLOWER_H
