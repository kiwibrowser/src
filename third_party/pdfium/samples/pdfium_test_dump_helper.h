// Copyright 2018 The PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SAMPLES_PDFIUM_TEST_DUMP_HELPER_H_
#define SAMPLES_PDFIUM_TEST_DUMP_HELPER_H_

#include "public/fpdfview.h"

void DumpChildStructure(FPDF_STRUCTELEMENT child, int indent);
void DumpPageStructure(FPDF_PAGE page, const int page_idx);
void DumpMetaData(FPDF_DOCUMENT doc);

#endif  // SAMPLES_PDFIUM_TEST_DUMP_HELPER_H_
