// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/services/printing/printing_service.h"

#include "build/build_config.h"
#include "chrome/services/printing/pdf_to_pwg_raster_converter.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

#if defined(OS_WIN)
#include "chrome/services/printing/pdf_to_emf_converter.h"
#include "chrome/services/printing/pdf_to_emf_converter_factory.h"
#endif

namespace printing {

namespace {

#if defined(OS_WIN)
void OnPdfToEmfConverterFactoryRequest(
    service_manager::ServiceContextRefFactory* ref_factory,
    printing::mojom::PdfToEmfConverterFactoryRequest request) {
  mojo::MakeStrongBinding(std::make_unique<printing::PdfToEmfConverterFactory>(
                              ref_factory->CreateRef()),
                          std::move(request));
}
#endif

void OnPdfToPwgRasterConverterRequest(
    service_manager::ServiceContextRefFactory* ref_factory,
    printing::mojom::PdfToPwgRasterConverterRequest request) {
  mojo::MakeStrongBinding(std::make_unique<printing::PdfToPwgRasterConverter>(
                              ref_factory->CreateRef()),
                          std::move(request));
}

}  // namespace

PrintingService::PrintingService() = default;

PrintingService::~PrintingService() = default;

std::unique_ptr<service_manager::Service> PrintingService::CreateService() {
  return std::make_unique<PrintingService>();
}

void PrintingService::OnStart() {
  ref_factory_ = std::make_unique<service_manager::ServiceContextRefFactory>(
      context()->CreateQuitClosure());
#if defined(OS_WIN)
  registry_.AddInterface(
      base::Bind(&OnPdfToEmfConverterFactoryRequest, ref_factory_.get()));
#endif
  registry_.AddInterface(
      base::Bind(&OnPdfToPwgRasterConverterRequest, ref_factory_.get()));
}

void PrintingService::OnBindInterface(
    const service_manager::BindSourceInfo& source_info,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle interface_pipe) {
  registry_.BindInterface(interface_name, std::move(interface_pipe));
}

}  // namespace printing
