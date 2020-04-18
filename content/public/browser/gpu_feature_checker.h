// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_GPU_FEATURE_CHECKER_H_
#define CONTENT_PUBLIC_BROWSER_GPU_FEATURE_CHECKER_H_

#include "base/callback_forward.h"
#include "base/memory/ref_counted.h"
#include "content/common/content_export.h"
#include "gpu/config/gpu_feature_type.h"

namespace content {

class GpuFeatureChecker : public base::RefCountedThreadSafe<GpuFeatureChecker> {
 public:
  typedef base::Callback<void(bool feature_available)> FeatureAvailableCallback;

  CONTENT_EXPORT static scoped_refptr<GpuFeatureChecker> Create(
      gpu::GpuFeatureType feature,
      FeatureAvailableCallback callback);

  // Checks to see if |feature_| is available on the current GPU. |callback_|
  // will be called to indicate the availability of the feature. Must be called
  // from the the UI thread.
  virtual void CheckGpuFeatureAvailability() = 0;

 protected:
  friend class base::RefCountedThreadSafe<GpuFeatureChecker>;

  virtual ~GpuFeatureChecker();
};

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_GPU_FEATURE_CHECKER_H_
