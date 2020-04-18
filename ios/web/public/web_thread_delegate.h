// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_PUBLIC_WEB_THREAD_DELEGATE_H_
#define IOS_WEB_PUBLIC_WEB_THREAD_DELEGATE_H_

namespace web {

// A class with this type may be registered via WebThread::SetDelegate.
//
// If registered as such, it will schedule to run Init() before the
// message loop begins, and receive a CleanUp() call right after the message
// loop ends (and before the WebThread has done its own clean-up).
class WebThreadDelegate {
 public:
  virtual ~WebThreadDelegate() {}

  // Called prior to starting the message loop
  virtual void Init() = 0;

  // Called just after the message loop ends.
  virtual void CleanUp() = 0;
};

}  // namespace web

#endif  // IOS_WEB_PUBLIC_WEB_THREAD_DELEGATE_H_
