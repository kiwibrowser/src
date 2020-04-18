// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/services/printing/pdf_to_emf_converter_factory.h"

#include <utility>

#include "chrome/services/printing/pdf_to_emf_converter.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "mojo/public/cpp/system/platform_handle.h"

namespace printing {

PdfToEmfConverterFactory::PdfToEmfConverterFactory(
    std::unique_ptr<service_manager::ServiceContextRef> service_ref)
    : service_ref_(std::move(service_ref)) {}

PdfToEmfConverterFactory::PdfToEmfConverterFactory() = default;

PdfToEmfConverterFactory::~PdfToEmfConverterFactory() = default;

void PdfToEmfConverterFactory::CreateConverter(
    base::ReadOnlySharedMemoryRegion pdf_region,
    const PdfRenderSettings& render_settings,
    mojom::PdfToEmfConverterClientPtr client,
    CreateConverterCallback callback) {
  auto converter = std::make_unique<PdfToEmfConverter>(
      std::move(pdf_region), render_settings, std::move(client));
  uint32_t page_count = converter->total_page_count();
  mojom::PdfToEmfConverterPtr converter_ptr;
  mojo::MakeStrongBinding(std::move(converter),
                          mojo::MakeRequest(&converter_ptr));

  std::move(callback).Run(std::move(converter_ptr), page_count);
}

// static
void PdfToEmfConverterFactory::Create(
    mojom::PdfToEmfConverterFactoryRequest request) {
  mojo::MakeStrongBinding(std::make_unique<PdfToEmfConverterFactory>(),
                          std::move(request));
}
}  // namespace printing
