// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMECAST_BROWSER_ANDROID_CAST_WEB_CONTENTS_SURFACE_HELPER_H_
#define CHROMECAST_BROWSER_ANDROID_CAST_WEB_CONTENTS_SURFACE_HELPER_H_

#include <jni.h>

#include "base/android/scoped_java_ref.h"
#include "base/macros.h"
#include "base/supports_user_data.h"

namespace content {
class WebContents;
}

namespace chromecast {
namespace shell {

// Helper class to get android UI reference, CastWebContentsActivity or
// CastWebContentsFragment, displaying a given web_contents.
// This class is lazily created through the Get function and
// will manage its own lifetime via SupportsUserData.
class CastWebContentsSurfaceHelper : public base::SupportsUserData::Data {
 public:
  ~CastWebContentsSurfaceHelper() override;

  static CastWebContentsSurfaceHelper* Get(content::WebContents* web_contents);

  base::android::ScopedJavaLocalRef<jobject> GetContentVideoViewEmbedder();
  void SetContentVideoViewEmbedder(
      const base::android::JavaParamRef<jobject>& content_video_view_embedder);

 private:
  CastWebContentsSurfaceHelper();

  base::android::ScopedJavaGlobalRef<jobject> content_video_view_embedder_;

  DISALLOW_COPY_AND_ASSIGN(CastWebContentsSurfaceHelper);
};

}  // namespace shell
}  // namespace chromecast

#endif  // CHROMECAST_BROWSER_ANDROID_CAST_WEB_CONTENTS_SURFACE_HELPER_H_
