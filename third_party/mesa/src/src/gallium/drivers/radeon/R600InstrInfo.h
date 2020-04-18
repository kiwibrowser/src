//===-- R600InstrInfo.h - R600 Instruction Info Interface -------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Interface definition for R600InstrInfo
//
//===----------------------------------------------------------------------===//

#ifndef R600INSTRUCTIONINFO_H_
#define R600INSTRUCTIONINFO_H_

#include "AMDIL.h"
#include "AMDGPUInstrInfo.h"
#include "R600RegisterInfo.h"

#include <map>

namespace llvm {

  class AMDGPUTargetMachine;
  class DFAPacketizer;
  class ScheduleDAG;
  class MachineFunction;
  class MachineInstr;
  class MachineInstrBuilder;

  class R600InstrInfo : public AMDGPUInstrInfo {
  private:
  const R600RegisterInfo RI;
  AMDGPUTargetMachine &TM;

  int getBranchInstr(const MachineOperand &op) const;

  public:
  explicit R600InstrInfo(AMDGPUTargetMachine &tm);

  const R600RegisterInfo &getRegisterInfo() const;
  virtual void copyPhysReg(MachineBasicBlock &MBB,
                           MachineBasicBlock::iterator MI, DebugLoc DL,
                           unsigned DestReg, unsigned SrcReg,
                           bool KillSrc) const;

  bool isTrig(const MachineInstr &MI) const;
  bool isPlaceHolderOpcode(unsigned opcode) const;
  bool isReductionOp(unsigned opcode) const;
  bool isCubeOp(unsigned opcode) const;

  /// isVector - Vector instructions are instructions that must fill all
  /// instruction slots within an instruction group.
  bool isVector(const MachineInstr &MI) const;

  virtual MachineInstr * getMovImmInstr(MachineFunction *MF, unsigned DstReg,
                                        int64_t Imm) const;

  virtual unsigned getIEQOpcode() const;
  virtual bool isMov(unsigned Opcode) const;

  DFAPacketizer *CreateTargetScheduleState(const TargetMachine *TM,
                                           const ScheduleDAG *DAG) const;

  bool ReverseBranchCondition(SmallVectorImpl<MachineOperand> &Cond) const;

  bool AnalyzeBranch(MachineBasicBlock &MBB, MachineBasicBlock *&TBB, MachineBasicBlock *&FBB,
                     SmallVectorImpl<MachineOperand> &Cond, bool AllowModify) const;

  unsigned InsertBranch(MachineBasicBlock &MBB, MachineBasicBlock *TBB, MachineBasicBlock *FBB, const SmallVectorImpl<MachineOperand> &Cond, DebugLoc DL) const;

  unsigned RemoveBranch(MachineBasicBlock &MBB) const;

  bool isPredicated(const MachineInstr *MI) const;

  bool isPredicable(MachineInstr *MI) const;

  bool
   isProfitableToDupForIfCvt(MachineBasicBlock &MBB, unsigned NumCyles,
                             const BranchProbability &Probability) const;

  bool isProfitableToIfCvt(MachineBasicBlock &MBB, unsigned NumCyles,
                           unsigned ExtraPredCycles,
                           const BranchProbability &Probability) const ;

  bool
   isProfitableToIfCvt(MachineBasicBlock &TMBB,
                       unsigned NumTCycles, unsigned ExtraTCycles,
                       MachineBasicBlock &FMBB,
                       unsigned NumFCycles, unsigned ExtraFCycles,
                       const BranchProbability &Probability) const;

  bool DefinesPredicate(MachineInstr *MI,
                                  std::vector<MachineOperand> &Pred) const;

  bool SubsumesPredicate(const SmallVectorImpl<MachineOperand> &Pred1,
                         const SmallVectorImpl<MachineOperand> &Pred2) const;

  bool isProfitableToUnpredicate(MachineBasicBlock &TMBB,
                                          MachineBasicBlock &FMBB) const;

  bool PredicateInstruction(MachineInstr *MI,
                        const SmallVectorImpl<MachineOperand> &Pred) const;

  int getInstrLatency(const InstrItineraryData *ItinData,
                              const MachineInstr *MI,
                              unsigned *PredCost = 0) const;

  virtual int getInstrLatency(const InstrItineraryData *ItinData,
                              SDNode *Node) const { return 1;}

  ///hasFlagOperand - Returns true if this instruction has an operand for
  /// storing target flags.
  bool hasFlagOperand(const MachineInstr &MI) const;

  ///addFlag - Add one of the MO_FLAG* flags to the specified Operand.
  void addFlag(MachineInstr *MI, unsigned Operand, unsigned Flag) const;

  ///isFlagSet - Determine if the specified flag is set on this Operand.
  bool isFlagSet(const MachineInstr &MI, unsigned Operand, unsigned Flag) const;

  ///getFlagOp - Return the operand containing the flags for this instruction.
  MachineOperand &getFlagOp(MachineInstr *MI) const;

  ///clearFlag - Clear the specified flag on the instruction.
  void clearFlag(MachineInstr *MI, unsigned Operand, unsigned Flag) const;
};

} // End llvm namespace

namespace R600_InstFlag {
	enum TIF {
		TRANS_ONLY = (1 << 0),
		TEX = (1 << 1),
		REDUCTION = (1 << 2),
		FC = (1 << 3),
		TRIG = (1 << 4),
		OP3 = (1 << 5),
		VECTOR = (1 << 6)
    //FlagOperand bits 7, 8
	};
}

#endif // R600INSTRINFO_H_
