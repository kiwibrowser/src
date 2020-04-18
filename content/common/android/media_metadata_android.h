// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_ANDROID_MEDIA_METADATA_ANDROID_H_
#define CONTENT_COMMON_ANDROID_MEDIA_METADATA_ANDROID_H_

#include <jni.h>

#include "base/android/scoped_java_ref.h"

namespace content {

struct MediaMetadata;

class MediaMetadataAndroid {
 public:
  static base::android::ScopedJavaLocalRef<jobject> CreateJavaObject(
      JNIEnv* env, const MediaMetadata& metadata);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(MediaMetadataAndroid);
};

}  // namespace content

#endif  // CONTENT_COMMON_ANDROID_MEDIA_METADATA_ANDROID_H_
