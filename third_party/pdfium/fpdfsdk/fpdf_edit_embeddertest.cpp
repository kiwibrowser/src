// Copyright 2016 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "core/fpdfapi/font/cpdf_font.h"
#include "core/fpdfapi/page/cpdf_page.h"
#include "core/fpdfapi/parser/cpdf_array.h"
#include "core/fpdfapi/parser/cpdf_dictionary.h"
#include "core/fpdfapi/parser/cpdf_number.h"
#include "core/fpdfapi/parser/cpdf_stream.h"
#include "core/fxcrt/fx_system.h"
#include "fpdfsdk/cpdfsdk_helpers.h"
#include "public/cpp/fpdf_scopers.h"
#include "public/fpdf_annot.h"
#include "public/fpdf_edit.h"
#include "public/fpdfview.h"
#include "testing/embedder_test.h"
#include "testing/gmock/include/gmock/gmock-matchers.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/test_support.h"

class FPDFEditEmbeddertest : public EmbedderTest {
 protected:
  FPDF_DOCUMENT CreateNewDocument() {
    document_ = FPDF_CreateNewDocument();
    cpdf_doc_ = CPDFDocumentFromFPDFDocument(document_);
    return document_;
  }

  void CheckFontDescriptor(CPDF_Dictionary* font_dict,
                           int font_type,
                           bool bold,
                           bool italic,
                           uint32_t size,
                           const uint8_t* data) {
    CPDF_Dictionary* font_desc = font_dict->GetDictFor("FontDescriptor");
    ASSERT_TRUE(font_desc);
    EXPECT_EQ("FontDescriptor", font_desc->GetStringFor("Type"));
    EXPECT_EQ(font_dict->GetStringFor("BaseFont"),
              font_desc->GetStringFor("FontName"));

    // Check that the font descriptor has the required keys according to spec
    // 1.7 Table 5.19
    ASSERT_TRUE(font_desc->KeyExist("Flags"));

    int font_flags = font_desc->GetIntegerFor("Flags");
    EXPECT_EQ(bold, FontStyleIsBold(font_flags));
    EXPECT_EQ(italic, FontStyleIsItalic(font_flags));
    EXPECT_TRUE(FontStyleIsNonSymbolic(font_flags));
    ASSERT_TRUE(font_desc->KeyExist("FontBBox"));

    CPDF_Array* fontBBox = font_desc->GetArrayFor("FontBBox");
    ASSERT_TRUE(fontBBox);
    EXPECT_EQ(4U, fontBBox->GetCount());
    // Check that the coordinates are in the preferred order according to spec
    // 1.7 Section 3.8.4
    EXPECT_TRUE(fontBBox->GetIntegerAt(0) < fontBBox->GetIntegerAt(2));
    EXPECT_TRUE(fontBBox->GetIntegerAt(1) < fontBBox->GetIntegerAt(3));

    EXPECT_TRUE(font_desc->KeyExist("ItalicAngle"));
    EXPECT_TRUE(font_desc->KeyExist("Ascent"));
    EXPECT_TRUE(font_desc->KeyExist("Descent"));
    EXPECT_TRUE(font_desc->KeyExist("CapHeight"));
    EXPECT_TRUE(font_desc->KeyExist("StemV"));
    ByteString present("FontFile");
    ByteString absent("FontFile2");
    if (font_type == FPDF_FONT_TRUETYPE)
      std::swap(present, absent);
    EXPECT_TRUE(font_desc->KeyExist(present));
    EXPECT_FALSE(font_desc->KeyExist(absent));

    // Check that the font stream is the one that was provided
    CPDF_Stream* font_stream = font_desc->GetStreamFor(present);
    ASSERT_EQ(size, font_stream->GetRawSize());
    if (font_type == FPDF_FONT_TRUETYPE) {
      ASSERT_EQ(static_cast<int>(size),
                font_stream->GetDict()->GetIntegerFor("Length1"));
    }
    uint8_t* stream_data = font_stream->GetRawData();
    for (size_t j = 0; j < size; j++)
      EXPECT_EQ(data[j], stream_data[j]) << " at byte " << j;
  }

  void CheckCompositeFontWidths(CPDF_Array* widths_array,
                                CPDF_Font* typed_font) {
    // Check that W array is in a format that conforms to PDF spec 1.7 section
    // "Glyph Metrics in CIDFonts" (these checks are not
    // implementation-specific).
    EXPECT_GT(widths_array->GetCount(), 1U);
    int num_cids_checked = 0;
    int cur_cid = 0;
    for (size_t idx = 0; idx < widths_array->GetCount(); idx++) {
      int cid = widths_array->GetNumberAt(idx);
      EXPECT_GE(cid, cur_cid);
      ASSERT_FALSE(++idx == widths_array->GetCount());
      CPDF_Object* next = widths_array->GetObjectAt(idx);
      if (next->IsArray()) {
        // We are in the c [w1 w2 ...] case
        CPDF_Array* arr = next->AsArray();
        int cnt = static_cast<int>(arr->GetCount());
        size_t inner_idx = 0;
        for (cur_cid = cid; cur_cid < cid + cnt; cur_cid++) {
          uint32_t width = arr->GetNumberAt(inner_idx++);
          EXPECT_EQ(width, typed_font->GetCharWidthF(cur_cid))
              << " at cid " << cur_cid;
        }
        num_cids_checked += cnt;
        continue;
      }
      // Otherwise, are in the c_first c_last w case.
      ASSERT_TRUE(next->IsNumber());
      int last_cid = next->AsNumber()->GetInteger();
      ASSERT_FALSE(++idx == widths_array->GetCount());
      uint32_t width = widths_array->GetNumberAt(idx);
      for (cur_cid = cid; cur_cid <= last_cid; cur_cid++) {
        EXPECT_EQ(width, typed_font->GetCharWidthF(cur_cid))
            << " at cid " << cur_cid;
      }
      num_cids_checked += last_cid - cid + 1;
    }
    // Make sure we have a good amount of cids described
    EXPECT_GT(num_cids_checked, 900);
  }
  CPDF_Document* cpdf_doc() { return cpdf_doc_; }

 private:
  CPDF_Document* cpdf_doc_;
};

namespace {

const char kExpectedPDF[] =
    "%PDF-1.7\r\n"
    "%\xA1\xB3\xC5\xD7\r\n"
    "1 0 obj\r\n"
    "<</Pages 2 0 R /Type/Catalog>>\r\n"
    "endobj\r\n"
    "2 0 obj\r\n"
    "<</Count 1/Kids\\[ 4 0 R \\]/Type/Pages>>\r\n"
    "endobj\r\n"
    "3 0 obj\r\n"
    "<</CreationDate\\(D:.*\\)/Creator\\(PDFium\\)>>\r\n"
    "endobj\r\n"
    "4 0 obj\r\n"
    "<</MediaBox\\[ 0 0 640 480\\]/Parent 2 0 R "
    "/Resources<</ExtGState<</FXE1 5 0 R >>>>"
    "/Rotate 0/Type/Page"
    ">>\r\n"
    "endobj\r\n"
    "5 0 obj\r\n"
    "<</BM/Normal/CA 1/ca 1>>\r\n"
    "endobj\r\n"
    "xref\r\n"
    "0 6\r\n"
    "0000000000 65535 f\r\n"
    "0000000017 00000 n\r\n"
    "0000000066 00000 n\r\n"
    "0000000122 00000 n\r\n"
    "0000000192 00000 n\r\n"
    "0000000311 00000 n\r\n"
    "trailer\r\n"
    "<<\r\n"
    "/Root 1 0 R\r\n"
    "/Info 3 0 R\r\n"
    "/Size 6/ID\\[<.*><.*>\\]>>\r\n"
    "startxref\r\n"
    "354\r\n"
    "%%EOF\r\n";

}  // namespace

TEST_F(FPDFEditEmbeddertest, EmptyCreation) {
  EXPECT_TRUE(CreateEmptyDocument());
  FPDF_PAGE page = FPDFPage_New(document(), 0, 640.0, 480.0);
  EXPECT_NE(nullptr, page);
  // The FPDFPage_GenerateContent call should do nothing.
  EXPECT_TRUE(FPDFPage_GenerateContent(page));
  EXPECT_TRUE(FPDF_SaveAsCopy(document(), this, 0));

  EXPECT_THAT(GetString(), testing::MatchesRegex(std::string(
                               kExpectedPDF, sizeof(kExpectedPDF))));
  FPDF_ClosePage(page);
}

// Regression test for https://crbug.com/667012
TEST_F(FPDFEditEmbeddertest, RasterizePDF) {
  const char kAllBlackMd5sum[] = "5708fc5c4a8bd0abde99c8e8f0390615";

  // Get the bitmap for the original document/
  ScopedFPDFBitmap orig_bitmap;
  {
    EXPECT_TRUE(OpenDocument("black.pdf"));
    FPDF_PAGE orig_page = LoadPage(0);
    ASSERT_TRUE(orig_page);
    orig_bitmap = RenderLoadedPage(orig_page);
    CompareBitmap(orig_bitmap.get(), 612, 792, kAllBlackMd5sum);
    UnloadPage(orig_page);
  }

  // Create a new document from |orig_bitmap| and save it.
  {
    FPDF_DOCUMENT temp_doc = FPDF_CreateNewDocument();
    FPDF_PAGE temp_page = FPDFPage_New(temp_doc, 0, 612, 792);

    // Add the bitmap to an image object and add the image object to the output
    // page.
    FPDF_PAGEOBJECT temp_img = FPDFPageObj_NewImageObj(temp_doc);
    EXPECT_TRUE(
        FPDFImageObj_SetBitmap(&temp_page, 1, temp_img, orig_bitmap.get()));
    EXPECT_TRUE(FPDFImageObj_SetMatrix(temp_img, 612, 0, 0, 792, 0, 0));
    FPDFPage_InsertObject(temp_page, temp_img);
    EXPECT_TRUE(FPDFPage_GenerateContent(temp_page));
    EXPECT_TRUE(FPDF_SaveAsCopy(temp_doc, this, 0));
    FPDF_ClosePage(temp_page);
    FPDF_CloseDocument(temp_doc);
  }

  // Get the generated content. Make sure it is at least as big as the original
  // PDF.
  EXPECT_GT(GetString().size(), 923U);
  VerifySavedDocument(612, 792, kAllBlackMd5sum);
}

