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

#ifndef COMMON_GPUINFO_H
#define COMMON_GPUINFO_H

#include <cstdint>

using PCIVendorID = uint32_t;
using PCIDeviceID = uint32_t;

namespace gpu_info {

    static constexpr PCIVendorID kVendorID_AMD = 0x1002;
    static constexpr PCIVendorID kVendorID_ARM = 0x13B5;
    static constexpr PCIVendorID kVendorID_ImgTec = 0x1010;
    static constexpr PCIVendorID kVendorID_Intel = 0x8086;
    static constexpr PCIVendorID kVendorID_Nvidia = 0x10DE;
    static constexpr PCIVendorID kVendorID_Qualcomm = 0x5143;
    static constexpr PCIVendorID kVendorID_Google = 0x1AE0;
    static constexpr PCIVendorID kVendorID_Microsoft = 0x1414;

    static constexpr PCIDeviceID kDeviceID_Swiftshader = 0xC0DE;
    static constexpr PCIDeviceID kDeviceID_WARP = 0x8c;

    bool IsAMD(PCIVendorID vendorId);
    bool IsARM(PCIVendorID vendorId);
    bool IsImgTec(PCIVendorID vendorId);
    bool IsIntel(PCIVendorID vendorId);
    bool IsNvidia(PCIVendorID vendorId);
    bool IsQualcomm(PCIVendorID vendorId);
    bool IsSwiftshader(PCIVendorID vendorId, PCIDeviceID deviceId);
    bool IsWARP(PCIVendorID vendorId, PCIDeviceID deviceId);

}  // namespace gpu_info
#endif  // COMMON_GPUINFO_H
