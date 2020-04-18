// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "pdf/pdf.h"

#include <stdint.h>

#if defined(OS_WIN)
#include <windows.h>
#endif

#include "base/logging.h"
#include "pdf/out_of_process_instance.h"
#include "pdf/pdf_ppapi.h"

namespace chrome_pdf {

#if defined(OS_WIN)
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
                       bool use_color) {
  if (!IsSDKInitializedViaPepper()) {
    if (!InitializeSDK()) {
      return false;
    }
  }
  PDFEngineExports* engine_exports = PDFEngineExports::Get();
  PDFEngineExports::RenderingSettings settings(
      dpi_x, dpi_y,
      pp::Rect(bounds_origin_x, bounds_origin_y, bounds_width, bounds_height),
      fit_to_bounds, stretch_to_bounds, keep_aspect_ratio, center_in_bounds,
      autorotate, use_color);
  bool ret = engine_exports->RenderPDFPageToDC(pdf_buffer, buffer_size,
                                               page_number, settings, dc);
  if (!IsSDKInitializedViaPepper())
    ShutdownSDK();

  return ret;
}

void SetPDFEnsureTypefaceCharactersAccessible(
    PDFEnsureTypefaceCharactersAccessible func) {
  PDFEngineExports::Get()->SetPDFEnsureTypefaceCharactersAccessible(func);
}

void SetPDFUseGDIPrinting(bool enable) {
  PDFEngineExports::Get()->SetPDFUseGDIPrinting(enable);
}

void SetPDFUsePrintMode(int mode) {
  PDFEngineExports::Get()->SetPDFUsePrintMode(mode);
}
#endif  // defined(OS_WIN)

bool GetPDFDocInfo(const void* pdf_buffer,
                   int buffer_size,
                   int* page_count,
                   double* max_page_width) {
  if (!IsSDKInitializedViaPepper()) {
    if (!InitializeSDK())
      return false;
  }
  PDFEngineExports* engine_exports = PDFEngineExports::Get();
  bool ret = engine_exports->GetPDFDocInfo(pdf_buffer, buffer_size, page_count,
                                           max_page_width);
  if (!IsSDKInitializedViaPepper())
    ShutdownSDK();

  return ret;
}

bool GetPDFPageSizeByIndex(const void* pdf_buffer,
                           int pdf_buffer_size,
                           int page_number,
                           double* width,
                           double* height) {
  if (!IsSDKInitializedViaPepper()) {
    if (!chrome_pdf::InitializeSDK())
      return false;
  }
  chrome_pdf::PDFEngineExports* engine_exports =
      chrome_pdf::PDFEngineExports::Get();
  bool ret = engine_exports->GetPDFPageSizeByIndex(pdf_buffer, pdf_buffer_size,
                                                   page_number, width, height);
  if (!IsSDKInitializedViaPepper())
    chrome_pdf::ShutdownSDK();
  return ret;
}

bool RenderPDFPageToBitmap(const void* pdf_buffer,
                           int pdf_buffer_size,
                           int page_number,
                           void* bitmap_buffer,
                           int bitmap_width,
                           int bitmap_height,
                           int dpi_x,
                           int dpi_y,
                           bool autorotate,
                           bool use_color) {
  if (!IsSDKInitializedViaPepper()) {
    if (!InitializeSDK())
      return false;
  }
  PDFEngineExports* engine_exports = PDFEngineExports::Get();
  PDFEngineExports::RenderingSettings settings(
      dpi_x, dpi_y, pp::Rect(bitmap_width, bitmap_height), true, false, true,
      true, autorotate, use_color);
  bool ret = engine_exports->RenderPDFPageToBitmap(
      pdf_buffer, pdf_buffer_size, page_number, settings, bitmap_buffer);
  if (!IsSDKInitializedViaPepper())
    ShutdownSDK();

  return ret;
}

}  // namespace chrome_pdf
