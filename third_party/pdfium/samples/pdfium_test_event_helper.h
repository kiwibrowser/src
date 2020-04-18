// Copyright 2018 The PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SAMPLES_PDFIUM_TEST_EVENT_HELPER_H_
#define SAMPLES_PDFIUM_TEST_EVENT_HELPER_H_

#include <string>

#include "public/fpdf_formfill.h"
#include "public/fpdfview.h"

void SendPageEvents(FPDF_FORMHANDLE form,
                    FPDF_PAGE page,
                    const std::string& events);

#endif  // SAMPLES_PDFIUM_TEST_EVENT_HELPER_H_
