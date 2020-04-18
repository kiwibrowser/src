//===------- AMDILSIDevice.h - Define SI Device for AMDIL -*- C++ -*------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//==-----------------------------------------------------------------------===//
//
// Interface for the subtarget data classes.
//
//===---------------------------------------------------------------------===//
// This file will define the interface that each generation needs to
// implement in order to correctly answer queries on the capabilities of the
// specific hardware.
//===---------------------------------------------------------------------===//
#ifndef _AMDILSIDEVICE_H_
#define _AMDILSIDEVICE_H_
#include "AMDILEvergreenDevice.h"
#include "AMDGPUSubtarget.h"

namespace llvm {
  class AMDGPUSubtarget;
//===---------------------------------------------------------------------===//
// SI generation of devices and their respective sub classes
//===---------------------------------------------------------------------===//

// The AMDGPUSIDevice is the base class for all Northern Island series of
// cards. It is very similiar to the AMDGPUEvergreenDevice, with the major
// exception being differences in wavefront size and hardware capabilities.  The
// SI devices are all 64 wide wavefronts and also add support for signed 24 bit
// integer operations

  class AMDGPUSIDevice : public AMDGPUEvergreenDevice {
    public:
      AMDGPUSIDevice(AMDGPUSubtarget*);
      virtual ~AMDGPUSIDevice();
      virtual size_t getMaxLDSSize() const;
      virtual uint32_t getGeneration() const;
      virtual std::string getDataLayout() const;
    protected:
  }; // AMDGPUSIDevice

} // namespace llvm
#endif // _AMDILSIDEVICE_H_
