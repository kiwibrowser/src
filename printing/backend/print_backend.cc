// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "printing/backend/print_backend.h"

namespace {

// PrintBackend override for testing.
printing::PrintBackend* g_print_backend_for_test = nullptr;

}  // namespace

namespace printing {

PrinterBasicInfo::PrinterBasicInfo()
    : printer_status(0),
      is_default(false) {}

PrinterBasicInfo::PrinterBasicInfo(const PrinterBasicInfo& other) = default;

PrinterBasicInfo::~PrinterBasicInfo() = default;

PrinterSemanticCapsAndDefaults::PrinterSemanticCapsAndDefaults()
    : collate_capable(false),
      collate_default(false),
      copies_capable(false),
      duplex_capable(false),
      duplex_default(UNKNOWN_DUPLEX_MODE),
      color_changeable(false),
      color_default(false),
      color_model(UNKNOWN_COLOR_MODEL),
      bw_model(UNKNOWN_COLOR_MODEL)
{}

PrinterSemanticCapsAndDefaults::PrinterSemanticCapsAndDefaults(
    const PrinterSemanticCapsAndDefaults& other) = default;

PrinterSemanticCapsAndDefaults::~PrinterSemanticCapsAndDefaults() = default;

PrinterCapsAndDefaults::PrinterCapsAndDefaults() = default;

PrinterCapsAndDefaults::PrinterCapsAndDefaults(
    const PrinterCapsAndDefaults& other) = default;

PrinterCapsAndDefaults::~PrinterCapsAndDefaults() = default;

PrintBackend::~PrintBackend() = default;

// static
scoped_refptr<PrintBackend> PrintBackend::CreateInstance(
    const base::DictionaryValue* print_backend_settings) {
  return g_print_backend_for_test
             ? g_print_backend_for_test
             : PrintBackend::CreateInstanceImpl(print_backend_settings);
}

// static
void PrintBackend::SetPrintBackendForTesting(PrintBackend* backend) {
  g_print_backend_for_test = backend;
}

}  // namespace printing
