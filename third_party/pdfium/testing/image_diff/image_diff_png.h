// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TESTING_IMAGE_DIFF_IMAGE_DIFF_PNG_H_
#define TESTING_IMAGE_DIFF_IMAGE_DIFF_PNG_H_

#include <stdlib.h>  // for size_t.

#include <vector>

namespace image_diff_png {

// Decode a PNG into an RGBA pixel array.
bool DecodePNG(const unsigned char* input,
               size_t input_size,
               std::vector<unsigned char>* output,
               int* width,
               int* height);

// Encode a BGR pixel array into a PNG.
bool EncodeBGRPNG(const unsigned char* input,
                  int width,
                  int height,
                  int row_byte_width,
                  std::vector<unsigned char>* output);

// Encode an RGBA pixel array into a PNG.
bool EncodeRGBAPNG(const unsigned char* input,
                   int width,
                   int height,
                   int row_byte_width,
                   std::vector<unsigned char>* output);

// Encode an BGRA pixel array into a PNG.
bool EncodeBGRAPNG(const unsigned char* input,
                   int width,
                   int height,
                   int row_byte_width,
                   bool discard_transparency,
                   std::vector<unsigned char>* output);

// Encode a grayscale pixel array into a PNG.
bool EncodeGrayPNG(const unsigned char* input,
                   int width,
                   int height,
                   int row_byte_width,
                   std::vector<unsigned char>* output);

}  // namespace image_diff_png

#endif  // TESTING_IMAGE_DIFF_IMAGE_DIFF_PNG_H_
