// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_SNAPSHOT_SNAPSHOT_H_
#define UI_SNAPSHOT_SNAPSHOT_H_

#include <vector>

#include "base/callback_forward.h"
#include "base/memory/ref_counted.h"
#include "base/memory/ref_counted_memory.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/snapshot/snapshot_export.h"

namespace gfx {
class Rect;
class Image;
class Size;
}

namespace ui {

// Grabs a snapshot of the window/view. No security checks are done. This is
// intended to be used for debugging purposes where no BrowserProcess instance
// is available (ie. tests). This function is synchronous, so it should NOT be
// used in a result of user action. Support for async vs synchronous
// GrabWindowSnapshot differs by platform.  To be most general, use the
// synchronous function first and if it returns false call the async one.
SNAPSHOT_EXPORT bool GrabWindowSnapshot(gfx::NativeWindow window,
                                        const gfx::Rect& snapshot_bounds,
                                        gfx::Image* image);

SNAPSHOT_EXPORT bool GrabViewSnapshot(gfx::NativeView view,
                                      const gfx::Rect& snapshot_bounds,
                                      gfx::Image* image);

// These functions take a snapshot of |source_rect|, specified in layer space
// coordinates (DIP for desktop, physical pixels for Android), and scale the
// snapshot to |target_size| (in physical pixels), asynchronously.
typedef base::Callback<void(gfx::Image snapshot)>
    GrabWindowSnapshotAsyncCallback;
SNAPSHOT_EXPORT void GrabWindowSnapshotAndScaleAsync(
    gfx::NativeWindow window,
    const gfx::Rect& source_rect,
    const gfx::Size& target_size,
    const GrabWindowSnapshotAsyncCallback& callback);

SNAPSHOT_EXPORT void GrabWindowSnapshotAsync(
    gfx::NativeWindow window,
    const gfx::Rect& source_rect,
    const GrabWindowSnapshotAsyncCallback& callback);

SNAPSHOT_EXPORT void GrabViewSnapshotAsync(
    gfx::NativeView view,
    const gfx::Rect& source_rect,
    const GrabWindowSnapshotAsyncCallback& callback);

using GrabWindowSnapshotAsyncPNGCallback =
    base::Callback<void(scoped_refptr<base::RefCountedMemory> data)>;
SNAPSHOT_EXPORT void GrabWindowSnapshotAsyncPNG(
    gfx::NativeWindow window,
    const gfx::Rect& source_rect,
    const GrabWindowSnapshotAsyncPNGCallback& callback);

using GrabWindowSnapshotAsyncJPEGCallback =
    base::Callback<void(scoped_refptr<base::RefCountedMemory> data)>;
SNAPSHOT_EXPORT void GrabWindowSnapshotAsyncJPEG(
    gfx::NativeWindow window,
    const gfx::Rect& source_rect,
    const GrabWindowSnapshotAsyncJPEGCallback& callback);

}  // namespace ui

#endif  // UI_SNAPSHOT_SNAPSHOT_H_
