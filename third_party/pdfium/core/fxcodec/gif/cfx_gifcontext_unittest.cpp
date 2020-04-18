// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxcodec/gif/cfx_gifcontext.h"

#include "core/fxcrt/unowned_ptr.h"
#include "testing/gtest/include/gtest/gtest.h"

class CFX_GifContextForTest : public CFX_GifContext {
 public:
  CFX_GifContextForTest(CCodec_GifModule* gif_module,
                        CCodec_GifModule::Delegate* delegate)
      : CFX_GifContext(gif_module, delegate) {}
  ~CFX_GifContextForTest() override {}

  using CFX_GifContext::ReadData;
  using CFX_GifContext::ReadGifSignature;
  using CFX_GifContext::ReadLogicalScreenDescriptor;

  CFX_MemoryStream* InputBuffer() const { return input_buffer_.Get(); }
};

TEST(CFX_GifContext, SetInputBuffer) {
  CFX_GifContextForTest context(nullptr, nullptr);

  context.SetInputBuffer(nullptr, 0);
  EXPECT_EQ(nullptr, context.InputBuffer()->GetBuffer());
  EXPECT_EQ(0, context.InputBuffer()->GetSize());
  EXPECT_EQ(0, context.InputBuffer()->GetPosition());

  context.SetInputBuffer(nullptr, 100);
  EXPECT_EQ(nullptr, context.InputBuffer()->GetBuffer());
  EXPECT_EQ(100, context.InputBuffer()->GetSize());
  EXPECT_EQ(0, context.InputBuffer()->GetPosition());

  uint8_t buffer[] = {0x00, 0x01, 0x02};
  context.SetInputBuffer(buffer, 0);
  EXPECT_EQ(buffer, context.InputBuffer()->GetBuffer());
  EXPECT_EQ(0, context.InputBuffer()->GetSize());
  EXPECT_EQ(0, context.InputBuffer()->GetPosition());

  context.SetInputBuffer(buffer, 3);
  EXPECT_EQ(buffer, context.InputBuffer()->GetBuffer());
  EXPECT_EQ(3, context.InputBuffer()->GetSize());
  EXPECT_EQ(0, context.InputBuffer()->GetPosition());

  context.SetInputBuffer(buffer, 100);
  EXPECT_EQ(buffer, context.InputBuffer()->GetBuffer());
  EXPECT_EQ(100, context.InputBuffer()->GetSize());
  EXPECT_EQ(0, context.InputBuffer()->GetPosition());
}

TEST(CFX_GifContext, ReadData) {
  CFX_GifContextForTest context(nullptr, nullptr);

  context.SetInputBuffer(nullptr, 0);
  EXPECT_FALSE(context.ReadData(nullptr, 0));
  EXPECT_FALSE(context.ReadData(nullptr, 10));

  std::vector<uint8_t> dest_buffer;
  EXPECT_FALSE(context.ReadData(dest_buffer.data(), 0));
  EXPECT_FALSE(context.ReadData(dest_buffer.data(), 10));

  uint8_t src_buffer[] = {0x00, 0x01, 0x02, 0x03, 0x04,
                          0x05, 0x06, 0x07, 0x08, 0x09};
  context.SetInputBuffer(src_buffer, 0);
  dest_buffer.resize(sizeof(src_buffer));
  EXPECT_FALSE(context.ReadData(dest_buffer.data(), sizeof(src_buffer)));

  context.SetInputBuffer(src_buffer, 1);
  EXPECT_FALSE(context.ReadData(dest_buffer.data(), sizeof(src_buffer)));
  EXPECT_EQ(0, context.InputBuffer()->GetPosition());
  EXPECT_FALSE(context.ReadData(nullptr, sizeof(src_buffer)));
  EXPECT_FALSE(context.ReadData(nullptr, 1));
  EXPECT_TRUE(context.ReadData(dest_buffer.data(), 1));
  EXPECT_EQ(src_buffer[0], dest_buffer[0]);

  context.SetInputBuffer(src_buffer, sizeof(src_buffer));
  EXPECT_FALSE(context.ReadData(nullptr, sizeof(src_buffer)));
  EXPECT_TRUE(context.ReadData(dest_buffer.data(), sizeof(src_buffer)));
  for (size_t i = 0; i < sizeof(src_buffer); i++)
    EXPECT_EQ(src_buffer[i], dest_buffer[i]);

  context.SetInputBuffer(src_buffer, sizeof(src_buffer));
  for (size_t i = 0; i < sizeof(src_buffer); i++) {
    EXPECT_TRUE(context.ReadData(dest_buffer.data(), 1));
    EXPECT_EQ(src_buffer[i], dest_buffer[0]);
  }
}

