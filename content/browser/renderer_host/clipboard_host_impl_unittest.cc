// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/clipboard_host_impl.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "base/memory/ref_counted.h"
#include "base/process/process_handle.h"
#include "base/run_loop.h"
#include "content/browser/blob_storage/chrome_blob_storage_context.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "mojo/public/cpp/system/platform_handle.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/base/test/test_clipboard.h"
#include "ui/gfx/geometry/size.h"

namespace content {

class ClipboardHostImplTest : public ::testing::Test {
 protected:
  ClipboardHostImplTest()
      : host_(new ClipboardHostImpl(nullptr)),
        clipboard_(ui::TestClipboard::CreateForCurrentThread()) {}

  ~ClipboardHostImplTest() override {
    ui::Clipboard::DestroyClipboardForCurrentThread();
  }

  void CallWriteImage(const gfx::Size& size,
                      mojo::ScopedSharedBufferHandle shared_memory,
                      size_t shared_memory_size) {
    host_->WriteImage(
        ui::CLIPBOARD_TYPE_COPY_PASTE, size,
        shared_memory->Clone(mojo::SharedBufferHandle::AccessMode::READ_ONLY));
  }

  void CallCommitWrite() {
    host_->CommitWrite(ui::CLIPBOARD_TYPE_COPY_PASTE);
    base::RunLoop().RunUntilIdle();
  }

  ui::Clipboard* clipboard() { return clipboard_; }

 private:
  const TestBrowserThreadBundle thread_bundle_;
  const std::unique_ptr<ClipboardHostImpl> host_;
  ui::Clipboard* const clipboard_;
};

// Test that it actually works.
TEST_F(ClipboardHostImplTest, SimpleImage) {
  static const uint32_t bitmap_data[] = {
      0x33333333, 0xdddddddd, 0xeeeeeeee, 0x00000000, 0x88888888, 0x66666666,
      0x55555555, 0xbbbbbbbb, 0x44444444, 0xaaaaaaaa, 0x99999999, 0x77777777,
      0xffffffff, 0x11111111, 0x22222222, 0xcccccccc,
  };

  mojo::ScopedSharedBufferHandle shared_memory =
      mojo::SharedBufferHandle::Create(sizeof(bitmap_data));
  mojo::ScopedSharedBufferMapping mapping =
      shared_memory->Map(sizeof(bitmap_data));

  memcpy(mapping.get(), bitmap_data, sizeof(bitmap_data));

  CallWriteImage(gfx::Size(4, 4), std::move(shared_memory),
                 sizeof(bitmap_data));
  uint64_t sequence_number =
      clipboard()->GetSequenceNumber(ui::CLIPBOARD_TYPE_COPY_PASTE);
  CallCommitWrite();

  EXPECT_NE(sequence_number,
            clipboard()->GetSequenceNumber(ui::CLIPBOARD_TYPE_COPY_PASTE));
  EXPECT_FALSE(clipboard()->IsFormatAvailable(
      ui::Clipboard::GetPlainTextFormatType(), ui::CLIPBOARD_TYPE_COPY_PASTE));
  EXPECT_TRUE(clipboard()->IsFormatAvailable(
      ui::Clipboard::GetBitmapFormatType(), ui::CLIPBOARD_TYPE_COPY_PASTE));

  SkBitmap actual = clipboard()->ReadImage(ui::CLIPBOARD_TYPE_COPY_PASTE);
  EXPECT_EQ(sizeof(bitmap_data), actual.computeByteSize());
  EXPECT_EQ(0,
            memcmp(bitmap_data, actual.getAddr32(0, 0), sizeof(bitmap_data)));
}

// Test with a size that would overflow a naive 32-bit row bytes calculation.
TEST_F(ClipboardHostImplTest, ImageSizeOverflows32BitRowBytes) {
  mojo::ScopedSharedBufferHandle shared_memory =
      mojo::SharedBufferHandle::Create(0x20000000);

  mojo::ScopedSharedBufferMapping mapping = shared_memory->Map(0x20000000);

  CallWriteImage(gfx::Size(0x20000000, 1), std::move(shared_memory),
                 0x20000000);
  uint64_t sequence_number =
      clipboard()->GetSequenceNumber(ui::CLIPBOARD_TYPE_COPY_PASTE);
  CallCommitWrite();

  EXPECT_EQ(sequence_number,
            clipboard()->GetSequenceNumber(ui::CLIPBOARD_TYPE_COPY_PASTE));
}

}  // namespace content
