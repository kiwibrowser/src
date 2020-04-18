// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/services/printing/pdf_to_pwg_raster_converter.h"

#include <limits>
#include <string>
#include <utility>

#include "components/pwg_encoder/bitmap_image.h"
#include "components/pwg_encoder/pwg_encoder.h"
#include "components/services/pdf_compositor/public/cpp/pdf_service_mojo_utils.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "pdf/pdf.h"
#include "printing/pdf_render_settings.h"

namespace printing {

namespace {

base::ReadOnlySharedMemoryRegion RenderPdfPagesToPwgRaster(
    base::ReadOnlySharedMemoryRegion pdf_region,
    const PdfRenderSettings& settings,
    const PwgRasterSettings& bitmap_settings,
    uint32_t* page_count) {
  base::ReadOnlySharedMemoryRegion invalid_pwg_region;
  base::ReadOnlySharedMemoryMapping pdf_mapping = pdf_region.Map();
  if (!pdf_mapping.IsValid())
    return invalid_pwg_region;

  // Get the page count and reserve 64 KB per page in |pwg_data| below.
  static constexpr size_t kEstimatedSizePerPage = 64 * 1024;
  static constexpr size_t kMaxPageCount =
      std::numeric_limits<size_t>::max() / kEstimatedSizePerPage;
  int total_page_count = 0;
  if (!chrome_pdf::GetPDFDocInfo(pdf_mapping.memory(), pdf_mapping.size(),
                                 &total_page_count, nullptr) ||
      total_page_count <= 0 ||
      static_cast<size_t>(total_page_count) >= kMaxPageCount) {
    return invalid_pwg_region;
  }

  std::string pwg_data;
  pwg_data.reserve(total_page_count * kEstimatedSizePerPage);
  pwg_data = pwg_encoder::PwgEncoder::GetDocumentHeader();
  pwg_encoder::BitmapImage image(settings.area.size(),
                                 pwg_encoder::BitmapImage::BGRA);
  for (int i = 0; i < total_page_count; ++i) {
    int page_number = i;

    if (bitmap_settings.reverse_page_order)
      page_number = total_page_count - 1 - page_number;

    if (!chrome_pdf::RenderPDFPageToBitmap(
            pdf_mapping.memory(), pdf_mapping.size(), page_number,
            image.pixel_data(), image.size().width(), image.size().height(),
            settings.dpi.width(), settings.dpi.height(), settings.autorotate,
            settings.use_color)) {
      return invalid_pwg_region;
    }

    pwg_encoder::PwgHeaderInfo header_info;
    header_info.dpi = settings.dpi;
    header_info.total_pages = total_page_count;
    header_info.color_space = bitmap_settings.use_color
                                  ? pwg_encoder::PwgHeaderInfo::SRGB
                                  : pwg_encoder::PwgHeaderInfo::SGRAY;

    // Transform odd pages.
    if (page_number % 2) {
      switch (bitmap_settings.odd_page_transform) {
        case TRANSFORM_NORMAL:
          break;
        case TRANSFORM_ROTATE_180:
          header_info.flipx = true;
          header_info.flipy = true;
          break;
        case TRANSFORM_FLIP_HORIZONTAL:
          header_info.flipx = true;
          break;
        case TRANSFORM_FLIP_VERTICAL:
          header_info.flipy = true;
          break;
      }
    }

    if (bitmap_settings.rotate_all_pages) {
      header_info.flipx = !header_info.flipx;
      header_info.flipy = !header_info.flipy;
    }

    std::string pwg_page =
        pwg_encoder::PwgEncoder::EncodePage(image, header_info);
    if (pwg_page.empty())
      return invalid_pwg_region;
    pwg_data += pwg_page;
  }

  base::MappedReadOnlyRegion region_mapping =
      CreateReadOnlySharedMemoryRegion(pwg_data.size());
  if (!region_mapping.IsValid())
    return invalid_pwg_region;

  *page_count = total_page_count;
  memcpy(region_mapping.mapping.memory(), pwg_data.data(), pwg_data.size());
  return std::move(region_mapping.region);
}

}  // namespace

PdfToPwgRasterConverter::PdfToPwgRasterConverter(
    std::unique_ptr<service_manager::ServiceContextRef> service_ref)
    : service_ref_(std::move(service_ref)) {}

PdfToPwgRasterConverter::~PdfToPwgRasterConverter() {}

void PdfToPwgRasterConverter::Convert(
    base::ReadOnlySharedMemoryRegion pdf_region,
    const PdfRenderSettings& pdf_settings,
    const PwgRasterSettings& pwg_raster_settings,
    ConvertCallback callback) {
  uint32_t page_count = 0;
  base::ReadOnlySharedMemoryRegion region = RenderPdfPagesToPwgRaster(
      std::move(pdf_region), pdf_settings, pwg_raster_settings, &page_count);
  std::move(callback).Run(std::move(region), page_count);
}

}  // namespace printing