TEST_F(FPDFEditEmbeddertest, AddPaths) {
  // Start with a blank page
  FPDF_PAGE page = FPDFPage_New(CreateNewDocument(), 0, 612, 792);
  ASSERT_TRUE(page);

  // We will first add a red rectangle
  FPDF_PAGEOBJECT red_rect = FPDFPageObj_CreateNewRect(10, 10, 20, 20);
  ASSERT_TRUE(red_rect);
  // Expect false when trying to set colors out of range
  EXPECT_FALSE(FPDFPath_SetStrokeColor(red_rect, 100, 100, 100, 300));
  EXPECT_FALSE(FPDFPath_SetFillColor(red_rect, 200, 256, 200, 0));

  // Fill rectangle with red and insert to the page
  EXPECT_TRUE(FPDFPath_SetFillColor(red_rect, 255, 0, 0, 255));
  EXPECT_TRUE(FPDFPath_SetDrawMode(red_rect, FPDF_FILLMODE_ALTERNATE, 0));
  FPDFPage_InsertObject(page, red_rect);
  {
    ScopedFPDFBitmap page_bitmap = RenderPageWithFlags(page, nullptr, 0);
    CompareBitmap(page_bitmap.get(), 612, 792,
                  "66d02eaa6181e2c069ce2ea99beda497");
  }

  // Now add to that a green rectangle with some medium alpha
  FPDF_PAGEOBJECT green_rect = FPDFPageObj_CreateNewRect(100, 100, 40, 40);
  EXPECT_TRUE(FPDFPath_SetFillColor(green_rect, 0, 255, 0, 128));

  // Make sure the type of the rectangle is a path.
  EXPECT_EQ(FPDF_PAGEOBJ_PATH, FPDFPageObj_GetType(green_rect));

  // Make sure we get back the same color we set previously.
  unsigned int R;
  unsigned int G;
  unsigned int B;
  unsigned int A;
  EXPECT_TRUE(FPDFPath_GetFillColor(green_rect, &R, &G, &B, &A));
  EXPECT_EQ(0U, R);
  EXPECT_EQ(255U, G);
  EXPECT_EQ(0U, B);
  EXPECT_EQ(128U, A);

  // Make sure the path has 5 points (1 FXPT_TYPE::MoveTo and 4
  // FXPT_TYPE::LineTo).
  ASSERT_EQ(5, FPDFPath_CountSegments(green_rect));
  // Verify actual coordinates.
  FPDF_PATHSEGMENT segment = FPDFPath_GetPathSegment(green_rect, 0);
  float x;
  float y;
  EXPECT_TRUE(FPDFPathSegment_GetPoint(segment, &x, &y));
  EXPECT_EQ(100, x);
  EXPECT_EQ(100, y);
  EXPECT_EQ(FPDF_SEGMENT_MOVETO, FPDFPathSegment_GetType(segment));
  EXPECT_FALSE(FPDFPathSegment_GetClose(segment));
  segment = FPDFPath_GetPathSegment(green_rect, 1);
  EXPECT_TRUE(FPDFPathSegment_GetPoint(segment, &x, &y));
  EXPECT_EQ(100, x);
  EXPECT_EQ(140, y);
  EXPECT_EQ(FPDF_SEGMENT_LINETO, FPDFPathSegment_GetType(segment));
  EXPECT_FALSE(FPDFPathSegment_GetClose(segment));
  segment = FPDFPath_GetPathSegment(green_rect, 2);
  EXPECT_TRUE(FPDFPathSegment_GetPoint(segment, &x, &y));
  EXPECT_EQ(140, x);
  EXPECT_EQ(140, y);
  EXPECT_EQ(FPDF_SEGMENT_LINETO, FPDFPathSegment_GetType(segment));
  EXPECT_FALSE(FPDFPathSegment_GetClose(segment));
  segment = FPDFPath_GetPathSegment(green_rect, 3);
  EXPECT_TRUE(FPDFPathSegment_GetPoint(segment, &x, &y));
  EXPECT_EQ(140, x);
  EXPECT_EQ(100, y);
  EXPECT_EQ(FPDF_SEGMENT_LINETO, FPDFPathSegment_GetType(segment));
  EXPECT_FALSE(FPDFPathSegment_GetClose(segment));
  segment = FPDFPath_GetPathSegment(green_rect, 4);
  EXPECT_TRUE(FPDFPathSegment_GetPoint(segment, &x, &y));
  EXPECT_EQ(100, x);
  EXPECT_EQ(100, y);
  EXPECT_EQ(FPDF_SEGMENT_LINETO, FPDFPathSegment_GetType(segment));
  EXPECT_TRUE(FPDFPathSegment_GetClose(segment));

  EXPECT_TRUE(FPDFPath_SetDrawMode(green_rect, FPDF_FILLMODE_WINDING, 0));
  FPDFPage_InsertObject(page, green_rect);
  {
    ScopedFPDFBitmap page_bitmap = RenderPageWithFlags(page, nullptr, 0);
    CompareBitmap(page_bitmap.get(), 612, 792,
                  "7b0b87604594e773add528fae567a558");
  }

  // Add a black triangle.
  FPDF_PAGEOBJECT black_path = FPDFPageObj_CreateNewPath(400, 100);
  EXPECT_TRUE(FPDFPath_SetFillColor(black_path, 0, 0, 0, 200));
  EXPECT_TRUE(FPDFPath_SetDrawMode(black_path, FPDF_FILLMODE_ALTERNATE, 0));
  EXPECT_TRUE(FPDFPath_LineTo(black_path, 400, 200));
  EXPECT_TRUE(FPDFPath_LineTo(black_path, 300, 100));
  EXPECT_TRUE(FPDFPath_Close(black_path));

  // Make sure the path has 3 points (1 FXPT_TYPE::MoveTo and 2
  // FXPT_TYPE::LineTo).
  ASSERT_EQ(3, FPDFPath_CountSegments(black_path));
  // Verify actual coordinates.
  segment = FPDFPath_GetPathSegment(black_path, 0);
  EXPECT_TRUE(FPDFPathSegment_GetPoint(segment, &x, &y));
  EXPECT_EQ(400, x);
  EXPECT_EQ(100, y);
  EXPECT_EQ(FPDF_SEGMENT_MOVETO, FPDFPathSegment_GetType(segment));
  EXPECT_FALSE(FPDFPathSegment_GetClose(segment));
  segment = FPDFPath_GetPathSegment(black_path, 1);
  EXPECT_TRUE(FPDFPathSegment_GetPoint(segment, &x, &y));
  EXPECT_EQ(400, x);
  EXPECT_EQ(200, y);
  EXPECT_EQ(FPDF_SEGMENT_LINETO, FPDFPathSegment_GetType(segment));
  EXPECT_FALSE(FPDFPathSegment_GetClose(segment));
  segment = FPDFPath_GetPathSegment(black_path, 2);
  EXPECT_TRUE(FPDFPathSegment_GetPoint(segment, &x, &y));
  EXPECT_EQ(300, x);
  EXPECT_EQ(100, y);
  EXPECT_EQ(FPDF_SEGMENT_LINETO, FPDFPathSegment_GetType(segment));
  EXPECT_TRUE(FPDFPathSegment_GetClose(segment));
  // Make sure out of bounds index access fails properly.
  EXPECT_EQ(nullptr, FPDFPath_GetPathSegment(black_path, 3));

  FPDFPage_InsertObject(page, black_path);
  {
    ScopedFPDFBitmap page_bitmap = RenderPageWithFlags(page, nullptr, 0);
    CompareBitmap(page_bitmap.get(), 612, 792,
                  "eadc8020a14dfcf091da2688733d8806");
  }

  // Now add a more complex blue path.
  FPDF_PAGEOBJECT blue_path = FPDFPageObj_CreateNewPath(200, 200);
  EXPECT_TRUE(FPDFPath_SetFillColor(blue_path, 0, 0, 255, 255));
  EXPECT_TRUE(FPDFPath_SetDrawMode(blue_path, FPDF_FILLMODE_WINDING, 0));
  EXPECT_TRUE(FPDFPath_LineTo(blue_path, 230, 230));
  EXPECT_TRUE(FPDFPath_BezierTo(blue_path, 250, 250, 280, 280, 300, 300));
  EXPECT_TRUE(FPDFPath_LineTo(blue_path, 325, 325));
  EXPECT_TRUE(FPDFPath_LineTo(blue_path, 350, 325));
  EXPECT_TRUE(FPDFPath_BezierTo(blue_path, 375, 330, 390, 360, 400, 400));
  EXPECT_TRUE(FPDFPath_Close(blue_path));
  FPDFPage_InsertObject(page, blue_path);
  const char kLastMD5[] = "9823e1a21bd9b72b6a442ba4f12af946";
  {
    ScopedFPDFBitmap page_bitmap = RenderPageWithFlags(page, nullptr, 0);
    CompareBitmap(page_bitmap.get(), 612, 792, kLastMD5);
  }

  // Now save the result, closing the page and document
  EXPECT_TRUE(FPDFPage_GenerateContent(page));
  EXPECT_TRUE(FPDF_SaveAsCopy(document(), this, 0));
  FPDF_ClosePage(page);

  // Render the saved result
  VerifySavedDocument(612, 792, kLastMD5);
}

TEST_F(FPDFEditEmbeddertest, RemovePageObject) {
  // Load document with some text.
  EXPECT_TRUE(OpenDocument("hello_world.pdf"));
  FPDF_PAGE page = LoadPage(0);
  ASSERT_TRUE(page);

  // Show what the original file looks like.
  {
#if _FX_PLATFORM_ == _FX_PLATFORM_APPLE_
    const char kOriginalMD5[] = "b90475ca64d1348c3bf5e2b77ad9187a";
#elif _FX_PLATFORM_ == _FX_PLATFORM_WINDOWS_
    const char kOriginalMD5[] = "e5a6fa28298db07484cd922f3e210c88";
#else
    const char kOriginalMD5[] = "2baa4c0e1758deba1b9c908e1fbd04ed";
#endif
    ScopedFPDFBitmap page_bitmap = RenderPageWithFlags(page, nullptr, 0);
    CompareBitmap(page_bitmap.get(), 200, 200, kOriginalMD5);
  }

  // Get the "Hello, world!" text object and remove it.
  ASSERT_EQ(2, FPDFPage_CountObjects(page));
  FPDF_PAGEOBJECT page_object = FPDFPage_GetObject(page, 0);
  ASSERT_TRUE(page_object);
  EXPECT_TRUE(FPDFPage_RemoveObject(page, page_object));

  // Verify the "Hello, world!" text is gone.
  {
#if _FX_PLATFORM_ == _FX_PLATFORM_APPLE_
    const char kRemovedMD5[] = "af760c4702467cb1492a57fb8215efaa";
#elif _FX_PLATFORM_ == _FX_PLATFORM_WINDOWS_
    const char kRemovedMD5[] = "72be917349bf7004a5c39661fe1fc433";
#else
    const char kRemovedMD5[] = "b76df015fe88009c3c342395df96abf1";
#endif
    ScopedFPDFBitmap page_bitmap = RenderPageWithFlags(page, nullptr, 0);
    CompareBitmap(page_bitmap.get(), 200, 200, kRemovedMD5);
  }
  ASSERT_EQ(1, FPDFPage_CountObjects(page));

  UnloadPage(page);
  FPDFPageObj_Destroy(page_object);
}

