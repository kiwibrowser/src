// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file is auto-generated from
//    gpu/config/process_json.py
// It's formatted by clang-format using chromium coding style:
//    clang-format -i -style=chromium filename
// DO NOT EDIT!

#ifndef GPU_CONFIG_GPU_CONTROL_LIST_TESTING_ENTRY_ENUMS_AUTOGEN_H_
#define GPU_CONFIG_GPU_CONTROL_LIST_TESTING_ENTRY_ENUMS_AUTOGEN_H_

namespace gpu {
enum GpuControlListTestingEntryEnum {
  kGpuControlListEntryTest_DetailedEntry = 0,
  kGpuControlListEntryTest_VendorOnAllOsEntry = 1,
  kGpuControlListEntryTest_VendorOnLinuxEntry = 2,
  kGpuControlListEntryTest_AllExceptNVidiaOnLinuxEntry = 3,
  kGpuControlListEntryTest_AllExceptIntelOnLinuxEntry = 4,
  kGpuControlListEntryTest_DateOnWindowsEntry = 5,
  kGpuControlListEntryTest_MultipleDevicesEntry = 6,
  kGpuControlListEntryTest_ChromeOSEntry = 7,
  kGpuControlListEntryTest_GlVersionGLESEntry = 8,
  kGpuControlListEntryTest_GlVersionANGLEEntry = 9,
  kGpuControlListEntryTest_GlVersionGLEntry = 10,
  kGpuControlListEntryTest_GlVendorEqual = 11,
  kGpuControlListEntryTest_GlVendorWithDot = 12,
  kGpuControlListEntryTest_GlRendererContains = 13,
  kGpuControlListEntryTest_GlRendererCaseInsensitive = 14,
  kGpuControlListEntryTest_GlExtensionsEndWith = 15,
  kGpuControlListEntryTest_OptimusEntry = 16,
  kGpuControlListEntryTest_AMDSwitchableEntry = 17,
  kGpuControlListEntryTest_DriverVendorBeginWith = 18,
  kGpuControlListEntryTest_LexicalDriverVersionEntry = 19,
  kGpuControlListEntryTest_NeedsMoreInfoEntry = 20,
  kGpuControlListEntryTest_NeedsMoreInfoForExceptionsEntry = 21,
  kGpuControlListEntryTest_NeedsMoreInfoForGlVersionEntry = 22,
  kGpuControlListEntryTest_FeatureTypeAllEntry = 23,
  kGpuControlListEntryTest_FeatureTypeAllEntryWithExceptions = 24,
  kGpuControlListEntryTest_SingleActiveGPU = 25,
  kGpuControlListEntryTest_MachineModelName = 26,
  kGpuControlListEntryTest_MachineModelNameException = 27,
  kGpuControlListEntryTest_MachineModelVersion = 28,
  kGpuControlListEntryTest_MachineModelVersionException = 29,
  kGpuControlListEntryDualGPUTest_CategoryAny_Intel = 30,
  kGpuControlListEntryDualGPUTest_CategoryAny_NVidia = 31,
  kGpuControlListEntryDualGPUTest_CategorySecondary = 32,
  kGpuControlListEntryDualGPUTest_CategoryPrimary = 33,
  kGpuControlListEntryDualGPUTest_CategoryDefault = 34,
  kGpuControlListEntryDualGPUTest_ActiveSecondaryGPU = 35,
  kGpuControlListEntryDualGPUTest_VendorOnlyActiveSecondaryGPU = 36,
  kGpuControlListEntryDualGPUTest_ActivePrimaryGPU = 37,
  kGpuControlListEntryDualGPUTest_VendorOnlyActivePrimaryGPU = 38,
  kGpuControlListEntryTest_PixelShaderVersion = 39,
  kGpuControlListEntryTest_OsVersionZeroLT = 40,
  kGpuControlListEntryTest_OsVersionZeroAny = 41,
  kGpuControlListEntryTest_OsComparisonAny = 42,
  kGpuControlListEntryTest_OsComparisonGE = 43,
  kGpuControlListEntryTest_ExceptionWithoutVendorId = 44,
  kGpuControlListEntryTest_MultiGpuStyleAMDSwitchableDiscrete = 45,
  kGpuControlListEntryTest_MultiGpuStyleAMDSwitchableIntegrated = 46,
  kGpuControlListEntryTest_InProcessGPU = 47,
  kGpuControlListEntryTest_SameGPUTwiceTest = 48,
  kGpuControlListEntryTest_NVidiaNumberingScheme = 49,
  kGpuControlListTest_NeedsMoreInfo = 50,
  kGpuControlListTest_NeedsMoreInfoForExceptions = 51,
  kGpuControlListTest_IgnorableEntries_0 = 52,
  kGpuControlListTest_IgnorableEntries_1 = 53,
  kGpuControlListTest_DisabledExtensionTest_0 = 54,
  kGpuControlListTest_DisabledExtensionTest_1 = 55,
  kGpuControlListEntryTest_DirectRendering = 56,
  kGpuControlListTest_LinuxKernelVersion = 57,
  kGpuControlListTest_TestGroup_0 = 58,
  kGpuControlListTest_TestGroup_1 = 59,
  kGpuControlListEntryTest_GpuSeries = 60,
  kGpuControlListEntryTest_GpuSeriesActive = 61,
  kGpuControlListEntryTest_GpuSeriesAny = 62,
  kGpuControlListEntryTest_GpuSeriesPrimary = 63,
  kGpuControlListEntryTest_GpuSeriesSecondary = 64,
  kGpuControlListEntryTest_GpuSeriesInException = 65,
  kGpuControlListEntryTest_MultipleDrivers = 66,
};
}  // namespace gpu

#endif  // GPU_CONFIG_GPU_CONTROL_LIST_TESTING_ENTRY_ENUMS_AUTOGEN_H_
