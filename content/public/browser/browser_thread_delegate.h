// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_BROWSER_THREAD_DELEGATE_H_
#define CONTENT_PUBLIC_BROWSER_BROWSER_THREAD_DELEGATE_H_

#include "content/common/content_export.h"

namespace content {

// A Delegate for content embedders to perform extra initialization/cleanup on
// BrowserThread::IO.
class BrowserThreadDelegate {
 public:
  virtual ~BrowserThreadDelegate() = default;

  // Called prior to completing initialization of BrowserThread::IO.
  virtual void Init() = 0;

  // Called during teardown of BrowserThread::IO.
  virtual void CleanUp() = 0;
};

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_BROWSER_THREAD_DELEGATE_H_
