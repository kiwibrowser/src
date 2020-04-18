// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "pdf/pdf_engine.h"

namespace chrome_pdf {

PDFEngine::PageFeatures::PageFeatures(){};

PDFEngine::PageFeatures::PageFeatures(const PageFeatures& other)
    : index(other.index), annotation_types(other.annotation_types) {}

PDFEngine::PageFeatures::~PageFeatures(){};

bool PDFEngine::PageFeatures::IsInitialized() const {
  return index >= 0;
}

}  // namespace chrome_pdf
