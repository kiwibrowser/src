// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_CHROME_CONTEXT_UTIL_H_
#define CHROME_BROWSER_ANDROID_CHROME_CONTEXT_UTIL_H_

namespace chrome {
namespace android {

class ChromeContextUtil {
 public:
  // Smallest possible screen size in density-independent pixels.
  static int GetSmallestDIPWidth();

 private:
  ChromeContextUtil();
};

}  // namespace android
}  // namespace chrome

#endif  // CHROME_BROWSER_ANDROID_CHROME_CONTEXT_UTIL_H_
