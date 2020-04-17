// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file is auto-generated from
//    gpu/config/process_json.py
// It's formatted by clang-format using chromium coding style:
//    clang-format -i -style=chromium filename
// DO NOT EDIT!

#ifndef GPU_CONFIG_GPU_CONTROL_LIST_TESTING_ARRAYS_AND_STRUCTS_AUTOGEN_H_
#define GPU_CONFIG_GPU_CONTROL_LIST_TESTING_ARRAYS_AND_STRUCTS_AUTOGEN_H_

#include "gpu/config/gpu_control_list_testing_data.h"

namespace gpu {
const int kFeatureListForEntry1[1] = {
    TEST_FEATURE_0,
};

const char* const kDisabledExtensionsForEntry1[2] = {
    "test_extension1", "test_extension2",
};

const uint32_t kCrBugsForEntry1[2] = {
    1024, 678,
};

const uint32_t kDeviceIDsForEntry1[1] = {
    0x0640,
};

const GpuControlList::DriverInfo kDriverInfoForEntry1 = {
    nullptr,  // driver_vendor
    {GpuControlList::kEQ, GpuControlList::kVersionStyleNumerical, "1.6.18",
     nullptr},  // driver_version
    {GpuControlList::kUnknown, GpuControlList::kVersionStyleNumerical, nullptr,
     nullptr},  // driver_date
};

const int kFeatureListForEntry2[1] = {
    TEST_FEATURE_0,
};

const int kFeatureListForEntry3[1] = {
    TEST_FEATURE_0,
};

const int kFeatureListForEntry4[1] = {
    TEST_FEATURE_0,
};

const int kFeatureListForEntry5[1] = {
    TEST_FEATURE_0,
};

const int kFeatureListForEntry6[1] = {
    TEST_FEATURE_0,
};

const GpuControlList::DriverInfo kDriverInfoForEntry6 = {
    nullptr,  // driver_vendor
    {GpuControlList::kUnknown, GpuControlList::kVersionStyleNumerical, nullptr,
     nullptr},  // driver_version
    {GpuControlList::kLT, GpuControlList::kVersionStyleNumerical, "2010.5.8",
     nullptr},  // driver_date
};

const int kFeatureListForEntry7[1] = {
    TEST_FEATURE_0,
};

const uint32_t kDeviceIDsForEntry7[2] = {
    0x1023, 0x0640,
};

const int kFeatureListForEntry8[1] = {
    TEST_FEATURE_0,
};

const int kFeatureListForEntry9[1] = {
    TEST_FEATURE_0,
};

const GpuControlList::More kMoreForEntry9 = {
    GpuControlList::kGLTypeGLES,  // gl_type
    {GpuControlList::kEQ, GpuControlList::kVersionStyleNumerical, "3.0",
     nullptr},  // gl_version
    {GpuControlList::kUnknown, GpuControlList::kVersionStyleNumerical, nullptr,
     nullptr},  // pixel_shader_version
    false,      // in_process_gpu
    0,          // gl_reset_notification_strategy
    true,       // direct_rendering
    {GpuControlList::kUnknown, GpuControlList::kVersionStyleNumerical, nullptr,
     nullptr},  // gpu_count
    0,          // test_group
};

const int kFeatureListForEntry10[1] = {
    TEST_FEATURE_0,
};

const GpuControlList::More kMoreForEntry10 = {
    GpuControlList::kGLTypeANGLE,  // gl_type
    {GpuControlList::kGT, GpuControlList::kVersionStyleNumerical, "2.0",
     nullptr},  // gl_version
    {GpuControlList::kUnknown, GpuControlList::kVersionStyleNumerical, nullptr,
     nullptr},  // pixel_shader_version
    false,      // in_process_gpu
    0,          // gl_reset_notification_strategy
    true,       // direct_rendering
    {GpuControlList::kUnknown, GpuControlList::kVersionStyleNumerical, nullptr,
     nullptr},  // gpu_count
    0,          // test_group
};

const int kFeatureListForEntry11[1] = {
    TEST_FEATURE_0,
};

const GpuControlList::More kMoreForEntry11 = {
    GpuControlList::kGLTypeGL,  // gl_type
    {GpuControlList::kLT, GpuControlList::kVersionStyleNumerical, "4.0",
     nullptr},  // gl_version
    {GpuControlList::kUnknown, GpuControlList::kVersionStyleNumerical, nullptr,
     nullptr},  // pixel_shader_version
    false,      // in_process_gpu
    0,          // gl_reset_notification_strategy
    true,       // direct_rendering
    {GpuControlList::kUnknown, GpuControlList::kVersionStyleNumerical, nullptr,
     nullptr},  // gpu_count
    0,          // test_group
};

const int kFeatureListForEntry12[1] = {
    TEST_FEATURE_0,
};

const GpuControlList::GLStrings kGLStringsForEntry12 = {
    "NVIDIA", nullptr, nullptr, nullptr,
};

const int kFeatureListForEntry13[1] = {
    TEST_FEATURE_0,
};

const GpuControlList::GLStrings kGLStringsForEntry13 = {
    "X\\.Org.*", nullptr, nullptr, nullptr,
};

const int kFeatureListForEntry14[1] = {
    TEST_FEATURE_0,
};

const GpuControlList::GLStrings kGLStringsForEntry14 = {
    nullptr, ".*GeForce.*", nullptr, nullptr,
};

const int kFeatureListForEntry15[1] = {
    TEST_FEATURE_0,
};

const GpuControlList::GLStrings kGLStringsForEntry15 = {
    nullptr, "(?i).*software.*", nullptr, nullptr,
};

const int kFeatureListForEntry16[1] = {
    TEST_FEATURE_0,
};

const GpuControlList::GLStrings kGLStringsForEntry16 = {
    nullptr, nullptr, ".*GL_SUN_slice_accum", nullptr,
};

const int kFeatureListForEntry17[1] = {
    TEST_FEATURE_0,
};

const int kFeatureListForEntry18[1] = {
    TEST_FEATURE_0,
};

const int kFeatureListForEntry19[1] = {
    TEST_FEATURE_0,
};

const GpuControlList::DriverInfo kDriverInfoForEntry19 = {
    "NVIDIA.*",  // driver_vendor
    {GpuControlList::kUnknown, GpuControlList::kVersionStyleNumerical, nullptr,
     nullptr},  // driver_version
    {GpuControlList::kUnknown, GpuControlList::kVersionStyleNumerical, nullptr,
     nullptr},  // driver_date
};

const int kFeatureListForEntry20[1] = {
    TEST_FEATURE_0,
};

const GpuControlList::DriverInfo kDriverInfoForEntry20 = {
    nullptr,  // driver_vendor
    {GpuControlList::kEQ, GpuControlList::kVersionStyleLexical, "8.76",
     nullptr},  // driver_version
    {GpuControlList::kUnknown, GpuControlList::kVersionStyleNumerical, nullptr,
     nullptr},  // driver_date
};

const int kFeatureListForEntry21[1] = {
    TEST_FEATURE_1,
};

const GpuControlList::DriverInfo kDriverInfoForEntry21 = {
    nullptr,  // driver_vendor
    {GpuControlList::kLT, GpuControlList::kVersionStyleNumerical, "10.7",
     nullptr},  // driver_version
    {GpuControlList::kUnknown, GpuControlList::kVersionStyleNumerical, nullptr,
     nullptr},  // driver_date
};

const int kFeatureListForEntry22[1] = {
    TEST_FEATURE_1,
};

const GpuControlList::GLStrings kGLStringsForEntry22Exception0 = {
    nullptr, ".*mesa.*", nullptr, nullptr,
};

const int kFeatureListForEntry23[1] = {
    TEST_FEATURE_1,
};

const GpuControlList::More kMoreForEntry23 = {
    GpuControlList::kGLTypeGL,  // gl_type
    {GpuControlList::kLT, GpuControlList::kVersionStyleNumerical, "3.5",
     nullptr},  // gl_version
    {GpuControlList::kUnknown, GpuControlList::kVersionStyleNumerical, nullptr,
     nullptr},  // pixel_shader_version
    false,      // in_process_gpu
    0,          // gl_reset_notification_strategy
    true,       // direct_rendering
    {GpuControlList::kUnknown, GpuControlList::kVersionStyleNumerical, nullptr,
     nullptr},  // gpu_count
    0,          // test_group
};

const int kFeatureListForEntry24[3] = {
    TEST_FEATURE_0, TEST_FEATURE_1, TEST_FEATURE_2,
};

const int kFeatureListForEntry25[2] = {
    TEST_FEATURE_1, TEST_FEATURE_2,
};

const int kFeatureListForEntry26[1] = {
    TEST_FEATURE_0,
};

const uint32_t kDeviceIDsForEntry26[1] = {
    0x0640,
};

const int kFeatureListForEntry27[1] = {
    TEST_FEATURE_0,
};

const char* const kMachineModelNameForEntry27[4] = {
    "Nexus 4", "XT1032", "GT-.*", "SCH-.*",
};

const GpuControlList::MachineModelInfo kMachineModelInfoForEntry27 = {
    base::size(kMachineModelNameForEntry27),  // machine model name size
    kMachineModelNameForEntry27,              // machine model names
    {GpuControlList::kUnknown, GpuControlList::kVersionStyleNumerical, nullptr,
     nullptr},  // machine model version
};

const int kFeatureListForEntry28[1] = {
    TEST_FEATURE_0,
};

const char* const kMachineModelNameForEntry28Exception0[1] = {
    "Nexus.*",
};

const GpuControlList::MachineModelInfo kMachineModelInfoForEntry28Exception0 = {
    base::size(
        kMachineModelNameForEntry28Exception0),  // machine model name size
    kMachineModelNameForEntry28Exception0,       // machine model names
    {GpuControlList::kUnknown, GpuControlList::kVersionStyleNumerical, nullptr,
     nullptr},  // machine model version
};

const int kFeatureListForEntry29[1] = {
    TEST_FEATURE_0,
};

const char* const kMachineModelNameForEntry29[1] = {
    "MacBookPro",
};

const GpuControlList::MachineModelInfo kMachineModelInfoForEntry29 = {
    base::size(kMachineModelNameForEntry29),  // machine model name size
    kMachineModelNameForEntry29,              // machine model names
    {GpuControlList::kEQ, GpuControlList::kVersionStyleNumerical, "7.1",
     nullptr},  // machine model version
};

const int kFeatureListForEntry30[1] = {
    TEST_FEATURE_0,
};

const char* const kMachineModelNameForEntry30[1] = {
    "MacBookPro",
};

const GpuControlList::MachineModelInfo kMachineModelInfoForEntry30 = {
    base::size(kMachineModelNameForEntry30),  // machine model name size
    kMachineModelNameForEntry30,              // machine model names
    {GpuControlList::kUnknown, GpuControlList::kVersionStyleNumerical, nullptr,
     nullptr},  // machine model version
};

const GpuControlList::MachineModelInfo kMachineModelInfoForEntry30Exception0 = {
    0,        // machine model name size
    nullptr,  // machine model names
    {GpuControlList::kGT, GpuControlList::kVersionStyleNumerical, "7.1",
     nullptr},  // machine model version
};

const int kFeatureListForEntry31[1] = {
    TEST_FEATURE_0,
};

const uint32_t kDeviceIDsForEntry31[1] = {
    0x0166,
};

const int kFeatureListForEntry32[1] = {
    TEST_FEATURE_0,
};

const uint32_t kDeviceIDsForEntry32[1] = {
    0x0640,
};

const int kFeatureListForEntry33[1] = {
    TEST_FEATURE_0,
};

const uint32_t kDeviceIDsForEntry33[1] = {
    0x0166,
};

const int kFeatureListForEntry34[1] = {
    TEST_FEATURE_0,
};

const uint32_t kDeviceIDsForEntry34[1] = {
    0x0166,
};

const int kFeatureListForEntry35[1] = {
    TEST_FEATURE_0,
};

const uint32_t kDeviceIDsForEntry35[1] = {
    0x0166,
};

const int kFeatureListForEntry36[1] = {
    TEST_FEATURE_0,
};

const uint32_t kDeviceIDsForEntry36[2] = {
    0x0166, 0x0168,
};

const int kFeatureListForEntry37[1] = {
    TEST_FEATURE_0,
};

const int kFeatureListForEntry38[1] = {
    TEST_FEATURE_0,
};

const uint32_t kDeviceIDsForEntry38[1] = {
    0x0640,
};

const int kFeatureListForEntry39[1] = {
    TEST_FEATURE_0,
};

const int kFeatureListForEntry40[1] = {
    TEST_FEATURE_0,
};

const GpuControlList::More kMoreForEntry40 = {
    GpuControlList::kGLTypeNone,  // gl_type
    {GpuControlList::kUnknown, GpuControlList::kVersionStyleNumerical, nullptr,
     nullptr},  // gl_version
    {GpuControlList::kLT, GpuControlList::kVersionStyleNumerical, "4.1",
     nullptr},  // pixel_shader_version
    false,      // in_process_gpu
    0,          // gl_reset_notification_strategy
    true,       // direct_rendering
    {GpuControlList::kUnknown, GpuControlList::kVersionStyleNumerical, nullptr,
     nullptr},  // gpu_count
    0,          // test_group
};

const int kFeatureListForEntry41[1] = {
    TEST_FEATURE_0,
};

const int kFeatureListForEntry42[1] = {
    TEST_FEATURE_0,
};

const int kFeatureListForEntry43[1] = {
    TEST_FEATURE_0,
};

const int kFeatureListForEntry44[1] = {
    TEST_FEATURE_0,
};

const int kFeatureListForEntry45[1] = {
    TEST_FEATURE_0,
};

const uint32_t kDeviceIDsForEntry45Exception0[1] = {
    0x2a06,
};

const GpuControlList::DriverInfo kDriverInfoForEntry45Exception0 = {
    nullptr,  // driver_vendor
    {GpuControlList::kGE, GpuControlList::kVersionStyleNumerical, "8.1",
     nullptr},  // driver_version
    {GpuControlList::kUnknown, GpuControlList::kVersionStyleNumerical, nullptr,
     nullptr},  // driver_date
};

const uint32_t kDeviceIDsForEntry45Exception1[1] = {
    0x2a02,
};

const GpuControlList::DriverInfo kDriverInfoForEntry45Exception1 = {
    nullptr,  // driver_vendor
    {GpuControlList::kGE, GpuControlList::kVersionStyleNumerical, "9.1",
     nullptr},  // driver_version
    {GpuControlList::kUnknown, GpuControlList::kVersionStyleNumerical, nullptr,
     nullptr},  // driver_date
};

const int kFeatureListForEntry46[1] = {
    TEST_FEATURE_0,
};

const int kFeatureListForEntry47[1] = {
    TEST_FEATURE_0,
};

const int kFeatureListForEntry48[1] = {
    TEST_FEATURE_0,
};

const GpuControlList::More kMoreForEntry48 = {
    GpuControlList::kGLTypeNone,  // gl_type
    {GpuControlList::kUnknown, GpuControlList::kVersionStyleNumerical, nullptr,
     nullptr},  // gl_version
    {GpuControlList::kUnknown, GpuControlList::kVersionStyleNumerical, nullptr,
     nullptr},  // pixel_shader_version
    true,       // in_process_gpu
    0,          // gl_reset_notification_strategy
    true,       // direct_rendering
    {GpuControlList::kUnknown, GpuControlList::kVersionStyleNumerical, nullptr,
     nullptr},  // gpu_count
    0,          // test_group
};

const int kFeatureListForEntry49[1] = {
    TEST_FEATURE_0,
};

const int kFeatureListForEntry50[1] = {
    TEST_FEATURE_0,
};

const GpuControlList::DriverInfo kDriverInfoForEntry50 = {
    nullptr,  // driver_vendor
    {GpuControlList::kLE, GpuControlList::kVersionStyleNumerical,
     "8.17.12.6973", nullptr},  // driver_version
    {GpuControlList::kUnknown, GpuControlList::kVersionStyleNumerical, nullptr,
     nullptr},  // driver_date
};

const int kFeatureListForEntry51[1] = {
    TEST_FEATURE_0,
};

const GpuControlList::DriverInfo kDriverInfoForEntry51 = {
    nullptr,  // driver_vendor
    {GpuControlList::kLT, GpuControlList::kVersionStyleNumerical, "12",
     nullptr},  // driver_version
    {GpuControlList::kUnknown, GpuControlList::kVersionStyleNumerical, nullptr,
     nullptr},  // driver_date
};

const int kFeatureListForEntry52[1] = {
    TEST_FEATURE_0,
};

const GpuControlList::GLStrings kGLStringsForEntry52Exception0 = {
    nullptr, ".*mesa.*", nullptr, nullptr,
};

const int kFeatureListForEntry53[1] = {
    TEST_FEATURE_0,
};

const int kFeatureListForEntry54[1] = {
    TEST_FEATURE_0,
};

const GpuControlList::DriverInfo kDriverInfoForEntry54 = {
    nullptr,  // driver_vendor
    {GpuControlList::kLT, GpuControlList::kVersionStyleNumerical, "10.7",
     nullptr},  // driver_version
    {GpuControlList::kUnknown, GpuControlList::kVersionStyleNumerical, nullptr,
     nullptr},  // driver_date
};

const char* const kDisabledExtensionsForEntry55[2] = {
    "test_extension2", "test_extension1",
};

const char* const kDisabledExtensionsForEntry56[2] = {
    "test_extension3", "test_extension2",
};

const int kFeatureListForEntry57[1] = {
    TEST_FEATURE_1,
};

const GpuControlList::More kMoreForEntry57 = {
    GpuControlList::kGLTypeNone,  // gl_type
    {GpuControlList::kUnknown, GpuControlList::kVersionStyleNumerical, nullptr,
     nullptr},  // gl_version
    {GpuControlList::kUnknown, GpuControlList::kVersionStyleNumerical, nullptr,
     nullptr},  // pixel_shader_version
    false,      // in_process_gpu
    0,          // gl_reset_notification_strategy
    false,      // direct_rendering
    {GpuControlList::kUnknown, GpuControlList::kVersionStyleNumerical, nullptr,
     nullptr},  // gpu_count
    0,          // test_group
};

const int kFeatureListForEntry58[1] = {
    TEST_FEATURE_0,
};

const int kFeatureListForEntry59[1] = {
    TEST_FEATURE_0,
};

const GpuControlList::More kMoreForEntry59 = {
    GpuControlList::kGLTypeNone,  // gl_type
    {GpuControlList::kUnknown, GpuControlList::kVersionStyleNumerical, nullptr,
     nullptr},  // gl_version
    {GpuControlList::kUnknown, GpuControlList::kVersionStyleNumerical, nullptr,
     nullptr},  // pixel_shader_version
    false,      // in_process_gpu
    0,          // gl_reset_notification_strategy
    true,       // direct_rendering
    {GpuControlList::kUnknown, GpuControlList::kVersionStyleNumerical, nullptr,
     nullptr},  // gpu_count
    1,          // test_group
};

const int kFeatureListForEntry60[1] = {
    TEST_FEATURE_1,
};

const GpuControlList::More kMoreForEntry60 = {
    GpuControlList::kGLTypeNone,  // gl_type
    {GpuControlList::kUnknown, GpuControlList::kVersionStyleNumerical, nullptr,
     nullptr},  // gl_version
    {GpuControlList::kUnknown, GpuControlList::kVersionStyleNumerical, nullptr,
     nullptr},  // pixel_shader_version
    false,      // in_process_gpu
    0,          // gl_reset_notification_strategy
    true,       // direct_rendering
    {GpuControlList::kUnknown, GpuControlList::kVersionStyleNumerical, nullptr,
     nullptr},  // gpu_count
    2,          // test_group
};

const int kFeatureListForEntry61[1] = {
    TEST_FEATURE_0,
};

const GpuControlList::GpuSeriesType kGpuSeriesForEntry61[2] = {
    GpuControlList::GpuSeriesType::kIntelSkyLake,
    GpuControlList::GpuSeriesType::kIntelKabyLake,
};

const int kFeatureListForEntry62[1] = {
    TEST_FEATURE_0,
};

const GpuControlList::GpuSeriesType kGpuSeriesForEntry62[1] = {
    GpuControlList::GpuSeriesType::kIntelKabyLake,
};

const int kFeatureListForEntry63[1] = {
    TEST_FEATURE_0,
};

const GpuControlList::GpuSeriesType kGpuSeriesForEntry63[1] = {
    GpuControlList::GpuSeriesType::kIntelKabyLake,
};

const int kFeatureListForEntry64[1] = {
    TEST_FEATURE_0,
};

const GpuControlList::GpuSeriesType kGpuSeriesForEntry64[1] = {
    GpuControlList::GpuSeriesType::kIntelKabyLake,
};

const int kFeatureListForEntry65[1] = {
    TEST_FEATURE_0,
};

const GpuControlList::GpuSeriesType kGpuSeriesForEntry65[1] = {
    GpuControlList::GpuSeriesType::kIntelKabyLake,
};

const int kFeatureListForEntry66[1] = {
    TEST_FEATURE_0,
};

const GpuControlList::GpuSeriesType kGpuSeriesForEntry66Exception0[1] = {
    GpuControlList::GpuSeriesType::kIntelKabyLake,
};

const int kFeatureListForEntry67[1] = {
    TEST_FEATURE_0,
};

const GpuControlList::DriverInfo kDriverInfoForEntry67 = {
    nullptr,  // driver_vendor
    {GpuControlList::kLE, GpuControlList::kVersionStyleNumerical,
     "8.15.10.2702", nullptr},  // driver_version
    {GpuControlList::kUnknown, GpuControlList::kVersionStyleNumerical, nullptr,
     nullptr},  // driver_date
};

}  // namespace gpu

#endif  // GPU_CONFIG_GPU_CONTROL_LIST_TESTING_ARRAYS_AND_STRUCTS_AUTOGEN_H_
