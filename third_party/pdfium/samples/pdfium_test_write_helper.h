// Copyright 2018 The PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SAMPLES_PDFIUM_TEST_WRITE_HELPER_H_
#define SAMPLES_PDFIUM_TEST_WRITE_HELPER_H_

#include <string>

#include "public/fpdfview.h"

#ifdef PDF_ENABLE_SKIA
#include "third_party/skia/include/core/SkPictureRecorder.h"
#include "third_party/skia/include/core/SkStream.h"
#endif

std::string WritePpm(const char* pdf_name,
                     int num,
                     const void* buffer_void,
                     int stride,
                     int width,
                     int height);
void WriteText(FPDF_PAGE page, const char* pdf_name, int num);
void WriteAnnot(FPDF_PAGE page, const char* pdf_name, int num);
std::string WritePng(const char* pdf_name,
                     int num,
                     const void* buffer_void,
                     int stride,
                     int width,
                     int height);

#ifdef _WIN32
std::string WriteBmp(const char* pdf_name,
                     int num,
                     const void* buffer,
                     int stride,
                     int width,
                     int height);
void WriteEmf(FPDF_PAGE page, const char* pdf_name, int num);
void WritePS(FPDF_PAGE page, const char* pdf_name, int num);
#endif  // _WIN32

#ifdef PDF_ENABLE_SKIA
std::string WriteSkp(const char* pdf_name,
                     int num,
                     SkPictureRecorder* recorder);
#endif  // PDF_ENABLE_SKIA

void WriteAttachments(FPDF_DOCUMENT doc, const std::string& name);
void WriteImages(FPDF_PAGE page, const char* pdf_name, int page_num);

#endif  // SAMPLES_PDFIUM_TEST_WRITE_HELPER_H_
