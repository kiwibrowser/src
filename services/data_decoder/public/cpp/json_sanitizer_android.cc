// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/data_decoder/public/cpp/json_sanitizer.h"

#include "base/android/jni_string.h"
#include "base/bind.h"
#include "base/callback.h"
#include "base/location.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/string_util.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "jni/JsonSanitizer_jni.h"

using base::android::JavaParamRef;

namespace data_decoder {

namespace {

// An implementation of JsonSanitizer that calls into Java. It deals with
// malformed input (in particular malformed Unicode encodings) in the following
// steps:
// 1. The input string is checked for whether it is well-formed UTF-8. Malformed
//    UTF-8 is rejected.
// 2. The UTF-8 string is converted in native code to a Java String, which is
//    encoded as UTF-16.
// 2. The Java String is parsed as JSON in the memory-safe environment of the
//    Java VM and any string literals are unescaped.
// 3. The string literals themselves are now untrusted, so they are checked in
//    Java for whether they are valid UTF-16.
// 4. The parsed JSON with sanitized literals is encoded back into a Java
//    String and passed back to native code.
// 5. The Java String is converted back to UTF-8 in native code.
// This ensures that both invalid UTF-8 and invalid escaped UTF-16 will be
// rejected.
class JsonSanitizerAndroid : public JsonSanitizer {
 public:
  JsonSanitizerAndroid(const StringCallback& success_callback,
                       const StringCallback& error_callback);
  ~JsonSanitizerAndroid() {}

  void Sanitize(const std::string& unsafe_json);

  void OnSuccess(const std::string& json);
  void OnError(const std::string& error);

 private:
  StringCallback success_callback_;
  StringCallback error_callback_;

  DISALLOW_COPY_AND_ASSIGN(JsonSanitizerAndroid);
};

JsonSanitizerAndroid::JsonSanitizerAndroid(
    const StringCallback& success_callback,
    const StringCallback& error_callback)
    : success_callback_(success_callback), error_callback_(error_callback) {}

void JsonSanitizerAndroid::Sanitize(const std::string& unsafe_json) {
  // The JSON parser only accepts wellformed UTF-8.
  if (!base::IsStringUTF8(unsafe_json)) {
    OnError("Unsupported encoding");
    return;
  }

  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedJavaLocalRef<jstring> unsafe_json_java =
      base::android::ConvertUTF8ToJavaString(env, unsafe_json);

  // This will synchronously call either OnSuccess() or OnError().
  Java_JsonSanitizer_sanitize(env, reinterpret_cast<jlong>(this),
                              unsafe_json_java);
}

void JsonSanitizerAndroid::OnSuccess(const std::string& json) {
  base::SequencedTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::Bind(success_callback_, json));
}

void JsonSanitizerAndroid::OnError(const std::string& error) {
  base::SequencedTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::Bind(error_callback_, error));
}

}  // namespace

void JNI_JsonSanitizer_OnSuccess(JNIEnv* env,
                                 const JavaParamRef<jclass>& clazz,
                                 jlong jsanitizer,
                                 const JavaParamRef<jstring>& json) {
  JsonSanitizerAndroid* sanitizer =
      reinterpret_cast<JsonSanitizerAndroid*>(jsanitizer);
  sanitizer->OnSuccess(base::android::ConvertJavaStringToUTF8(env, json));
}

void JNI_JsonSanitizer_OnError(JNIEnv* env,
                               const JavaParamRef<jclass>& clazz,
                               jlong jsanitizer,
                               const JavaParamRef<jstring>& error) {
  JsonSanitizerAndroid* sanitizer =
      reinterpret_cast<JsonSanitizerAndroid*>(jsanitizer);
  sanitizer->OnError(base::android::ConvertJavaStringToUTF8(env, error));
}

// static
void JsonSanitizer::Sanitize(service_manager::Connector* connector,
                             const std::string& unsafe_json,
                             const StringCallback& success_callback,
                             const StringCallback& error_callback) {
  // JsonSanitizerAndroid does all its work synchronously, but posts any
  // callbacks to the current sequence. This means it can be destroyed at
  // the end of this method.
  JsonSanitizerAndroid sanitizer(success_callback, error_callback);
  sanitizer.Sanitize(unsafe_json);
}

}  // namespace data_decoder
