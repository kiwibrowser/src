// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_WIDGET_THUMBNAIL_GENERATOR_H_
#define CHROME_BROWSER_ANDROID_WIDGET_THUMBNAIL_GENERATOR_H_

#include <string>

#include "base/android/jni_android.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/image_decoder.h"

// Kicks off asynchronous pipelines for creating thumbnails for local files.
// The native-side ThumbnailGenerator is owned by the Java-side and can be
// safely destroyed while a request is being processed.
class ThumbnailGenerator {
 public:
  explicit ThumbnailGenerator(const base::android::JavaParamRef<jobject>& jobj);

  // Destroys the ThumbnailGenerator.  Any currently running ImageRequest will
  // delete itself when it has completed.
  void Destroy(JNIEnv* env, const base::android::JavaParamRef<jobject>& jobj);

  // Kicks off an asynchronous process to retrieve the thumbnail for the file
  // located at |file_path| with a max size of |icon_size| in each dimension.
  // Invokes the Java #onthumbnailRetrieved(String, int, Bitmap, boolean) method
  // when finished.
  void RetrieveThumbnail(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& jobj,
      const base::android::JavaParamRef<jstring>& jcontent_id,
      const base::android::JavaParamRef<jstring>& jfile_path,
      jint icon_size,
      const base::android::JavaParamRef<jobject>& callback);

 private:
  ~ThumbnailGenerator();

  // Called when the thumbnail is ready.  |thumbnail| will be empty on failure.
  void OnThumbnailRetrieved(
      const base::android::ScopedJavaGlobalRef<jstring>& content_id,
      int icon_size,
      const base::android::ScopedJavaGlobalRef<jobject>& callback,
      const SkBitmap& thumbnail);

  // This is a {@link ThumbnailGenerator} Java object.
  base::android::ScopedJavaGlobalRef<jobject> java_delegate_;
  base::WeakPtrFactory<ThumbnailGenerator> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(ThumbnailGenerator);
};

#endif  // CHROME_BROWSER_ANDROID_WIDGET_THUMBNAIL_GENERATOR_H_
