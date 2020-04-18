//===-- R600RegisterInfo.cpp - R600 Register Information ------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// The file contains the R600 implementation of the TargetRegisterInfo class.
//
//===----------------------------------------------------------------------===//

#include "R600RegisterInfo.h"
#include "AMDGPUTargetMachine.h"
#include "R600MachineFunctionInfo.h"

using namespace llvm;

R600RegisterInfo::R600RegisterInfo(AMDGPUTargetMachine &tm,
    const TargetInstrInfo &tii)
: AMDGPURegisterInfo(tm, tii),
  TM(tm),
  TII(tii)
  { }

BitVector R600RegisterInfo::getReservedRegs(const MachineFunction &MF) const
{
  BitVector Reserved(getNumRegs());
  const R600MachineFunctionInfo * MFI = MF.getInfo<R600MachineFunctionInfo>();

  Reserved.set(AMDGPU::ZERO);
  Reserved.set(AMDGPU::HALF);
  Reserved.set(AMDGPU::ONE);
  Reserved.set(AMDGPU::ONE_INT);
  Reserved.set(AMDGPU::NEG_HALF);
  Reserved.set(AMDGPU::NEG_ONE);
  Reserved.set(AMDGPU::PV_X);
  Reserved.set(AMDGPU::ALU_LITERAL_X);
  Reserved.set(AMDGPU::PREDICATE_BIT);
  Reserved.set(AMDGPU::PRED_SEL_OFF);
  Reserved.set(AMDGPU::PRED_SEL_ZERO);
  Reserved.set(AMDGPU::PRED_SEL_ONE);

  for (TargetRegisterClass::iterator I = AMDGPU::R600_CReg32RegClass.begin(),
                        E = AMDGPU::R600_CReg32RegClass.end(); I != E; ++I) {
    Reserved.set(*I);
  }

  for (std::vector<unsigned>::const_iterator I = MFI->ReservedRegs.begin(),
                                    E = MFI->ReservedRegs.end(); I != E; ++I) {
    Reserved.set(*I);
  }

  return Reserved;
}

const TargetRegisterClass *
R600RegisterInfo::getISARegClass(const TargetRegisterClass * rc) const
{
  switch (rc->getID()) {
  case AMDGPU::GPRF32RegClassID:
  case AMDGPU::GPRI32RegClassID:
    return &AMDGPU::R600_Reg32RegClass;
  default: return rc;
  }
}

unsigned R600RegisterInfo::getHWRegIndex(unsigned reg) const
{
  switch(reg) {
  case AMDGPU::ZERO: return 248;
  case AMDGPU::ONE:
  case AMDGPU::NEG_ONE: return 249;
  case AMDGPU::ONE_INT: return 250;
  case AMDGPU::HALF:
  case AMDGPU::NEG_HALF: return 252;
  case AMDGPU::ALU_LITERAL_X: return 253;
  case AMDGPU::PREDICATE_BIT:
  case AMDGPU::PRED_SEL_OFF:
  case AMDGPU::PRED_SEL_ZERO:
  case AMDGPU::PRED_SEL_ONE:
    return 0;
  default: return getHWRegIndexGen(reg);
  }
}

unsigned R600RegisterInfo::getHWRegChan(unsigned reg) const
{
  switch(reg) {
  case AMDGPU::ZERO:
  case AMDGPU::ONE:
  case AMDGPU::ONE_INT:
  case AMDGPU::NEG_ONE:
  case AMDGPU::HALF:
  case AMDGPU::NEG_HALF:
  case AMDGPU::ALU_LITERAL_X:
  case AMDGPU::PREDICATE_BIT:
  case AMDGPU::PRED_SEL_OFF:
  case AMDGPU::PRED_SEL_ZERO:
  case AMDGPU::PRED_SEL_ONE:
    return 0;
  default: return getHWRegChanGen(reg);
  }
}

const TargetRegisterClass * R600RegisterInfo::getCFGStructurizerRegClass(
                                                                   MVT VT) const
{
  switch(VT.SimpleTy) {
  default:
  case MVT::i32: return &AMDGPU::R600_TReg32RegClass;
  }
}

unsigned R600RegisterInfo::getSubRegFromChannel(unsigned Channel) const
{
  switch (Channel) {
    default: assert(!"Invalid channel index"); return 0;
    case 0: return AMDGPU::sel_x;
    case 1: return AMDGPU::sel_y;
    case 2: return AMDGPU::sel_z;
    case 3: return AMDGPU::sel_w;
  }
}

#include "R600HwRegInfo.include"