TEST(CFX_GifContext, ReadGifSignature) {
  CFX_GifContextForTest context(nullptr, nullptr);

  {
    uint8_t data[1];
    context.SetInputBuffer(data, 0);
    EXPECT_EQ(CFX_GifDecodeStatus::Unfinished, context.ReadGifSignature());
    EXPECT_EQ(0, context.InputBuffer()->GetPosition());
  }
  // Make sure testing the entire signature
  {
    uint8_t data[] = {'G', 'I', 'F'};
    context.SetInputBuffer(data, sizeof(data));
    EXPECT_EQ(CFX_GifDecodeStatus::Unfinished, context.ReadGifSignature());
    EXPECT_EQ(0, context.InputBuffer()->GetPosition());
  }
  {
    uint8_t data[] = {'N', 'O', 'T', 'G', 'I', 'F'};
    context.SetInputBuffer(data, sizeof(data));
    EXPECT_EQ(CFX_GifDecodeStatus::Error, context.ReadGifSignature());
    EXPECT_EQ(6, context.InputBuffer()->GetPosition());
  }
  // Make sure not matching GIF8*a
  {
    uint8_t data[] = {'G', 'I', 'F', '8', '0', 'a'};
    context.SetInputBuffer(data, sizeof(data));
    EXPECT_EQ(CFX_GifDecodeStatus::Error, context.ReadGifSignature());
    EXPECT_EQ(6, context.InputBuffer()->GetPosition());
  }
  // Make sure not matching GIF**a
  {
    uint8_t data[] = {'G', 'I', 'F', '9', '2', 'a'};
    context.SetInputBuffer(data, sizeof(data));
    EXPECT_EQ(CFX_GifDecodeStatus::Error, context.ReadGifSignature());
    EXPECT_EQ(6, context.InputBuffer()->GetPosition());
  }
  // One valid signature
  {
    uint8_t data[] = {'G', 'I', 'F', '8', '7', 'a'};
    context.SetInputBuffer(data, sizeof(data));
    EXPECT_EQ(CFX_GifDecodeStatus::Success, context.ReadGifSignature());
    EXPECT_EQ(6, context.InputBuffer()->GetPosition());
  }
  // The other valid signature
  {
    uint8_t data[] = {'G', 'I', 'F', '8', '9', 'a'};
    context.SetInputBuffer(data, sizeof(data));
    EXPECT_EQ(CFX_GifDecodeStatus::Success, context.ReadGifSignature());
    EXPECT_EQ(6, context.InputBuffer()->GetPosition());
  }
}

TEST(CFX_GifContext, ReadLocalScreenDescriptor) {
  CFX_GifContextForTest context(nullptr, nullptr);
  {
    uint8_t data[1];
    context.SetInputBuffer(data, 0);
    EXPECT_EQ(CFX_GifDecodeStatus::Unfinished,
              context.ReadLogicalScreenDescriptor());
  }
  // LSD with all the values zero'd
  {
    uint8_t lsd[sizeof(CFX_GifLocalScreenDescriptor)];
    memset(&lsd, 0, sizeof(CFX_GifLocalScreenDescriptor));
    context.SetInputBuffer(lsd, sizeof(CFX_GifLocalScreenDescriptor));

    EXPECT_EQ(CFX_GifDecodeStatus::Success,
              context.ReadLogicalScreenDescriptor());

    EXPECT_EQ(sizeof(CFX_GifLocalScreenDescriptor),
              static_cast<size_t>(context.InputBuffer()->GetPosition()));
    EXPECT_EQ(0, context.width_);
    EXPECT_EQ(0, context.height_);
    EXPECT_EQ(0u, context.bc_index_);
    EXPECT_EQ(0u, context.pixel_aspect_);
  }
  // LSD with no global palette
  {
    uint8_t lsd[sizeof(CFX_GifLocalScreenDescriptor)] = {0x0A, 0x00, 0x00, 0x0F,
                                                         0x00, 0x01, 0x02};
    context.SetInputBuffer(lsd, sizeof(CFX_GifLocalScreenDescriptor));

    EXPECT_EQ(CFX_GifDecodeStatus::Success,
              context.ReadLogicalScreenDescriptor());

    EXPECT_EQ(sizeof(CFX_GifLocalScreenDescriptor),
              static_cast<size_t>(context.InputBuffer()->GetPosition()));
    EXPECT_EQ(0x000A, context.width_);
    EXPECT_EQ(0x0F00, context.height_);
    EXPECT_EQ(0u, context.bc_index_);  // bc_index_ is 0 if no global palette
    EXPECT_EQ(2u, context.pixel_aspect_);
  }
  // LSD with global palette bit set, but no global palette
  {
    uint8_t lsd[sizeof(CFX_GifLocalScreenDescriptor)] = {0x0A, 0x00, 0x00, 0x0F,
                                                         0x80, 0x01, 0x02};
    context.SetInputBuffer(lsd, sizeof(CFX_GifLocalScreenDescriptor));

    EXPECT_EQ(CFX_GifDecodeStatus::Unfinished,
              context.ReadLogicalScreenDescriptor());

    EXPECT_EQ(0, context.InputBuffer()->GetPosition());
  }
  // LSD with global palette
  {
    struct {
      uint8_t lsd[sizeof(CFX_GifLocalScreenDescriptor)];
      uint8_t palette[4 * sizeof(CFX_GifPalette)];
    } data = {{0x0A, 0x00, 0x00, 0x0F, 0xA9, 0x01, 0x02},
              {0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1}};
    context.SetInputBuffer(reinterpret_cast<uint8_t*>(&data), sizeof(data));

    EXPECT_EQ(CFX_GifDecodeStatus::Success,
              context.ReadLogicalScreenDescriptor());

    EXPECT_EQ(sizeof(data),
              static_cast<size_t>(context.InputBuffer()->GetPosition()));
    EXPECT_EQ(0x000A, context.width_);
    EXPECT_EQ(0x0F00, context.height_);
    EXPECT_EQ(1u, context.bc_index_);
    EXPECT_EQ(2u, context.pixel_aspect_);

    EXPECT_EQ(1u, context.global_pal_exp_);
    EXPECT_EQ(1, context.global_sort_flag_);
    EXPECT_EQ(2, context.global_color_resolution_);
    EXPECT_TRUE(0 == memcmp(data.palette, context.global_palette_.data(),
                            sizeof(data.palette)));
  }
}

