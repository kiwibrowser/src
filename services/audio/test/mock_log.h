// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_AUDIO_TEST_MOCK_LOG_H_
#define SERVICES_AUDIO_TEST_MOCK_LOG_H_

#include <string>

#include "base/bind.h"
#include "media/base/audio_parameters.h"
#include "media/mojo/interfaces/audio_logging.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace audio {

class MockLog : public media::mojom::AudioLog {
 public:
  MockLog();
  ~MockLog() override;

  // Should only be called once.
  media::mojom::AudioLogPtr MakePtr() {
    media::mojom::AudioLogPtr ptr;
    binding_.Bind(mojo::MakeRequest(&ptr));
    binding_.set_connection_error_handler(base::BindOnce(
        &MockLog::BindingConnectionError, base::Unretained(this)));
    return ptr;
  }

  void CloseBinding() { binding_.Close(); }

  MOCK_METHOD2(OnCreated,
               void(const media::AudioParameters& params,
                    const std::string& device_id));
  MOCK_METHOD0(OnStarted, void());
  MOCK_METHOD0(OnStopped, void());
  MOCK_METHOD0(OnClosed, void());
  MOCK_METHOD0(OnError, void());
  MOCK_METHOD1(OnSetVolume, void(double));
  MOCK_METHOD1(OnLogMessage, void(const std::string&));

  MOCK_METHOD0(BindingConnectionError, void());

 private:
  mojo::Binding<media::mojom::AudioLog> binding_;

  DISALLOW_COPY_AND_ASSIGN(MockLog);
};

}  // namespace audio

#endif  // SERVICES_AUDIO_TEST_MOCK_LOG_H_
