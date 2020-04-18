// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_VIDEO_CAPTURE_TEST_MOCK_PRODUCER_H_
#define SERVICES_VIDEO_CAPTURE_TEST_MOCK_PRODUCER_H_

#include "media/mojo/interfaces/media_types.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/video_capture/public/mojom/producer.mojom.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace video_capture {

class MockProducer : public mojom::Producer {
 public:
  MockProducer(mojom::ProducerRequest request);
  ~MockProducer() override;

  // Use forwarding method to work around gmock not supporting move-only types.
  void OnNewBufferHandle(int32_t buffer_id,
                         mojo::ScopedSharedBufferHandle buffer_handle,
                         OnNewBufferHandleCallback callback) override;

  MOCK_METHOD3(DoOnNewBufferHandle,
               void(int32_t,
                    mojo::ScopedSharedBufferHandle*,
                    OnNewBufferHandleCallback& callback));
  MOCK_METHOD1(OnBufferRetired, void(int32_t));

 private:
  const mojo::Binding<mojom::Producer> binding_;
};

}  // namespace video_capture

#endif  // SERVICES_VIDEO_CAPTURE_TEST_MOCK_PRODUCER_H_