TEST(CFX_GifContext, ReadHeader) {
  CFX_GifContextForTest context(nullptr, nullptr);
  // Bad signature
  {
    struct {
      uint8_t signature[6];
      uint8_t lsd[sizeof(CFX_GifLocalScreenDescriptor)];
    } data = {{'N', 'O', 'T', 'G', 'I', 'F'},
              {0x0A, 0x00, 0x00, 0x0F, 0x00, 0x01, 0x02}};
    context.SetInputBuffer(reinterpret_cast<uint8_t*>(&data), sizeof(data));

    EXPECT_EQ(CFX_GifDecodeStatus::Error, context.ReadHeader());

    EXPECT_EQ(sizeof(data.signature),
              static_cast<size_t>(context.InputBuffer()->GetPosition()));
  }
  // Short after signature
  {
    uint8_t signature[] = {'G', 'I', 'F', '8', '7', 'a'};
    context.SetInputBuffer(reinterpret_cast<uint8_t*>(&signature),
                           sizeof(signature));

    EXPECT_EQ(CFX_GifDecodeStatus::Unfinished, context.ReadHeader());

    EXPECT_EQ(sizeof(signature),
              static_cast<size_t>(context.InputBuffer()->GetPosition()));
  }
  // Success without global palette
  {
    struct {
      uint8_t signature[6];
      uint8_t lsd[sizeof(CFX_GifLocalScreenDescriptor)];
    } data = {{'G', 'I', 'F', '8', '7', 'a'},
              {0x0A, 0x00, 0x00, 0x0F, 0x00, 0x01, 0x02}};
    context.SetInputBuffer(reinterpret_cast<uint8_t*>(&data), sizeof(data));

    EXPECT_EQ(CFX_GifDecodeStatus::Success, context.ReadHeader());

    EXPECT_EQ(sizeof(data),
              static_cast<size_t>(context.InputBuffer()->GetPosition()));
    EXPECT_EQ(0x000A, context.width_);
    EXPECT_EQ(0x0F00, context.height_);
    EXPECT_EQ(0u, context.bc_index_);  // bc_index_ is 0 if no global palette
    EXPECT_EQ(2u, context.pixel_aspect_);
  }
  // Missing Global Palette
  {
    struct {
      uint8_t signature[6];
      uint8_t lsd[sizeof(CFX_GifLocalScreenDescriptor)];
    } data = {{'G', 'I', 'F', '8', '7', 'a'},
              {0x0A, 0x00, 0x00, 0x0F, 0x80, 0x01, 0x02}};
    context.SetInputBuffer(reinterpret_cast<uint8_t*>(&data), sizeof(data));

    EXPECT_EQ(CFX_GifDecodeStatus::Unfinished, context.ReadHeader());

    EXPECT_EQ(sizeof(data.signature),
              static_cast<size_t>(context.InputBuffer()->GetPosition()));
  }
  // Success with global palette
  {
    struct {
      uint8_t signature[6];
      uint8_t lsd[sizeof(CFX_GifLocalScreenDescriptor)];
      uint8_t palette[4 * sizeof(CFX_GifPalette)];
    } data = {{'G', 'I', 'F', '8', '7', 'a'},
              {0x0A, 0x00, 0x00, 0x0F, 0xA9, 0x01, 0x02},
              {0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1}};
    context.SetInputBuffer(reinterpret_cast<uint8_t*>(&data), sizeof(data));

    EXPECT_EQ(CFX_GifDecodeStatus::Success, context.ReadHeader());

    EXPECT_EQ(sizeof(data),
              static_cast<size_t>(context.InputBuffer()->GetPosition()));
    EXPECT_EQ(0x000A, context.width_);
    EXPECT_EQ(0x0F00, context.height_);
    EXPECT_EQ(1u, context.bc_index_);
    EXPECT_EQ(2u, context.pixel_aspect_);
    EXPECT_EQ(1u, context.global_pal_exp_);
    EXPECT_EQ(1, context.global_sort_flag_);
    EXPECT_EQ(2, context.global_color_resolution_);
    EXPECT_TRUE(0 == memcmp(data.palette, context.global_palette_.data(),
                            sizeof(data.palette)));
  }
}