TEST_F(FPDFEditEmbeddertest, RemoveMarkedObjectsPrime) {
  // Load document with some text.
  EXPECT_TRUE(OpenDocument("text_in_page_marked.pdf"));
  FPDF_PAGE page = LoadPage(0);
  ASSERT_TRUE(page);

  // Show what the original file looks like.
  {
#if _FX_PLATFORM_ == _FX_PLATFORM_APPLE_
    const char kOriginalMD5[] = "5a5eb63cb21cc15084fea1f14284b8df";
#elif _FX_PLATFORM_ == _FX_PLATFORM_WINDOWS_
    const char kOriginalMD5[] = "587c507a40f613f9c530b2ce2d58d655";
#else
    const char kOriginalMD5[] = "2edc6e70d54889aa0c0b7bdf3e168f86";
#endif
    ScopedFPDFBitmap page_bitmap = RenderPageWithFlags(page, nullptr, 0);
    CompareBitmap(page_bitmap.get(), 200, 200, kOriginalMD5);
  }

  // Iterate over all objects, counting the number of times each content mark
  // name appears.
  int object_count = FPDFPage_CountObjects(page);
  ASSERT_EQ(19, object_count);

  unsigned long prime_count = 0;
  unsigned long square_count = 0;
  unsigned long greater_than_ten_count = 0;
  std::vector<FPDF_PAGEOBJECT> primes;
  for (int i = 0; i < object_count; ++i) {
    FPDF_PAGEOBJECT page_object = FPDFPage_GetObject(page, i);

    int mark_count = FPDFPageObj_CountMarks(page_object);
    for (int j = 0; j < mark_count; ++j) {
      FPDF_PAGEOBJECTMARK mark = FPDFPageObj_GetMark(page_object, j);

      char buffer[256];
      ASSERT_GT(FPDFPageObjMark_GetName(mark, buffer, 256), 0u);
      std::wstring name =
          GetPlatformWString(reinterpret_cast<unsigned short*>(buffer));
      if (name == L"Prime") {
        prime_count++;
        EXPECT_EQ(0, FPDFPageObjMark_CountParams(mark));
        primes.push_back(page_object);
      } else if (name == L"Square") {
        square_count++;
        EXPECT_EQ(1, FPDFPageObjMark_CountParams(mark));
        ASSERT_GT(FPDFPageObjMark_GetParamKey(mark, 0, buffer, 256), 0u);
        std::wstring key =
            GetPlatformWString(reinterpret_cast<unsigned short*>(buffer));
        EXPECT_EQ(L"Factor", key);
        EXPECT_EQ(FPDF_OBJECT_NUMBER,
                  FPDFPageObjMark_GetParamValueType(mark, 0));
        int square_root = FPDFPageObjMark_GetParamIntValue(mark, 0);
        EXPECT_EQ(i + 1, square_root * square_root);
      } else if (name == L"GreaterThanTen") {
        greater_than_ten_count++;
        EXPECT_EQ(0, FPDFPageObjMark_CountParams(mark));
      } else if (name == L"Bounds") {
        EXPECT_EQ(1, FPDFPageObjMark_CountParams(mark));
        ASSERT_GT(FPDFPageObjMark_GetParamKey(mark, 0, buffer, 256), 0u);
        std::wstring key =
            GetPlatformWString(reinterpret_cast<unsigned short*>(buffer));
        EXPECT_EQ(L"Position", key);
        EXPECT_EQ(FPDF_OBJECT_STRING,
                  FPDFPageObjMark_GetParamValueType(mark, 0));
        ASSERT_GT(FPDFPageObjMark_GetParamStringValue(mark, 0, buffer, 256),
                  0u);
        std::wstring value =
            GetPlatformWString(reinterpret_cast<unsigned short*>(buffer));
        EXPECT_EQ(L"Last", value);
        EXPECT_EQ(18, i);
      } else {
        FAIL();
      }
    }
  }

  // Expect certain number of tagged objects. The test file contains strings
  // from 1 to 19.
  EXPECT_EQ(8u, prime_count);
  EXPECT_EQ(4u, square_count);
  EXPECT_EQ(9u, greater_than_ten_count);

  // Remove all objects marked with "Prime".
  for (FPDF_PAGEOBJECT page_object : primes) {
    EXPECT_TRUE(FPDFPage_RemoveObject(page, page_object));
    FPDFPageObj_Destroy(page_object);
  }

  EXPECT_EQ(11, FPDFPage_CountObjects(page));

  {
#if _FX_PLATFORM_ == _FX_PLATFORM_APPLE_
    const char kNonPrimesMD5[] = "57e76dc7375d896704f0fd6d6d1b9e65";
#elif _FX_PLATFORM_ == _FX_PLATFORM_WINDOWS_
    const char kNonPrimesMD5[] = "4d906b57fba36c70c600cf50d60f508c";
#else
    const char kNonPrimesMD5[] = "33d9c45bec41ead92a295e252f6b7922";
#endif
    ScopedFPDFBitmap page_bitmap = RenderPageWithFlags(page, nullptr, 0);
    CompareBitmap(page_bitmap.get(), 200, 200, kNonPrimesMD5);
  }

  UnloadPage(page);
}

// Fails due to pdfium:1051.
TEST_F(FPDFEditEmbeddertest, DISABLED_RemoveExistingPageObject) {
  // Load document with some text.
  EXPECT_TRUE(OpenDocument("hello_world.pdf"));
  FPDF_PAGE page = LoadPage(0);
  ASSERT_TRUE(page);

  // Get the "Hello, world!" text object and remove it.
  ASSERT_EQ(2, FPDFPage_CountObjects(page));
  FPDF_PAGEOBJECT page_object = FPDFPage_GetObject(page, 0);
  ASSERT_TRUE(page_object);
  EXPECT_TRUE(FPDFPage_RemoveObject(page, page_object));

  // Verify the "Hello, world!" text is gone.
  ASSERT_EQ(1, FPDFPage_CountObjects(page));

  // Save the file
  EXPECT_TRUE(FPDFPage_GenerateContent(page));
  EXPECT_TRUE(FPDF_SaveAsCopy(document(), this, 0));
  UnloadPage(page);
  FPDFPageObj_Destroy(page_object);

  // Re-open the file and check the page object count is still 1.
  OpenSavedDocument();
  FPDF_PAGE saved_page = LoadSavedPage(0);
  EXPECT_EQ(1, FPDFPage_CountObjects(saved_page));
  CloseSavedPage(saved_page);
  CloseSavedDocument();
}

TEST_F(FPDFEditEmbeddertest, InsertPageObjectAndSave) {
  // Load document with some text.
  EXPECT_TRUE(OpenDocument("hello_world.pdf"));
  FPDF_PAGE page = LoadPage(0);
  ASSERT_TRUE(page);

  // Add a red rectangle.
  ASSERT_EQ(2, FPDFPage_CountObjects(page));
  FPDF_PAGEOBJECT red_rect = FPDFPageObj_CreateNewRect(20, 100, 50, 50);
  EXPECT_TRUE(FPDFPath_SetFillColor(red_rect, 255, 0, 0, 255));
  EXPECT_TRUE(FPDFPath_SetDrawMode(red_rect, FPDF_FILLMODE_ALTERNATE, 0));
  FPDFPage_InsertObject(page, red_rect);

  // Verify the red rectangle was added.
  ASSERT_EQ(3, FPDFPage_CountObjects(page));

  // Save the file
  EXPECT_TRUE(FPDFPage_GenerateContent(page));
  EXPECT_TRUE(FPDF_SaveAsCopy(document(), this, 0));
  UnloadPage(page);

  // Re-open the file and check the page object count is still 3.
  OpenSavedDocument();
  FPDF_PAGE saved_page = LoadSavedPage(0);
  EXPECT_EQ(3, FPDFPage_CountObjects(saved_page));
  CloseSavedPage(saved_page);
  CloseSavedDocument();
}

TEST_F(FPDFEditEmbeddertest, AddAndRemovePaths) {
  // Start with a blank page.
  FPDF_PAGE page = FPDFPage_New(CreateNewDocument(), 0, 612, 792);
  ASSERT_TRUE(page);

  // Render the blank page and verify it's a blank bitmap.
  const char kBlankMD5[] = "1940568c9ba33bac5d0b1ee9558c76b3";
  {
    ScopedFPDFBitmap page_bitmap = RenderPageWithFlags(page, nullptr, 0);
    CompareBitmap(page_bitmap.get(), 612, 792, kBlankMD5);
  }
  ASSERT_EQ(0, FPDFPage_CountObjects(page));

  // Add a red rectangle.
  FPDF_PAGEOBJECT red_rect = FPDFPageObj_CreateNewRect(10, 10, 20, 20);
  ASSERT_TRUE(red_rect);
  EXPECT_TRUE(FPDFPath_SetFillColor(red_rect, 255, 0, 0, 255));
  EXPECT_TRUE(FPDFPath_SetDrawMode(red_rect, FPDF_FILLMODE_ALTERNATE, 0));
  FPDFPage_InsertObject(page, red_rect);
  const char kRedRectangleMD5[] = "66d02eaa6181e2c069ce2ea99beda497";
  {
    ScopedFPDFBitmap page_bitmap = RenderPageWithFlags(page, nullptr, 0);
    CompareBitmap(page_bitmap.get(), 612, 792, kRedRectangleMD5);
  }
  EXPECT_EQ(1, FPDFPage_CountObjects(page));

  // Remove rectangle and verify it does not render anymore and the bitmap is
  // back to a blank one.
  EXPECT_TRUE(FPDFPage_RemoveObject(page, red_rect));
  {
    ScopedFPDFBitmap page_bitmap = RenderPageWithFlags(page, nullptr, 0);
    CompareBitmap(page_bitmap.get(), 612, 792, kBlankMD5);
  }
  EXPECT_EQ(0, FPDFPage_CountObjects(page));

  // Trying to remove an object not in the page should return false.
  EXPECT_FALSE(FPDFPage_RemoveObject(page, red_rect));

  FPDF_ClosePage(page);
  FPDFPageObj_Destroy(red_rect);
}

