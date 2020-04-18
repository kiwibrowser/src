// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/config/gpu_util.h"

#include <memory>
#include <vector>

#include "base/command_line.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/stringprintf.h"
#include "build/build_config.h"
#include "gpu/config/gpu_blacklist.h"
#include "gpu/config/gpu_crash_keys.h"
#include "gpu/config/gpu_driver_bug_list.h"
#include "gpu/config/gpu_driver_bug_workaround_type.h"
#include "gpu/config/gpu_feature_type.h"
#include "gpu/config/gpu_finch_features.h"
#include "gpu/config/gpu_info_collector.h"
#include "gpu/config/gpu_switches.h"
#include "ui/gl/extension_set.h"
#include "ui/gl/gl_features.h"
#include "ui/gl/gl_switches.h"

#if defined(OS_ANDROID)
#include "base/no_destructor.h"
#include "base/synchronization/lock.h"
#include "ui/gl/init/gl_factory.h"
#endif  // OS_ANDROID

namespace gpu {

namespace {

GpuFeatureStatus GetGpuRasterizationFeatureStatus(
    const std::set<int>& blacklisted_features,
    const base::CommandLine& command_line) {
  if (command_line.HasSwitch(switches::kDisableGpuRasterization))
    return kGpuFeatureStatusDisabled;
  else if (command_line.HasSwitch(switches::kEnableGpuRasterization))
    return kGpuFeatureStatusEnabled;

  if (blacklisted_features.count(GPU_FEATURE_TYPE_GPU_RASTERIZATION))
    return kGpuFeatureStatusBlacklisted;

  // Gpu Rasterization on platforms that are not fully enabled is controlled by
  // a finch experiment.
  if (!base::FeatureList::IsEnabled(features::kDefaultEnableGpuRasterization))
    return kGpuFeatureStatusDisabled;

  return kGpuFeatureStatusEnabled;
}

GpuFeatureStatus GetWebGLFeatureStatus(
    const std::set<int>& blacklisted_features,
    bool use_swift_shader) {
  if (use_swift_shader)
    return kGpuFeatureStatusEnabled;
  if (blacklisted_features.count(GPU_FEATURE_TYPE_ACCELERATED_WEBGL))
    return kGpuFeatureStatusBlacklisted;
  return kGpuFeatureStatusEnabled;
}

GpuFeatureStatus GetWebGL2FeatureStatus(
    const std::set<int>& blacklisted_features,
    bool use_swift_shader) {
  if (use_swift_shader)
    return kGpuFeatureStatusEnabled;
  if (blacklisted_features.count(GPU_FEATURE_TYPE_ACCELERATED_WEBGL2))
    return kGpuFeatureStatusBlacklisted;
  return kGpuFeatureStatusEnabled;
}

GpuFeatureStatus Get2DCanvasFeatureStatus(
    const std::set<int>& blacklisted_features,
    bool use_swift_shader) {
  if (use_swift_shader) {
    // This is for testing only. Chrome should exercise the GPU accelerated
    // path on top of SwiftShader driver.
    return kGpuFeatureStatusEnabled;
  }
  if (blacklisted_features.count(GPU_FEATURE_TYPE_ACCELERATED_2D_CANVAS))
    return kGpuFeatureStatusSoftware;
  return kGpuFeatureStatusEnabled;
}

GpuFeatureStatus GetFlash3DFeatureStatus(
    const std::set<int>& blacklisted_features,
    bool use_swift_shader) {
  if (use_swift_shader) {
    // This is for testing only. Chrome should exercise the GPU accelerated
    // path on top of SwiftShader driver.
    return kGpuFeatureStatusEnabled;
  }
  if (blacklisted_features.count(GPU_FEATURE_TYPE_FLASH3D))
    return kGpuFeatureStatusBlacklisted;
  return kGpuFeatureStatusEnabled;
}

GpuFeatureStatus GetFlashStage3DFeatureStatus(
    const std::set<int>& blacklisted_features,
    bool use_swift_shader) {
  if (use_swift_shader) {
    // This is for testing only. Chrome should exercise the GPU accelerated
    // path on top of SwiftShader driver.
    return kGpuFeatureStatusEnabled;
  }
  if (blacklisted_features.count(GPU_FEATURE_TYPE_FLASH_STAGE3D))
    return kGpuFeatureStatusBlacklisted;
  return kGpuFeatureStatusEnabled;
}

GpuFeatureStatus GetFlashStage3DBaselineFeatureStatus(
    const std::set<int>& blacklisted_features,
    bool use_swift_shader) {
  if (use_swift_shader) {
    // This is for testing only. Chrome should exercise the GPU accelerated
    // path on top of SwiftShader driver.
    return kGpuFeatureStatusEnabled;
  }
  if (blacklisted_features.count(GPU_FEATURE_TYPE_FLASH_STAGE3D) ||
      blacklisted_features.count(GPU_FEATURE_TYPE_FLASH_STAGE3D_BASELINE))
    return kGpuFeatureStatusBlacklisted;
  return kGpuFeatureStatusEnabled;
}

GpuFeatureStatus GetAcceleratedVideoDecodeFeatureStatus(
    const std::set<int>& blacklisted_features,
    bool use_swift_shader) {
  if (use_swift_shader)
    return kGpuFeatureStatusDisabled;
  if (blacklisted_features.count(GPU_FEATURE_TYPE_ACCELERATED_VIDEO_DECODE))
    return kGpuFeatureStatusBlacklisted;
  return kGpuFeatureStatusEnabled;
}

GpuFeatureStatus GetGpuCompositingFeatureStatus(
    const std::set<int>& blacklisted_features,
    bool use_swift_shader) {
  if (use_swift_shader) {
    // This is for testing only. Chrome should exercise the GPU accelerated
    // path on top of SwiftShader driver.
    return kGpuFeatureStatusEnabled;
  }
  if (blacklisted_features.count(GPU_FEATURE_TYPE_GPU_COMPOSITING))
    return kGpuFeatureStatusBlacklisted;
  return kGpuFeatureStatusEnabled;
}

GpuFeatureStatus GetProtectedVideoDecodeFeatureStatus(
    const std::set<int>& blacklisted_features,
    const GPUInfo& gpu_info,
    bool use_swift_shader) {
  if (use_swift_shader)
    return kGpuFeatureStatusDisabled;
  if (blacklisted_features.count(GPU_FEATURE_TYPE_PROTECTED_VIDEO_DECODE))
    return kGpuFeatureStatusBlacklisted;
  return kGpuFeatureStatusEnabled;
}

void AppendWorkaroundsToCommandLine(const GpuFeatureInfo& gpu_feature_info,
                                    base::CommandLine* command_line) {
  if (gpu_feature_info.IsWorkaroundEnabled(DISABLE_D3D11)) {
    command_line->AppendSwitch(switches::kDisableD3D11);
  }
  if (gpu_feature_info.IsWorkaroundEnabled(DISABLE_ES3_GL_CONTEXT)) {
    command_line->AppendSwitch(switches::kDisableES3GLContext);
  }
  if (gpu_feature_info.IsWorkaroundEnabled(DISABLE_DIRECT_COMPOSITION)) {
    command_line->AppendSwitch(switches::kDisableDirectComposition);
  }
}

// Adjust gpu feature status based on enabled gpu driver bug workarounds.
void AdjustGpuFeatureStatusToWorkarounds(GpuFeatureInfo* gpu_feature_info) {
  if (gpu_feature_info->IsWorkaroundEnabled(DISABLE_D3D11) ||
      gpu_feature_info->IsWorkaroundEnabled(DISABLE_ES3_GL_CONTEXT)) {
    gpu_feature_info->status_values[GPU_FEATURE_TYPE_ACCELERATED_WEBGL2] =
        kGpuFeatureStatusBlacklisted;
  }
}

GPUInfo* g_gpu_info_cache = nullptr;
GpuFeatureInfo* g_gpu_feature_info_cache = nullptr;

}  // namespace anonymous

GpuFeatureInfo ComputeGpuFeatureInfoWithHardwareAccelerationDisabled() {
  GpuFeatureInfo gpu_feature_info;
  gpu_feature_info.status_values[GPU_FEATURE_TYPE_ACCELERATED_2D_CANVAS] =
      kGpuFeatureStatusSoftware;
  gpu_feature_info.status_values[GPU_FEATURE_TYPE_GPU_COMPOSITING] =
      kGpuFeatureStatusDisabled;
  gpu_feature_info.status_values[GPU_FEATURE_TYPE_ACCELERATED_WEBGL] =
      kGpuFeatureStatusSoftware;
  gpu_feature_info.status_values[GPU_FEATURE_TYPE_FLASH3D] =
      kGpuFeatureStatusDisabled;
  gpu_feature_info.status_values[GPU_FEATURE_TYPE_FLASH_STAGE3D] =
      kGpuFeatureStatusDisabled;
  gpu_feature_info.status_values[GPU_FEATURE_TYPE_ACCELERATED_VIDEO_DECODE] =
      kGpuFeatureStatusDisabled;
  gpu_feature_info.status_values[GPU_FEATURE_TYPE_FLASH_STAGE3D_BASELINE] =
      kGpuFeatureStatusDisabled;
  gpu_feature_info.status_values[GPU_FEATURE_TYPE_GPU_RASTERIZATION] =
      kGpuFeatureStatusDisabled;
  gpu_feature_info.status_values[GPU_FEATURE_TYPE_ACCELERATED_WEBGL2] =
      kGpuFeatureStatusSoftware;
  gpu_feature_info.status_values[GPU_FEATURE_TYPE_PROTECTED_VIDEO_DECODE] =
      kGpuFeatureStatusDisabled;
#if DCHECK_IS_ON()
  for (int ii = 0; ii < NUMBER_OF_GPU_FEATURE_TYPES; ++ii) {
    DCHECK_NE(kGpuFeatureStatusUndefined, gpu_feature_info.status_values[ii]);
  }
#endif
  return gpu_feature_info;
}

GpuFeatureInfo ComputeGpuFeatureInfoWithNoGpu() {
  GpuFeatureInfo gpu_feature_info;
  gpu_feature_info.status_values[GPU_FEATURE_TYPE_ACCELERATED_2D_CANVAS] =
      kGpuFeatureStatusSoftware;
  gpu_feature_info.status_values[GPU_FEATURE_TYPE_GPU_COMPOSITING] =
      kGpuFeatureStatusDisabled;
  gpu_feature_info.status_values[GPU_FEATURE_TYPE_ACCELERATED_WEBGL] =
      kGpuFeatureStatusDisabled;
  gpu_feature_info.status_values[GPU_FEATURE_TYPE_FLASH3D] =
      kGpuFeatureStatusDisabled;
  gpu_feature_info.status_values[GPU_FEATURE_TYPE_FLASH_STAGE3D] =
      kGpuFeatureStatusDisabled;
  gpu_feature_info.status_values[GPU_FEATURE_TYPE_ACCELERATED_VIDEO_DECODE] =
      kGpuFeatureStatusDisabled;
  gpu_feature_info.status_values[GPU_FEATURE_TYPE_FLASH_STAGE3D_BASELINE] =
      kGpuFeatureStatusDisabled;
  gpu_feature_info.status_values[GPU_FEATURE_TYPE_GPU_RASTERIZATION] =
      kGpuFeatureStatusDisabled;
  gpu_feature_info.status_values[GPU_FEATURE_TYPE_ACCELERATED_WEBGL2] =
      kGpuFeatureStatusDisabled;
  gpu_feature_info.status_values[GPU_FEATURE_TYPE_PROTECTED_VIDEO_DECODE] =
      kGpuFeatureStatusDisabled;
#if DCHECK_IS_ON()
  for (int ii = 0; ii < NUMBER_OF_GPU_FEATURE_TYPES; ++ii) {
    DCHECK_NE(kGpuFeatureStatusUndefined, gpu_feature_info.status_values[ii]);
  }
#endif
  return gpu_feature_info;
}

GpuFeatureInfo ComputeGpuFeatureInfoForSwiftShader() {
  GpuFeatureInfo gpu_feature_info;
  gpu_feature_info.status_values[GPU_FEATURE_TYPE_ACCELERATED_2D_CANVAS] =
      kGpuFeatureStatusSoftware;
  gpu_feature_info.status_values[GPU_FEATURE_TYPE_GPU_COMPOSITING] =
      kGpuFeatureStatusDisabled;
  gpu_feature_info.status_values[GPU_FEATURE_TYPE_ACCELERATED_WEBGL] =
      kGpuFeatureStatusSoftware;
  gpu_feature_info.status_values[GPU_FEATURE_TYPE_FLASH3D] =
      kGpuFeatureStatusDisabled;
  gpu_feature_info.status_values[GPU_FEATURE_TYPE_FLASH_STAGE3D] =
      kGpuFeatureStatusDisabled;
  gpu_feature_info.status_values[GPU_FEATURE_TYPE_ACCELERATED_VIDEO_DECODE] =
      kGpuFeatureStatusDisabled;
  gpu_feature_info.status_values[GPU_FEATURE_TYPE_FLASH_STAGE3D_BASELINE] =
      kGpuFeatureStatusDisabled;
  gpu_feature_info.status_values[GPU_FEATURE_TYPE_GPU_RASTERIZATION] =
      kGpuFeatureStatusDisabled;
  gpu_feature_info.status_values[GPU_FEATURE_TYPE_ACCELERATED_WEBGL2] =
      kGpuFeatureStatusSoftware;
  gpu_feature_info.status_values[GPU_FEATURE_TYPE_PROTECTED_VIDEO_DECODE] =
      kGpuFeatureStatusDisabled;
#if DCHECK_IS_ON()
  for (int ii = 0; ii < NUMBER_OF_GPU_FEATURE_TYPES; ++ii) {
    DCHECK_NE(kGpuFeatureStatusUndefined, gpu_feature_info.status_values[ii]);
  }
#endif
  return gpu_feature_info;
}

GpuFeatureInfo ComputeGpuFeatureInfo(const GPUInfo& gpu_info,
                                     bool ignore_gpu_blacklist,
                                     bool disable_gpu_driver_bug_workarounds,
                                     bool log_gpu_control_list_decisions,
                                     base::CommandLine* command_line,
                                     bool* needs_more_info) {
  DCHECK(!needs_more_info || !(*needs_more_info));
  bool use_swift_shader = false;
  bool use_swift_shader_for_webgl = false;
  if (command_line->HasSwitch(switches::kUseGL)) {
    std::string use_gl = command_line->GetSwitchValueASCII(switches::kUseGL);
    if (use_gl == gl::kGLImplementationSwiftShaderName)
      use_swift_shader = true;
    else if (use_gl == gl::kGLImplementationSwiftShaderForWebGLName)
      use_swift_shader_for_webgl = true;
  }
  if (use_swift_shader_for_webgl)
    return ComputeGpuFeatureInfoForSwiftShader();

  GpuFeatureInfo gpu_feature_info;
  std::set<int> blacklisted_features;
  if (!ignore_gpu_blacklist &&
      !command_line->HasSwitch(switches::kUseGpuInTests)) {
    std::unique_ptr<GpuBlacklist> list(GpuBlacklist::Create());
    if (log_gpu_control_list_decisions)
      list->EnableControlListLogging("gpu_blacklist");
    unsigned target_test_group = 0u;
    if (command_line->HasSwitch(switches::kGpuBlacklistTestGroup)) {
      std::string test_group_string =
          command_line->GetSwitchValueASCII(switches::kGpuBlacklistTestGroup);
      if (!base::StringToUint(test_group_string, &target_test_group))
        target_test_group = 0u;
    }
    blacklisted_features = list->MakeDecision(
        GpuControlList::kOsAny, std::string(), gpu_info, target_test_group);
    gpu_feature_info.applied_gpu_blacklist_entries = list->GetActiveEntries();
    if (needs_more_info) {
      *needs_more_info = list->needs_more_info();
    }
  }

  gpu_feature_info.status_values[GPU_FEATURE_TYPE_GPU_RASTERIZATION] =
      GetGpuRasterizationFeatureStatus(blacklisted_features, *command_line);
  gpu_feature_info.status_values[GPU_FEATURE_TYPE_ACCELERATED_WEBGL] =
      GetWebGLFeatureStatus(blacklisted_features, use_swift_shader);
  gpu_feature_info.status_values[GPU_FEATURE_TYPE_ACCELERATED_WEBGL2] =
      GetWebGL2FeatureStatus(blacklisted_features, use_swift_shader);
  gpu_feature_info.status_values[GPU_FEATURE_TYPE_ACCELERATED_2D_CANVAS] =
      Get2DCanvasFeatureStatus(blacklisted_features, use_swift_shader);
  gpu_feature_info.status_values[GPU_FEATURE_TYPE_FLASH3D] =
      GetFlash3DFeatureStatus(blacklisted_features, use_swift_shader);
  gpu_feature_info.status_values[GPU_FEATURE_TYPE_FLASH_STAGE3D] =
      GetFlashStage3DFeatureStatus(blacklisted_features, use_swift_shader);
  gpu_feature_info.status_values[GPU_FEATURE_TYPE_FLASH_STAGE3D_BASELINE] =
      GetFlashStage3DBaselineFeatureStatus(blacklisted_features,
                                           use_swift_shader);
  gpu_feature_info.status_values[GPU_FEATURE_TYPE_ACCELERATED_VIDEO_DECODE] =
      GetAcceleratedVideoDecodeFeatureStatus(blacklisted_features,
                                             use_swift_shader);
  gpu_feature_info.status_values[GPU_FEATURE_TYPE_GPU_COMPOSITING] =
      GetGpuCompositingFeatureStatus(blacklisted_features, use_swift_shader);
  gpu_feature_info.status_values[GPU_FEATURE_TYPE_PROTECTED_VIDEO_DECODE] =
      GetProtectedVideoDecodeFeatureStatus(blacklisted_features, gpu_info,
                                           use_swift_shader);
#if DCHECK_IS_ON()
  for (int ii = 0; ii < NUMBER_OF_GPU_FEATURE_TYPES; ++ii) {
    DCHECK_NE(kGpuFeatureStatusUndefined, gpu_feature_info.status_values[ii]);
  }
#endif

  gl::ExtensionSet all_disabled_extensions;
  std::string disabled_gl_extensions_value =
      command_line->GetSwitchValueASCII(switches::kDisableGLExtensions);
  if (!disabled_gl_extensions_value.empty()) {
    std::vector<base::StringPiece> command_line_disabled_extensions =
        base::SplitStringPiece(disabled_gl_extensions_value, ", ;",
                               base::KEEP_WHITESPACE,
                               base::SPLIT_WANT_NONEMPTY);
    all_disabled_extensions.insert(command_line_disabled_extensions.begin(),
                                   command_line_disabled_extensions.end());
  }

  std::set<int> enabled_driver_bug_workarounds;
  std::vector<std::string> driver_bug_disabled_extensions;
  if (!disable_gpu_driver_bug_workarounds) {
    std::unique_ptr<gpu::GpuDriverBugList> list(GpuDriverBugList::Create());
    unsigned target_test_group = 0u;
    if (command_line->HasSwitch(switches::kGpuDriverBugListTestGroup)) {
      std::string test_group_string = command_line->GetSwitchValueASCII(
          switches::kGpuDriverBugListTestGroup);
      if (!base::StringToUint(test_group_string, &target_test_group))
        target_test_group = 0u;
    }
    enabled_driver_bug_workarounds = list->MakeDecision(
        GpuControlList::kOsAny, std::string(), gpu_info, target_test_group);
    gpu_feature_info.applied_gpu_driver_bug_list_entries =
        list->GetActiveEntries();

    driver_bug_disabled_extensions = list->GetDisabledExtensions();
    all_disabled_extensions.insert(driver_bug_disabled_extensions.begin(),
                                   driver_bug_disabled_extensions.end());

    // Disabling WebGL extensions only occurs via the blacklist, so
    // the logic is simpler.
    gl::ExtensionSet disabled_webgl_extensions;
    std::vector<std::string> disabled_webgl_extension_list =
        list->GetDisabledWebGLExtensions();
    disabled_webgl_extensions.insert(disabled_webgl_extension_list.begin(),
                                     disabled_webgl_extension_list.end());
    gpu_feature_info.disabled_webgl_extensions =
        gl::MakeExtensionString(disabled_webgl_extensions);
  }
  gpu::GpuDriverBugList::AppendWorkaroundsFromCommandLine(
      &enabled_driver_bug_workarounds, *command_line);

  gpu_feature_info.enabled_gpu_driver_bug_workarounds.insert(
      gpu_feature_info.enabled_gpu_driver_bug_workarounds.begin(),
      enabled_driver_bug_workarounds.begin(),
      enabled_driver_bug_workarounds.end());

  if (all_disabled_extensions.size()) {
    gpu_feature_info.disabled_extensions =
        gl::MakeExtensionString(all_disabled_extensions);
  }

  AdjustGpuFeatureStatusToWorkarounds(&gpu_feature_info);

  // TODO(zmo): Find a better way to communicate these settings to bindings
  // initialization than commandline switches.
  AppendWorkaroundsToCommandLine(gpu_feature_info, command_line);

  return gpu_feature_info;
}

void SetKeysForCrashLogging(const GPUInfo& gpu_info) {
#if !defined(OS_ANDROID)
  crash_keys::gpu_vendor_id.Set(
      base::StringPrintf("0x%04x", gpu_info.gpu.vendor_id));
  crash_keys::gpu_device_id.Set(
      base::StringPrintf("0x%04x", gpu_info.gpu.device_id));
#endif
  crash_keys::gpu_driver_version.Set(gpu_info.driver_version);
  crash_keys::gpu_pixel_shader_version.Set(gpu_info.pixel_shader_version);
  crash_keys::gpu_vertex_shader_version.Set(gpu_info.vertex_shader_version);
#if defined(OS_MACOSX)
  crash_keys::gpu_gl_version.Set(gpu_info.gl_version);
#elif defined(OS_POSIX)
  crash_keys::gpu_vendor.Set(gpu_info.gl_vendor);
  crash_keys::gpu_renderer.Set(gpu_info.gl_renderer);
#endif
}

void CacheGPUInfo(const GPUInfo& gpu_info) {
  DCHECK(!g_gpu_info_cache);
  g_gpu_info_cache = new GPUInfo;
  *g_gpu_info_cache = gpu_info;
}

bool PopGPUInfoCache(GPUInfo* gpu_info) {
  if (!g_gpu_info_cache)
    return false;
  *gpu_info = *g_gpu_info_cache;
  delete g_gpu_info_cache;
  g_gpu_info_cache = nullptr;
  return true;
}

void CacheGpuFeatureInfo(const GpuFeatureInfo& gpu_feature_info) {
  DCHECK(!g_gpu_feature_info_cache);
  g_gpu_feature_info_cache = new GpuFeatureInfo;
  *g_gpu_feature_info_cache = gpu_feature_info;
}

bool PopGpuFeatureInfoCache(GpuFeatureInfo* gpu_feature_info) {
  if (!g_gpu_feature_info_cache)
    return false;
  *gpu_feature_info = *g_gpu_feature_info_cache;
  delete g_gpu_feature_info_cache;
  g_gpu_feature_info_cache = nullptr;
  return true;
}

#if defined(OS_ANDROID)
bool InitializeGLThreadSafe(base::CommandLine* command_line,
                            bool ignore_gpu_blacklist,
                            bool disable_gpu_driver_bug_workarounds,
                            bool log_gpu_control_list_decisions,
                            GPUInfo* out_gpu_info,
                            GpuFeatureInfo* out_gpu_feature_info) {
  static base::NoDestructor<base::Lock> gl_bindings_initialization_lock;
  base::AutoLock auto_lock(*gl_bindings_initialization_lock);
  DCHECK(command_line);
  DCHECK(out_gpu_info && out_gpu_feature_info);
  bool gpu_info_cached = PopGPUInfoCache(out_gpu_info);
  bool gpu_feature_info_cached = PopGpuFeatureInfoCache(out_gpu_feature_info);
  DCHECK_EQ(gpu_info_cached, gpu_feature_info_cached);
  if (gpu_info_cached) {
    // GL bindings have already been initialized in another thread.
    DCHECK_NE(gl::kGLImplementationNone, gl::GetGLImplementation());
    return true;
  }
  if (gl::GetGLImplementation() == gl::kGLImplementationNone) {
    // Some tests initialize bindings by themselves.
    if (!gl::init::InitializeGLNoExtensionsOneOff()) {
      VLOG(1) << "gl::init::InitializeGLNoExtensionsOneOff failed";
      return false;
    }
  }
  CollectContextGraphicsInfo(out_gpu_info);
  *out_gpu_feature_info = ComputeGpuFeatureInfo(
      *out_gpu_info, ignore_gpu_blacklist, disable_gpu_driver_bug_workarounds,
      log_gpu_control_list_decisions, command_line, nullptr);
  if (!out_gpu_feature_info->disabled_extensions.empty()) {
    gl::init::SetDisabledExtensionsPlatform(
        out_gpu_feature_info->disabled_extensions);
  }
  if (!gl::init::InitializeExtensionSettingsOneOffPlatform()) {
    VLOG(1) << "gl::init::InitializeExtensionSettingsOneOffPlatform failed";
    return false;
  }
  CacheGPUInfo(*out_gpu_info);
  CacheGpuFeatureInfo(*out_gpu_feature_info);
  return true;
}
#endif  // OS_ANDROID

bool ShouldEnableSwiftShader(base::CommandLine* command_line,
                             const GpuFeatureInfo& gpu_feature_info,
                             bool disable_software_rasterizer,
                             bool blacklist_needs_more_info) {
#if BUILDFLAG(ENABLE_SWIFTSHADER)
  if (disable_software_rasterizer)
    return false;
  // Don't overwrite user preference.
  if (command_line->HasSwitch(switches::kUseGL))
    return false;
  if (!blacklist_needs_more_info &&
      gpu_feature_info.status_values[GPU_FEATURE_TYPE_ACCELERATED_WEBGL] !=
          kGpuFeatureStatusEnabled) {
    command_line->AppendSwitchASCII(
        switches::kUseGL, gl::kGLImplementationSwiftShaderForWebGLName);
    return true;
  }
  return false;
#else
  return false;
#endif
}

}  // namespace gpu
