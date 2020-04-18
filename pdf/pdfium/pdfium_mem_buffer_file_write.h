// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PDF_PDFIUM_PDFIUM_MEM_BUFFER_FILE_WRITE_H_
#define PDF_PDFIUM_PDFIUM_MEM_BUFFER_FILE_WRITE_H_

#include <stddef.h>

#include <string>

#include "third_party/pdfium/public/fpdf_save.h"

namespace chrome_pdf {

// Implementation of FPDF_FILEWRITE into a memory buffer.
class PDFiumMemBufferFileWrite : public FPDF_FILEWRITE {
 public:
  PDFiumMemBufferFileWrite();
  ~PDFiumMemBufferFileWrite();

  const std::basic_string<unsigned char>& buffer() { return buffer_; }
  size_t size() { return buffer_.size(); }

 private:
  int DoWriteBlock(const void* data, unsigned long size);
  static int WriteBlockImpl(FPDF_FILEWRITE* this_file_write,
                            const void* data,
                            unsigned long size);

  std::basic_string<unsigned char> buffer_;
};

}  // namespace chrome_pdf

#endif  // PDF_PDFIUM_PDFIUM_MEM_BUFFER_FILE_WRITE_H_