TEST_F(FPDFEditEmbeddertest, PathsPoints) {
  CreateNewDocument();
  FPDF_PAGEOBJECT img = FPDFPageObj_NewImageObj(document_);
  // This should fail gracefully, even if img is not a path.
  ASSERT_EQ(-1, FPDFPath_CountSegments(img));

  // This should fail gracefully, even if path is NULL.
  ASSERT_EQ(-1, FPDFPath_CountSegments(nullptr));

  // FPDFPath_GetPathSegment() with a non-path.
  ASSERT_EQ(nullptr, FPDFPath_GetPathSegment(img, 0));
  // FPDFPath_GetPathSegment() with a NULL path.
  ASSERT_EQ(nullptr, FPDFPath_GetPathSegment(nullptr, 0));
  float x;
  float y;
  // FPDFPathSegment_GetPoint() with a NULL segment.
  EXPECT_FALSE(FPDFPathSegment_GetPoint(nullptr, &x, &y));

  // FPDFPathSegment_GetType() with a NULL segment.
  ASSERT_EQ(FPDF_SEGMENT_UNKNOWN, FPDFPathSegment_GetType(nullptr));

  // FPDFPathSegment_GetClose() with a NULL segment.
  EXPECT_FALSE(FPDFPathSegment_GetClose(nullptr));

  FPDFPageObj_Destroy(img);
}

TEST_F(FPDFEditEmbeddertest, PathOnTopOfText) {
  // Load document with some text
  EXPECT_TRUE(OpenDocument("hello_world.pdf"));
  FPDF_PAGE page = LoadPage(0);
  ASSERT_TRUE(page);

  // Add an opaque rectangle on top of some of the text.
  FPDF_PAGEOBJECT red_rect = FPDFPageObj_CreateNewRect(20, 100, 50, 50);
  EXPECT_TRUE(FPDFPath_SetFillColor(red_rect, 255, 0, 0, 255));
  EXPECT_TRUE(FPDFPath_SetDrawMode(red_rect, FPDF_FILLMODE_ALTERNATE, 0));
  FPDFPage_InsertObject(page, red_rect);

  // Add a transparent triangle on top of other part of the text.
  FPDF_PAGEOBJECT black_path = FPDFPageObj_CreateNewPath(20, 50);
  EXPECT_TRUE(FPDFPath_SetFillColor(black_path, 0, 0, 0, 100));
  EXPECT_TRUE(FPDFPath_SetDrawMode(black_path, FPDF_FILLMODE_ALTERNATE, 0));
  EXPECT_TRUE(FPDFPath_LineTo(black_path, 30, 80));
  EXPECT_TRUE(FPDFPath_LineTo(black_path, 40, 10));
  EXPECT_TRUE(FPDFPath_Close(black_path));
  FPDFPage_InsertObject(page, black_path);

  // Render and check the result. Text is slightly different on Mac.
  ScopedFPDFBitmap bitmap = RenderLoadedPage(page);
#if _FX_PLATFORM_ == _FX_PLATFORM_APPLE_
  const char md5[] = "f9e6fa74230f234286bfcada9f7606d8";
#else
  const char md5[] = "aa71b09b93b55f467f1290e5111babee";
#endif
  CompareBitmap(bitmap.get(), 200, 200, md5);
  UnloadPage(page);
}

TEST_F(FPDFEditEmbeddertest, EditOverExistingContent) {
  // Load document with existing content
  EXPECT_TRUE(OpenDocument("bug_717.pdf"));
  FPDF_PAGE page = LoadPage(0);
  ASSERT_TRUE(page);

  // Add a transparent rectangle on top of the existing content
  FPDF_PAGEOBJECT red_rect2 = FPDFPageObj_CreateNewRect(90, 700, 25, 50);
  EXPECT_TRUE(FPDFPath_SetFillColor(red_rect2, 255, 0, 0, 100));
  EXPECT_TRUE(FPDFPath_SetDrawMode(red_rect2, FPDF_FILLMODE_ALTERNATE, 0));
  FPDFPage_InsertObject(page, red_rect2);

  // Add an opaque rectangle on top of the existing content
  FPDF_PAGEOBJECT red_rect = FPDFPageObj_CreateNewRect(115, 700, 25, 50);
  EXPECT_TRUE(FPDFPath_SetFillColor(red_rect, 255, 0, 0, 255));
  EXPECT_TRUE(FPDFPath_SetDrawMode(red_rect, FPDF_FILLMODE_ALTERNATE, 0));
  FPDFPage_InsertObject(page, red_rect);

  ScopedFPDFBitmap bitmap = RenderLoadedPage(page);
  CompareBitmap(bitmap.get(), 612, 792, "ad04e5bd0f471a9a564fb034bd0fb073");
  EXPECT_TRUE(FPDFPage_GenerateContent(page));

  // Now save the result, closing the page and document
  EXPECT_TRUE(FPDF_SaveAsCopy(document(), this, 0));
  UnloadPage(page);

  OpenSavedDocument();
  FPDF_PAGE saved_page = LoadSavedPage(0);
  VerifySavedRendering(saved_page, 612, 792,
                       "ad04e5bd0f471a9a564fb034bd0fb073");

  ClearString();
  // Add another opaque rectangle on top of the existing content
  FPDF_PAGEOBJECT green_rect = FPDFPageObj_CreateNewRect(150, 700, 25, 50);
  EXPECT_TRUE(FPDFPath_SetFillColor(green_rect, 0, 255, 0, 255));
  EXPECT_TRUE(FPDFPath_SetDrawMode(green_rect, FPDF_FILLMODE_ALTERNATE, 0));
  FPDFPage_InsertObject(saved_page, green_rect);

  // Add another transparent rectangle on top of existing content
  FPDF_PAGEOBJECT green_rect2 = FPDFPageObj_CreateNewRect(175, 700, 25, 50);
  EXPECT_TRUE(FPDFPath_SetFillColor(green_rect2, 0, 255, 0, 100));
  EXPECT_TRUE(FPDFPath_SetDrawMode(green_rect2, FPDF_FILLMODE_ALTERNATE, 0));
  FPDFPage_InsertObject(saved_page, green_rect2);
  const char kLastMD5[] = "4b5b00f824620f8c9b8801ebb98e1cdd";
  {
    ScopedFPDFBitmap new_bitmap = RenderSavedPage(saved_page);
    CompareBitmap(new_bitmap.get(), 612, 792, kLastMD5);
  }
  EXPECT_TRUE(FPDFPage_GenerateContent(saved_page));

  // Now save the result, closing the page and document
  EXPECT_TRUE(FPDF_SaveAsCopy(saved_document_, this, 0));

  CloseSavedPage(saved_page);
  CloseSavedDocument();

  // Render the saved result
  VerifySavedDocument(612, 792, kLastMD5);
}

TEST_F(FPDFEditEmbeddertest, AddStrokedPaths) {
  // Start with a blank page
  FPDF_PAGE page = FPDFPage_New(CreateNewDocument(), 0, 612, 792);

  // Add a large stroked rectangle (fill color should not affect it).
  FPDF_PAGEOBJECT rect = FPDFPageObj_CreateNewRect(20, 20, 200, 400);
  EXPECT_TRUE(FPDFPath_SetFillColor(rect, 255, 0, 0, 255));
  EXPECT_TRUE(FPDFPath_SetStrokeColor(rect, 0, 255, 0, 255));
  EXPECT_TRUE(FPDFPath_SetStrokeWidth(rect, 15.0f));

  float width = 0;
  EXPECT_TRUE(FPDFPageObj_GetStrokeWidth(rect, &width));
  EXPECT_EQ(15.0f, width);

  EXPECT_TRUE(FPDFPath_SetDrawMode(rect, 0, 1));
  FPDFPage_InsertObject(page, rect);
  {
    ScopedFPDFBitmap page_bitmap = RenderPageWithFlags(page, nullptr, 0);
    CompareBitmap(page_bitmap.get(), 612, 792,
                  "64bd31f862a89e0a9e505a5af6efd506");
  }

  // Add crossed-checkmark
  FPDF_PAGEOBJECT check = FPDFPageObj_CreateNewPath(300, 500);
  EXPECT_TRUE(FPDFPath_LineTo(check, 400, 400));
  EXPECT_TRUE(FPDFPath_LineTo(check, 600, 600));
  EXPECT_TRUE(FPDFPath_MoveTo(check, 400, 600));
  EXPECT_TRUE(FPDFPath_LineTo(check, 600, 400));
  EXPECT_TRUE(FPDFPath_SetStrokeColor(check, 128, 128, 128, 180));
  EXPECT_TRUE(FPDFPath_SetStrokeWidth(check, 8.35f));
  EXPECT_TRUE(FPDFPath_SetDrawMode(check, 0, 1));
  FPDFPage_InsertObject(page, check);
  {
    ScopedFPDFBitmap page_bitmap = RenderPageWithFlags(page, nullptr, 0);
    CompareBitmap(page_bitmap.get(), 612, 792,
                  "4b6f3b9d25c4e194821217d5016c3724");
  }

  // Add stroked and filled oval-ish path.
  FPDF_PAGEOBJECT path = FPDFPageObj_CreateNewPath(250, 100);
  EXPECT_TRUE(FPDFPath_BezierTo(path, 180, 166, 180, 233, 250, 300));
  EXPECT_TRUE(FPDFPath_LineTo(path, 255, 305));
  EXPECT_TRUE(FPDFPath_BezierTo(path, 325, 233, 325, 166, 255, 105));
  EXPECT_TRUE(FPDFPath_Close(path));
  EXPECT_TRUE(FPDFPath_SetFillColor(path, 200, 128, 128, 100));
  EXPECT_TRUE(FPDFPath_SetStrokeColor(path, 128, 200, 128, 150));
  EXPECT_TRUE(FPDFPath_SetStrokeWidth(path, 10.5f));
  EXPECT_TRUE(FPDFPath_SetDrawMode(path, FPDF_FILLMODE_ALTERNATE, 1));
  FPDFPage_InsertObject(page, path);
  {
    ScopedFPDFBitmap page_bitmap = RenderPageWithFlags(page, nullptr, 0);
    CompareBitmap(page_bitmap.get(), 612, 792,
                  "ff3e6a22326754944cc6e56609acd73b");
  }
  FPDF_ClosePage(page);
}

