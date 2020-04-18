// Copyright 2016 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PNGPREFIX_H
#define PNGPREFIX_H

// The purpose of this file is to rename conflicting functions
// when this version of libpng and chromium's version of it are
// both simultaneously present.

#define png_get_uint_32 PDFIUM_png_get_uint_32
#define png_get_uint_16 PDFIUM_png_get_uint_16
#define png_get_int_32 PDFIUM_png_get_int_32
#define png_get_int_16 PDFIUM_png_get_int_16

#endif  // PNGPREFIX_H
