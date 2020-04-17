// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "osp/public/presentation/presentation_receiver.h"

#include <memory>

#include "osp/impl/quic/quic_client.h"
#include "osp/impl/quic/quic_server.h"
#include "osp/impl/quic/testing/fake_quic_connection_factory.h"
#include "osp/impl/quic/testing/quic_test_support.h"
#include "platform/test/fake_clock.h"
#include "osp/public/network_service_manager.h"
#include "osp/public/protocol_connection_server.h"
#include "osp/public/testing/message_demuxer_test_support.h"
#include "third_party/googletest/src/googlemock/include/gmock/gmock.h"
#include "third_party/googletest/src/googletest/include/gtest/gtest.h"

namespace openscreen {
namespace presentation {
namespace {

using ::testing::_;
using ::testing::Invoke;

class MockConnectionDelegate final : public Connection::Delegate {
 public:
  ~MockConnectionDelegate() override = default;

  MOCK_METHOD0(OnConnected, void());
  MOCK_METHOD0(OnClosedByRemote, void());
  MOCK_METHOD0(OnDiscarded, void());
  MOCK_METHOD1(OnError, void(const absl::string_view message));
  MOCK_METHOD0(OnTerminated, void());
  MOCK_METHOD1(OnStringMessage, void(const absl::string_view message));
  MOCK_METHOD1(OnBinaryMessage, void(const std::vector<uint8_t>& data));
};

class MockConnectRequest final
    : public ProtocolConnectionClient::ConnectionRequestCallback {
 public:
  ~MockConnectRequest() override = default;

  // TODO(jophba): remove trampoline once the following work item is completed:
  // https://github.com/google/googletest/issues/2130
  void OnConnectionOpened(
      uint64_t request_id,
      std::unique_ptr<ProtocolConnection> connection) override {
    OnConnectionOpenedMock(request_id, connection.release());
  }
  MOCK_METHOD2(OnConnectionOpenedMock,
               void(uint64_t request_id, ProtocolConnection* connection));
  MOCK_METHOD1(OnConnectionFailed, void(uint64_t request_id));
};

class MockReceiverDelegate final : public ReceiverDelegate {
 public:
  ~MockReceiverDelegate() override = default;

  MOCK_METHOD3(
      OnUrlAvailabilityRequest,
      std::vector<msgs::UrlAvailability>(uint64_t watch_id,
                                         uint64_t watch_duration,
                                         std::vector<std::string> urls));
  MOCK_METHOD3(StartPresentation,
               bool(const Connection::PresentationInfo& info,
                    uint64_t source_id,
                    const std::vector<msgs::HttpHeader>& http_headers));
  MOCK_METHOD3(ConnectToPresentation,
               bool(uint64_t request_id,
                    const std::string& id,
                    uint64_t source_id));
  MOCK_METHOD2(TerminatePresentation,
               void(const std::string& id, TerminationReason reason));
};

class PresentationReceiverTest : public ::testing::Test {
 protected:
  std::unique_ptr<ProtocolConnection> MakeClientStream() {
    MockConnectRequest mock_connect_request;
    NetworkServiceManager::Get()->GetProtocolConnectionClient()->Connect(
        quic_bridge_.kReceiverEndpoint, &mock_connect_request);
    ProtocolConnection* stream;
    EXPECT_CALL(mock_connect_request, OnConnectionOpenedMock(_, _))
        .WillOnce(::testing::SaveArg<1>(&stream));
    quic_bridge_.RunTasksUntilIdle();
    return std::unique_ptr<ProtocolConnection>(stream);
  }

  void SetUp() override {
    NetworkServiceManager::Create(nullptr, nullptr,
                                  std::move(quic_bridge_.quic_client),
                                  std::move(quic_bridge_.quic_server));
    Receiver::Get()->Init();
    Receiver::Get()->SetReceiverDelegate(&mock_receiver_delegate_);
  }

  void TearDown() override {
    Receiver::Get()->SetReceiverDelegate(nullptr);
    Receiver::Get()->Deinit();
    NetworkServiceManager::Dispose();
  }