TEST_F(FPDFEditEmbeddertest, AddStandardFontText) {
  // Start with a blank page
  FPDF_PAGE page = FPDFPage_New(CreateNewDocument(), 0, 612, 792);

  // Add some text to the page
  FPDF_PAGEOBJECT text_object1 =
      FPDFPageObj_NewTextObj(document(), "Arial", 12.0f);
  EXPECT_TRUE(text_object1);
  std::unique_ptr<unsigned short, pdfium::FreeDeleter> text1 =
      GetFPDFWideString(L"I'm at the bottom of the page");
  EXPECT_TRUE(FPDFText_SetText(text_object1, text1.get()));
  FPDFPageObj_Transform(text_object1, 1, 0, 0, 1, 20, 20);
  FPDFPage_InsertObject(page, text_object1);
  {
    ScopedFPDFBitmap page_bitmap = RenderPageWithFlags(page, nullptr, 0);
#if _FX_PLATFORM_ == _FX_PLATFORM_APPLE_
    const char md5[] = "a4dddc1a3930fa694bbff9789dab4161";
#else
    const char md5[] = "eacaa24573b8ce997b3882595f096f00";
#endif
    CompareBitmap(page_bitmap.get(), 612, 792, md5);
  }

  // Try another font
  FPDF_PAGEOBJECT text_object2 =
      FPDFPageObj_NewTextObj(document(), "TimesNewRomanBold", 15.0f);
  EXPECT_TRUE(text_object2);
  std::unique_ptr<unsigned short, pdfium::FreeDeleter> text2 =
      GetFPDFWideString(L"Hi, I'm Bold. Times New Roman Bold.");
  EXPECT_TRUE(FPDFText_SetText(text_object2, text2.get()));
  FPDFPageObj_Transform(text_object2, 1, 0, 0, 1, 100, 600);
  FPDFPage_InsertObject(page, text_object2);
  {
    ScopedFPDFBitmap page_bitmap = RenderPageWithFlags(page, nullptr, 0);
#if _FX_PLATFORM_ == _FX_PLATFORM_APPLE_
    const char md5_2[] = "a5c4ace4c6f27644094813fe1441a21c";
#elif _FX_PLATFORM_ == _FX_PLATFORM_WINDOWS_
    const char md5_2[] = "2587eac9a787e97a37636d54d11bd28d";
#else
    const char md5_2[] = "76fcc7d08aa15445efd2e2ceb7c6cc3b";
#endif
    CompareBitmap(page_bitmap.get(), 612, 792, md5_2);
  }

  // And some randomly transformed text
  FPDF_PAGEOBJECT text_object3 =
      FPDFPageObj_NewTextObj(document(), "Courier-Bold", 20.0f);
  EXPECT_TRUE(text_object3);
  std::unique_ptr<unsigned short, pdfium::FreeDeleter> text3 =
      GetFPDFWideString(L"Can you read me? <:)>");
  EXPECT_TRUE(FPDFText_SetText(text_object3, text3.get()));
  FPDFPageObj_Transform(text_object3, 1, 1.5, 2, 0.5, 200, 200);
  FPDFPage_InsertObject(page, text_object3);
  {
    ScopedFPDFBitmap page_bitmap = RenderPageWithFlags(page, nullptr, 0);
#if _FX_PLATFORM_ == _FX_PLATFORM_APPLE_
    const char md5_3[] = "40b3ef04f915ff4c4208948001763544";
#elif _FX_PLATFORM_ == _FX_PLATFORM_WINDOWS_
    const char md5_3[] = "7cb61ec112cf400b489360d443ffc9d2";
#else
    const char md5_3[] = "b8a21668f1dab625af7c072e07fcefc4";
#endif
    CompareBitmap(page_bitmap.get(), 612, 792, md5_3);
  }

  // TODO(npm): Why are there issues with text rotated by 90 degrees?
  // TODO(npm): FPDF_SaveAsCopy not giving the desired result after this.
  FPDF_ClosePage(page);
}

TEST_F(FPDFEditEmbeddertest, GraphicsData) {
  // New page
  ScopedFPDFPage page(FPDFPage_New(CreateNewDocument(), 0, 612, 792));

  // Create a rect with nontrivial graphics
  FPDF_PAGEOBJECT rect1 = FPDFPageObj_CreateNewRect(10, 10, 100, 100);
  FPDFPageObj_SetBlendMode(rect1, "Color");
  FPDFPage_InsertObject(page.get(), rect1);
  EXPECT_TRUE(FPDFPage_GenerateContent(page.get()));

  // Check that the ExtGState was created
  CPDF_Page* cpage = CPDFPageFromFPDFPage(page.get());
  CPDF_Dictionary* graphics_dict = cpage->m_pResources->GetDictFor("ExtGState");
  ASSERT_TRUE(graphics_dict);
  EXPECT_EQ(2, static_cast<int>(graphics_dict->GetCount()));

  // Add a text object causing no change to the graphics dictionary
  FPDF_PAGEOBJECT text1 = FPDFPageObj_NewTextObj(document(), "Arial", 12.0f);
  // Only alpha, the last component, matters for the graphics dictionary. And
  // the default value is 255.
  EXPECT_TRUE(FPDFText_SetFillColor(text1, 100, 100, 100, 255));
  FPDFPage_InsertObject(page.get(), text1);
  EXPECT_TRUE(FPDFPage_GenerateContent(page.get()));
  EXPECT_EQ(2, static_cast<int>(graphics_dict->GetCount()));

  // Add a text object increasing the size of the graphics dictionary
  FPDF_PAGEOBJECT text2 =
      FPDFPageObj_NewTextObj(document(), "Times-Roman", 12.0f);
  FPDFPage_InsertObject(page.get(), text2);
  FPDFPageObj_SetBlendMode(text2, "Darken");
  EXPECT_TRUE(FPDFText_SetFillColor(text2, 0, 0, 255, 150));
  EXPECT_TRUE(FPDFPage_GenerateContent(page.get()));
  EXPECT_EQ(3, static_cast<int>(graphics_dict->GetCount()));

  // Add a path that should reuse graphics
  FPDF_PAGEOBJECT path = FPDFPageObj_CreateNewPath(400, 100);
  FPDFPageObj_SetBlendMode(path, "Darken");
  EXPECT_TRUE(FPDFPath_SetFillColor(path, 200, 200, 100, 150));
  FPDFPage_InsertObject(page.get(), path);
  EXPECT_TRUE(FPDFPage_GenerateContent(page.get()));
  EXPECT_EQ(3, static_cast<int>(graphics_dict->GetCount()));

  // Add a rect increasing the size of the graphics dictionary
  FPDF_PAGEOBJECT rect2 = FPDFPageObj_CreateNewRect(10, 10, 100, 100);
  FPDFPageObj_SetBlendMode(rect2, "Darken");
  EXPECT_TRUE(FPDFPath_SetFillColor(rect2, 0, 0, 255, 150));
  EXPECT_TRUE(FPDFPath_SetStrokeColor(rect2, 0, 0, 0, 200));
  FPDFPage_InsertObject(page.get(), rect2);
  EXPECT_TRUE(FPDFPage_GenerateContent(page.get()));
  EXPECT_EQ(4, static_cast<int>(graphics_dict->GetCount()));
}

TEST_F(FPDFEditEmbeddertest, DoubleGenerating) {
  // Start with a blank page
  FPDF_PAGE page = FPDFPage_New(CreateNewDocument(), 0, 612, 792);

  // Add a red rectangle with some non-default alpha
  FPDF_PAGEOBJECT rect = FPDFPageObj_CreateNewRect(10, 10, 100, 100);
  EXPECT_TRUE(FPDFPath_SetFillColor(rect, 255, 0, 0, 128));
  EXPECT_TRUE(FPDFPath_SetDrawMode(rect, FPDF_FILLMODE_WINDING, 0));
  FPDFPage_InsertObject(page, rect);
  EXPECT_TRUE(FPDFPage_GenerateContent(page));

  // Check the ExtGState
  CPDF_Page* cpage = CPDFPageFromFPDFPage(page);
  CPDF_Dictionary* graphics_dict = cpage->m_pResources->GetDictFor("ExtGState");
  ASSERT_TRUE(graphics_dict);
  EXPECT_EQ(2, static_cast<int>(graphics_dict->GetCount()));

  // Check the bitmap
  {
    ScopedFPDFBitmap page_bitmap = RenderPageWithFlags(page, nullptr, 0);
    CompareBitmap(page_bitmap.get(), 612, 792,
                  "5384da3406d62360ffb5cac4476fff1c");
  }

  // Never mind, my new favorite color is blue, increase alpha
  EXPECT_TRUE(FPDFPath_SetFillColor(rect, 0, 0, 255, 180));
  EXPECT_TRUE(FPDFPage_GenerateContent(page));
  EXPECT_EQ(3, static_cast<int>(graphics_dict->GetCount()));

  // Check that bitmap displays changed content
  {
    ScopedFPDFBitmap page_bitmap = RenderPageWithFlags(page, nullptr, 0);
    CompareBitmap(page_bitmap.get(), 612, 792,
                  "2e51656f5073b0bee611d9cd086aa09c");
  }

  // And now generate, without changes
  EXPECT_TRUE(FPDFPage_GenerateContent(page));
  EXPECT_EQ(3, static_cast<int>(graphics_dict->GetCount()));
  {
    ScopedFPDFBitmap page_bitmap = RenderPageWithFlags(page, nullptr, 0);
    CompareBitmap(page_bitmap.get(), 612, 792,
                  "2e51656f5073b0bee611d9cd086aa09c");
  }

  // Add some text to the page
  FPDF_PAGEOBJECT text_object =
      FPDFPageObj_NewTextObj(document(), "Arial", 12.0f);
  std::unique_ptr<unsigned short, pdfium::FreeDeleter> text =
      GetFPDFWideString(L"Something something #text# something");
  EXPECT_TRUE(FPDFText_SetText(text_object, text.get()));
  FPDFPageObj_Transform(text_object, 1, 0, 0, 1, 300, 300);
  FPDFPage_InsertObject(page, text_object);
  EXPECT_TRUE(FPDFPage_GenerateContent(page));
  CPDF_Dictionary* font_dict = cpage->m_pResources->GetDictFor("Font");
  ASSERT_TRUE(font_dict);
  EXPECT_EQ(1, static_cast<int>(font_dict->GetCount()));

  // Generate yet again, check dicts are reasonably sized
  EXPECT_TRUE(FPDFPage_GenerateContent(page));
  EXPECT_EQ(3, static_cast<int>(graphics_dict->GetCount()));
  EXPECT_EQ(1, static_cast<int>(font_dict->GetCount()));
  FPDF_ClosePage(page);
}

