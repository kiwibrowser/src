//===- AMDILIntrinsicInfo.cpp - AMDIL Intrinsic Information ------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//==-----------------------------------------------------------------------===//
//
// This file contains the AMDIL Implementation of the IntrinsicInfo class.
//
//===-----------------------------------------------------------------------===//

#include "AMDILIntrinsicInfo.h"
#include "AMDIL.h"
#include "AMDGPUSubtarget.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Intrinsics.h"
#include "llvm/Module.h"

using namespace llvm;

#define GET_LLVM_INTRINSIC_FOR_GCC_BUILTIN
#include "AMDGPUGenIntrinsics.inc"
#undef GET_LLVM_INTRINSIC_FOR_GCC_BUILTIN

AMDGPUIntrinsicInfo::AMDGPUIntrinsicInfo(TargetMachine *tm) 
  : TargetIntrinsicInfo()
{
}

std::string 
AMDGPUIntrinsicInfo::getName(unsigned int IntrID, Type **Tys,
    unsigned int numTys) const 
{
  static const char* const names[] = {
#define GET_INTRINSIC_NAME_TABLE
#include "AMDGPUGenIntrinsics.inc"
#undef GET_INTRINSIC_NAME_TABLE
  };

  //assert(!isOverloaded(IntrID)
  //&& "AMDGPU Intrinsics are not overloaded");
  if (IntrID < Intrinsic::num_intrinsics) {
    return 0;
  }
  assert(IntrID < AMDGPUIntrinsic::num_AMDGPU_intrinsics
      && "Invalid intrinsic ID");

  std::string Result(names[IntrID - Intrinsic::num_intrinsics]);
  return Result;
}

unsigned int
AMDGPUIntrinsicInfo::lookupName(const char *Name, unsigned int Len) const 
{
#define GET_FUNCTION_RECOGNIZER
#include "AMDGPUGenIntrinsics.inc"
#undef GET_FUNCTION_RECOGNIZER
  AMDGPUIntrinsic::ID IntrinsicID
    = (AMDGPUIntrinsic::ID)Intrinsic::not_intrinsic;
  IntrinsicID = getIntrinsicForGCCBuiltin("AMDIL", Name);

  if (IntrinsicID != (AMDGPUIntrinsic::ID)Intrinsic::not_intrinsic) {
    return IntrinsicID;
  }
  return 0;
}

bool 
AMDGPUIntrinsicInfo::isOverloaded(unsigned id) const 
{
  // Overload Table
#define GET_INTRINSIC_OVERLOAD_TABLE
#include "AMDGPUGenIntrinsics.inc"
#undef GET_INTRINSIC_OVERLOAD_TABLE
}

/// This defines the "getAttributes(ID id)" method.
#define GET_INTRINSIC_ATTRIBUTES
#include "AMDGPUGenIntrinsics.inc"
#undef GET_INTRINSIC_ATTRIBUTES

Function*
AMDGPUIntrinsicInfo::getDeclaration(Module *M, unsigned IntrID,
    Type **Tys,
    unsigned numTys) const 
{
  //Silence a warning
  AttrListPtr List = getAttributes((AMDGPUIntrinsic::ID)IntrID);
  (void)List;
  assert(!"Not implemented");
}
