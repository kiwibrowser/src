// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_FEED_FEED_IMAGE_LOADER_BRIDGE_H_
#define CHROME_BROWSER_ANDROID_FEED_FEED_IMAGE_LOADER_BRIDGE_H_

#include "base/android/scoped_java_ref.h"
#include "base/memory/weak_ptr.h"
#include "ui/gfx/image/image.h"

namespace feed {

using base::android::JavaParamRef;
using base::android::ScopedJavaGlobalRef;

class FeedImageManager;

// Native counterpart of FeedImageLoaderBridge.java. Holds non-owning pointers
// to native implementation, to which operations are delegated. Results are
// passed back by a single argument callback so
// base::android::RunCallbackAndroid() can be used. This bridge is instantiated,
// owned, and destroyed from Java.
class FeedImageLoaderBridge {
 public:
  explicit FeedImageLoaderBridge(FeedImageManager* feed_image_manager);
  ~FeedImageLoaderBridge();

  void Destroy(JNIEnv* j_env, const JavaParamRef<jobject>& j_this);

  void FetchImage(JNIEnv* j_env,
                  const JavaParamRef<jobject>& j_this,
                  const JavaParamRef<jobjectArray>& j_urls,
                  const JavaParamRef<jobject>& j_callback);

 private:
  void OnImageFetched(ScopedJavaGlobalRef<jobject> callback,
                      const gfx::Image& image);

  FeedImageManager* feed_image_manager_;

  base::WeakPtrFactory<FeedImageLoaderBridge> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(FeedImageLoaderBridge);
};

}  // namespace feed

#endif  // CHROME_BROWSER_ANDROID_FEED_FEED_IMAGE_LOADER_BRIDGE_H_