TEST_F(FPDFEditEmbeddertest, LoadSimpleType1Font) {
  CreateNewDocument();
  // TODO(npm): use other fonts after disallowing loading any font as any type
  const CPDF_Font* stock_font =
      CPDF_Font::GetStockFont(cpdf_doc(), "Times-Bold");
  const uint8_t* data = stock_font->GetFont()->GetFontData();
  const uint32_t size = stock_font->GetFont()->GetSize();
  ScopedFPDFFont font(
      FPDFText_LoadFont(document(), data, size, FPDF_FONT_TYPE1, false));
  ASSERT_TRUE(font.get());
  CPDF_Font* typed_font = CPDFFontFromFPDFFont(font.get());
  EXPECT_TRUE(typed_font->IsType1Font());

  CPDF_Dictionary* font_dict = typed_font->GetFontDict();
  EXPECT_EQ("Font", font_dict->GetStringFor("Type"));
  EXPECT_EQ("Type1", font_dict->GetStringFor("Subtype"));
  EXPECT_EQ("Times New Roman Bold", font_dict->GetStringFor("BaseFont"));
  ASSERT_TRUE(font_dict->KeyExist("FirstChar"));
  ASSERT_TRUE(font_dict->KeyExist("LastChar"));
  EXPECT_EQ(32, font_dict->GetIntegerFor("FirstChar"));
  EXPECT_EQ(255, font_dict->GetIntegerFor("LastChar"));

  CPDF_Array* widths_array = font_dict->GetArrayFor("Widths");
  ASSERT_TRUE(widths_array);
  ASSERT_EQ(224U, widths_array->GetCount());
  EXPECT_EQ(250, widths_array->GetNumberAt(0));
  EXPECT_EQ(569, widths_array->GetNumberAt(11));
  EXPECT_EQ(500, widths_array->GetNumberAt(223));
  CheckFontDescriptor(font_dict, FPDF_FONT_TYPE1, true, false, size, data);
}

TEST_F(FPDFEditEmbeddertest, LoadSimpleTrueTypeFont) {
  CreateNewDocument();
  const CPDF_Font* stock_font = CPDF_Font::GetStockFont(cpdf_doc(), "Courier");
  const uint8_t* data = stock_font->GetFont()->GetFontData();
  const uint32_t size = stock_font->GetFont()->GetSize();
  ScopedFPDFFont font(
      FPDFText_LoadFont(document(), data, size, FPDF_FONT_TRUETYPE, false));
  ASSERT_TRUE(font.get());
  CPDF_Font* typed_font = CPDFFontFromFPDFFont(font.get());
  EXPECT_TRUE(typed_font->IsTrueTypeFont());

  CPDF_Dictionary* font_dict = typed_font->GetFontDict();
  EXPECT_EQ("Font", font_dict->GetStringFor("Type"));
  EXPECT_EQ("TrueType", font_dict->GetStringFor("Subtype"));
  EXPECT_EQ("Courier New", font_dict->GetStringFor("BaseFont"));
  ASSERT_TRUE(font_dict->KeyExist("FirstChar"));
  ASSERT_TRUE(font_dict->KeyExist("LastChar"));
  EXPECT_EQ(32, font_dict->GetIntegerFor("FirstChar"));
  EXPECT_EQ(255, font_dict->GetIntegerFor("LastChar"));

  CPDF_Array* widths_array = font_dict->GetArrayFor("Widths");
  ASSERT_TRUE(widths_array);
  ASSERT_EQ(224U, widths_array->GetCount());
  EXPECT_EQ(600, widths_array->GetNumberAt(33));
  EXPECT_EQ(600, widths_array->GetNumberAt(74));
  EXPECT_EQ(600, widths_array->GetNumberAt(223));
  CheckFontDescriptor(font_dict, FPDF_FONT_TRUETYPE, false, false, size, data);
}

TEST_F(FPDFEditEmbeddertest, LoadCIDType0Font) {
  CreateNewDocument();
  const CPDF_Font* stock_font =
      CPDF_Font::GetStockFont(cpdf_doc(), "Times-Roman");
  const uint8_t* data = stock_font->GetFont()->GetFontData();
  const uint32_t size = stock_font->GetFont()->GetSize();
  ScopedFPDFFont font(
      FPDFText_LoadFont(document(), data, size, FPDF_FONT_TYPE1, 1));
  ASSERT_TRUE(font.get());
  CPDF_Font* typed_font = CPDFFontFromFPDFFont(font.get());
  EXPECT_TRUE(typed_font->IsCIDFont());

  // Check font dictionary entries
  CPDF_Dictionary* font_dict = typed_font->GetFontDict();
  EXPECT_EQ("Font", font_dict->GetStringFor("Type"));
  EXPECT_EQ("Type0", font_dict->GetStringFor("Subtype"));
  EXPECT_EQ("Times New Roman-Identity-H", font_dict->GetStringFor("BaseFont"));
  EXPECT_EQ("Identity-H", font_dict->GetStringFor("Encoding"));
  CPDF_Array* descendant_array = font_dict->GetArrayFor("DescendantFonts");
  ASSERT_TRUE(descendant_array);
  EXPECT_EQ(1U, descendant_array->GetCount());

  // Check the CIDFontDict
  CPDF_Dictionary* cidfont_dict = descendant_array->GetDictAt(0);
  EXPECT_EQ("Font", cidfont_dict->GetStringFor("Type"));
  EXPECT_EQ("CIDFontType0", cidfont_dict->GetStringFor("Subtype"));
  EXPECT_EQ("Times New Roman", cidfont_dict->GetStringFor("BaseFont"));
  CPDF_Dictionary* cidinfo_dict = cidfont_dict->GetDictFor("CIDSystemInfo");
  ASSERT_TRUE(cidinfo_dict);
  EXPECT_EQ("Adobe", cidinfo_dict->GetStringFor("Registry"));
  EXPECT_EQ("Identity", cidinfo_dict->GetStringFor("Ordering"));
  EXPECT_EQ(0, cidinfo_dict->GetNumberFor("Supplement"));
  CheckFontDescriptor(cidfont_dict, FPDF_FONT_TYPE1, false, false, size, data);

  // Check widths
  CPDF_Array* widths_array = cidfont_dict->GetArrayFor("W");
  ASSERT_TRUE(widths_array);
  EXPECT_GT(widths_array->GetCount(), 1U);
  CheckCompositeFontWidths(widths_array, typed_font);
}

TEST_F(FPDFEditEmbeddertest, LoadCIDType2Font) {
  CreateNewDocument();
  const CPDF_Font* stock_font =
      CPDF_Font::GetStockFont(cpdf_doc(), "Helvetica-Oblique");
  const uint8_t* data = stock_font->GetFont()->GetFontData();
  const uint32_t size = stock_font->GetFont()->GetSize();

  ScopedFPDFFont font(
      FPDFText_LoadFont(document(), data, size, FPDF_FONT_TRUETYPE, 1));
  ASSERT_TRUE(font.get());
  CPDF_Font* typed_font = CPDFFontFromFPDFFont(font.get());
  EXPECT_TRUE(typed_font->IsCIDFont());

  // Check font dictionary entries
  CPDF_Dictionary* font_dict = typed_font->GetFontDict();
  EXPECT_EQ("Font", font_dict->GetStringFor("Type"));
  EXPECT_EQ("Type0", font_dict->GetStringFor("Subtype"));
  EXPECT_EQ("Arial Italic", font_dict->GetStringFor("BaseFont"));
  EXPECT_EQ("Identity-H", font_dict->GetStringFor("Encoding"));
  CPDF_Array* descendant_array = font_dict->GetArrayFor("DescendantFonts");
  ASSERT_TRUE(descendant_array);
  EXPECT_EQ(1U, descendant_array->GetCount());

  // Check the CIDFontDict
  CPDF_Dictionary* cidfont_dict = descendant_array->GetDictAt(0);
  EXPECT_EQ("Font", cidfont_dict->GetStringFor("Type"));
  EXPECT_EQ("CIDFontType2", cidfont_dict->GetStringFor("Subtype"));
  EXPECT_EQ("Arial Italic", cidfont_dict->GetStringFor("BaseFont"));
  CPDF_Dictionary* cidinfo_dict = cidfont_dict->GetDictFor("CIDSystemInfo");
  ASSERT_TRUE(cidinfo_dict);
  EXPECT_EQ("Adobe", cidinfo_dict->GetStringFor("Registry"));
  EXPECT_EQ("Identity", cidinfo_dict->GetStringFor("Ordering"));
  EXPECT_EQ(0, cidinfo_dict->GetNumberFor("Supplement"));
  CheckFontDescriptor(cidfont_dict, FPDF_FONT_TRUETYPE, false, true, size,
                      data);

  // Check widths
  CPDF_Array* widths_array = cidfont_dict->GetArrayFor("W");
  ASSERT_TRUE(widths_array);
  CheckCompositeFontWidths(widths_array, typed_font);
}

TEST_F(FPDFEditEmbeddertest, NormalizeNegativeRotation) {
  // Load document with a -90 degree rotation
  EXPECT_TRUE(OpenDocument("bug_713197.pdf"));
  FPDF_PAGE page = LoadPage(0);
  EXPECT_NE(nullptr, page);

  EXPECT_EQ(3, FPDFPage_GetRotation(page));
  UnloadPage(page);
}

