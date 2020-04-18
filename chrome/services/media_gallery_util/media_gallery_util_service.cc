// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/services/media_gallery_util/media_gallery_util_service.h"

#include "build/build_config.h"
#include "chrome/services/media_gallery_util/media_parser_factory.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace {

void OnMediaParserFactoryRequest(
    service_manager::ServiceContextRefFactory* ref_factory,
    chrome::mojom::MediaParserFactoryRequest request) {
  mojo::MakeStrongBinding(
      std::make_unique<MediaParserFactory>(ref_factory->CreateRef()),
      std::move(request));
}

}  // namespace

MediaGalleryUtilService::MediaGalleryUtilService() = default;

MediaGalleryUtilService::~MediaGalleryUtilService() = default;

std::unique_ptr<service_manager::Service>
MediaGalleryUtilService::CreateService() {
  return std::make_unique<MediaGalleryUtilService>();
}

void MediaGalleryUtilService::OnStart() {
  ref_factory_ = std::make_unique<service_manager::ServiceContextRefFactory>(
      context()->CreateQuitClosure());
  registry_.AddInterface(
      base::Bind(&OnMediaParserFactoryRequest, ref_factory_.get()));
}

void MediaGalleryUtilService::OnBindInterface(
    const service_manager::BindSourceInfo& source_info,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle interface_pipe) {
  registry_.BindInterface(interface_name, std::move(interface_pipe));
}
