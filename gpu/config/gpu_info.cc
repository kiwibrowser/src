// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdint.h>

#include "gpu/config/gpu_info.h"

namespace {

void EnumerateGPUDevice(const gpu::GPUInfo::GPUDevice& device,
                        gpu::GPUInfo::Enumerator* enumerator) {
  enumerator->BeginGPUDevice();
  enumerator->AddInt("vendorId", device.vendor_id);
  enumerator->AddInt("deviceId", device.device_id);
  enumerator->AddBool("active", device.active);
  enumerator->AddString("vendorString", device.vendor_string);
  enumerator->AddString("deviceString", device.device_string);
  enumerator->EndGPUDevice();
}

void EnumerateVideoDecodeAcceleratorSupportedProfile(
    const gpu::VideoDecodeAcceleratorSupportedProfile& profile,
    gpu::GPUInfo::Enumerator* enumerator) {
  enumerator->BeginVideoDecodeAcceleratorSupportedProfile();
  enumerator->AddInt("profile", profile.profile);
  enumerator->AddInt("maxResolutionWidth", profile.max_resolution.width());
  enumerator->AddInt("maxResolutionHeight", profile.max_resolution.height());
  enumerator->AddInt("minResolutionWidth", profile.min_resolution.width());
  enumerator->AddInt("minResolutionHeight", profile.min_resolution.height());
  enumerator->AddBool("encrypted_only", profile.encrypted_only);
  enumerator->EndVideoDecodeAcceleratorSupportedProfile();
}

void EnumerateVideoEncodeAcceleratorSupportedProfile(
    const gpu::VideoEncodeAcceleratorSupportedProfile& profile,
    gpu::GPUInfo::Enumerator* enumerator) {
  enumerator->BeginVideoEncodeAcceleratorSupportedProfile();
  enumerator->AddInt("profile", profile.profile);
  enumerator->AddInt("maxResolutionWidth", profile.max_resolution.width());
  enumerator->AddInt("maxResolutionHeight", profile.max_resolution.height());
  enumerator->AddInt("maxFramerateNumerator", profile.max_framerate_numerator);
  enumerator->AddInt("maxFramerateDenominator",
                     profile.max_framerate_denominator);
  enumerator->EndVideoEncodeAcceleratorSupportedProfile();
}

}  // namespace

