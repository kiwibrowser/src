// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/app/android/chrome_main_delegate_android.h"

ChromeMainDelegateAndroid* ChromeMainDelegateAndroid::Create() {
  return new ChromeMainDelegateAndroid();
}
