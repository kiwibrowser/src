// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_OZONE_PLATFORM_IMPL_DRM_VSYNC_PROVIDER_H_
#define UI_OZONE_PLATFORM_IMPL_DRM_VSYNC_PROVIDER_H_

#include "base/macros.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/gfx/vsync_provider.h"

namespace ui {

class DrmWindowProxy;

class DrmVSyncProvider : public gfx::VSyncProvider {
 public:
  DrmVSyncProvider(DrmWindowProxy* window);
  ~DrmVSyncProvider() override;

  void GetVSyncParameters(const UpdateVSyncCallback& callback) override;
  bool GetVSyncParametersIfAvailable(base::TimeTicks* timebase,
                                     base::TimeDelta* interval) override;
  bool SupportGetVSyncParametersIfAvailable() const override;
  bool IsHWClock() const override;

 private:
  DrmWindowProxy* window_;

  DISALLOW_COPY_AND_ASSIGN(DrmVSyncProvider);
};

}  // namespace ui

#endif  // UI_OZONE_PLATFORM_IMPL_DRM_VSYNC_PROVIDER_H_
