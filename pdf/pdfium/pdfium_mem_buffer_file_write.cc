// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "pdf/pdfium/pdfium_mem_buffer_file_write.h"

namespace chrome_pdf {

PDFiumMemBufferFileWrite::PDFiumMemBufferFileWrite() {
  version = 1;
  WriteBlock = &WriteBlockImpl;
}

PDFiumMemBufferFileWrite::~PDFiumMemBufferFileWrite() = default;

int PDFiumMemBufferFileWrite::WriteBlockImpl(FPDF_FILEWRITE* this_file_write,
                                             const void* data,
                                             unsigned long size) {
  PDFiumMemBufferFileWrite* mem_buffer_file_write =
      static_cast<PDFiumMemBufferFileWrite*>(this_file_write);
  return mem_buffer_file_write->DoWriteBlock(data, size);
}

int PDFiumMemBufferFileWrite::DoWriteBlock(const void* data,
                                           unsigned long size) {
  buffer_.append(static_cast<const unsigned char*>(data), size);
  return 1;
}

}  // namespace chrome_pdf
