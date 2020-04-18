// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_WEB_RESTRICTIONS_BROWSER_WEB_RESTRICTIONS_CLIENT_RESULT_H_
#define COMPONENTS_WEB_RESTRICTIONS_BROWSER_WEB_RESTRICTIONS_CLIENT_RESULT_H_

#include <string>

#include "base/android/jni_android.h"
#include "base/android/scoped_java_ref.h"

namespace web_restrictions {

// Wrapper for Java WebRestrictionsResult
class WebRestrictionsClientResult {
 public:
  WebRestrictionsClientResult(
      base::android::ScopedJavaGlobalRef<jobject>& jresult);
  WebRestrictionsClientResult(const WebRestrictionsClientResult& other);
  ~WebRestrictionsClientResult();

  bool ShouldProceed() const;
  int GetColumnCount() const;
  bool IsString(int column) const;
  int GetInt(int column) const;
  std::string GetString(int column) const;
  std::string GetColumnName(int column) const;

 private:
  base::android::ScopedJavaGlobalRef<jobject> jresult_;
};

}  // namespace web_restrictions

#endif  // COMPONENTS_WEB_RESTRICTIONS_BROWSER_WEB_RESTRICTIONS_CLIENT_RESULT_H_
