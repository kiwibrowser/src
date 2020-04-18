//===- subzero/src/IceTargetLoweringX8632.h - x86-32 lowering ---*- C++ -*-===//
//
//                        The Subzero Code Generator
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// \brief Declares the TargetLoweringX8632 class, which implements the
/// TargetLowering interface for the x86-32 architecture.
///
//===----------------------------------------------------------------------===//

#ifndef SUBZERO_SRC_ICETARGETLOWERINGX8632_H
#define SUBZERO_SRC_ICETARGETLOWERINGX8632_H

#include "IceAssemblerX8632.h"
#include "IceDefs.h"
#include "IceRegistersX8632.h"
#include "IceTargetLowering.h"
#include "IceInstX8632.h"
#define X86NAMESPACE X8632
#include "IceTargetLoweringX86Base.h"
#undef X86NAMESPACE
#include "IceTargetLoweringX8632Traits.h"

namespace Ice {
namespace X8632 {

class TargetX8632 final : public ::Ice::X8632::TargetX86Base<X8632::Traits> {
  TargetX8632() = delete;
  TargetX8632(const TargetX8632 &) = delete;
  TargetX8632 &operator=(const TargetX8632 &) = delete;

public:
  ~TargetX8632() = default;

  static std::unique_ptr<::Ice::TargetLowering> create(Cfg *Func) {
    return makeUnique<TargetX8632>(Func);
  }

  std::unique_ptr<::Ice::Assembler> createAssembler() const override {
    return makeUnique<X8632::AssemblerX8632>();
  }

protected:
  void _add_sp(Operand *Adjustment);
  void _mov_sp(Operand *NewValue);
  Traits::X86OperandMem *_sandbox_mem_reference(X86OperandMem *Mem);
  void _sub_sp(Operand *Adjustment);
  void _link_bp();
  void _unlink_bp();
  void _push_reg(Variable *Reg);

  void initRebasePtr();
  void initSandbox();
  bool legalizeOptAddrForSandbox(OptAddr *Addr);
  void emitSandboxedReturn();
  void lowerIndirectJump(Variable *JumpTarget);
  void emitGetIP(CfgNode *Node);
  Inst *emitCallToTarget(Operand *CallTarget, Variable *ReturnReg) override;
  Variable *moveReturnValueToRegister(Operand *Value, Type ReturnType) override;

private:
  ENABLE_MAKE_UNIQUE;
  friend class X8632::TargetX86Base<X8632::Traits>;

  explicit TargetX8632(Cfg *Func) : TargetX86Base(Func) {}

  Operand *createNaClReadTPSrcOperand() {
    Constant *Zero = Ctx->getConstantZero(IceType_i32);
    return Traits::X86OperandMem::create(Func, IceType_i32, nullptr, Zero,
                                         nullptr, 0,
                                         Traits::X86OperandMem::SegReg_GS);
  }
};

} // end of namespace X8632
} // end of namespace Ice

#endif // SUBZERO_SRC_ICETARGETLOWERINGX8632_H
