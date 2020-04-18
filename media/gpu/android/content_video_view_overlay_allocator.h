// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_GPU_ANDROID_CONTENT_VIDEO_VIEW_OVERLAY_ALLOCATOR_H_
#define MEDIA_GPU_ANDROID_CONTENT_VIDEO_VIEW_OVERLAY_ALLOCATOR_H_

#include <stddef.h>

#include "base/containers/flat_map.h"
#include "base/macros.h"
#include "base/threading/thread_checker.h"
#include "media/base/android/android_overlay.h"
#include "media/gpu/media_gpu_export.h"

namespace media {

class AVDACodecAllocator;
class ContentVideoViewOverlayAllocatorTest;

// ContentVideoViewOverlayAllocator lets different instances of CVVOverlay that
// share the same surface ID to be synchronized with respect to each other.
// It also manages synchronous surface destruction.
class MEDIA_GPU_EXPORT ContentVideoViewOverlayAllocator {
 public:
  class Client : public AndroidOverlay {
   public:
    // Called when the requested SurfaceView becomes available after a call to
    // AllocateSurface()
    virtual void OnSurfaceAvailable(bool success) = 0;

    // Called when the allocated surface is being destroyed. This must either
    // replace the surface with MediaCodec#setSurface, or release the MediaCodec
    // it's attached to. The client no longer owns the surface and doesn't
    // need to call DeallocateSurface();
    virtual void OnSurfaceDestroyed() = 0;

    // Return the surface id of the client's ContentVideoView.
    virtual int32_t GetSurfaceId() = 0;

   protected:
    ~Client() override {}
  };

  static ContentVideoViewOverlayAllocator* GetInstance();

  // Called synchronously when the given surface is being destroyed on the
  // browser UI thread.
  void OnSurfaceDestroyed(int32_t surface_id);

  // Returns true if the caller now owns the surface, or false if someone else
  // owns the surface. |client| will be notified when the surface is available
  // via OnSurfaceAvailable().
  bool AllocateSurface(Client* client);

  // Relinquish ownership of the surface or stop waiting for it to be available.
  // The caller must guarantee that when calling this the surface is either no
  // longer attached to a MediaCodec, or the MediaCodec it was attached to is
  // was released with ReleaseMediaCodec().
  void DeallocateSurface(Client* client);

 private:
  friend class ContentVideoViewOverlayAllocatorTest;

  ContentVideoViewOverlayAllocator(AVDACodecAllocator* allocator);
  ~ContentVideoViewOverlayAllocator();

  struct OwnerRecord {
    Client* owner = nullptr;
    Client* waiter = nullptr;
  };

  // Indexed by surface id.
  using OwnerMap = base::flat_map<int32_t, OwnerRecord>;
  OwnerMap surface_owners_;

  AVDACodecAllocator* allocator_;

  base::ThreadChecker thread_checker_;

  DISALLOW_COPY_AND_ASSIGN(ContentVideoViewOverlayAllocator);
};

}  // namespace media

#endif  // MEDIA_GPU_ANDROID_CONTENT_VIDEO_VIEW_OVERLAY_ALLOCATOR_H_
