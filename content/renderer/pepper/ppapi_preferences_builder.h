// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_PEPPER_PPAPI_PREFERENCES_BUILDER_H_
#define CONTENT_RENDERER_PEPPER_PPAPI_PREFERENCES_BUILDER_H_

namespace gpu {
struct GpuFeatureInfo;
}
namespace ppapi {
struct Preferences;
}

namespace content {

struct WebPreferences;

class PpapiPreferencesBuilder {
 public:
  static ppapi::Preferences Build(const WebPreferences& prefs,
                                  const gpu::GpuFeatureInfo& gpu_feature_info);
};

}  // namespace content

#endif  // CONTENT_RENDERER_PEPPER_PPAPI_PREFERENCES_BUILDER_H_
