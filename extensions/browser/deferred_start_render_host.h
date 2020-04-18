// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_DEFERRED_START_RENDER_HOST_H_
#define EXTENSIONS_BROWSER_DEFERRED_START_RENDER_HOST_H_

namespace extensions {
class DeferredStartRenderHostObserver;

// A browser component that tracks a renderer. It allows for its renderer
// startup to be deferred, to throttle resource usage upon profile startup.
// To be used with ExtensionHostQueue.
//
// Note that if BackgroundContents and ExtensionHost are unified
// (crbug.com/77790), this interface will be no longer needed.
class DeferredStartRenderHost {
 public:
  virtual ~DeferredStartRenderHost() {}

  // DeferredStartRenderHost lifetime can be observed.
  virtual void AddDeferredStartRenderHostObserver(
      DeferredStartRenderHostObserver* observer) = 0;
  virtual void RemoveDeferredStartRenderHostObserver(
      DeferredStartRenderHostObserver* observer) = 0;

  // DO NOT CALL THIS unless you're implementing an ExtensionHostQueue.
  // Called by the ExtensionHostQueue to create the RenderView.
  virtual void CreateRenderViewNow() = 0;
};

}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_DEFERRED_START_RENDER_HOST_H_
