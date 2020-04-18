// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/files/file_path.h"
#include "content/test/image_decoder_test.h"
#include "third_party/blink/public/web/web_image_decoder.h"

using blink::WebImageDecoder;

class ICOImageDecoderTest : public ImageDecoderTest {
 public:
  ICOImageDecoderTest() : ImageDecoderTest("ico") { }

 protected:
  blink::WebImageDecoder* CreateWebKitImageDecoder() const override {
    return new blink::WebImageDecoder(blink::WebImageDecoder::kTypeICO);
  }
};

TEST_F(ICOImageDecoderTest, Decoding) {
  TestDecoding();
}

TEST_F(ICOImageDecoderTest, ImageNonZeroFrameIndex) {
  if (data_dir().empty())
    return;
  // Test that the decoder decodes multiple sizes of icons which have them.
  // Load an icon that has both favicon-size and larger entries.
  base::FilePath multisize_icon_path(data_dir().AppendASCII("yahoo.ico"));
  const base::FilePath md5_sum_path(
      GetMD5SumPath(multisize_icon_path).value() + FILE_PATH_LITERAL("2"));
  static const int kDesiredFrameIndex = 3;
  TestWebKitImageDecoder(multisize_icon_path, md5_sum_path, kDesiredFrameIndex);
}
