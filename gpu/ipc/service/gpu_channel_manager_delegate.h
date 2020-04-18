// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_IPC_SERVICE_GPU_CHANNEL_MANAGER_DELEGATE_H_
#define GPU_IPC_SERVICE_GPU_CHANNEL_MANAGER_DELEGATE_H_

#include "gpu/command_buffer/common/constants.h"
#include "gpu/ipc/common/surface_handle.h"

class GURL;

namespace gpu {

class GpuChannelManagerDelegate {
 public:
  // Called on any successful context creation.
  virtual void DidCreateContextSuccessfully() = 0;

  // Tells the delegate that an offscreen context was created for the provided
  // |active_url|.
  virtual void DidCreateOffscreenContext(const GURL& active_url) = 0;

  // Notification from GPU that the channel is destroyed.
  virtual void DidDestroyChannel(int client_id) = 0;

  // Tells the delegate that an offscreen context was destroyed for the provided
  // |active_url|.
  virtual void DidDestroyOffscreenContext(const GURL& active_url) = 0;

  // Tells the delegate that a context was lost.
  virtual void DidLoseContext(bool offscreen,
                              error::ContextLostReason reason,
                              const GURL& active_url) = 0;

  // Tells the delegate to cache the given shader information in persistent
  // storage. The embedder is expected to repopulate the in-memory cache through
  // the respective GpuChannelManager API.
  virtual void StoreShaderToDisk(int32_t client_id,
                                 const std::string& key,
                                 const std::string& shader) = 0;

#if defined(OS_WIN)
  virtual void SendAcceleratedSurfaceCreatedChildWindow(
      SurfaceHandle parent_window,
      SurfaceHandle child_window) = 0;
#endif

  // Sets the currently active URL.  Use GURL() to clear the URL.
  virtual void SetActiveURL(const GURL& url) = 0;

 protected:
  virtual ~GpuChannelManagerDelegate() = default;
};

}  // namespace gpu

#endif  // GPU_IPC_SERVICE_GPU_CHANNEL_MANAGER_DELEGATE_H_
