// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/platform/drm/gpu/mock_dumb_buffer_generator.h"

#include "third_party/skia/include/core/SkImageInfo.h"
#include "ui/ozone/platform/drm/gpu/drm_buffer.h"

namespace ui {

MockDumbBufferGenerator::MockDumbBufferGenerator() {}

MockDumbBufferGenerator::~MockDumbBufferGenerator() {}

scoped_refptr<ScanoutBuffer> MockDumbBufferGenerator::Create(
    const scoped_refptr<DrmDevice>& drm,
    uint32_t format,
    const std::vector<uint64_t>& modifiers,
    const gfx::Size& size) {
  scoped_refptr<DrmBuffer> buffer(new DrmBuffer(drm));
  SkImageInfo info = SkImageInfo::MakeN32Premul(size.width(), size.height());
  if (!buffer->Initialize(info, true /* should_register_framebuffer */))
    return NULL;

  return buffer;
}

}  // namespace ui
