// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MEDIA_WEBRTC_MOCK_DATA_CHANNEL_IMPL_H_
#define CONTENT_RENDERER_MEDIA_WEBRTC_MOCK_DATA_CHANNEL_IMPL_H_

#include <stdint.h>

#include <string>

#include "base/macros.h"
#include "third_party/webrtc/api/peerconnectioninterface.h"

namespace content {

class MockDataChannel : public webrtc::DataChannelInterface {
 public:
  MockDataChannel(const std::string& label,
                  const webrtc::DataChannelInit* config);

  void RegisterObserver(webrtc::DataChannelObserver* observer) override;
  void UnregisterObserver() override;
  std::string label() const override;
  bool reliable() const override;
  bool ordered() const override;
  uint16_t maxRetransmitTime() const override;
  uint16_t maxRetransmits() const override;
  std::string protocol() const override;
  bool negotiated() const override;
  int id() const override;
  DataState state() const override;
  uint32_t messages_sent() const override;
  uint64_t bytes_sent() const override;
  uint32_t messages_received() const override;
  uint64_t bytes_received() const override;
  uint64_t buffered_amount() const override;
  void Close() override;
  bool Send(const webrtc::DataBuffer& buffer) override;

  // For testing.
  void changeState(DataState state);

 protected:
  ~MockDataChannel() override;

 private:
  std::string label_;
  bool reliable_;
  webrtc::DataChannelInterface::DataState state_;
  webrtc::DataChannelInit config_;
  webrtc::DataChannelObserver* observer_;

  DISALLOW_COPY_AND_ASSIGN(MockDataChannel);
};

}  // namespace content

#endif  // CONTENT_RENDERER_MEDIA_WEBRTC_MOCK_DATA_CHANNEL_IMPL_H_
