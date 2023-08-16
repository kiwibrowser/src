// Copyright 2019 The Dawn Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "common/GPUInfo.h"

namespace gpu_info {
    bool IsAMD(PCIVendorID vendorId) {
        return vendorId == kVendorID_AMD;
    }
    bool IsARM(PCIVendorID vendorId) {
        return vendorId == kVendorID_ARM;
    }
    bool IsImgTec(PCIVendorID vendorId) {
        return vendorId == kVendorID_ImgTec;
    }
    bool IsIntel(PCIVendorID vendorId) {
        return vendorId == kVendorID_Intel;
    }
    bool IsNvidia(PCIVendorID vendorId) {
        return vendorId == kVendorID_Nvidia;
    }
    bool IsQualcomm(PCIVendorID vendorId) {
        return vendorId == kVendorID_Qualcomm;
    }
    bool IsSwiftshader(PCIVendorID vendorId, PCIDeviceID deviceId) {
        return vendorId == kVendorID_Google && deviceId == kDeviceID_Swiftshader;
    }
    bool IsWARP(PCIVendorID vendorId, PCIDeviceID deviceId) {
        return vendorId == kVendorID_Microsoft && deviceId == kDeviceID_WARP;
    }
}  // namespace gpu_info
