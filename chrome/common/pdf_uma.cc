// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/pdf_uma.h"

#include "base/metrics/histogram_macros.h"

void ReportPDFLoadStatus(PDFLoadStatus status) {
  UMA_HISTOGRAM_ENUMERATION("PDF.LoadStatus", status,
                            PDFLoadStatus::kPdfLoadStatusCount);
}
