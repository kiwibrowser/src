// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PRINTING_PAGE_SIZE_MARGINS_H_
#define PRINTING_PAGE_SIZE_MARGINS_H_

#include "printing/printing_export.h"

namespace printing {

// Struct that holds margin and content area sizes of a page. Units are
// arbitrary and can be chosen by the programmer.
struct PageSizeMargins {
  double content_width;
  double content_height;
  double margin_top;
  double margin_right;
  double margin_bottom;
  double margin_left;
};

}  // namespace printing

#endif  // PRINTING_PAGE_SIZE_MARGINS_H_
