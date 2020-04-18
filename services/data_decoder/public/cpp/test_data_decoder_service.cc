// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/data_decoder/public/cpp/test_data_decoder_service.h"

#include "services/data_decoder/data_decoder_service.h"

namespace data_decoder {

TestDataDecoderService::TestDataDecoderService()
    : connector_factory_(
          service_manager::TestConnectorFactory::CreateForUniqueService(
              std::make_unique<DataDecoderService>())),
      connector_(connector_factory_->CreateConnector()) {}

TestDataDecoderService::~TestDataDecoderService() = default;

CrashyDataDecoderService::CrashyDataDecoderService(bool crash_json,
                                                   bool crash_image)
    : real_service_(DataDecoderService::Create()),
      crash_json_(crash_json),
      crash_image_(crash_image) {}

CrashyDataDecoderService::~CrashyDataDecoderService() = default;

// service_manager::Service:
void CrashyDataDecoderService::OnStart() {
  real_service_->OnStart();
}

void CrashyDataDecoderService::OnBindInterface(
    const service_manager::BindSourceInfo& source_info,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle interface_pipe) {
  DCHECK(interface_name == mojom::JsonParser::Name_ ||
         interface_name == mojom::ImageDecoder::Name_);
  if (interface_name == mojom::JsonParser::Name_ && crash_json_) {
    DCHECK(!json_parser_binding_);
    json_parser_binding_ = std::make_unique<mojo::Binding<mojom::JsonParser>>(
        this, mojom::JsonParserRequest(std::move(interface_pipe)));
    return;
  }
  if (interface_name == mojom::ImageDecoder::Name_ && crash_image_) {
    DCHECK(!image_decoder_binding_);
    image_decoder_binding_ =
        std::make_unique<mojo::Binding<mojom::ImageDecoder>>(
            this, mojom::ImageDecoderRequest(std::move(interface_pipe)));
    return;
  }
  real_service_->OnBindInterface(source_info, interface_name,
                                 std::move(interface_pipe));
}

// Overridden from mojom::ImageDecoder:
void CrashyDataDecoderService::DecodeImage(
    const std::vector<uint8_t>& encoded_data,
    mojom::ImageCodec codec,
    bool shrink_to_fit,
    int64_t max_size_in_bytes,
    const gfx::Size& desired_image_frame_size,
    DecodeImageCallback callback) {
  image_decoder_binding_.reset();
}

void CrashyDataDecoderService::DecodeAnimation(
    const std::vector<uint8_t>& encoded_data,
    bool shrink_to_fit,
    int64_t max_size_in_bytes,
    DecodeAnimationCallback callback) {
  image_decoder_binding_.reset();
}

void CrashyDataDecoderService::Parse(const std::string& json,
                                     ParseCallback callback) {
  json_parser_binding_.reset();
}

}  // namespace data_decoder