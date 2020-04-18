// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_ANDROID_REMOTE_MEDIA_CONTROLLER_BRIDGE_H_
#define CHROME_BROWSER_MEDIA_ANDROID_REMOTE_MEDIA_CONTROLLER_BRIDGE_H_

#include "base/android/scoped_java_ref.h"
#include "base/time/time.h"
#include "content/public/browser/media_controller.h"

namespace media_router {

// Allows native code to call into a Java MediaController.
class MediaControllerBridge : public content::MediaController {
 public:
  explicit MediaControllerBridge(
      base::android::ScopedJavaGlobalRef<jobject> controller);
  ~MediaControllerBridge() override;

  // MediaController implementation.
  void Play() override;
  void Pause() override;
  void SetMute(bool mute) override;
  void SetVolume(float volume) override;
  void Seek(base::TimeDelta time) override;

 private:
  // Java MediaControllerBridge instance.
  base::android::ScopedJavaGlobalRef<jobject> j_media_controller_bridge_;

  DISALLOW_COPY_AND_ASSIGN(MediaControllerBridge);
};

}  // namespace media_router

#endif  // CHROME_BROWSER_MEDIA_ANDROID_REMOTE_MEDIA_CONTROLLER_BRIDGE_H_
