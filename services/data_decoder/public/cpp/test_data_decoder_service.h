// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_DATA_DECODER_PUBLIC_CPP_TEST_DATA_DECODER_SERVICE_H_
#define SERVICES_DATA_DECODER_PUBLIC_CPP_TEST_DATA_DECODER_SERVICE_H_

#include <memory>

#include "base/macros.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/data_decoder/data_decoder_service.h"
#include "services/data_decoder/public/mojom/image_decoder.mojom.h"
#include "services/data_decoder/public/mojom/json_parser.mojom.h"
#include "services/service_manager/public/cpp/service.h"
#include "services/service_manager/public/cpp/test/test_connector_factory.h"

namespace service_manager {
class Connector;
}

namespace data_decoder {

// A class that can be used by tests that need access to a Connector that can
// bind the DataDecoder's interfaces. Bypasses the Service Manager entirely.
class TestDataDecoderService {
 public:
  TestDataDecoderService();
  ~TestDataDecoderService();

  // Returns a connector that can be used to bind DataDecoder's interfaces.
  // The returned connector is valid as long as |this| is valid.
  service_manager::Connector* connector() const { return connector_.get(); }

 private:
  std::unique_ptr<service_manager::TestConnectorFactory> connector_factory_;
  std::unique_ptr<service_manager::Connector> connector_;

  DISALLOW_COPY_AND_ASSIGN(TestDataDecoderService);
};

// An implementation of the DataDecoderService that closes the connection when
// a call is made on an interface.
// Can be used with a TestConnectorFactory to simulate crashes in the service
// while processing a call.
class CrashyDataDecoderService : public service_manager::Service,
                                 public mojom::ImageDecoder,
                                 public mojom::JsonParser {
 public:
  CrashyDataDecoderService(bool crash_json, bool crash_image);
  ~CrashyDataDecoderService() override;

  // service_manager::Service:
  void OnStart() override;
  void OnBindInterface(const service_manager::BindSourceInfo& source_info,
                       const std::string& interface_name,
                       mojo::ScopedMessagePipeHandle interface_pipe) override;

  // mojom::ImageDecoder implementation:
  void DecodeImage(const std::vector<uint8_t>& encoded_data,
                   mojom::ImageCodec codec,
                   bool shrink_to_fit,
                   int64_t max_size_in_bytes,
                   const gfx::Size& desired_image_frame_size,
                   DecodeImageCallback callback) override;
  void DecodeAnimation(const std::vector<uint8_t>& encoded_data,
                       bool shrink_to_fit,
                       int64_t max_size_in_bytes,
                       DecodeAnimationCallback callback) override;

  // mojom::JsonParser implementation.
  void Parse(const std::string& json, ParseCallback callback) override;

 private:
  std::unique_ptr<mojo::Binding<mojom::ImageDecoder>> image_decoder_binding_;
  std::unique_ptr<mojo::Binding<mojom::JsonParser>> json_parser_binding_;

  // An instance of the actual DataDecoderService we forward requests to for
  // interfaces that should not crash.
  std::unique_ptr<service_manager::Service> real_service_;

  bool crash_json_ = false;
  bool crash_image_ = false;

  DISALLOW_COPY_AND_ASSIGN(CrashyDataDecoderService);
};

}  // namespace data_decoder

#endif  // SERVICES_DATA_DECODER_PUBLIC_CPP_TEST_DATA_DECODER_SERVICE_H_