namespace gpu {

VideoDecodeAcceleratorCapabilities::VideoDecodeAcceleratorCapabilities()
    : flags(0) {}

VideoDecodeAcceleratorCapabilities::VideoDecodeAcceleratorCapabilities(
    const VideoDecodeAcceleratorCapabilities& other) = default;

VideoDecodeAcceleratorCapabilities::~VideoDecodeAcceleratorCapabilities() =
    default;

GPUInfo::GPUDevice::GPUDevice()
    : vendor_id(0),
      device_id(0),
      active(false) {
}

GPUInfo::GPUDevice::~GPUDevice() = default;

GPUInfo::GPUInfo()
    : optimus(false),
      amd_switchable(false),
      gl_reset_notification_strategy(0),
      software_rendering(false),
      direct_rendering(true),
      sandboxed(false),
      in_process_gpu(true),
      passthrough_cmd_decoder(false),
      jpeg_decode_accelerator_supported(false)
#if defined(USE_X11)
      ,
      system_visual(0),
      rgba_visual(0)
#endif
{
}

GPUInfo::GPUInfo(const GPUInfo& other) = default;

GPUInfo::~GPUInfo() = default;

const GPUInfo::GPUDevice& GPUInfo::active_gpu() const {
  if (gpu.active || secondary_gpus.empty())
    return gpu;
  for (const GPUDevice& secondary_gpu : secondary_gpus) {
    if (secondary_gpu.active)
      return secondary_gpu;
  }
  DLOG(WARNING) << "No active GPU found, returning primary GPU.";
  return gpu;
}

bool GPUInfo::IsInitialized() const {
  return gpu.vendor_id != 0 || !gl_vendor.empty();
}

void GPUInfo::EnumerateFields(Enumerator* enumerator) const {
  struct GPUInfoKnownFields {
    base::TimeDelta initialization_time;
    bool optimus;
    bool amd_switchable;
    GPUDevice gpu;
    std::vector<GPUDevice> secondary_gpus;
    std::string driver_vendor;
    std::string driver_version;
    std::string driver_date;
    std::string pixel_shader_version;
    std::string vertex_shader_version;
    std::string max_msaa_samples;
    std::string machine_model_name;
    std::string machine_model_version;
    std::string gl_version_string;
    std::string gl_vendor;
    std::string gl_renderer;
    std::string gl_extensions;
    std::string gl_ws_vendor;
    std::string gl_ws_version;
    std::string gl_ws_extensions;
    uint32_t gl_reset_notification_strategy;
    bool software_rendering;
    bool direct_rendering;
    bool sandboxed;
    bool in_process_gpu;
    bool passthrough_cmd_decoder;
    bool direct_composition;
    bool supports_overlays;
    bool can_support_threaded_texture_mailbox;
#if defined(OS_WIN)
    DxDiagNode dx_diagnostics;
    bool supports_dx12;
    bool supports_vulkan;
    uint32_t d3d12_feature_level;
    uint32_t vulkan_version;
#endif

    VideoDecodeAcceleratorCapabilities video_decode_accelerator_capabilities;
    VideoEncodeAcceleratorSupportedProfiles
        video_encode_accelerator_supported_profiles;
    bool jpeg_decode_accelerator_supported;
#if defined(USE_X11)
    VisualID system_visual;
    VisualID rgba_visual;
#endif
  };

  // If this assert fails then most likely something below needs to be updated.
  // Note that this assert is only approximate. If a new field is added to
  // GPUInfo which fits within the current padding then it will not be caught.
  static_assert(
      sizeof(GPUInfo) == sizeof(GPUInfoKnownFields),
      "fields have changed in GPUInfo, GPUInfoKnownFields must be updated");

  // Required fields (according to DevTools protocol) first.
  enumerator->AddString("machineModelName", machine_model_name);
  enumerator->AddString("machineModelVersion", machine_model_version);
  EnumerateGPUDevice(gpu, enumerator);
  for (const auto& secondary_gpu: secondary_gpus)
    EnumerateGPUDevice(secondary_gpu, enumerator);

  enumerator->BeginAuxAttributes();
  enumerator->AddTimeDeltaInSecondsF("initializationTime",
                                     initialization_time);
  enumerator->AddBool("optimus", optimus);
  enumerator->AddBool("amdSwitchable", amd_switchable);
  enumerator->AddString("driverVendor", driver_vendor);
  enumerator->AddString("driverVersion", driver_version);
  enumerator->AddString("driverDate", driver_date);
  enumerator->AddString("pixelShaderVersion", pixel_shader_version);
  enumerator->AddString("vertexShaderVersion", vertex_shader_version);
  enumerator->AddString("maxMsaaSamples", max_msaa_samples);
  enumerator->AddString("glVersion", gl_version);
  enumerator->AddString("glVendor", gl_vendor);
  enumerator->AddString("glRenderer", gl_renderer);
  enumerator->AddString("glExtensions", gl_extensions);
  enumerator->AddString("glWsVendor", gl_ws_vendor);
  enumerator->AddString("glWsVersion", gl_ws_version);
  enumerator->AddString("glWsExtensions", gl_ws_extensions);
  enumerator->AddInt(
      "glResetNotificationStrategy",
      static_cast<int>(gl_reset_notification_strategy));
  // TODO(kbr): add performance_stats.
  enumerator->AddBool("softwareRendering", software_rendering);
  enumerator->AddBool("directRendering", direct_rendering);
  enumerator->AddBool("sandboxed", sandboxed);
  enumerator->AddBool("inProcessGpu", in_process_gpu);
  enumerator->AddBool("passthroughCmdDecoder", passthrough_cmd_decoder);
  enumerator->AddBool("directComposition", direct_composition);
  enumerator->AddBool("supportsOverlays", supports_overlays);
  enumerator->AddBool("canSupportThreadedTextureMailbox",
                      can_support_threaded_texture_mailbox);
  // TODO(kbr): add dx_diagnostics on Windows.
#if defined(OS_WIN)
  enumerator->AddBool("supportsDX12", supports_dx12);
  enumerator->AddBool("supportsVulkan", supports_vulkan);
  enumerator->AddInt("d3dFeatureLevel", d3d12_feature_level);
  enumerator->AddInt("vulkanVersion", vulkan_version);
#endif
  enumerator->AddInt("videoDecodeAcceleratorFlags",
                     video_decode_accelerator_capabilities.flags);
  for (const auto& profile :
       video_decode_accelerator_capabilities.supported_profiles)
    EnumerateVideoDecodeAcceleratorSupportedProfile(profile, enumerator);
  for (const auto& profile : video_encode_accelerator_supported_profiles)
    EnumerateVideoEncodeAcceleratorSupportedProfile(profile, enumerator);
  enumerator->AddBool("jpegDecodeAcceleratorSupported",
      jpeg_decode_accelerator_supported);
#if defined(USE_X11)
  enumerator->AddInt64("systemVisual", system_visual);
  enumerator->AddInt64("rgbaVisual", rgba_visual);
#endif
  enumerator->EndAuxAttributes();
}

}  // namespace gpu
