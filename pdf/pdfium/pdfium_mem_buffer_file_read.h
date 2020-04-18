// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PDF_PDFIUM_PDFIUM_MEM_BUFFER_FILE_READ_H_
#define PDF_PDFIUM_PDFIUM_MEM_BUFFER_FILE_READ_H_

#include <stddef.h>
#include <stdlib.h>

#include "third_party/pdfium/public/fpdfview.h"

namespace chrome_pdf {

// Implementation of FPDF_FILEACCESS from a memory buffer.
class PDFiumMemBufferFileRead : public FPDF_FILEACCESS {
 public:
  PDFiumMemBufferFileRead(const void* data, size_t size);
  ~PDFiumMemBufferFileRead();

 private:
  static int GetBlock(void* param,
                      unsigned long position,
                      unsigned char* buf,
                      unsigned long size);
  const unsigned char* data_;
};

}  // namespace chrome_pdf

#endif  // PDF_PDFIUM_PDFIUM_MEM_BUFFER_FILE_READ_H_
