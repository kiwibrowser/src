// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_GPU_GPU_DATA_MANAGER_IMPL_H_
#define CONTENT_BROWSER_GPU_GPU_DATA_MANAGER_IMPL_H_

#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <string>

#include "base/compiler_specific.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/singleton.h"
#include "base/process/kill.h"
#include "base/synchronization/lock.h"
#include "base/time/time.h"
#include "base/values.h"
#include "content/public/browser/gpu_data_manager.h"
#include "content/public/common/three_d_api_types.h"
#include "gpu/config/gpu_control_list.h"
#include "gpu/config/gpu_feature_info.h"
#include "gpu/config/gpu_info.h"

class GURL;

namespace base {
class CommandLine;
}

namespace gpu {
struct GpuPreferences;
struct VideoMemoryUsageStats;
}

namespace content {

class GpuDataManagerImplPrivate;

class CONTENT_EXPORT GpuDataManagerImpl : public GpuDataManager {
 public:
  // Indicates the guilt level of a domain which caused a GPU reset.
  // If a domain is 100% known to be guilty of resetting the GPU, then
  // it will generally not cause other domains' use of 3D APIs to be
  // blocked, unless system stability would be compromised.
  enum DomainGuilt {
    DOMAIN_GUILT_KNOWN,
    DOMAIN_GUILT_UNKNOWN
  };

  // Indicates the reason that access to a given client API (like
  // WebGL or Pepper 3D) was blocked or not. This state is distinct
  // from blacklisting of an entire feature.
  enum DomainBlockStatus {
    DOMAIN_BLOCK_STATUS_BLOCKED,
    DOMAIN_BLOCK_STATUS_ALL_DOMAINS_BLOCKED,
    DOMAIN_BLOCK_STATUS_NOT_BLOCKED
  };

  // Getter for the singleton. This will return NULL on failure.
  static GpuDataManagerImpl* GetInstance();

  // GpuDataManager implementation.
  void BlacklistWebGLForTesting() override;
  gpu::GPUInfo GetGPUInfo() const override;
  bool GpuAccessAllowed(std::string* reason) const override;
  void RequestCompleteGpuInfoIfNeeded() override;
  bool IsEssentialGpuInfoAvailable() const override;
  void RequestVideoMemoryUsageStatsUpdate(
      const base::Callback<void(const gpu::VideoMemoryUsageStats& stats)>&
          callback) const override;
  // TODO(kbr): the threading model for the GpuDataManagerObservers is
  // not well defined, and it's impossible for callers to correctly
  // delete observers from anywhere except in one of the observer's
  // notification methods. Observer addition and removal, and their
  // callbacks, should probably be required to occur on the UI thread.
  void AddObserver(GpuDataManagerObserver* observer) override;
  void RemoveObserver(GpuDataManagerObserver* observer) override;
  void DisableHardwareAcceleration() override;
  bool HardwareAccelerationEnabled() const override;

  void RequestGpuSupportedRuntimeVersion() const;
  bool GpuProcessStartAllowed() const;

  bool IsGpuFeatureInfoAvailable() const;
  gpu::GpuFeatureStatus GetFeatureStatus(gpu::GpuFeatureType feature) const;

  // Only update if the current GPUInfo is not finalized.  If blacklist is
  // loaded, run through blacklist and update blacklisted features.
  void UpdateGpuInfo(
      const gpu::GPUInfo& gpu_info,
      const base::Optional<gpu::GPUInfo>& gpu_info_for_hardware_gpu);

  // Update the GPU feature info. This updates the blacklist and enabled status
  // of GPU rasterization. In the future this will be used for more features.
  void UpdateGpuFeatureInfo(const gpu::GpuFeatureInfo& gpu_feature_info,
                            const base::Optional<gpu::GpuFeatureInfo>&
                                gpu_feature_info_for_hardware_gpu);

  gpu::GpuFeatureInfo GetGpuFeatureInfo() const;

  gpu::GPUInfo GetGPUInfoForHardwareGpu() const;
  gpu::GpuFeatureInfo GetGpuFeatureInfoForHardwareGpu() const;

  // Insert switches into gpu process command line: kUseGL, etc.
  void AppendGpuCommandLine(base::CommandLine* command_line) const;

  // Update GpuPreferences based on blacklisting decisions.
  void UpdateGpuPreferences(gpu::GpuPreferences* gpu_preferences) const;

  void AddLogMessage(int level,
                     const std::string& header,
                     const std::string& message);

  void ProcessCrashed(base::TerminationStatus exit_code);

  // Returns a new copy of the ListValue.
  std::unique_ptr<base::ListValue> GetLogMessages() const;

  // Called when switching gpu.
  void HandleGpuSwitch();

  // Maintenance of domains requiring explicit user permission before
  // using client-facing 3D APIs (WebGL, Pepper 3D), either because
  // the domain has caused the GPU to reset, or because too many GPU
  // resets have been observed globally recently, and system stability
  // might be compromised.
  //
  // The given URL may be a partial URL (including at least the host)
  // or a full URL to a page.
  void BlockDomainFrom3DAPIs(const GURL& url, DomainGuilt guilt);
  bool Are3DAPIsBlocked(const GURL& top_origin_url,
                        int render_process_id,
                        int render_frame_id,
                        ThreeDAPIType requester);
  void UnblockDomainFrom3DAPIs(const GURL& url);

  // Disables domain blocking for 3D APIs. For use only in tests.
  void DisableDomainBlockingFor3DAPIsForTesting();

  // Set the active gpu.
  // Return true if it's a different GPU from the previous active one.
  bool UpdateActiveGpu(uint32_t vendor_id, uint32_t device_id);

  // Notify all observers whenever there is a GPU info or GPU feature
  // status update.
  void NotifyGpuInfoUpdate();

  // Called when GPU process initialization failed.
  void OnGpuProcessInitFailure();

  void BlockSwiftShader();
  bool SwiftShaderAllowed() const;

  // Returns false if the latest GPUInfo gl_renderer is from SwiftShader or
  // Disabled (in the viz case).
  bool IsGpuProcessUsingHardwareGpu() const;

 private:
  friend class GpuDataManagerImplPrivate;
  friend class GpuDataManagerImplPrivateTest;
  friend struct base::DefaultSingletonTraits<GpuDataManagerImpl>;

  // It's similar to AutoUnlock, but we want to make it a no-op
  // if the owner GpuDataManagerImpl is null.
  // This should only be used by GpuDataManagerImplPrivate where
  // callbacks are called, during which re-entering
  // GpuDataManagerImpl is possible.
  class UnlockedSession {
   public:
    explicit UnlockedSession(GpuDataManagerImpl* owner)
        : owner_(owner) {
      DCHECK(owner_);
      owner_->lock_.AssertAcquired();
      owner_->lock_.Release();
    }

    ~UnlockedSession() {
      DCHECK(owner_);
      owner_->lock_.Acquire();
    }

   private:
    GpuDataManagerImpl* owner_;
    DISALLOW_COPY_AND_ASSIGN(UnlockedSession);
  };

  GpuDataManagerImpl();
  ~GpuDataManagerImpl() override;

  mutable base::Lock lock_;
  std::unique_ptr<GpuDataManagerImplPrivate> private_;

  DISALLOW_COPY_AND_ASSIGN(GpuDataManagerImpl);
};

}  // namespace content

#endif  // CONTENT_BROWSER_GPU_GPU_DATA_MANAGER_IMPL_H_
