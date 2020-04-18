// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/services/media_gallery_util/media_parser_factory.h"

#include "base/allocator/buildflags.h"
#include "chrome/services/media_gallery_util/media_parser.h"
#include "media/base/media.h"
#include "media/media_buildflags.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

MediaParserFactory::MediaParserFactory(
    std::unique_ptr<service_manager::ServiceContextRef> service_ref)
    : service_ref_(std::move(service_ref)) {}

MediaParserFactory::~MediaParserFactory() = default;

void MediaParserFactory::CreateMediaParser(int64_t libyuv_cpu_flags,
                                           int64_t libavutil_cpu_flags,
                                           CreateMediaParserCallback callback) {
  media::InitializeMediaLibraryInSandbox(libyuv_cpu_flags, libavutil_cpu_flags);

  chrome::mojom::MediaParserPtr media_parser_ptr;
  mojo::MakeStrongBinding(std::make_unique<MediaParser>(service_ref_->Clone()),
                          mojo::MakeRequest(&media_parser_ptr));

  std::move(callback).Run(std::move(media_parser_ptr));
}
