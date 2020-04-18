// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>
#include <stdint.h>

#include <vector>

#include "base/bind.h"
#include "base/callback_helpers.h"
#define PNG_INTERNAL
#include "third_party/libpng/png.h"

void* limited_malloc(png_structp, png_alloc_size_t size) {
  // libpng may allocate large amounts of memory that the fuzzer reports as
  // an error. In order to silence these errors, make libpng fail when trying
  // to allocate a large amount.
  // This number is chosen to match the default png_user_chunk_malloc_max.
  if (size > 8000000)
    return nullptr;

  return malloc(size);
}

void default_free(png_structp, png_voidp ptr) {
  return free(ptr);
}

#ifndef PNG_FUZZ_PROGRESSIVE

// Read sequentially, with png_read_row.
struct BufState {
  const uint8_t* data;
  size_t bytes_left;
};

void user_read_data(png_structp png_ptr, png_bytep data, png_size_t length) {
  BufState* buf_state = static_cast<BufState*>(png_get_io_ptr(png_ptr));
  if (length > buf_state->bytes_left) {
    png_error(png_ptr, "read error");
  }
  memcpy(data, buf_state->data, length);
  buf_state->bytes_left -= length;
  buf_state->data += length;
}

#endif  // PNG_FUZZ_PROGRESSIVE

static const int kPngHeaderSize = 8;

// Entry point for LibFuzzer.
// Roughly follows the libpng book example:
// http://www.libpng.org/pub/png/book/chapter13.html
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  if (size < kPngHeaderSize) {
    return 0;
  }

  std::vector<unsigned char> v(data, data + size);
  if (png_sig_cmp(v.data(), 0, kPngHeaderSize)) {
    // not a PNG.
    return 0;
  }

  png_structp png_ptr = png_create_read_struct
    (PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
  assert(png_ptr);

#ifdef MEMORY_SANITIZER
  // To avoid OOM with MSan (crbug.com/648073). These values are recommended as
  // safe settings by https://github.com/glennrp/libpng/blob/libpng16/pngusr.dfa
  png_set_user_limits(png_ptr, 65535, 65535);
#endif

  // Not all potential OOM are due to images with large widths and heights.
  // Use a custom allocator that fails for large allocations.
  png_set_mem_fn(png_ptr, nullptr, limited_malloc, default_free);

  png_set_crc_action(png_ptr, PNG_CRC_QUIET_USE, PNG_CRC_QUIET_USE);

  png_infop info_ptr = png_create_info_struct(png_ptr);
  assert(info_ptr);

  base::ScopedClosureRunner struct_deleter(base::Bind(
        &png_destroy_read_struct, &png_ptr, &info_ptr, nullptr));

#ifdef PNG_FUZZ_PROGRESSIVE
  if (setjmp(png_jmpbuf(png_ptr))) {
    return 0;
  }

  png_set_progressive_read_fn(png_ptr, nullptr, nullptr, nullptr, nullptr);
  png_process_data(png_ptr, info_ptr, const_cast<uint8_t*>(data), size);
#else
  // Setting up reading from buffer.
  std::unique_ptr<BufState> buf_state(new BufState());
  buf_state->data = data + kPngHeaderSize;
  buf_state->bytes_left = size - kPngHeaderSize;
  png_set_read_fn(png_ptr, buf_state.get(), user_read_data);
  png_set_sig_bytes(png_ptr, kPngHeaderSize);

  // libpng error handling.
  if (setjmp(png_jmpbuf(png_ptr))) {
    return 0;
  }

  // Reading.
  png_read_info(png_ptr, info_ptr);
  png_voidp row = png_malloc(png_ptr, png_get_rowbytes(png_ptr, info_ptr));
  base::ScopedClosureRunner png_deleter(base::Bind(
        &png_free, png_ptr, row));

  // reset error handler to put png_deleter into scope.
  if (setjmp(png_jmpbuf(png_ptr))) {
    return 0;
  }

  png_uint_32 width, height;
  int bit_depth, color_type, interlace_type, compression_type;
  int filter_type;

  if (!png_get_IHDR(png_ptr, info_ptr, &width, &height,
        &bit_depth, &color_type, &interlace_type,
        &compression_type, &filter_type)) {
    return 0;
  }

  // This is going to be too slow.
  if (width && height > 100000000 / width)
    return 0;

  int passes = png_set_interlace_handling(png_ptr);
  png_start_read_image(png_ptr);

  for (int pass = 0; pass < passes; ++pass) {
    for (png_uint_32 y = 0; y < height; ++y) {
      png_read_row(png_ptr, static_cast<png_bytep>(row), NULL);
    }
  }
#endif  // PNG_FUZZ_PROGRESSIVE

  return 0;
}
