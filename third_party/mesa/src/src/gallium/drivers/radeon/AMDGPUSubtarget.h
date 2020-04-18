//=====-- AMDGPUSubtarget.h - Define Subtarget for the AMDIL ---*- C++ -*-====//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//==-----------------------------------------------------------------------===//
//
// This file declares the AMDGPU specific subclass of TargetSubtarget.
//
//===----------------------------------------------------------------------===//

#ifndef _AMDGPUSUBTARGET_H_
#define _AMDGPUSUBTARGET_H_
#include "AMDILDevice.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Target/TargetSubtargetInfo.h"

#define GET_SUBTARGETINFO_HEADER
#include "AMDGPUGenSubtargetInfo.inc"

#define MAX_CB_SIZE (1 << 16)

namespace llvm {

class AMDGPUSubtarget : public AMDGPUGenSubtargetInfo
{
private:
  bool CapsOverride[AMDGPUDeviceInfo::MaxNumberCapabilities];
  const AMDGPUDevice *mDevice;
  size_t mDefaultSize[3];
  size_t mMinimumSize[3];
  std::string mDevName;
  bool mIs64bit;
  bool mIs32on64bit;
  bool mDumpCode;

  InstrItineraryData InstrItins;

public:
  AMDGPUSubtarget(StringRef TT, StringRef CPU, StringRef FS);
  virtual ~AMDGPUSubtarget();

  const InstrItineraryData &getInstrItineraryData() const { return InstrItins; }
  virtual void ParseSubtargetFeatures(llvm::StringRef CPU, llvm::StringRef FS);

  bool isOverride(AMDGPUDeviceInfo::Caps) const;
  bool is64bit() const;

  // Helper functions to simplify if statements
  bool isTargetELF() const;
  const AMDGPUDevice* device() const;
  std::string getDataLayout() const;
  std::string getDeviceName() const;
  virtual size_t getDefaultSize(uint32_t dim) const;
  bool dumpCode() const { return mDumpCode; }

};

} // End namespace llvm

#endif // AMDGPUSUBTARGET_H_
