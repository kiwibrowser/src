// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PDF_PDFIUM_PDFIUM_PRINT_H_
#define PDF_PDFIUM_PDFIUM_PRINT_H_

#include <vector>

#include "base/macros.h"
#include "ppapi/cpp/dev/buffer_dev.h"
#include "third_party/pdfium/public/fpdfview.h"

struct PP_PdfPrintSettings_Dev;
struct PP_PrintSettings_Dev;
struct PP_PrintPageNumberRange_Dev;

namespace chrome_pdf {

class PDFiumEngine;
class PDFiumPage;

class PDFiumPrint {
 public:
  explicit PDFiumPrint(PDFiumEngine* engine);
  ~PDFiumPrint();

  static std::vector<uint32_t> GetPageNumbersFromPrintPageNumberRange(
      const PP_PrintPageNumberRange_Dev* page_ranges,
      uint32_t page_range_count);

  pp::Buffer_Dev PrintPagesAsRasterPDF(
      const PP_PrintPageNumberRange_Dev* page_ranges,
      uint32_t page_range_count,
      const PP_PrintSettings_Dev& print_settings,
      const PP_PdfPrintSettings_Dev& pdf_print_settings);
  pp::Buffer_Dev PrintPagesAsPDF(
      const PP_PrintPageNumberRange_Dev* page_ranges,
      uint32_t page_range_count,
      const PP_PrintSettings_Dev& print_settings,
      const PP_PdfPrintSettings_Dev& pdf_print_settings);

 private:
  FPDF_DOCUMENT CreateSinglePageRasterPdf(
      double source_page_width,
      double source_page_height,
      const PP_PrintSettings_Dev& print_settings,
      PDFiumPage* page_to_print);

  // Perform N-up PDF generation from |doc| based on |num_pages_per_sheet| and
  // the parameters in |print_settings|.
  // On success, the returned buffer contains the N-up version of |doc|.
  // On failure, the returned buffer is empty.
  pp::Buffer_Dev NupPdfToPdf(FPDF_DOCUMENT doc,
                             uint32_t num_pages_per_sheet,
                             const PP_PrintSettings_Dev& print_settings);

  bool FlattenPrintData(FPDF_DOCUMENT doc);
  pp::Buffer_Dev GetPrintData(FPDF_DOCUMENT doc);
  pp::Buffer_Dev GetFlattenedPrintData(FPDF_DOCUMENT doc);

  PDFiumEngine* const engine_;

  DISALLOW_COPY_AND_ASSIGN(PDFiumPrint);
};

}  // namespace chrome_pdf

#endif  // PDF_PDFIUM_PDFIUM_PRINT_H_
