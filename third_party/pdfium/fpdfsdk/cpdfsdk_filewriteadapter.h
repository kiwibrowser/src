// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FPDFSDK_CPDFSDK_FILEWRITEADAPTER_H_
#define FPDFSDK_CPDFSDK_FILEWRITEADAPTER_H_

#include "core/fxcrt/fx_stream.h"
#include "core/fxcrt/retain_ptr.h"
#include "public/fpdf_save.h"

class CPDFSDK_FileWriteAdapter : public IFX_WriteStream {
 public:
  template <typename T, typename... Args>
  friend RetainPtr<T> pdfium::MakeRetain(Args&&... args);

  // IFX_WriteStream:
  bool WriteBlock(const void* data, size_t size) override;
  bool WriteString(const ByteStringView& str) override;

 private:
  explicit CPDFSDK_FileWriteAdapter(FPDF_FILEWRITE* fileWriteStruct);
  ~CPDFSDK_FileWriteAdapter() override;

  FPDF_FILEWRITE* fileWriteStruct_;
};

#endif  // FPDFSDK_CPDFSDK_FILEWRITEADAPTER_H_
