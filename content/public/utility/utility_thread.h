// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_UTILITY_UTILITY_THREAD_H_
#define CONTENT_PUBLIC_UTILITY_UTILITY_THREAD_H_

#include "build/build_config.h"
#include "content/public/child/child_thread.h"

namespace service_manager {
class Connector;
}

namespace content {

class CONTENT_EXPORT UtilityThread : virtual public ChildThread {
 public:
  // Returns the one utility thread for this process.  Note that this can only
  // be accessed when running on the utility thread itself.
  static UtilityThread* Get();

  UtilityThread();
  ~UtilityThread() override;

  // Releases the process.
  virtual void ReleaseProcess() = 0;

  // Initializes blink if it hasn't already been initialized.
  virtual void EnsureBlinkInitialized() = 0;

#if defined(OS_POSIX) && !defined(OS_ANDROID) && !defined(OS_FUCHSIA)
  // Initializes blink with web sandbox support.
  virtual void EnsureBlinkInitializedWithSandboxSupport() = 0;
#endif

#if defined(OS_MACOSX)
  // Initializes a connection to FontLoaderMac service.
  // Generic utility processes don't possess the content_browser's font_loader
  // capability to access the interface. Specialized utility processes, such as
  // pdf_compositor service, do require it.
  virtual void InitializeFontLoaderMac(
      service_manager::Connector* connector) = 0;
#endif
};

}  // namespace content

#endif  // CONTENT_PUBLIC_UTILITY_UTILITY_THREAD_H_
