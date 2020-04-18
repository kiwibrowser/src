// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/services/file_util/file_util_service.h"

#include <memory>

#include "build/build_config.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

#if defined(FULL_SAFE_BROWSING)
#include "chrome/services/file_util/safe_archive_analyzer.h"
#endif

#if defined(OS_CHROMEOS)
#include "chrome/services/file_util/zip_file_creator.h"
#endif

namespace {

#if defined(FULL_SAFE_BROWSING)
void OnSafeArchiveAnalyzerRequest(
    service_manager::ServiceContextRefFactory* ref_factory,
    chrome::mojom::SafeArchiveAnalyzerRequest request) {
  mojo::MakeStrongBinding(
      std::make_unique<SafeArchiveAnalyzer>(ref_factory->CreateRef()),
      std::move(request));
}
#endif

#if defined(OS_CHROMEOS)
void OnZipFileCreatorRequest(
    service_manager::ServiceContextRefFactory* ref_factory,
    chrome::mojom::ZipFileCreatorRequest request) {
  mojo::MakeStrongBinding(
      std::make_unique<chrome::ZipFileCreator>(ref_factory->CreateRef()),
      std::move(request));
}
#endif

}  // namespace

FileUtilService::FileUtilService() = default;

FileUtilService::~FileUtilService() = default;

std::unique_ptr<service_manager::Service> FileUtilService::CreateService() {
  return std::make_unique<FileUtilService>();
}

void FileUtilService::OnStart() {
  ref_factory_ = std::make_unique<service_manager::ServiceContextRefFactory>(
      context()->CreateQuitClosure());
#if defined(OS_CHROMEOS)
  registry_.AddInterface(
      base::Bind(&OnZipFileCreatorRequest, ref_factory_.get()));
#endif
#if defined(FULL_SAFE_BROWSING)
  registry_.AddInterface(
      base::Bind(&OnSafeArchiveAnalyzerRequest, ref_factory_.get()));
#endif
}

void FileUtilService::OnBindInterface(
    const service_manager::BindSourceInfo& source_info,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle interface_pipe) {
  registry_.BindInterface(interface_name, std::move(interface_pipe));
}
