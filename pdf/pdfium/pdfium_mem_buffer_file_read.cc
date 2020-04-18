// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "pdf/pdfium/pdfium_mem_buffer_file_read.h"

#include <stddef.h>
#include <string.h>

namespace chrome_pdf {

PDFiumMemBufferFileRead::PDFiumMemBufferFileRead(const void* data,
                                                 size_t size) {
  m_FileLen = size;
  m_Param = this;
  m_GetBlock = &GetBlock;
  data_ = reinterpret_cast<const unsigned char*>(data);
}

PDFiumMemBufferFileRead::~PDFiumMemBufferFileRead() = default;

int PDFiumMemBufferFileRead::GetBlock(void* param,
                                      unsigned long position,
                                      unsigned char* buf,
                                      unsigned long size) {
  const PDFiumMemBufferFileRead* data =
      reinterpret_cast<const PDFiumMemBufferFileRead*>(param);
  if (!data || position + size > data->m_FileLen)
    return 0;
  memcpy(buf, data->data_ + position, size);
  return 1;
}

}  // namespace chrome_pdf