TEST_F(FPDFEditEmbeddertest, AddTrueTypeFontText) {
  // Start with a blank page
  FPDF_PAGE page = FPDFPage_New(CreateNewDocument(), 0, 612, 792);
  {
    const CPDF_Font* stock_font = CPDF_Font::GetStockFont(cpdf_doc(), "Arial");
    const uint8_t* data = stock_font->GetFont()->GetFontData();
    const uint32_t size = stock_font->GetFont()->GetSize();
    ScopedFPDFFont font(
        FPDFText_LoadFont(document(), data, size, FPDF_FONT_TRUETYPE, 0));
    ASSERT_TRUE(font.get());

    // Add some text to the page
    FPDF_PAGEOBJECT text_object =
        FPDFPageObj_CreateTextObj(document(), font.get(), 12.0f);
    EXPECT_TRUE(text_object);
    std::unique_ptr<unsigned short, pdfium::FreeDeleter> text =
        GetFPDFWideString(L"I am testing my loaded font, WEE.");
    EXPECT_TRUE(FPDFText_SetText(text_object, text.get()));
    FPDFPageObj_Transform(text_object, 1, 0, 0, 1, 400, 400);
    FPDFPage_InsertObject(page, text_object);
    ScopedFPDFBitmap page_bitmap = RenderPageWithFlags(page, nullptr, 0);
#if _FX_PLATFORM_ == _FX_PLATFORM_APPLE_
    const char md5[] = "17d2b6cd574cf66170b09c8927529a94";
#else
    const char md5[] = "70592859010ffbf532a2237b8118bcc4";
#endif  // _FX_PLATFORM_ == _FX_PLATFORM_APPLE_
    CompareBitmap(page_bitmap.get(), 612, 792, md5);

    // Add some more text, same font
    FPDF_PAGEOBJECT text_object2 =
        FPDFPageObj_CreateTextObj(document(), font.get(), 15.0f);
    std::unique_ptr<unsigned short, pdfium::FreeDeleter> text2 =
        GetFPDFWideString(L"Bigger font size");
    EXPECT_TRUE(FPDFText_SetText(text_object2, text2.get()));
    FPDFPageObj_Transform(text_object2, 1, 0, 0, 1, 200, 200);
    FPDFPage_InsertObject(page, text_object2);
  }
  ScopedFPDFBitmap page_bitmap2 = RenderPageWithFlags(page, nullptr, 0);
#if _FX_PLATFORM_ == _FX_PLATFORM_APPLE_
  const char md5_2[] = "8eded4193ff1f0f77b8b600a825e97ea";
#else
  const char md5_2[] = "c1d10cce1761c4a998a16b2562030568";
#endif  // _FX_PLATFORM_ == _FX_PLATFORM_APPLE_
  CompareBitmap(page_bitmap2.get(), 612, 792, md5_2);

  EXPECT_TRUE(FPDFPage_GenerateContent(page));
  EXPECT_TRUE(FPDF_SaveAsCopy(document(), this, 0));
  FPDF_ClosePage(page);

  VerifySavedDocument(612, 792, md5_2);
}

TEST_F(FPDFEditEmbeddertest, TransformAnnot) {
  // Open a file with one annotation and load its first page.
  ASSERT_TRUE(OpenDocument("annotation_highlight_long_content.pdf"));
  FPDF_PAGE page = LoadPage(0);
  ASSERT_TRUE(page);

  {
    // Add an underline annotation to the page without specifying its rectangle.
    ScopedFPDFAnnotation annot(
        FPDFPage_CreateAnnot(page, FPDF_ANNOT_UNDERLINE));
    ASSERT_TRUE(annot);

    // FPDFPage_TransformAnnots() should run without errors when modifying
    // annotation rectangles.
    FPDFPage_TransformAnnots(page, 1, 2, 3, 4, 5, 6);
  }
  UnloadPage(page);
}

// TODO(npm): Add tests using Japanese fonts in other OS.
#if _FX_PLATFORM_ == _FX_PLATFORM_LINUX_
TEST_F(FPDFEditEmbeddertest, AddCIDFontText) {
  // Start with a blank page
  FPDF_PAGE page = FPDFPage_New(CreateNewDocument(), 0, 612, 792);
  CFX_Font CIDfont;
  {
    // First, get the data from the font
    CIDfont.LoadSubst("IPAGothic", 1, 0, 400, 0, 932, 0);
    EXPECT_EQ("IPAGothic", CIDfont.GetFaceName());
    const uint8_t* data = CIDfont.GetFontData();
    const uint32_t size = CIDfont.GetSize();

    // Load the data into a FPDF_Font.
    ScopedFPDFFont font(
        FPDFText_LoadFont(document(), data, size, FPDF_FONT_TRUETYPE, 1));
    ASSERT_TRUE(font.get());

    // Add some text to the page
    FPDF_PAGEOBJECT text_object =
        FPDFPageObj_CreateTextObj(document(), font.get(), 12.0f);
    ASSERT_TRUE(text_object);
    std::wstring wstr = L"ABCDEFGhijklmnop.";
    std::unique_ptr<unsigned short, pdfium::FreeDeleter> text =
        GetFPDFWideString(wstr);
    EXPECT_TRUE(FPDFText_SetText(text_object, text.get()));
    FPDFPageObj_Transform(text_object, 1, 0, 0, 1, 200, 200);
    FPDFPage_InsertObject(page, text_object);

    // And add some Japanese characters
    FPDF_PAGEOBJECT text_object2 =
        FPDFPageObj_CreateTextObj(document(), font.get(), 18.0f);
    ASSERT_TRUE(text_object2);
    std::wstring wstr2 =
        L"\u3053\u3093\u306B\u3061\u306f\u4e16\u754C\u3002\u3053\u3053\u306B1"
        L"\u756A";
    std::unique_ptr<unsigned short, pdfium::FreeDeleter> text2 =
        GetFPDFWideString(wstr2);
    EXPECT_TRUE(FPDFText_SetText(text_object2, text2.get()));
    FPDFPageObj_Transform(text_object2, 1, 0, 0, 1, 100, 500);
    FPDFPage_InsertObject(page, text_object2);
  }

  // Check that the text renders properly.
  const char md5[] = "c68cd79aa72bf83a7b25271370d46b21";
  {
    ScopedFPDFBitmap page_bitmap = RenderPageWithFlags(page, nullptr, 0);
    CompareBitmap(page_bitmap.get(), 612, 792, md5);
  }

  // Save the document, close the page.
  EXPECT_TRUE(FPDFPage_GenerateContent(page));
  EXPECT_TRUE(FPDF_SaveAsCopy(document(), this, 0));
  FPDF_ClosePage(page);

  VerifySavedDocument(612, 792, md5);
}
#endif  // _FX_PLATFORM_ == _FX_PLATFORM_LINUX_

TEST_F(FPDFEditEmbeddertest, SaveAndRender) {
  const char md5[] = "3c20472b0552c0c22b88ab1ed8c6202b";
  {
    EXPECT_TRUE(OpenDocument("bug_779.pdf"));
    FPDF_PAGE page = LoadPage(0);
    ASSERT_NE(nullptr, page);

    // Now add a more complex blue path.
    FPDF_PAGEOBJECT green_path = FPDFPageObj_CreateNewPath(20, 20);
    EXPECT_TRUE(FPDFPath_SetFillColor(green_path, 0, 255, 0, 200));
    // TODO(npm): stroking will cause the MD5s to differ.
    EXPECT_TRUE(FPDFPath_SetDrawMode(green_path, FPDF_FILLMODE_WINDING, 0));
    EXPECT_TRUE(FPDFPath_LineTo(green_path, 20, 63));
    EXPECT_TRUE(FPDFPath_BezierTo(green_path, 55, 55, 78, 78, 90, 90));
    EXPECT_TRUE(FPDFPath_LineTo(green_path, 133, 133));
    EXPECT_TRUE(FPDFPath_LineTo(green_path, 133, 33));
    EXPECT_TRUE(FPDFPath_BezierTo(green_path, 38, 33, 39, 36, 40, 40));
    EXPECT_TRUE(FPDFPath_Close(green_path));
    FPDFPage_InsertObject(page, green_path);
    ScopedFPDFBitmap page_bitmap = RenderLoadedPage(page);
    CompareBitmap(page_bitmap.get(), 612, 792, md5);

    // Now save the result, closing the page and document
    EXPECT_TRUE(FPDFPage_GenerateContent(page));
    EXPECT_TRUE(FPDF_SaveAsCopy(document(), this, 0));
    UnloadPage(page);
  }

  VerifySavedDocument(612, 792, md5);
}

TEST_F(FPDFEditEmbeddertest, ExtractImageBitmap) {
  ASSERT_TRUE(OpenDocument("embedded_images.pdf"));
  FPDF_PAGE page = LoadPage(0);
  ASSERT_TRUE(page);
  ASSERT_EQ(39, FPDFPage_CountObjects(page));

  FPDF_PAGEOBJECT obj = FPDFPage_GetObject(page, 32);
  EXPECT_NE(FPDF_PAGEOBJ_IMAGE, FPDFPageObj_GetType(obj));
  EXPECT_FALSE(FPDFImageObj_GetBitmap(obj));

  obj = FPDFPage_GetObject(page, 33);
  ASSERT_EQ(FPDF_PAGEOBJ_IMAGE, FPDFPageObj_GetType(obj));
  FPDF_BITMAP bitmap = FPDFImageObj_GetBitmap(obj);
  EXPECT_EQ(FPDFBitmap_BGR, FPDFBitmap_GetFormat(bitmap));
  CompareBitmap(bitmap, 109, 88, "d65e98d968d196abf13f78aec655ffae");
  FPDFBitmap_Destroy(bitmap);

  obj = FPDFPage_GetObject(page, 34);
  ASSERT_EQ(FPDF_PAGEOBJ_IMAGE, FPDFPageObj_GetType(obj));
  bitmap = FPDFImageObj_GetBitmap(obj);
  EXPECT_EQ(FPDFBitmap_BGR, FPDFBitmap_GetFormat(bitmap));
  CompareBitmap(bitmap, 103, 75, "1287711c84dbef767c435d11697661d6");
  FPDFBitmap_Destroy(bitmap);

  obj = FPDFPage_GetObject(page, 35);
  ASSERT_EQ(FPDF_PAGEOBJ_IMAGE, FPDFPageObj_GetType(obj));
  bitmap = FPDFImageObj_GetBitmap(obj);
  EXPECT_EQ(FPDFBitmap_Gray, FPDFBitmap_GetFormat(bitmap));
  CompareBitmap(bitmap, 92, 68, "9c6d76cb1e37ef8514f9455d759391f3");
  FPDFBitmap_Destroy(bitmap);

  obj = FPDFPage_GetObject(page, 36);
  ASSERT_EQ(FPDF_PAGEOBJ_IMAGE, FPDFPageObj_GetType(obj));
  bitmap = FPDFImageObj_GetBitmap(obj);
  EXPECT_EQ(FPDFBitmap_BGR, FPDFBitmap_GetFormat(bitmap));
  CompareBitmap(bitmap, 79, 60, "15cb6a49a2e354ed0e9f45dd34e3da1a");
  FPDFBitmap_Destroy(bitmap);

  obj = FPDFPage_GetObject(page, 37);
  ASSERT_EQ(FPDF_PAGEOBJ_IMAGE, FPDFPageObj_GetType(obj));
  bitmap = FPDFImageObj_GetBitmap(obj);
  EXPECT_EQ(FPDFBitmap_BGR, FPDFBitmap_GetFormat(bitmap));
  CompareBitmap(bitmap, 126, 106, "be5a64ba7890d2657522af6524118534");
  FPDFBitmap_Destroy(bitmap);

  obj = FPDFPage_GetObject(page, 38);
  ASSERT_EQ(FPDF_PAGEOBJ_IMAGE, FPDFPageObj_GetType(obj));
  bitmap = FPDFImageObj_GetBitmap(obj);
  EXPECT_EQ(FPDFBitmap_BGR, FPDFBitmap_GetFormat(bitmap));
  CompareBitmap(bitmap, 194, 119, "f9e24207ee1bc0db6c543d33a5f12ec5");
  FPDFBitmap_Destroy(bitmap);
  UnloadPage(page);
}

