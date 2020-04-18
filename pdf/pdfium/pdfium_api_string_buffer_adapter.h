// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PDF_PDFIUM_PDFIUM_API_STRING_BUFFER_ADAPTER_H_
#define PDF_PDFIUM_PDFIUM_API_STRING_BUFFER_ADAPTER_H_

#include <stddef.h>

#include "base/macros.h"
#include "base/numerics/safe_math.h"

namespace chrome_pdf {

// Helper to deal with the fact that many PDFium APIs write the null-terminator
// into string buffers that are passed to them, but the PDF plugin likes to pass
// in std::strings / base::string16s, where one should not count on the internal
// string buffers to be null-terminated.

template <class StringType>
class PDFiumAPIStringBufferAdapter {
 public:
  // |str| is the string to write into.
  // |expected_size| is the number of characters the PDFium API will write,
  // including the null-terminator. It should be at least 1.
  // |check_expected_size| whether to check the actual number of characters
  // written into |str| against |expected_size| when calling Close().
  PDFiumAPIStringBufferAdapter(StringType* str,
                               size_t expected_size,
                               bool check_expected_size);
  ~PDFiumAPIStringBufferAdapter();

  // Returns a pointer to |str_|'s buffer. The buffer's size is large enough to
  // hold |expected_size_| + 1 characters, so the PDFium API that uses the
  // pointer has space to write a null-terminator.
  void* GetData();

  // Resizes |str_| to |actual_size| - 1 characters, thereby removing the extra
  // null-terminator. This must be called prior to the adapter's destruction.
  // The pointer returned by GetData() should be considered invalid.
  void Close(size_t actual_size);

  template <typename IntType>
  void Close(IntType actual_size) {
    Close(base::checked_cast<size_t>(actual_size));
  }

 private:
  StringType* const str_;
  void* const data_;
  const size_t expected_size_;
  const bool check_expected_size_;
  bool is_closed_;

  DISALLOW_COPY_AND_ASSIGN(PDFiumAPIStringBufferAdapter);
};

// Helper to deal with the fact that many PDFium APIs write the null-terminator
// into string buffers that are passed to them, but the PDF plugin likes to pass
// in std::strings / base::string16s, where one should not count on the internal
// string buffers to be null-terminated. This version is suitable for APIs that
// work in terms of number of bytes instead of the number of characters.
template <class StringType>
class PDFiumAPIStringBufferSizeInBytesAdapter {
 public:
  // |str| is the string to write into.
  // |expected_size| is the number of bytes the PDFium API will write,
  // including the null-terminator. It should be at least the size of a
  // character in bytes.
  // |check_expected_size| whether to check the actual number of bytes
  // written into |str| against |expected_size| when calling Close().
  PDFiumAPIStringBufferSizeInBytesAdapter(StringType* str,
                                          size_t expected_size,
                                          bool check_expected_size);
  ~PDFiumAPIStringBufferSizeInBytesAdapter();

  // Returns a pointer to |str_|'s buffer. The buffer's size is large enough to
  // hold |expected_size_| + sizeof(StringType::value_type) bytes, so the PDFium
  // API that uses the pointer has space to write a null-terminator.
  void* GetData();

  // Resizes |str_| to |actual_size| - sizeof(StringType::value_type) bytes,
  // thereby removing the extra null-terminator. This must be called prior to
  // the adapter's destruction. The pointer returned by GetData() should be
  // considered invalid.
  void Close(size_t actual_size);

  template <typename IntType>
  void Close(IntType actual_size) {
    Close(base::checked_cast<size_t>(actual_size));
  }

 private:
  PDFiumAPIStringBufferAdapter<StringType> adapter_;
};

}  // namespace chrome_pdf

#endif  // PDF_PDFIUM_PDFIUM_API_STRING_BUFFER_ADAPTER_H_
