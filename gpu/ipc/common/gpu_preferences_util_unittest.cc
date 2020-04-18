// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <algorithm>
#include <cstring>

#include "build/build_config.h"
#include "gpu/ipc/common/gpu_preferences.mojom.h"
#include "gpu/ipc/common/gpu_preferences_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace gpu {

namespace {

void CheckGpuPreferencesEqual(GpuPreferences left, GpuPreferences right) {
  EXPECT_EQ(left.single_process, right.single_process);
  EXPECT_EQ(left.in_process_gpu, right.in_process_gpu);
  EXPECT_EQ(left.disable_accelerated_video_decode,
            right.disable_accelerated_video_decode);
  EXPECT_EQ(left.disable_accelerated_video_encode,
            right.disable_accelerated_video_encode);
  EXPECT_EQ(left.gpu_startup_dialog, right.gpu_startup_dialog);
  EXPECT_EQ(left.disable_gpu_watchdog, right.disable_gpu_watchdog);
  EXPECT_EQ(left.gpu_sandbox_start_early, right.gpu_sandbox_start_early);
  EXPECT_EQ(left.enable_accelerated_vpx_decode,
            right.enable_accelerated_vpx_decode);
  EXPECT_EQ(left.enable_low_latency_dxva, right.enable_low_latency_dxva);
  EXPECT_EQ(left.enable_zero_copy_dxgi_video,
            right.enable_zero_copy_dxgi_video);
  EXPECT_EQ(left.enable_nv12_dxgi_video, right.enable_nv12_dxgi_video);
  EXPECT_EQ(left.enable_media_foundation_vea_on_windows7,
            right.enable_media_foundation_vea_on_windows7);
  EXPECT_EQ(left.disable_software_rasterizer,
            right.disable_software_rasterizer);
  EXPECT_EQ(left.log_gpu_control_list_decisions,
            right.log_gpu_control_list_decisions);
  EXPECT_EQ(left.compile_shader_always_succeeds,
            right.compile_shader_always_succeeds);
  EXPECT_EQ(left.disable_gl_error_limit, right.disable_gl_error_limit);
  EXPECT_EQ(left.disable_glsl_translator, right.disable_glsl_translator);
  EXPECT_EQ(left.disable_shader_name_hashing,
            right.disable_shader_name_hashing);
  EXPECT_EQ(left.enable_gpu_command_logging, right.enable_gpu_command_logging);
  EXPECT_EQ(left.enable_gpu_debugging, right.enable_gpu_debugging);
  EXPECT_EQ(left.enable_gpu_service_logging_gpu,
            right.enable_gpu_service_logging_gpu);
  EXPECT_EQ(left.enable_gpu_driver_debug_logging,
            right.enable_gpu_driver_debug_logging);
  EXPECT_EQ(left.disable_gpu_program_cache, right.disable_gpu_program_cache);
  EXPECT_EQ(left.enforce_gl_minimums, right.enforce_gl_minimums);
  EXPECT_EQ(left.force_gpu_mem_available, right.force_gpu_mem_available);
  EXPECT_EQ(left.gpu_program_cache_size, right.gpu_program_cache_size);
  EXPECT_EQ(left.disable_gpu_shader_disk_cache,
            right.disable_gpu_shader_disk_cache);
  EXPECT_EQ(left.enable_threaded_texture_mailboxes,
            right.enable_threaded_texture_mailboxes);
  EXPECT_EQ(left.gl_shader_interm_output, right.gl_shader_interm_output);
  EXPECT_EQ(left.emulate_shader_precision, right.emulate_shader_precision);
  EXPECT_EQ(left.enable_gpu_service_logging, right.enable_gpu_service_logging);
  EXPECT_EQ(left.enable_gpu_service_tracing, right.enable_gpu_service_tracing);
  EXPECT_EQ(left.use_passthrough_cmd_decoder,
            right.use_passthrough_cmd_decoder);
  EXPECT_EQ(left.disable_biplanar_gpu_memory_buffers_for_video_frames,
            right.disable_biplanar_gpu_memory_buffers_for_video_frames);
  EXPECT_EQ(left.texture_target_exception_list,
            right.texture_target_exception_list);
  EXPECT_EQ(left.disable_gpu_driver_bug_workarounds,
            right.disable_gpu_driver_bug_workarounds);
  EXPECT_EQ(left.ignore_gpu_blacklist, right.ignore_gpu_blacklist);
  EXPECT_EQ(left.enable_oop_rasterization, right.enable_oop_rasterization);
  EXPECT_EQ(left.use_gpu_fences_for_overlay_planes,
            right.use_gpu_fences_for_overlay_planes);
}

}  // namespace

