// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gfx/linux/native_pixmap_dmabuf.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace gfx {

class NativePixmapDmabufTest : public testing::Test {
 protected:
  gfx::NativePixmapHandle CreateMockNativePixmapHandle(gfx::Size image_size) {
    gfx::NativePixmapHandle handle;

    for (int i = 0; i < 4; ++i) {
      // These values are arbitrarily chosen to be different from each other.
      const int stride = (i + 1) * image_size.width();
      const int offset = i * image_size.width() * image_size.height();
      const uint64_t size = stride * image_size.height();
      const uint64_t modifiers = 1 << i;

      handle.fds.emplace_back(
          base::FileDescriptor(i + 1, true /* auto_close */));

      handle.planes.emplace_back(stride, offset, size, modifiers);
    }

    return handle;
  }
};

// Verifies NativePixmapDmaBuf conversion from and to NativePixmapHandle.
TEST_F(NativePixmapDmabufTest, Convert) {
  const gfx::Size image_size(128, 64);
  const gfx::BufferFormat format = gfx::BufferFormat::RGBX_8888;

  gfx::NativePixmapHandle origin_handle =
      CreateMockNativePixmapHandle(image_size);

  // NativePixmapHandle to NativePixmapDmabuf
  scoped_refptr<gfx::NativePixmap> native_pixmap_dmabuf(
      new gfx::NativePixmapDmaBuf(image_size, format, origin_handle));
  EXPECT_TRUE(native_pixmap_dmabuf->AreDmaBufFdsValid());

  // NativePixmap to NativePixmapHandle.
  gfx::NativePixmapHandle handle;
  for (size_t i = 0; i < native_pixmap_dmabuf->GetDmaBufFdCount(); ++i) {
    handle.fds.emplace_back(base::FileDescriptor(
        native_pixmap_dmabuf->GetDmaBufFd(i), true /* auto_close */));

    handle.planes.emplace_back(
        native_pixmap_dmabuf->GetDmaBufPitch(i),
        native_pixmap_dmabuf->GetDmaBufOffset(i),
        native_pixmap_dmabuf->GetDmaBufPitch(i) * image_size.height(),
        native_pixmap_dmabuf->GetDmaBufModifier(i));
  }

  // NativePixmapHandle is unchanged during convertion to NativePixmapDmabuf.
  EXPECT_EQ(origin_handle.fds, handle.fds);
  EXPECT_EQ(origin_handle.fds.size(), handle.planes.size());
  EXPECT_EQ(origin_handle.planes.size(), handle.planes.size());
  for (size_t i = 0; i < origin_handle.planes.size(); ++i) {
    EXPECT_EQ(origin_handle.planes[i].stride, handle.planes[i].stride);
    EXPECT_EQ(origin_handle.planes[i].offset, handle.planes[i].offset);
    EXPECT_EQ(origin_handle.planes[i].size, handle.planes[i].size);
    EXPECT_EQ(origin_handle.planes[i].modifier, handle.planes[i].modifier);
  }
}

}  // namespace gfx
