// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_OZONE_PLATFORM_DRM_HOST_DRM_OVERLAY_MANAGER_H_
#define UI_OZONE_PLATFORM_DRM_HOST_DRM_OVERLAY_MANAGER_H_

#include <stdint.h>

#include <vector>

#include "base/containers/mru_cache.h"
#include "base/macros.h"
#include "base/synchronization/lock.h"
#include "ui/ozone/platform/drm/host/gpu_thread_adapter.h"
#include "ui/ozone/public/overlay_candidates_ozone.h"
#include "ui/ozone/public/overlay_manager_ozone.h"

namespace ui {
class DrmWindowHostManager;
class OverlaySurfaceCandidate;

class DrmOverlayManager : public OverlayManagerOzone {
 public:
  DrmOverlayManager(GpuThreadAdapter* proxy,
                    DrmWindowHostManager* window_manager);
  ~DrmOverlayManager() override;

  // OverlayManagerOzone:
  std::unique_ptr<OverlayCandidatesOzone> CreateOverlayCandidates(
      gfx::AcceleratedWidget w) override;

  bool SupportsOverlays() const override;

  // Invoked on changes to the window (aka display) that require re-populating
  // the cache from the DRM thread.
  void ResetCache();

  // Communication-free implementations of actions performed in response to
  // messages from the GPU thread.
  void GpuSentOverlayResult(const gfx::AcceleratedWidget& widget,
                            const OverlaySurfaceCandidateList& params,
                            const OverlayStatusList& returns);

  // Service method for DrmOverlayCandidatesHost
  void CheckOverlaySupport(
      OverlayCandidatesOzone::OverlaySurfaceCandidateList* candidates,
      gfx::AcceleratedWidget widget);

  void set_supports_overlays(bool yes) { supports_overlays_ = yes; }

 private:
  // Value for the request cache, that keeps track of how many times a
  // specific validation has been requested, if there is a GPU validation
  // in flight, and at last the result of the validation.
  struct OverlayValidationCacheValue {
    OverlayValidationCacheValue();
    OverlayValidationCacheValue(const OverlayValidationCacheValue&);
    ~OverlayValidationCacheValue();
    int request_num = 0;
    std::vector<OverlayStatus> status;
  };

  void SendOverlayValidationRequest(
      const OverlaySurfaceCandidateList& candidates,
      gfx::AcceleratedWidget widget) const;
  bool CanHandleCandidate(const OverlaySurfaceCandidate& candidate,
                          gfx::AcceleratedWidget widget) const;

  // Whether we have DRM atomic capabilities and we can support HW overlays.
  bool supports_overlays_ = false;

  GpuThreadAdapter* proxy_;               // Not owned.
  DrmWindowHostManager* window_manager_;  // Not owned.

  // List of all OverlaySurfaceCandidate instances which have been requested
  // for validation and/or validated.
  base::MRUCache<OverlaySurfaceCandidateList, OverlayValidationCacheValue>
      cache_;
  // The cache can be accessed from multiple threads in some cases (e.g. with
  // mus, it can be accessed from the UI thread, and the window-service
  // thread.)
  // TODO(rjkroege): In the future (with --enable-viz), this code will not need
  // the lock, but will require farther refactoring.
  base::Lock cache_lock_;

  DISALLOW_COPY_AND_ASSIGN(DrmOverlayManager);
};

}  // namespace ui

#endif  // UI_OZONE_PLATFORM_DRM_HOST_DRM_OVERLAY_MANAGER_H_
