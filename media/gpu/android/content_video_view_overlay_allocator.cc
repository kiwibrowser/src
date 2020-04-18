// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/gpu/android/content_video_view_overlay_allocator.h"
#include "base/threading/thread_task_runner_handle.h"

#include "media/gpu/android/avda_codec_allocator.h"

namespace media {

// static
ContentVideoViewOverlayAllocator*
ContentVideoViewOverlayAllocator::GetInstance() {
  static ContentVideoViewOverlayAllocator* allocator =
      new ContentVideoViewOverlayAllocator(
          AVDACodecAllocator::GetInstance(base::ThreadTaskRunnerHandle::Get()));
  return allocator;
}

ContentVideoViewOverlayAllocator::ContentVideoViewOverlayAllocator(
    AVDACodecAllocator* allocator)
    : allocator_(allocator) {}

ContentVideoViewOverlayAllocator::~ContentVideoViewOverlayAllocator() {}

bool ContentVideoViewOverlayAllocator::AllocateSurface(Client* client) {
  DCHECK(thread_checker_.CalledOnValidThread());

  const int32_t surface_id = client->GetSurfaceId();
  DVLOG(1) << __func__ << ": " << surface_id;
  DCHECK_NE(surface_id, SurfaceManager::kNoSurfaceID);

  // If it's not owned or being released, |client| now owns it.
  // Note: it's owned until it's released, since AVDACodecAllocator does that.
  // It keeps the bundle around (and also the overlay that's the current owner)
  // until the codec is done with it.  That's required to use AndroidOverlay.
  // So, we don't need to check for 'being released'; the owner is good enough.
  auto it = surface_owners_.find(surface_id);
  if (it == surface_owners_.end()) {
    OwnerRecord record;
    record.owner = client;
    surface_owners_.insert(OwnerMap::value_type(surface_id, record));
    return true;
  }

  // Otherwise |client| replaces the previous waiter (if any).
  OwnerRecord& record = it->second;
  if (record.waiter)
    record.waiter->OnSurfaceAvailable(false);
  record.waiter = client;
  return false;
}

void ContentVideoViewOverlayAllocator::DeallocateSurface(Client* client) {
  DCHECK(thread_checker_.CalledOnValidThread());

  const int32_t surface_id = client->GetSurfaceId();
  DCHECK_NE(surface_id, SurfaceManager::kNoSurfaceID);

  // If we time out waiting for the surface to be destroyed, then we might have
  // already removed |surface_id|.  If it's now trying to deallocate, then
  // maybe we just weren't patient enough, or mediaserver restarted.
  auto it = surface_owners_.find(surface_id);
  if (it == surface_owners_.end())
    return;

  OwnerRecord& record = it->second;
  if (record.owner == client)
    record.owner = nullptr;
  else if (record.waiter == client)
    record.waiter = nullptr;

  // Promote the waiter if possible.
  if (record.waiter && !record.owner) {
    record.owner = record.waiter;
    record.waiter = nullptr;
    record.owner->OnSurfaceAvailable(true);
    return;
  }

  // Remove the record if it's now unused.
  if (!record.owner && !record.waiter)
    surface_owners_.erase(it);
}

// During surface teardown we have to handle the following cases.
// 1) No AVDA has acquired the surface, or the surface has already been
//    completely released.
//    This case is easy -- there's no owner or waiter, and we can return.
//
// 2) A MediaCodec is currently being configured with the surface on another
//    thread. Whether an AVDA owns the surface or has already deallocated it,
//    the MediaCodec should be dropped when configuration completes.
//    In this case, we'll find an owner.  We'll notify it about the destruction.
//    Note that AVDA doesn't handle this case correctly right now, since it
//    doesn't know the state of codec creation on the codec thread.  This is
//    only a problem because CVV has the 'wait on main thread' semantics.
//
// 3) An AVDA owns the surface and it responds to OnSurfaceDestroyed() by:
//     a) Replacing the destroyed surface by calling MediaCodec#setSurface().
//     b) Releasing the MediaCodec it's attached to.
//     In case a, the surface will be destroyed during OnSurfaceDestroyed.
//     In case b, we'll have to wait for the release to complete.
//
// 4) No AVDA owns the surface, but the MediaCodec it's attached to is currently
//    being destroyed on another thread.
//    This is the same as 3b.
void ContentVideoViewOverlayAllocator::OnSurfaceDestroyed(int32_t surface_id) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DVLOG(1) << __func__ << ": " << surface_id;

  // If it isn't currently owned, then we're done.  Rememeber that the overlay
  // must outlive any user of it (MediaCodec!), and currently AVDACodecAllocator
  // is responsible for making sure that happens for AVDA.
  auto it = surface_owners_.find(surface_id);
  if (it == surface_owners_.end())
    return;

  // Notify the owner and waiter (if any).
  OwnerRecord& record = it->second;
  if (record.waiter) {
    record.waiter->OnSurfaceAvailable(false);
    record.waiter = nullptr;
  }

  DCHECK(record.owner);

  // |record| could be removed by the callback, if it deallocates the surface.
  record.owner->OnSurfaceDestroyed();

  // If owner deallocated the surface, then we don't need to wait.  Note that
  // the owner might have been deleted in that case.  Since CVVOverlay only
  // deallocates the surface during destruction, it's a safe bet.
  it = surface_owners_.find(surface_id);
  if (it == surface_owners_.end())
    return;

  // The surface is still in use, but should have been posted to another thread
  // for destruction.  Note that this isn't technically required for overlays
  // in general, but CVV requires it.  All of the pending release stuff should
  // be moved here, or to custom deleters of CVVOverlay.  However, in the
  // interest of not changing too much at once, we let AVDACodecAllocator
  // handle it.  Since we're deprecating CVVOverlay anyway, all of this can be
  // removed eventually.
  // If the wait fails, then clean up |surface_owners_| anyway, since the codec
  // release is probably hung up.
  if (!allocator_->WaitForPendingRelease(record.owner))
    surface_owners_.erase(it);
}

}  // namespace media