TEST(GpuPreferencesUtilTest, EncodeDecode) {
  {  // Testing default values.
    GpuPreferences input_prefs;
    GpuPreferences decoded_prefs;
    std::string encoded = GpuPreferencesToSwitchValue(input_prefs);
    bool flag = SwitchValueToGpuPreferences(encoded, &decoded_prefs);
    EXPECT_TRUE(flag);
    CheckGpuPreferencesEqual(input_prefs, decoded_prefs);
  }

  {  // Change all fields to non default values.
    GpuPreferences input_prefs;
    GpuPreferences decoded_prefs;

    GpuPreferences default_prefs;
    mojom::GpuPreferences prefs_mojom;

    // Make sure all feilds are included in mojo struct.
    // TODO(zmo): This test isn't perfect. If a field isn't included in
    // mojom::GpuPreferences, but because of alignment, the two struct sizes
    // could still be the same.
    EXPECT_EQ(sizeof(default_prefs), sizeof(prefs_mojom));

#define GPU_PREFERENCES_FIELD(name, value)         \
  input_prefs.name = value;                        \
  EXPECT_NE(default_prefs.name, input_prefs.name); \
  prefs_mojom.name = value;                        \
  EXPECT_EQ(input_prefs.name, prefs_mojom.name);

    GPU_PREFERENCES_FIELD(single_process, true)
    GPU_PREFERENCES_FIELD(in_process_gpu, true)
    GPU_PREFERENCES_FIELD(disable_accelerated_video_decode, true)
    GPU_PREFERENCES_FIELD(disable_accelerated_video_encode, true)
    GPU_PREFERENCES_FIELD(gpu_startup_dialog, true)
    GPU_PREFERENCES_FIELD(disable_gpu_watchdog, true)
    GPU_PREFERENCES_FIELD(gpu_sandbox_start_early, true)
    GPU_PREFERENCES_FIELD(enable_accelerated_vpx_decode,
                          GpuPreferences::VPX_VENDOR_AMD)
    GPU_PREFERENCES_FIELD(enable_low_latency_dxva, false)
    GPU_PREFERENCES_FIELD(enable_zero_copy_dxgi_video, true)
    GPU_PREFERENCES_FIELD(enable_nv12_dxgi_video, true)
    GPU_PREFERENCES_FIELD(enable_media_foundation_vea_on_windows7, true)
    GPU_PREFERENCES_FIELD(disable_software_rasterizer, true)
    GPU_PREFERENCES_FIELD(log_gpu_control_list_decisions, true)
    GPU_PREFERENCES_FIELD(compile_shader_always_succeeds, true)
    GPU_PREFERENCES_FIELD(disable_gl_error_limit, true)
    GPU_PREFERENCES_FIELD(disable_glsl_translator, true)
    GPU_PREFERENCES_FIELD(disable_shader_name_hashing, true)
    GPU_PREFERENCES_FIELD(enable_gpu_command_logging, true)
    GPU_PREFERENCES_FIELD(enable_gpu_debugging, true)
    GPU_PREFERENCES_FIELD(enable_gpu_service_logging_gpu, true)
    GPU_PREFERENCES_FIELD(enable_gpu_driver_debug_logging, true)
    GPU_PREFERENCES_FIELD(disable_gpu_program_cache, true)
    GPU_PREFERENCES_FIELD(enforce_gl_minimums, true)
    GPU_PREFERENCES_FIELD(force_gpu_mem_available, 4096)
    GPU_PREFERENCES_FIELD(gpu_program_cache_size,
                          kDefaultMaxProgramCacheMemoryBytes - 1)
    GPU_PREFERENCES_FIELD(disable_gpu_shader_disk_cache, true)
    GPU_PREFERENCES_FIELD(enable_threaded_texture_mailboxes, true)
    GPU_PREFERENCES_FIELD(gl_shader_interm_output, true)
    GPU_PREFERENCES_FIELD(emulate_shader_precision, true)
    GPU_PREFERENCES_FIELD(enable_gpu_service_logging, true)
    GPU_PREFERENCES_FIELD(enable_gpu_service_tracing, true)
    GPU_PREFERENCES_FIELD(use_passthrough_cmd_decoder, true)
    GPU_PREFERENCES_FIELD(disable_biplanar_gpu_memory_buffers_for_video_frames,
                          true)
    GPU_PREFERENCES_FIELD(disable_gpu_driver_bug_workarounds, true)
    GPU_PREFERENCES_FIELD(ignore_gpu_blacklist, true)
    GPU_PREFERENCES_FIELD(enable_oop_rasterization, true)
    GPU_PREFERENCES_FIELD(use_gpu_fences_for_overlay_planes, true)

    input_prefs.texture_target_exception_list.emplace_back(
        gfx::BufferUsage::SCANOUT, gfx::BufferFormat::RGBA_8888);
    input_prefs.texture_target_exception_list.emplace_back(
        gfx::BufferUsage::GPU_READ, gfx::BufferFormat::BGRA_8888);

    // Make sure every field is encoded/decoded.
    std::string encoded = GpuPreferencesToSwitchValue(input_prefs);
    bool flag = SwitchValueToGpuPreferences(encoded, &decoded_prefs);
    EXPECT_TRUE(flag);
    CheckGpuPreferencesEqual(input_prefs, decoded_prefs);
  }
}

}  // namespace gpu
