// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_ANDROID_JAVA_GIN_JAVA_JAVA_BRIDGE_THREAD_H_
#define CONTENT_BROWSER_ANDROID_JAVA_GIN_JAVA_JAVA_BRIDGE_THREAD_H_

#include "base/android/java_handler_thread.h"

namespace base {
class TaskRunner;
}

namespace content {

// The JavaBridge needs to use a Java thread so the callback
// will happen on a thread with a prepared Looper.
class JavaBridgeThread : public base::android::JavaHandlerThread {
 public:
  JavaBridgeThread();
  ~JavaBridgeThread() override;

  static bool CurrentlyOn();
  static base::TaskRunner* GetTaskRunner();
};

}  // namespace content

#endif  // CONTENT_BROWSER_ANDROID_JAVA_GIN_JAVA_JAVA_BRIDGE_THREAD_H_
