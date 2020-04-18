// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/services/media_gallery_util/public/cpp/media_parser_provider.h"

#include "base/allocator/buildflags.h"
#include "base/bind.h"
#include "chrome/services/media_gallery_util/public/mojom/constants.mojom.h"
#include "media/media_buildflags.h"
#include "services/service_manager/public/cpp/connector.h"
#include "third_party/libyuv/include/libyuv.h"

#if BUILDFLAG(ENABLE_FFMPEG)
#include "third_party/ffmpeg/ffmpeg_features.h"  // nogncheck
extern "C" {
#include <libavutil/cpu.h>
}
#endif

MediaParserProvider::MediaParserProvider() = default;

MediaParserProvider::~MediaParserProvider() = default;

void MediaParserProvider::RetrieveMediaParser(
    service_manager::Connector* connector) {
  DCHECK(!media_parser_factory_ptr_);
  DCHECK(!media_parser_ptr_);

  connector->BindInterface(chrome::mojom::kMediaGalleryUtilServiceName,
                           mojo::MakeRequest(&media_parser_factory_ptr_));
  media_parser_factory_ptr_.set_connection_error_handler(base::BindOnce(
      &MediaParserProvider::OnConnectionError, base::Unretained(this)));

  int libyuv_cpu_flags = libyuv::InitCpuFlags();

#if BUILDFLAG(ENABLE_FFMPEG)
  int avutil_cpu_flags = av_get_cpu_flags();
#else
  int avutil_cpu_flags = -1;
#endif

  media_parser_factory_ptr_->CreateMediaParser(
      libyuv_cpu_flags, avutil_cpu_flags,
      base::BindOnce(&MediaParserProvider::OnMediaParserCreatedImpl,
                     base::Unretained(this)));
}

void MediaParserProvider::OnMediaParserCreatedImpl(
    chrome::mojom::MediaParserPtr media_parser_ptr) {
  media_parser_ptr_ = std::move(media_parser_ptr);
  media_parser_ptr_.set_connection_error_handler(base::BindOnce(
      &MediaParserProvider::OnConnectionError, base::Unretained(this)));
  media_parser_factory_ptr_.reset();

  OnMediaParserCreated();
}

void MediaParserProvider::ResetMediaParser() {
  media_parser_ptr_.reset();
  media_parser_factory_ptr_.reset();
}
