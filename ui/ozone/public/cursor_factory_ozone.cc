// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/public/cursor_factory_ozone.h"

#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/threading/thread_local.h"

namespace ui {

namespace {

// TODO(mfomitchev): crbug.com/741106
// Until the above bug is fixed, CursorFactoryOzone singleton needs to be
// thread-local, because Ash creates its own instance.
base::LazyInstance<base::ThreadLocalPointer<CursorFactoryOzone>>::Leaky
    lazy_tls_ptr = LAZY_INSTANCE_INITIALIZER;

}  // namespace

CursorFactoryOzone::CursorFactoryOzone() {
  DCHECK(!lazy_tls_ptr.Pointer()->Get())
      << "There should only be a single CursorFactoryOzone per thread.";
  lazy_tls_ptr.Pointer()->Set(this);
}

CursorFactoryOzone::~CursorFactoryOzone() {
  DCHECK_EQ(lazy_tls_ptr.Pointer()->Get(), this);
  lazy_tls_ptr.Pointer()->Set(nullptr);
}

CursorFactoryOzone* CursorFactoryOzone::GetInstance() {
  CursorFactoryOzone* result = lazy_tls_ptr.Pointer()->Get();
  DCHECK(result) << "No CursorFactoryOzone implementation set.";
  return result;
}

PlatformCursor CursorFactoryOzone::GetDefaultCursor(CursorType type) {
  NOTIMPLEMENTED();
  return NULL;
}

PlatformCursor CursorFactoryOzone::CreateImageCursor(const SkBitmap& bitmap,
                                                     const gfx::Point& hotspot,
                                                     float bitmap_dpi) {
  NOTIMPLEMENTED();
  return NULL;
}

PlatformCursor CursorFactoryOzone::CreateAnimatedCursor(
    const std::vector<SkBitmap>& bitmaps,
    const gfx::Point& hotspot,
    int frame_delay_ms,
    float bitmap_dpi) {
  NOTIMPLEMENTED();
  return NULL;
}

void CursorFactoryOzone::RefImageCursor(PlatformCursor cursor) {
  NOTIMPLEMENTED();
}

void CursorFactoryOzone::UnrefImageCursor(PlatformCursor cursor) {
  NOTIMPLEMENTED();
}

}  // namespace ui
