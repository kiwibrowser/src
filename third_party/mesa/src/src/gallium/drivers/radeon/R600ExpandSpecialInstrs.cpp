//===-- R600ExpandSpecialInstrs.cpp - Expand special instructions ---------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
// Vector, Reduction, and Cube instructions need to fill the entire instruction
// group to work correctly.  This pass expands these individual instructions
// into several instructions that will completely fill the instruction group. 
//===----------------------------------------------------------------------===//

#include "AMDGPU.h"
#include "R600Defines.h"
#include "R600InstrInfo.h"
#include "R600RegisterInfo.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"

using namespace llvm;

namespace {

class R600ExpandSpecialInstrsPass : public MachineFunctionPass {

private:
  static char ID;
  const R600InstrInfo *TII;

public:
  R600ExpandSpecialInstrsPass(TargetMachine &tm) : MachineFunctionPass(ID),
    TII (static_cast<const R600InstrInfo *>(tm.getInstrInfo())) { }

  virtual bool runOnMachineFunction(MachineFunction &MF);

  const char *getPassName() const {
    return "R600 Expand special instructions pass";
  }
};

} // End anonymous namespace

char R600ExpandSpecialInstrsPass::ID = 0;

FunctionPass *llvm::createR600ExpandSpecialInstrsPass(TargetMachine &TM) {
  return new R600ExpandSpecialInstrsPass(TM);
}

bool R600ExpandSpecialInstrsPass::runOnMachineFunction(MachineFunction &MF) {

  const R600RegisterInfo &TRI = TII->getRegisterInfo();

  for (MachineFunction::iterator BB = MF.begin(), BB_E = MF.end();
                                                  BB != BB_E; ++BB) {
    MachineBasicBlock &MBB = *BB;
    MachineBasicBlock::iterator I = MBB.begin();
    while (I != MBB.end()) {
      MachineInstr &MI = *I;
      I = llvm::next(I);

      bool IsReduction = TII->isReductionOp(MI.getOpcode());
      bool IsVector = TII->isVector(MI);
	    bool IsCube = TII->isCubeOp(MI.getOpcode());
      if (!IsReduction && !IsVector && !IsCube) {
        continue;
      }

      // Expand the instruction
      //
      // Reduction instructions:
      // T0_X = DP4 T1_XYZW, T2_XYZW
      // becomes:
      // TO_X = DP4 T1_X, T2_X
      // TO_Y (write masked) = DP4 T1_Y, T2_Y
      // TO_Z (write masked) = DP4 T1_Z, T2_Z
      // TO_W (write masked) = DP4 T1_W, T2_W
      //
      // Vector instructions:
      // T0_X = MULLO_INT T1_X, T2_X
      // becomes:
      // T0_X = MULLO_INT T1_X, T2_X
      // T0_Y (write masked) = MULLO_INT T1_X, T2_X
      // T0_Z (write masked) = MULLO_INT T1_X, T2_X
      // T0_W (write masked) = MULLO_INT T1_X, T2_X
      //
      // Cube instructions:
      // T0_XYZW = CUBE T1_XYZW
      // becomes:
      // TO_X = CUBE T1_Z, T1_Y
      // T0_Y = CUBE T1_Z, T1_X
      // T0_Z = CUBE T1_X, T1_Z
      // T0_W = CUBE T1_Y, T1_Z
      for (unsigned Chan = 0; Chan < 4; Chan++) {
        unsigned DstReg = MI.getOperand(0).getReg();
        unsigned Src0 = MI.getOperand(1).getReg();
        unsigned Src1 = 0;

        // Determine the correct source registers
        if (!IsCube) {
          Src1 = MI.getOperand(2).getReg();
        }
        if (IsReduction) {
          unsigned SubRegIndex = TRI.getSubRegFromChannel(Chan);
          Src0 = TRI.getSubReg(Src0, SubRegIndex);
          Src1 = TRI.getSubReg(Src1, SubRegIndex);
        } else if (IsCube) {
          static const int CubeSrcSwz[] = {2, 2, 0, 1};
          unsigned SubRegIndex0 = TRI.getSubRegFromChannel(CubeSrcSwz[Chan]);
          unsigned SubRegIndex1 = TRI.getSubRegFromChannel(CubeSrcSwz[3 - Chan]);
          Src1 = TRI.getSubReg(Src0, SubRegIndex1);
          Src0 = TRI.getSubReg(Src0, SubRegIndex0);
        }

        // Determine the correct destination registers;
        unsigned Flags = 0;
        if (IsCube) {
          unsigned SubRegIndex = TRI.getSubRegFromChannel(Chan);
          DstReg = TRI.getSubReg(DstReg, SubRegIndex);
        } else {
          // Mask the write if the original instruction does not write to
          // the current Channel.
          Flags |= (Chan != TRI.getHWRegChan(DstReg) ? MO_FLAG_MASK : 0);
          unsigned DstBase = TRI.getHWRegIndex(DstReg);
          DstReg = AMDGPU::R600_TReg32RegClass.getRegister((DstBase * 4) + Chan);
        }

        // Set the IsLast bit
        Flags |= (Chan != 3 ? MO_FLAG_NOT_LAST : 0);

        // Add the new instruction
        unsigned Opcode;
        if (IsCube) {
          switch (MI.getOpcode()) {
          case AMDGPU::CUBE_r600_pseudo:
            Opcode = AMDGPU::CUBE_r600_real;
            break;
          case AMDGPU::CUBE_eg_pseudo:
            Opcode = AMDGPU::CUBE_eg_real;
            break;
          default:
            assert(!"Unknown CUBE instruction");
            Opcode = 0;
            break;
          }
        } else {
          Opcode = MI.getOpcode();
        }
        MachineInstr *NewMI =
          BuildMI(MBB, I, MBB.findDebugLoc(I), TII->get(Opcode), DstReg)
                  .addReg(Src0)
                  .addReg(Src1)
                  .addImm(0); // Flag

        NewMI->setIsInsideBundle(Chan != 0);
        TII->addFlag(NewMI, 0, Flags);
      }
      MI.eraseFromParent();
    }
  }
  return false;
}