TEST_F(FPDFEditEmbeddertest, ExtractJBigImageBitmap) {
  ASSERT_TRUE(OpenDocument("bug_631912.pdf"));
  FPDF_PAGE page = LoadPage(0);
  ASSERT_TRUE(page);
  ASSERT_EQ(1, FPDFPage_CountObjects(page));

  FPDF_PAGEOBJECT obj = FPDFPage_GetObject(page, 0);
  ASSERT_EQ(FPDF_PAGEOBJ_IMAGE, FPDFPageObj_GetType(obj));
  {
    ScopedFPDFBitmap bitmap(FPDFImageObj_GetBitmap(obj));
    ASSERT_TRUE(bitmap);
    EXPECT_EQ(FPDFBitmap_Gray, FPDFBitmap_GetFormat(bitmap.get()));
    CompareBitmap(bitmap.get(), 1152, 720, "3f6a48e2b3e91b799bf34567f55cb4de");
  }

  UnloadPage(page);
}

TEST_F(FPDFEditEmbeddertest, GetImageData) {
  EXPECT_TRUE(OpenDocument("embedded_images.pdf"));
  FPDF_PAGE page = LoadPage(0);
  ASSERT_TRUE(page);
  ASSERT_EQ(39, FPDFPage_CountObjects(page));

  // Retrieve an image object with flate-encoded data stream.
  FPDF_PAGEOBJECT obj = FPDFPage_GetObject(page, 33);
  ASSERT_EQ(FPDF_PAGEOBJ_IMAGE, FPDFPageObj_GetType(obj));

  // Check that the raw image data has the correct length and hash value.
  unsigned long len = FPDFImageObj_GetImageDataRaw(obj, nullptr, 0);
  std::vector<char> buf(len);
  EXPECT_EQ(4091u, FPDFImageObj_GetImageDataRaw(obj, buf.data(), len));
  EXPECT_EQ("f73802327d2e88e890f653961bcda81a",
            GenerateMD5Base16(reinterpret_cast<uint8_t*>(buf.data()), len));

  // Check that the decoded image data has the correct length and hash value.
  len = FPDFImageObj_GetImageDataDecoded(obj, nullptr, 0);
  buf.clear();
  buf.resize(len);
  EXPECT_EQ(28776u, FPDFImageObj_GetImageDataDecoded(obj, buf.data(), len));
  EXPECT_EQ("cb3637934bb3b95a6e4ae1ea9eb9e56e",
            GenerateMD5Base16(reinterpret_cast<uint8_t*>(buf.data()), len));

  // Retrieve an image obejct with DCTDecode-encoded data stream.
  obj = FPDFPage_GetObject(page, 37);
  ASSERT_EQ(FPDF_PAGEOBJ_IMAGE, FPDFPageObj_GetType(obj));

  // Check that the raw image data has the correct length and hash value.
  len = FPDFImageObj_GetImageDataRaw(obj, nullptr, 0);
  buf.clear();
  buf.resize(len);
  EXPECT_EQ(4370u, FPDFImageObj_GetImageDataRaw(obj, buf.data(), len));
  EXPECT_EQ("6aae1f3710335023a9e12191be66b64b",
            GenerateMD5Base16(reinterpret_cast<uint8_t*>(buf.data()), len));

  // Check that the decoded image data has the correct length and hash value,
  // which should be the same as those of the raw data, since this image is
  // encoded by a single DCTDecode filter and decoding is a noop.
  len = FPDFImageObj_GetImageDataDecoded(obj, nullptr, 0);
  buf.clear();
  buf.resize(len);
  EXPECT_EQ(4370u, FPDFImageObj_GetImageDataDecoded(obj, buf.data(), len));
  EXPECT_EQ("6aae1f3710335023a9e12191be66b64b",
            GenerateMD5Base16(reinterpret_cast<uint8_t*>(buf.data()), len));

  UnloadPage(page);
}

TEST_F(FPDFEditEmbeddertest, DestroyPageObject) {
  FPDF_PAGEOBJECT rect = FPDFPageObj_CreateNewRect(10, 10, 20, 20);
  ASSERT_TRUE(rect);

  // There should be no memory leaks with a call to FPDFPageObj_Destroy().
  FPDFPageObj_Destroy(rect);
}

TEST_F(FPDFEditEmbeddertest, GetImageFilters) {
  EXPECT_TRUE(OpenDocument("embedded_images.pdf"));
  FPDF_PAGE page = LoadPage(0);
  ASSERT_TRUE(page);

  // Verify that retrieving the filter of a non-image object would fail.
  FPDF_PAGEOBJECT obj = FPDFPage_GetObject(page, 32);
  ASSERT_NE(FPDF_PAGEOBJ_IMAGE, FPDFPageObj_GetType(obj));
  ASSERT_EQ(0, FPDFImageObj_GetImageFilterCount(obj));
  EXPECT_EQ(0u, FPDFImageObj_GetImageFilter(obj, 0, nullptr, 0));

  // Verify the returned filter string for an image object with a single filter.
  obj = FPDFPage_GetObject(page, 33);
  ASSERT_EQ(FPDF_PAGEOBJ_IMAGE, FPDFPageObj_GetType(obj));
  ASSERT_EQ(1, FPDFImageObj_GetImageFilterCount(obj));
  unsigned long len = FPDFImageObj_GetImageFilter(obj, 0, nullptr, 0);
  std::vector<char> buf(len);
  static constexpr char kFlateDecode[] = "FlateDecode";
  EXPECT_EQ(sizeof(kFlateDecode),
            FPDFImageObj_GetImageFilter(obj, 0, buf.data(), len));
  EXPECT_STREQ(kFlateDecode, buf.data());
  EXPECT_EQ(0u, FPDFImageObj_GetImageFilter(obj, 1, nullptr, 0));

  // Verify all the filters for an image object with a list of filters.
  obj = FPDFPage_GetObject(page, 38);
  ASSERT_EQ(FPDF_PAGEOBJ_IMAGE, FPDFPageObj_GetType(obj));
  ASSERT_EQ(2, FPDFImageObj_GetImageFilterCount(obj));
  len = FPDFImageObj_GetImageFilter(obj, 0, nullptr, 0);
  buf.clear();
  buf.resize(len);
  static constexpr char kASCIIHexDecode[] = "ASCIIHexDecode";
  EXPECT_EQ(sizeof(kASCIIHexDecode),
            FPDFImageObj_GetImageFilter(obj, 0, buf.data(), len));
  EXPECT_STREQ(kASCIIHexDecode, buf.data());

  len = FPDFImageObj_GetImageFilter(obj, 1, nullptr, 0);
  buf.clear();
  buf.resize(len);
  static constexpr char kDCTDecode[] = "DCTDecode";
  EXPECT_EQ(sizeof(kDCTDecode),
            FPDFImageObj_GetImageFilter(obj, 1, buf.data(), len));
  EXPECT_STREQ(kDCTDecode, buf.data());

  UnloadPage(page);
}

TEST_F(FPDFEditEmbeddertest, GetImageMetadata) {
  ASSERT_TRUE(OpenDocument("embedded_images.pdf"));
  FPDF_PAGE page = LoadPage(0);
  ASSERT_TRUE(page);

  // Check that getting the metadata of a null object would fail.
  FPDF_IMAGEOBJ_METADATA metadata;
  EXPECT_FALSE(FPDFImageObj_GetImageMetadata(nullptr, page, &metadata));

  // Check that receiving the metadata with a null metadata object would fail.
  FPDF_PAGEOBJECT obj = FPDFPage_GetObject(page, 35);
  EXPECT_FALSE(FPDFImageObj_GetImageMetadata(obj, page, nullptr));

  // Check that when retrieving an image object's metadata without passing in
  // |page|, all values are correct, with the last two being default values.
  ASSERT_EQ(FPDF_PAGEOBJ_IMAGE, FPDFPageObj_GetType(obj));
  ASSERT_TRUE(FPDFImageObj_GetImageMetadata(obj, nullptr, &metadata));
  EXPECT_EQ(7, metadata.marked_content_id);
  EXPECT_EQ(92u, metadata.width);
  EXPECT_EQ(68u, metadata.height);
  EXPECT_NEAR(96.000000, metadata.horizontal_dpi, 0.001);
  EXPECT_NEAR(96.000000, metadata.vertical_dpi, 0.001);
  EXPECT_EQ(0u, metadata.bits_per_pixel);
  EXPECT_EQ(FPDF_COLORSPACE_UNKNOWN, metadata.colorspace);

  // Verify the metadata of a bitmap image with indexed colorspace.
  ASSERT_TRUE(FPDFImageObj_GetImageMetadata(obj, page, &metadata));
  EXPECT_EQ(7, metadata.marked_content_id);
  EXPECT_EQ(92u, metadata.width);
  EXPECT_EQ(68u, metadata.height);
  EXPECT_NEAR(96.000000, metadata.horizontal_dpi, 0.001);
  EXPECT_NEAR(96.000000, metadata.vertical_dpi, 0.001);
  EXPECT_EQ(1u, metadata.bits_per_pixel);
  EXPECT_EQ(FPDF_COLORSPACE_INDEXED, metadata.colorspace);

  // Verify the metadata of an image with RGB colorspace.
  obj = FPDFPage_GetObject(page, 37);
  ASSERT_EQ(FPDF_PAGEOBJ_IMAGE, FPDFPageObj_GetType(obj));
  ASSERT_TRUE(FPDFImageObj_GetImageMetadata(obj, page, &metadata));
  EXPECT_EQ(9, metadata.marked_content_id);
  EXPECT_EQ(126u, metadata.width);
  EXPECT_EQ(106u, metadata.height);
  EXPECT_NEAR(162.173752, metadata.horizontal_dpi, 0.001);
  EXPECT_NEAR(162.555878, metadata.vertical_dpi, 0.001);
  EXPECT_EQ(24u, metadata.bits_per_pixel);
  EXPECT_EQ(FPDF_COLORSPACE_DEVICERGB, metadata.colorspace);

  UnloadPage(page);
}
