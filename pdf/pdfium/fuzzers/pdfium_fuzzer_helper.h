// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PDF_PDFIUM_FUZZERS_PDFIUM_FUZZER_HELPER_H_
#define PDF_PDFIUM_FUZZERS_PDFIUM_FUZZER_HELPER_H_

// This fuzzer is simplified & cleaned up pdfium/samples/pdfium_test.cc

#include "third_party/pdfium/public/fpdf_ext.h"
#include "third_party/pdfium/public/fpdf_formfill.h"
#include "third_party/pdfium/public/fpdfview.h"
#include "v8/include/v8.h"

class PDFiumFuzzerHelper {
 public:
  virtual ~PDFiumFuzzerHelper();

  void RenderPdf(const char* pBuf, size_t len);

  virtual int GetFormCallbackVersion() const = 0;
  virtual bool OnFormFillEnvLoaded(FPDF_DOCUMENT doc);

 protected:
  PDFiumFuzzerHelper();

 private:
  bool RenderPage(FPDF_DOCUMENT doc,
                  FPDF_FORMHANDLE form,
                  const int page_index);
};

#endif  // PDF_PDFIUM_FUZZERS_PDFIUM_FUZZER_HELPER_H_
