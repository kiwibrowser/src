// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "printing/pdf_metafile_cg_mac.h"

#import <ApplicationServices/ApplicationServices.h>
#include <stdint.h>

#include <string>
#include <vector>

#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/geometry/rect.h"

namespace printing {

TEST(PdfMetafileCgTest, Pdf) {
  // Test in-renderer constructor.
  PdfMetafileCg pdf;
  EXPECT_TRUE(pdf.Init());
  EXPECT_TRUE(pdf.context() != NULL);

  // Render page 1.
  gfx::Rect rect_1(10, 10, 520, 700);
  gfx::Size size_1(540, 720);
  pdf.StartPage(size_1, rect_1, 1.25);
  pdf.FinishPage();

  // Render page 2.
  gfx::Rect rect_2(10, 10, 520, 700);
  gfx::Size size_2(720, 540);
  pdf.StartPage(size_2, rect_2, 2.0);
  pdf.FinishPage();

  pdf.FinishDocument();

  // Check data size.
  uint32_t size = pdf.GetDataSize();
  EXPECT_GT(size, 0U);

  // Get resulting data.
  std::vector<char> buffer(size, 0);
  pdf.GetData(&buffer.front(), size);

  // Test browser-side constructor.
  PdfMetafileCg pdf2;
  EXPECT_TRUE(pdf2.InitFromData(&buffer.front(), size));

  // Get the first 4 characters from pdf2.
  std::vector<char> buffer2(4, 0);
  pdf2.GetData(&buffer2.front(), 4);

  // Test that the header begins with "%PDF".
  std::string header(&buffer2.front(), 4);
  EXPECT_EQ(0U, header.find("%PDF", 0));

  // Test that the PDF is correctly reconstructed.
  EXPECT_EQ(2U, pdf2.GetPageCount());
  gfx::Size page_size = pdf2.GetPageBounds(1).size();
  EXPECT_EQ(540, page_size.width());
  EXPECT_EQ(720, page_size.height());
  page_size = pdf2.GetPageBounds(2).size();
  EXPECT_EQ(720, page_size.width());
  EXPECT_EQ(540, page_size.height());
}

}  // namespace printing
