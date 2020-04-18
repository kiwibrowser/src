// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/base/android/android_util.h"

#include <stddef.h>
#include <vector>

#include "base/android/jni_android.h"
#include "base/android/jni_array.h"

namespace media {

std::string JavaBytesToString(JNIEnv* env, jbyteArray j_byte_array) {
  std::vector<uint8_t> byte_vector;
  base::android::JavaByteArrayToByteVector(env, j_byte_array, &byte_vector);
  return std::string(byte_vector.begin(), byte_vector.end());
}

base::android::ScopedJavaLocalRef<jbyteArray> StringToJavaBytes(
    JNIEnv* env,
    const std::string& str) {
  return base::android::ToJavaByteArray(
      env, reinterpret_cast<const uint8_t*>(str.data()), str.size());
}

JavaObjectPtr CreateJavaObjectPtr(jobject object) {
  JavaObjectPtr j_object_ptr(new base::android::ScopedJavaGlobalRef<jobject>());
  j_object_ptr->Reset(base::android::AttachCurrentThread(), object);
  return j_object_ptr;
}

}  // namespace media
