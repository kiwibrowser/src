// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PDF_PDF_H_
#define PDF_PDF_H_

#include "build/build_config.h"

#if defined(OS_WIN)
#include <windows.h>
#endif

#if defined(OS_WIN)
typedef void (*PDFEnsureTypefaceCharactersAccessible)(const LOGFONT* font,
                                                      const wchar_t* text,
                                                      size_t text_length);
#endif

namespace chrome_pdf {

#if defined(OS_WIN)
// Printing modes - type to convert PDF to for printing
enum PrintingMode {
  kEmf = 0,
  kTextOnly = 1,
  kPostScript2 = 2,
  kPostScript3 = 3,
};

// |pdf_buffer| is the buffer that contains the entire PDF document to be
//     rendered.
// |buffer_size| is the size of |pdf_buffer| in bytes.
// |page_number| is the 0-based index of the page to be rendered.
// |dc| is the device context to render into.
// |dpi_x| and |dpi_y| is the resolution.
// |bounds_origin_x|, |bounds_origin_y|, |bounds_width| and |bounds_height|
//     specify a bounds rectangle within the DC in which to render the PDF
//     page.
// |fit_to_bounds| specifies whether the output should be shrunk to fit the
//     supplied bounds if the page size is larger than the bounds in any
//     dimension. If this is false, parts of the PDF page that lie outside
//     the bounds will be clipped.
// |stretch_to_bounds| specifies whether the output should be stretched to fit
//     the supplied bounds if the page size is smaller than the bounds in any
//     dimension.
// If both |fit_to_bounds| and |stretch_to_bounds| are true, then
//     |fit_to_bounds| is honored first.
// |keep_aspect_ratio| If any scaling is to be done is true, this flag
//     specifies whether the original aspect ratio of the page should be
//     preserved while scaling.
// |center_in_bounds| specifies whether the final image (after any scaling is
//     done) should be centered within the given bounds.
// |autorotate| specifies whether the final image should be rotated to match
//     the output bound.
// |use_color| specifies color or grayscale.
// Returns false if the document or the page number are not valid.
bool RenderPDFPageToDC(const void* pdf_buffer,
                       int buffer_size,
                       int page_number,
                       HDC dc,
                       int dpi_x,
                       int dpi_y,
                       int bounds_origin_x,
                       int bounds_origin_y,
                       int bounds_width,
                       int bounds_height,
                       bool fit_to_bounds,
                       bool stretch_to_bounds,
                       bool keep_aspect_ratio,
                       bool center_in_bounds,
                       bool autorotate,
                       bool use_color);

void SetPDFEnsureTypefaceCharactersAccessible(
    PDFEnsureTypefaceCharactersAccessible func);

void SetPDFUseGDIPrinting(bool enable);

void SetPDFUsePrintMode(int mode);
#endif  // defined(OS_WIN)

// |page_count| and |max_page_width| are optional and can be NULL.
// Returns false if the document is not valid.
bool GetPDFDocInfo(const void* pdf_buffer,
                   int buffer_size,
                   int* page_count,
                   double* max_page_width);

// Gets the dimensions of a specific page in a document.
// |pdf_buffer| is the buffer that contains the entire PDF document to be
//     rendered.
// |pdf_buffer_size| is the size of |pdf_buffer| in bytes.
// |page_number| is the page number that the function will get the dimensions
//     of.
// |width| is the output for the width of the page in points.
// |height| is the output for the height of the page in points.
// Returns false if the document or the page number are not valid.
bool GetPDFPageSizeByIndex(const void* pdf_buffer,
                           int pdf_buffer_size,
                           int page_number,
                           double* width,
                           double* height);

// Renders PDF page into 4-byte per pixel BGRA color bitmap.
// |pdf_buffer| is the buffer that contains the entire PDF document to be
//     rendered.
// |pdf_buffer_size| is the size of |pdf_buffer| in bytes.
// |page_number| is the 0-based index of the page to be rendered.
// |bitmap_buffer| is the output buffer for bitmap.
// |bitmap_width| is the width of the output bitmap.
// |bitmap_height| is the height of the output bitmap.
// |dpi_x| and |dpi_y| is the resolution.
// |autorotate| specifies whether the final image should be rotated to match
//     the output bound.
// |use_color| specifies color or grayscale.
// Returns false if the document or the page number are not valid.
bool RenderPDFPageToBitmap(const void* pdf_buffer,
                           int pdf_buffer_size,
                           int page_number,
                           void* bitmap_buffer,
                           int bitmap_width,
                           int bitmap_height,
                           int dpi_x,
                           int dpi_y,
                           bool autorotate,
                           bool use_color);

}  // namespace chrome_pdf

#endif  // PDF_PDF_H_
