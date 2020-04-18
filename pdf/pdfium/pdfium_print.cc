// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "pdf/pdfium/pdfium_print.h"

#include <string>
#include <utility>

#include "base/strings/string_number_conversions.h"
#include "pdf/pdf_transform.h"
#include "pdf/pdfium/pdfium_engine.h"
#include "pdf/pdfium/pdfium_mem_buffer_file_read.h"
#include "pdf/pdfium/pdfium_mem_buffer_file_write.h"
#include "ppapi/c/dev/ppp_printing_dev.h"
#include "ppapi/c/private/ppp_pdf.h"
#include "printing/nup_parameters.h"
#include "printing/units.h"
#include "third_party/pdfium/public/cpp/fpdf_scopers.h"
#include "third_party/pdfium/public/fpdf_flatten.h"
#include "third_party/pdfium/public/fpdf_ppo.h"
#include "third_party/pdfium/public/fpdf_transformpage.h"
#include "ui/gfx/codec/jpeg_codec.h"
#include "ui/gfx/geometry/rect.h"

using printing::ConvertUnit;
using printing::ConvertUnitDouble;
using printing::kPointsPerInch;

namespace chrome_pdf {

namespace {

// UI should have done parameter sanity check, when execution
// reaches here, |num_pages_per_sheet| should be a positive integer.
bool ShouldDoNup(int num_pages_per_sheet) {
  return num_pages_per_sheet > 1;
}

// Check the source doc orientation.  Returns true if the doc is landscape.
// For now the orientation of the doc is determined by its first page's
// orientation.  Improvement can be added in the future to better determine the
// orientation of the source docs that have mixed orientation.
// TODO(xlou): rotate pages if the source doc has mixed orientation.  So that
// the orientation of all pages of the doc are uniform.  Pages of square size
// will not be rotated.
bool IsSourcePdfLandscape(FPDF_DOCUMENT doc) {
  DCHECK(doc);

  ScopedFPDFPage pdf_page(FPDF_LoadPage(doc, 0));
  DCHECK(pdf_page);

  bool is_source_landscape =
      FPDF_GetPageWidth(pdf_page.get()) > FPDF_GetPageHeight(pdf_page.get());
  return is_source_landscape;
}

// Set the destination page size and content area in points based on source
// page rotation and orientation.
//
// |rotated| True if source page is rotated 90 degree or 270 degree.
// |is_src_page_landscape| is true if the source page orientation is landscape.
// |page_size| has the actual destination page size in points.
// |content_rect| has the actual destination page printable area values in
// points.
void SetPageSizeAndContentRect(bool rotated,
                               bool is_src_page_landscape,
                               pp::Size* page_size,
                               pp::Rect* content_rect) {
  bool is_dst_page_landscape = page_size->width() > page_size->height();
  bool page_orientation_mismatched =
      is_src_page_landscape != is_dst_page_landscape;
  bool rotate_dst_page = rotated ^ page_orientation_mismatched;
  if (rotate_dst_page) {
    page_size->SetSize(page_size->height(), page_size->width());
    content_rect->SetRect(content_rect->y(), content_rect->x(),
                          content_rect->height(), content_rect->width());
  }
}

// Transform |page| contents to fit in the selected printer paper size.
void TransformPDFPageForPrinting(FPDF_PAGE page,
                                 double scale_factor,
                                 const PP_PrintSettings_Dev& print_settings) {
  // Get the source page width and height in points.
  const double src_page_width = FPDF_GetPageWidth(page);
  const double src_page_height = FPDF_GetPageHeight(page);

  const int src_page_rotation = FPDFPage_GetRotation(page);
  const bool fit_to_page = print_settings.print_scaling_option ==
                           PP_PRINTSCALINGOPTION_FIT_TO_PRINTABLE_AREA;

  pp::Size page_size(print_settings.paper_size);
  pp::Rect content_rect(print_settings.printable_area);
  const bool rotated = (src_page_rotation % 2 == 1);
  SetPageSizeAndContentRect(rotated, src_page_width > src_page_height,
                            &page_size, &content_rect);

  // Compute the screen page width and height in points.
  const int actual_page_width =
      rotated ? page_size.height() : page_size.width();
  const int actual_page_height =
      rotated ? page_size.width() : page_size.height();

  const gfx::Rect gfx_content_rect(content_rect.x(), content_rect.y(),
                                   content_rect.width(), content_rect.height());
  if (fit_to_page) {
    scale_factor = CalculateScaleFactor(gfx_content_rect, src_page_width,
                                        src_page_height, rotated);
  }

  // Calculate positions for the clip box.
  PdfRectangle media_box;
  PdfRectangle crop_box;
  bool has_media_box =
      !!FPDFPage_GetMediaBox(page, &media_box.left, &media_box.bottom,
                             &media_box.right, &media_box.top);
  bool has_crop_box = !!FPDFPage_GetCropBox(
      page, &crop_box.left, &crop_box.bottom, &crop_box.right, &crop_box.top);
  CalculateMediaBoxAndCropBox(rotated, has_media_box, has_crop_box, &media_box,
                              &crop_box);
  PdfRectangle source_clip_box = CalculateClipBoxBoundary(media_box, crop_box);
  ScalePdfRectangle(scale_factor, &source_clip_box);

  // Calculate the translation offset values.
  double offset_x = 0;
  double offset_y = 0;
  if (fit_to_page) {
    CalculateScaledClipBoxOffset(gfx_content_rect, source_clip_box, &offset_x,
                                 &offset_y);
  } else {
    CalculateNonScaledClipBoxOffset(gfx_content_rect, src_page_rotation,
                                    actual_page_width, actual_page_height,
                                    source_clip_box, &offset_x, &offset_y);
  }

  // Reset the media box and crop box. When the page has crop box and media box,
  // the plugin will display the crop box contents and not the entire media box.
  // If the pages have different crop box values, the plugin will display a
  // document of multiple page sizes. To give better user experience, we
  // decided to have same crop box and media box values. Hence, the user will
  // see a list of uniform pages.
  FPDFPage_SetMediaBox(page, 0, 0, page_size.width(), page_size.height());
  FPDFPage_SetCropBox(page, 0, 0, page_size.width(), page_size.height());

  // Transformation is not required, return. Do this check only after updating
  // the media box and crop box. For more detailed information, please refer to
  // the comment block right before FPDF_SetMediaBox and FPDF_GetMediaBox calls.
  if (scale_factor == 1.0 && offset_x == 0 && offset_y == 0)
    return;

  // All the positions have been calculated, now manipulate the PDF.
  FS_MATRIX matrix = {static_cast<float>(scale_factor),
                      0,
                      0,
                      static_cast<float>(scale_factor),
                      static_cast<float>(offset_x),
                      static_cast<float>(offset_y)};
  FS_RECTF cliprect = {static_cast<float>(source_clip_box.left + offset_x),
                       static_cast<float>(source_clip_box.top + offset_y),
                       static_cast<float>(source_clip_box.right + offset_x),
                       static_cast<float>(source_clip_box.bottom + offset_y)};
  FPDFPage_TransFormWithClip(page, &matrix, &cliprect);
  FPDFPage_TransformAnnots(page, scale_factor, 0, 0, scale_factor, offset_x,
                           offset_y);
}

void FitContentsToPrintableAreaIfRequired(
    FPDF_DOCUMENT doc,
    double scale_factor,
    const PP_PrintSettings_Dev& print_settings) {
  // Check to see if we need to fit pdf contents to printer paper size.
  if (print_settings.print_scaling_option == PP_PRINTSCALINGOPTION_SOURCE_SIZE)
    return;

  int num_pages = FPDF_GetPageCount(doc);
  // In-place transformation is more efficient than creating a new
  // transformed document from the source document. Therefore, transform
  // every page to fit the contents in the selected printer paper.
  for (int i = 0; i < num_pages; ++i) {
    ScopedFPDFPage page(FPDF_LoadPage(doc, i));
    TransformPDFPageForPrinting(page.get(), scale_factor, print_settings);
  }
}

int GetBlockForJpeg(void* param,
                    unsigned long pos,
                    unsigned char* buf,
                    unsigned long size) {
  std::vector<uint8_t>* data_vector = static_cast<std::vector<uint8_t>*>(param);
  if (pos + size < pos || pos + size > data_vector->size())
    return 0;
  memcpy(buf, data_vector->data() + pos, size);
  return 1;
}

std::string GetPageRangeStringFromRange(
    const PP_PrintPageNumberRange_Dev* page_ranges,
    uint32_t page_range_count) {
  DCHECK(page_range_count);

  std::string page_number_str;
  for (uint32_t i = 0; i < page_range_count; ++i) {
    if (!page_number_str.empty())
      page_number_str.push_back(',');
    const PP_PrintPageNumberRange_Dev& range = page_ranges[i];
    page_number_str.append(base::UintToString(range.first_page_number + 1));
    if (range.first_page_number != range.last_page_number) {
      page_number_str.push_back('-');
      page_number_str.append(base::UintToString(range.last_page_number + 1));
    }
  }
  return page_number_str;
}

}  // namespace

PDFiumPrint::PDFiumPrint(PDFiumEngine* engine) : engine_(engine) {}

PDFiumPrint::~PDFiumPrint() = default;

// static
std::vector<uint32_t> PDFiumPrint::GetPageNumbersFromPrintPageNumberRange(
    const PP_PrintPageNumberRange_Dev* page_ranges,
    uint32_t page_range_count) {
  DCHECK(page_range_count);

  std::vector<uint32_t> page_numbers;
  for (uint32_t i = 0; i < page_range_count; ++i) {
    for (uint32_t page_number = page_ranges[i].first_page_number;
         page_number <= page_ranges[i].last_page_number; ++page_number) {
      page_numbers.push_back(page_number);
    }
  }
  return page_numbers;
}

pp::Buffer_Dev PDFiumPrint::PrintPagesAsRasterPDF(
    const PP_PrintPageNumberRange_Dev* page_ranges,
    uint32_t page_range_count,
    const PP_PrintSettings_Dev& print_settings,
    const PP_PdfPrintSettings_Dev& pdf_print_settings) {
  std::vector<PDFiumPage> pages_to_print;
  // width and height of source PDF pages.
  std::vector<std::pair<double, double>> source_page_sizes;
  // Collect pages to print and sizes of source pages.
  std::vector<uint32_t> page_numbers =
      PDFiumPrint::GetPageNumbersFromPrintPageNumberRange(page_ranges,
                                                          page_range_count);
  for (uint32_t page_number : page_numbers) {
    ScopedFPDFPage pdf_page(FPDF_LoadPage(engine_->doc(), page_number));
    double source_page_width = FPDF_GetPageWidth(pdf_page.get());
    double source_page_height = FPDF_GetPageHeight(pdf_page.get());
    source_page_sizes.push_back(
        std::make_pair(source_page_width, source_page_height));
    // For computing size in pixels, use a square dpi since the source PDF page
    // has square DPI.
    int width_in_pixels =
        ConvertUnit(source_page_width, kPointsPerInch, print_settings.dpi);
    int height_in_pixels =
        ConvertUnit(source_page_height, kPointsPerInch, print_settings.dpi);

    pp::Rect rect(width_in_pixels, height_in_pixels);
    pages_to_print.push_back(PDFiumPage(engine_, page_number, rect, true));
  }

  ScopedFPDFDocument output_doc(FPDF_CreateNewDocument());
  DCHECK(output_doc);

  size_t i = 0;
  for (; i < pages_to_print.size(); ++i) {
    double source_page_width = source_page_sizes[i].first;
    double source_page_height = source_page_sizes[i].second;

    // Use |temp_doc| to compress image by saving PDF to |buffer|.
    pp::Buffer_Dev buffer;
    {
      ScopedFPDFDocument temp_doc(
          CreateSinglePageRasterPdf(source_page_width, source_page_height,
                                    print_settings, &pages_to_print[i]));

      if (!temp_doc)
        break;

      buffer = GetFlattenedPrintData(temp_doc.get());
    }

    PDFiumMemBufferFileRead file_read(buffer.data(), buffer.size());
    ScopedFPDFDocument temp_doc(FPDF_LoadCustomDocument(&file_read, nullptr));
    if (!FPDF_ImportPages(output_doc.get(), temp_doc.get(), "1", i))
      break;
  }

  pp::Buffer_Dev buffer;
  if (i == pages_to_print.size()) {
    FPDF_CopyViewerPreferences(output_doc.get(), engine_->doc());
    uint32_t num_pages_per_sheet = pdf_print_settings.num_pages_per_sheet;
    uint32_t scale_factor = pdf_print_settings.scale_factor;
    if (ShouldDoNup(num_pages_per_sheet)) {
      buffer =
          NupPdfToPdf(output_doc.get(), num_pages_per_sheet, print_settings);
    } else {
      FitContentsToPrintableAreaIfRequired(
          output_doc.get(), scale_factor / 100.0f, print_settings);
      buffer = GetPrintData(output_doc.get());
    }
  }

  return buffer;
}

pp::Buffer_Dev PDFiumPrint::PrintPagesAsPDF(
    const PP_PrintPageNumberRange_Dev* page_ranges,
    uint32_t page_range_count,
    const PP_PrintSettings_Dev& print_settings,
    const PP_PdfPrintSettings_Dev& pdf_print_settings) {
  ScopedFPDFDocument output_doc(FPDF_CreateNewDocument());
  DCHECK(output_doc);
  FPDF_CopyViewerPreferences(output_doc.get(), engine_->doc());

  std::string page_number_str =
      GetPageRangeStringFromRange(page_ranges, page_range_count);
  if (!FPDF_ImportPages(output_doc.get(), engine_->doc(),
                        page_number_str.c_str(), 0)) {
    return pp::Buffer_Dev();
  }

  // Now flatten all the output pages.
  if (!FlattenPrintData(output_doc.get()))
    return pp::Buffer_Dev();

  pp::Buffer_Dev buffer;
  uint32_t num_pages_per_sheet = pdf_print_settings.num_pages_per_sheet;
  uint32_t scale_factor = pdf_print_settings.scale_factor;
  if (ShouldDoNup(num_pages_per_sheet)) {
    buffer = NupPdfToPdf(output_doc.get(), num_pages_per_sheet, print_settings);
  } else {
    FitContentsToPrintableAreaIfRequired(output_doc.get(),
                                         scale_factor / 100.0f, print_settings);
    buffer = GetPrintData(output_doc.get());
  }

  return buffer;
}

FPDF_DOCUMENT PDFiumPrint::CreateSinglePageRasterPdf(
    double source_page_width,
    double source_page_height,
    const PP_PrintSettings_Dev& print_settings,
    PDFiumPage* page_to_print) {
  FPDF_DOCUMENT temp_doc = FPDF_CreateNewDocument();
  DCHECK(temp_doc);

  const pp::Size& bitmap_size(page_to_print->rect().size());

  pp::ImageData image =
      pp::ImageData(engine_->GetPluginInstance(),
                    PP_IMAGEDATAFORMAT_BGRA_PREMUL, bitmap_size, false);

  ScopedFPDFBitmap bitmap(
      FPDFBitmap_CreateEx(bitmap_size.width(), bitmap_size.height(),
                          FPDFBitmap_BGRx, image.data(), image.stride()));

  // Clear the bitmap
  FPDFBitmap_FillRect(bitmap.get(), 0, 0, bitmap_size.width(),
                      bitmap_size.height(), 0xFFFFFFFF);

  pp::Rect page_rect = page_to_print->rect();
  FPDF_RenderPageBitmap(bitmap.get(), page_to_print->GetPrintPage(),
                        page_rect.x(), page_rect.y(), page_rect.width(),
                        page_rect.height(), print_settings.orientation,
                        FPDF_PRINTING | FPDF_NO_CATCH);

  // Draw the forms.
  FPDF_FFLDraw(engine_->form(), bitmap.get(), page_to_print->GetPrintPage(),
               page_rect.x(), page_rect.y(), page_rect.width(),
               page_rect.height(), print_settings.orientation,
               FPDF_ANNOT | FPDF_PRINTING | FPDF_NO_CATCH);

  unsigned char* bitmap_data =
      static_cast<unsigned char*>(FPDFBitmap_GetBuffer(bitmap.get()));
  double ratio_x = ConvertUnitDouble(bitmap_size.width(), print_settings.dpi,
                                     kPointsPerInch);
  double ratio_y = ConvertUnitDouble(bitmap_size.height(), print_settings.dpi,
                                     kPointsPerInch);

  // Add the bitmap to an image object and add the image object to the output
  // page.
  FPDF_PAGEOBJECT temp_img = FPDFPageObj_NewImageObj(temp_doc);

  bool encoded = false;
  std::vector<uint8_t> compressed_bitmap_data;
  if (!(print_settings.format & PP_PRINTOUTPUTFORMAT_PDF)) {
    // Use quality = 40 as this does not significantly degrade the printed
    // document relative to a normal bitmap and provides better compression than
    // a higher quality setting.
    const int kQuality = 40;
    SkImageInfo info = SkImageInfo::Make(
        FPDFBitmap_GetWidth(bitmap.get()), FPDFBitmap_GetHeight(bitmap.get()),
        kBGRA_8888_SkColorType, kOpaque_SkAlphaType);
    SkPixmap src(info, bitmap_data, FPDFBitmap_GetStride(bitmap.get()));
    encoded = gfx::JPEGCodec::Encode(src, kQuality, &compressed_bitmap_data);
  }

  {
    ScopedFPDFPage temp_page_holder(
        FPDFPage_New(temp_doc, 0, source_page_width, source_page_height));
    FPDF_PAGE temp_page = temp_page_holder.get();
    if (encoded) {
      FPDF_FILEACCESS file_access = {};
      file_access.m_FileLen =
          static_cast<unsigned long>(compressed_bitmap_data.size());
      file_access.m_GetBlock = &GetBlockForJpeg;
      file_access.m_Param = &compressed_bitmap_data;

      FPDFImageObj_LoadJpegFileInline(&temp_page, 1, temp_img, &file_access);
    } else {
      FPDFImageObj_SetBitmap(&temp_page, 1, temp_img, bitmap.get());
    }

    FPDFImageObj_SetMatrix(temp_img, ratio_x, 0, 0, ratio_y, 0, 0);
    FPDFPage_InsertObject(temp_page, temp_img);
    FPDFPage_GenerateContent(temp_page);
  }

  page_to_print->ClosePrintPage();
  return temp_doc;
}

pp::Buffer_Dev PDFiumPrint::NupPdfToPdf(
    FPDF_DOCUMENT doc,
    uint32_t num_pages_per_sheet,
    const PP_PrintSettings_Dev& print_settings) {
  DCHECK(doc);
  DCHECK(ShouldDoNup(num_pages_per_sheet));

  PP_Size page_size = print_settings.paper_size;

  printing::NupParameters nup_params;
  bool is_landscape = IsSourcePdfLandscape(doc);
  nup_params.SetParameters(num_pages_per_sheet, is_landscape);

  // Import n pages to one.
  bool paper_is_landscape = page_size.width > page_size.height;
  if (nup_params.landscape() != paper_is_landscape)
    std::swap(page_size.width, page_size.height);

  ScopedFPDFDocument output_doc_nup(FPDF_ImportNPagesToOne(
      doc, page_size.width, page_size.height, nup_params.num_pages_on_x_axis(),
      nup_params.num_pages_on_y_axis()));
  if (!output_doc_nup)
    return pp::Buffer_Dev();

  FitContentsToPrintableAreaIfRequired(output_doc_nup.get(), 1.0f,
                                       print_settings);
  return GetPrintData(output_doc_nup.get());
}

bool PDFiumPrint::FlattenPrintData(FPDF_DOCUMENT doc) {
  DCHECK(doc);

  ScopedSubstFont scoped_subst_font(engine_);
  int page_count = FPDF_GetPageCount(doc);
  for (int i = 0; i < page_count; ++i) {
    ScopedFPDFPage page(FPDF_LoadPage(doc, i));
    DCHECK(page);
    if (FPDFPage_Flatten(page.get(), FLAT_PRINT) == FLATTEN_FAIL)
      return false;
  }
  return true;
}

pp::Buffer_Dev PDFiumPrint::GetPrintData(FPDF_DOCUMENT doc) {
  DCHECK(doc);

  pp::Buffer_Dev buffer;
  PDFiumMemBufferFileWrite output_file_write;
  if (FPDF_SaveAsCopy(doc, &output_file_write, 0)) {
    size_t size = output_file_write.size();
    buffer = pp::Buffer_Dev(engine_->GetPluginInstance(), size);
    if (!buffer.is_null())
      memcpy(buffer.data(), output_file_write.buffer().c_str(), size);
  }
  return buffer;
}

pp::Buffer_Dev PDFiumPrint::GetFlattenedPrintData(FPDF_DOCUMENT doc) {
  DCHECK(doc);

  pp::Buffer_Dev buffer;
  if (FlattenPrintData(doc))
    buffer = GetPrintData(doc);
  return buffer;
}

}  // namespace chrome_pdf
