//===-- SIRegisterInfo.cpp - SI Register Information ---------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the SI implementation of the TargetRegisterInfo class.
//
//===----------------------------------------------------------------------===//


#include "SIRegisterInfo.h"
#include "AMDGPUTargetMachine.h"

using namespace llvm;

SIRegisterInfo::SIRegisterInfo(AMDGPUTargetMachine &tm,
    const TargetInstrInfo &tii)
: AMDGPURegisterInfo(tm, tii),
  TM(tm),
  TII(tii)
  { }

BitVector SIRegisterInfo::getReservedRegs(const MachineFunction &MF) const
{
  BitVector Reserved(getNumRegs());
  return Reserved;
}

unsigned SIRegisterInfo::getBinaryCode(unsigned reg) const
{
  switch (reg) {
    case AMDGPU::M0: return 124;
    case AMDGPU::SREG_LIT_0: return 128;
    default: return getHWRegNum(reg);
  }
}

const TargetRegisterClass *
SIRegisterInfo::getISARegClass(const TargetRegisterClass * rc) const
{
  switch (rc->getID()) {
  case AMDGPU::GPRF32RegClassID:
    return &AMDGPU::VReg_32RegClass;
  default: return rc;
  }
}

const TargetRegisterClass * SIRegisterInfo::getCFGStructurizerRegClass(
                                                                   MVT VT) const
{
  switch(VT.SimpleTy) {
    default:
    case MVT::i32: return &AMDGPU::VReg_32RegClass;
  }
}
#include "SIRegisterGetHWRegNum.inc"