  const std::string url1_{"https://www.example.com/receiver.html"};
  FakeClock fake_clock_{
      platform::Clock::time_point(std::chrono::milliseconds(1298424))};
  FakeQuicBridge quic_bridge_{FakeClock::now};
  MockReceiverDelegate mock_receiver_delegate_;
};

}  // namespace

// TODO(btolsch): Availability CL includes watch duration, so when that lands,
// also test proper updating here.
TEST_F(PresentationReceiverTest, QueryAvailability) {
  MockMessageCallback mock_callback;
  MessageDemuxer::MessageWatch availability_watch =
      quic_bridge_.controller_demuxer->SetDefaultMessageTypeWatch(
          msgs::Type::kPresentationUrlAvailabilityResponse, &mock_callback);

  std::unique_ptr<ProtocolConnection> stream = MakeClientStream();
  ASSERT_TRUE(stream);

  msgs::PresentationUrlAvailabilityRequest request{/* .request_id = */ 0,
                                                   /* .urls = */ {url1_},
                                                   /* .watch_duration = */ 0,
                                                   /* .watch_id = */ 0};
  msgs::CborEncodeBuffer buffer;
  ASSERT_TRUE(msgs::EncodePresentationUrlAvailabilityRequest(request, &buffer));
  stream->Write(buffer.data(), buffer.size());

  EXPECT_CALL(mock_receiver_delegate_, OnUrlAvailabilityRequest(_, _, _))
      .WillOnce(Invoke([this](uint64_t watch_id, uint64_t watch_duration,
                              std::vector<std::string> urls) {
        EXPECT_EQ(std::vector<std::string>{url1_}, urls);

        return std::vector<msgs::UrlAvailability>{
            msgs::UrlAvailability::kAvailable};
      }));

  msgs::PresentationUrlAvailabilityResponse response;
  EXPECT_CALL(mock_callback, OnStreamMessage(_, _, _, _, _, _))
      .WillOnce(Invoke([&response](uint64_t endpoint_id, uint64_t cid,
                                   msgs::Type message_type,
                                   const uint8_t* buffer, size_t buffer_size,
                                   platform::Clock::time_point now) {
        ssize_t result = msgs::DecodePresentationUrlAvailabilityResponse(
            buffer, buffer_size, &response);
        return result;
      }));
  quic_bridge_.RunTasksUntilIdle();
  EXPECT_EQ(request.request_id, response.request_id);
  EXPECT_EQ(
      (std::vector<msgs::UrlAvailability>{msgs::UrlAvailability::kAvailable}),
      response.url_availabilities);
}

TEST_F(PresentationReceiverTest, StartPresentation) {
  MockMessageCallback mock_callback;
  MessageDemuxer::MessageWatch initiation_watch =
      quic_bridge_.controller_demuxer->SetDefaultMessageTypeWatch(
          msgs::Type::kPresentationStartResponse, &mock_callback);

  std::unique_ptr<ProtocolConnection> stream = MakeClientStream();
  ASSERT_TRUE(stream);

  const std::string presentation_id = "KMvyNqTCvvSv7v5X";
  msgs::PresentationStartRequest request;
  request.request_id = 0;
  request.presentation_id = presentation_id;
  request.url = url1_;
  request.headers = {msgs::HttpHeader{"Accept-Language", "de"}};
  msgs::CborEncodeBuffer buffer;
  ASSERT_TRUE(msgs::EncodePresentationStartRequest(request, &buffer));
  stream->Write(buffer.data(), buffer.size());
  Connection::PresentationInfo info;
  EXPECT_CALL(mock_receiver_delegate_, StartPresentation(_, _, request.headers))
      .WillOnce(::testing::DoAll(::testing::SaveArg<0>(&info),
                                 ::testing::Return(true)));
  quic_bridge_.RunTasksUntilIdle();
  EXPECT_EQ(presentation_id, info.id);
  EXPECT_EQ(url1_, info.url);

  MockConnectionDelegate null_connection_delegate;
  Connection connection(Connection::PresentationInfo{presentation_id, url1_},
                        &null_connection_delegate, Receiver::Get());
  Receiver::Get()->OnPresentationStarted(presentation_id, &connection,
                                         ResponseResult::kSuccess);
  msgs::PresentationStartResponse response;
  EXPECT_CALL(mock_callback, OnStreamMessage(_, _, _, _, _, _))
      .WillOnce(Invoke([&response](uint64_t endpoint_id, uint64_t cid,
                                   msgs::Type message_type,
                                   const uint8_t* buffer, size_t buffer_size,
                                   platform::Clock::time_point now) {
        ssize_t result = msgs::DecodePresentationStartResponse(
            buffer, buffer_size, &response);
        return result;
      }));
  quic_bridge_.RunTasksUntilIdle();
  EXPECT_EQ(msgs::Result::kSuccess, response.result);
  EXPECT_EQ(connection.connection_id(), response.connection_id);
}

// TODO(btolsch): Connect and reconnect.
// TODO(btolsch): Terminate request and event.

}  // namespace presentation
}  // namespace openscreen
