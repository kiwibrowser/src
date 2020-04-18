// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_SERVICES_PRINTING_PDF_TO_PWG_RASTER_CONVERTER_H_
#define CHROME_SERVICES_PRINTING_PDF_TO_PWG_RASTER_CONVERTER_H_

#include <memory>

#include "base/macros.h"
#include "chrome/services/printing/public/mojom/pdf_to_pwg_raster_converter.mojom.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "services/service_manager/public/cpp/service_context_ref.h"

namespace printing {

struct PdfRenderSettings;

class PdfToPwgRasterConverter
    : public printing::mojom::PdfToPwgRasterConverter {
 public:
  explicit PdfToPwgRasterConverter(
      std::unique_ptr<service_manager::ServiceContextRef> service_ref);
  ~PdfToPwgRasterConverter() override;

 private:
  // printing::mojom::PdfToPwgRasterConverter
  void Convert(base::ReadOnlySharedMemoryRegion pdf_region,
               const PdfRenderSettings& pdf_settings,
               const PwgRasterSettings& pwg_raster_settings,
               ConvertCallback callback) override;

  const std::unique_ptr<service_manager::ServiceContextRef> service_ref_;

  DISALLOW_COPY_AND_ASSIGN(PdfToPwgRasterConverter);
};

}  // namespace printing

#endif  // CHROME_SERVICES_PRINTING_PDF_TO_PWG_RASTER_CONVERTER_H_
