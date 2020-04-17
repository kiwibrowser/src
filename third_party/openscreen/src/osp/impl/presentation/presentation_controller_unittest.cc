// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "osp/public/presentation/presentation_controller.h"

#include <string>
#include <vector>

#include "osp/impl/quic/testing/quic_test_support.h"
#include "osp/impl/service_listener_impl.h"
#include "platform/test/fake_clock.h"
#include "osp/public/message_demuxer.h"
#include "osp/public/network_service_manager.h"
#include "osp/public/testing/message_demuxer_test_support.h"
#include "third_party/googletest/src/googlemock/include/gmock/gmock.h"
#include "third_party/googletest/src/googletest/include/gtest/gtest.h"

namespace openscreen {
namespace presentation {

using std::chrono::seconds;
using ::testing::_;
using ::testing::Invoke;

const char kTestUrl[] = "https://example.foo";

class MockServiceListenerDelegate final : public ServiceListenerImpl::Delegate {
 public:
  ~MockServiceListenerDelegate() override = default;

  ServiceListenerImpl* listener() { return listener_; }

  MOCK_METHOD0(StartListener, void());
  MOCK_METHOD0(StartAndSuspendListener, void());
  MOCK_METHOD0(StopListener, void());
  MOCK_METHOD0(SuspendListener, void());
  MOCK_METHOD0(ResumeListener, void());
  MOCK_METHOD1(SearchNow, void(ServiceListener::State from));
  MOCK_METHOD0(RunTasksListener, void());
};

class MockReceiverObserver final : public ReceiverObserver {
 public:
  ~MockReceiverObserver() override = default;

  MOCK_METHOD2(OnRequestFailed,
               void(const std::string& presentation_url,
                    const std::string& service_id));
  MOCK_METHOD2(OnReceiverAvailable,
               void(const std::string& presentation_url,
                    const std::string& service_id));
  MOCK_METHOD2(OnReceiverUnavailable,
               void(const std::string& presentation_url,
                    const std::string& service_id));
};

// TODO(btolsch): This is also used in multiple places now; factor out to
// separate file.
class MockConnectionDelegate final : public Connection::Delegate {
 public:
  MockConnectionDelegate() = default;
  ~MockConnectionDelegate() override = default;

  MOCK_METHOD0(OnConnected, void());
  MOCK_METHOD0(OnClosedByRemote, void());
  MOCK_METHOD0(OnDiscarded, void());
  MOCK_METHOD1(OnError, void(const absl::string_view message));
  MOCK_METHOD0(OnTerminated, void());
  MOCK_METHOD1(OnStringMessage, void(const absl::string_view message));
  MOCK_METHOD1(OnBinaryMessage, void(const std::vector<uint8_t>& data));
};

class MockRequestDelegate final : public RequestDelegate {
 public:
  MockRequestDelegate() = default;
  ~MockRequestDelegate() override = default;

  void OnConnection(std::unique_ptr<Connection> connection) override {
    OnConnectionMock(connection);
  }
  MOCK_METHOD1(OnConnectionMock, void(std::unique_ptr<Connection>& connection));
  MOCK_METHOD1(OnError, void(const Error& error));
};

class ControllerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    auto service_listener =
        std::make_unique<ServiceListenerImpl>(&mock_listener_delegate_);
    NetworkServiceManager::Create(std::move(service_listener), nullptr,
                                  std::move(quic_bridge_.quic_client),
                                  std::move(quic_bridge_.quic_server));
    controller_ = std::make_unique<Controller>(FakeClock::now);
    ON_CALL(quic_bridge_.mock_server_observer, OnIncomingConnectionMock(_))
        .WillByDefault(
            Invoke([this](std::unique_ptr<ProtocolConnection>& connection) {
              controller_endpoint_id_ = connection->endpoint_id();
            }));

    availability_watch_ =
        quic_bridge_.receiver_demuxer->SetDefaultMessageTypeWatch(
            msgs::Type::kPresentationUrlAvailabilityRequest, &mock_callback_);
  }

  void TearDown() override {
    availability_watch_ = MessageDemuxer::MessageWatch();
    controller_.reset();
    NetworkServiceManager::Dispose();
  }

  void ExpectAvailabilityRequest(
      MockMessageCallback* mock_callback,
      msgs::PresentationUrlAvailabilityRequest* request) {
    EXPECT_CALL(*mock_callback, OnStreamMessage(_, _, _, _, _, _))
        .WillOnce(Invoke([request](uint64_t endpoint_id, uint64_t cid,
                                   msgs::Type message_type,
                                   const uint8_t* buffer, size_t buffer_size,
                                   platform::Clock::time_point now) {
          ssize_t result = msgs::DecodePresentationUrlAvailabilityRequest(
              buffer, buffer_size, request);
          return result;
        }));
  }

  void SendAvailabilityResponse(
      const msgs::PresentationUrlAvailabilityResponse& response) {
    std::unique_ptr<ProtocolConnection> controller_connection =
        NetworkServiceManager::Get()
            ->GetProtocolConnectionServer()
            ->CreateProtocolConnection(controller_endpoint_id_);
    ASSERT_TRUE(controller_connection);
    ASSERT_EQ(Error::Code::kNone,
              controller_connection
                  ->WriteMessage(
                      response, msgs::EncodePresentationUrlAvailabilityResponse)
                  .code());
  }

  void SendStartResponse(const msgs::PresentationStartResponse& response) {
    std::unique_ptr<ProtocolConnection> protocol_connection =
        NetworkServiceManager::Get()
            ->GetProtocolConnectionServer()
            ->CreateProtocolConnection(controller_endpoint_id_);
    ASSERT_TRUE(protocol_connection);
    ASSERT_EQ(
        Error::Code::kNone,
        protocol_connection
            ->WriteMessage(response, msgs::EncodePresentationStartResponse)
            .code());
  }

  void SendAvailabilityEvent(
      const msgs::PresentationUrlAvailabilityEvent& event) {
    std::unique_ptr<ProtocolConnection> controller_connection =
        NetworkServiceManager::Get()
            ->GetProtocolConnectionServer()
            ->CreateProtocolConnection(controller_endpoint_id_);
    ASSERT_TRUE(controller_connection);
    ASSERT_EQ(
        Error::Code::kNone,
        controller_connection
            ->WriteMessage(event, msgs::EncodePresentationUrlAvailabilityEvent)
            .code());
  }

  void SendTerminationResponse(
      const msgs::PresentationTerminationResponse& response) {
    std::unique_ptr<ProtocolConnection> protocol_connection =
        NetworkServiceManager::Get()
            ->GetProtocolConnectionServer()
            ->CreateProtocolConnection(controller_endpoint_id_);
    ASSERT_TRUE(protocol_connection);
    ASSERT_EQ(Error::Code::kNone,
              protocol_connection
                  ->WriteMessage(response,
                                 msgs::EncodePresentationTerminationResponse)
                  .code());
  }

  void SendTerminationEvent(const msgs::PresentationTerminationEvent& event) {
    std::unique_ptr<ProtocolConnection> protocol_connection =
        NetworkServiceManager::Get()
            ->GetProtocolConnectionServer()
            ->CreateProtocolConnection(controller_endpoint_id_);
    ASSERT_TRUE(protocol_connection);
    ASSERT_EQ(
        Error::Code::kNone,
        protocol_connection
            ->WriteMessage(event, msgs::EncodePresentationTerminationEvent)
            .code());
  }

  void StartPresentation(MockMessageCallback* mock_callback,
                         MockConnectionDelegate* mock_connection_delegate,
                         std::unique_ptr<Connection>* connection) {
    MessageDemuxer::MessageWatch start_presentation_watch =
        quic_bridge_.receiver_demuxer->SetDefaultMessageTypeWatch(
            msgs::Type::kPresentationStartRequest, mock_callback);
    mock_listener_delegate_.listener()->OnReceiverAdded(receiver_info1);
    quic_bridge_.RunTasksUntilIdle();

    MockRequestDelegate mock_request_delegate;
    msgs::PresentationStartRequest request;
    msgs::Type msg_type;
    EXPECT_CALL(*mock_callback, OnStreamMessage(_, _, _, _, _, _))
        .WillOnce(
            Invoke([&request, &msg_type](
                       uint64_t endpoint_id, uint64_t cid,
                       msgs::Type message_type, const uint8_t* buffer,
                       size_t buffer_size, platform::Clock::time_point now) {
              msg_type = message_type;
              ssize_t result = msgs::DecodePresentationStartRequest(
                  buffer, buffer_size, &request);
              return result;
            }));
    Controller::ConnectRequest connect_request = controller_->StartPresentation(
        "https://example.com/receiver.html", receiver_info1.service_id,
        &mock_request_delegate, mock_connection_delegate);
    ASSERT_TRUE(connect_request);
    quic_bridge_.RunTasksUntilIdle();
    ASSERT_EQ(msgs::Type::kPresentationStartRequest, msg_type);

    msgs::PresentationStartResponse response;
    response.request_id = request.request_id;
    response.result = msgs::PresentationStartResponse_result::kSuccess;
    response.connection_id = 1;
    SendStartResponse(response);

    EXPECT_CALL(mock_request_delegate, OnConnectionMock(_))
        .WillOnce(Invoke([connection](std::unique_ptr<Connection>& c) {
          *connection = std::move(c);
        }));
    EXPECT_CALL(*mock_connection_delegate, OnConnected());
    quic_bridge_.RunTasksUntilIdle();

    ASSERT_TRUE(*connection);
  }

  MessageDemuxer::MessageWatch availability_watch_;
  MockMessageCallback mock_callback_;
  FakeClock fake_clock_{platform::Clock::time_point(seconds(11111))};
  FakeQuicBridge quic_bridge_{FakeClock::now};
  MockServiceListenerDelegate mock_listener_delegate_;
  std::unique_ptr<Controller> controller_;
  ServiceInfo receiver_info1{"service-id1",
                             "lucas-auer",
                             1,
                             quic_bridge_.kReceiverEndpoint,
                             {}};
  MockReceiverObserver mock_receiver_observer_;
  uint64_t controller_endpoint_id_{0};
};

TEST_F(ControllerTest, ReceiverWatchMoves) {
  std::vector<std::string> urls{"one fish", "two fish", "red fish", "gnu fish"};
  MockReceiverObserver mock_observer;

  Controller::ReceiverWatch watch1(controller_.get(), urls, &mock_observer);
  EXPECT_TRUE(watch1);
  Controller::ReceiverWatch watch2;
  EXPECT_FALSE(watch2);
  watch2 = std::move(watch1);
  EXPECT_FALSE(watch1);
  EXPECT_TRUE(watch2);
  Controller::ReceiverWatch watch3(std::move(watch2));
  EXPECT_FALSE(watch2);
  EXPECT_TRUE(watch3);
}

TEST_F(ControllerTest, ConnectRequestMoves) {
  std::string service_id{"service-id1"};
  uint64_t request_id = 7;

  Controller::ConnectRequest request1(controller_.get(), service_id, false,
                                      request_id);
  EXPECT_TRUE(request1);
  Controller::ConnectRequest request2;
  EXPECT_FALSE(request2);
  request2 = std::move(request1);
  EXPECT_FALSE(request1);
  EXPECT_TRUE(request2);
  Controller::ConnectRequest request3(std::move(request2));
  EXPECT_FALSE(request2);
  EXPECT_TRUE(request3);
}

TEST_F(ControllerTest, ReceiverAvailable) {
  mock_listener_delegate_.listener()->OnReceiverAdded(receiver_info1);
  Controller::ReceiverWatch watch =
      controller_->RegisterReceiverWatch({kTestUrl}, &mock_receiver_observer_);

  msgs::PresentationUrlAvailabilityRequest request;
  ExpectAvailabilityRequest(&mock_callback_, &request);
  quic_bridge_.RunTasksUntilIdle();

  msgs::PresentationUrlAvailabilityResponse response;
  response.request_id = request.request_id;
  response.url_availabilities.push_back(msgs::UrlAvailability::kAvailable);
  SendAvailabilityResponse(response);
  EXPECT_CALL(mock_receiver_observer_, OnReceiverAvailable(_, _));
  quic_bridge_.RunTasksUntilIdle();

  MockReceiverObserver mock_receiver_observer2;
  EXPECT_CALL(mock_receiver_observer2, OnReceiverAvailable(_, _));
  Controller::ReceiverWatch watch2 =
      controller_->RegisterReceiverWatch({kTestUrl}, &mock_receiver_observer2);
}

TEST_F(ControllerTest, ReceiverWatchCancel) {
  mock_listener_delegate_.listener()->OnReceiverAdded(receiver_info1);
  Controller::ReceiverWatch watch =
      controller_->RegisterReceiverWatch({kTestUrl}, &mock_receiver_observer_);

  msgs::PresentationUrlAvailabilityRequest request;
  ExpectAvailabilityRequest(&mock_callback_, &request);
  quic_bridge_.RunTasksUntilIdle();

  msgs::PresentationUrlAvailabilityResponse response;
  response.request_id = request.request_id;
  response.url_availabilities.push_back(msgs::UrlAvailability::kAvailable);
  SendAvailabilityResponse(response);
  EXPECT_CALL(mock_receiver_observer_, OnReceiverAvailable(_, _));
  quic_bridge_.RunTasksUntilIdle();

  MockReceiverObserver mock_receiver_observer2;
  EXPECT_CALL(mock_receiver_observer2, OnReceiverAvailable(_, _));
  Controller::ReceiverWatch watch2 =
      controller_->RegisterReceiverWatch({kTestUrl}, &mock_receiver_observer2);

  watch = Controller::ReceiverWatch();
  msgs::PresentationUrlAvailabilityEvent event;
  event.watch_id = request.watch_id;
  event.url_availabilities.push_back(msgs::UrlAvailability::kUnavailable);

  EXPECT_CALL(mock_receiver_observer2, OnReceiverUnavailable(_, _));
  EXPECT_CALL(mock_receiver_observer_, OnReceiverUnavailable(_, _)).Times(0);
  SendAvailabilityEvent(event);
  quic_bridge_.RunTasksUntilIdle();
}

TEST_F(ControllerTest, StartPresentation) {
  MockMessageCallback mock_callback;
  MockConnectionDelegate mock_connection_delegate;
  std::unique_ptr<Connection> connection;
  StartPresentation(&mock_callback, &mock_connection_delegate, &connection);
}

TEST_F(ControllerTest, TerminatePresentationFromController) {
  MockMessageCallback mock_callback;
  MockConnectionDelegate mock_connection_delegate;
  std::unique_ptr<Connection> connection;
  StartPresentation(&mock_callback, &mock_connection_delegate, &connection);

  MessageDemuxer::MessageWatch terminate_presentation_watch =
      quic_bridge_.receiver_demuxer->SetDefaultMessageTypeWatch(
          msgs::Type::kPresentationTerminationRequest, &mock_callback);
  msgs::PresentationTerminationRequest termination_request;
  msgs::Type msg_type;
  EXPECT_CALL(mock_callback, OnStreamMessage(_, _, _, _, _, _))
      .WillOnce(
          Invoke([&termination_request, &msg_type](
                     uint64_t endpoint_id, uint64_t cid,
                     msgs::Type message_type, const uint8_t* buffer,
                     size_t buffer_size, platform::Clock::time_point now) {
            msg_type = message_type;
            ssize_t result = msgs::DecodePresentationTerminationRequest(
                buffer, buffer_size, &termination_request);
            return result;
          }));
  connection->Terminate(TerminationReason::kControllerTerminateCalled);
  quic_bridge_.RunTasksUntilIdle();

  ASSERT_EQ(msgs::Type::kPresentationTerminationRequest, msg_type);
  msgs::PresentationTerminationResponse termination_response;
  termination_response.request_id = termination_request.request_id;
  termination_response.result =
      msgs::PresentationTerminationResponse_result::kSuccess;
  SendTerminationResponse(termination_response);

  // TODO(btolsch): Check OnTerminated of other connections when reconnect
  // lands.
  quic_bridge_.RunTasksUntilIdle();
}

TEST_F(ControllerTest, TerminatePresentationFromReceiver) {
  MockMessageCallback mock_callback;
  MockConnectionDelegate mock_connection_delegate;
  std::unique_ptr<Connection> connection;
  StartPresentation(&mock_callback, &mock_connection_delegate, &connection);

  msgs::PresentationTerminationEvent termination_event;
  termination_event.presentation_id = connection->presentation_info().id;
  termination_event.reason =
      msgs::PresentationTerminationEvent_reason::kReceiverCalledTerminate;
  SendTerminationEvent(termination_event);

  EXPECT_CALL(mock_connection_delegate, OnTerminated());
  quic_bridge_.RunTasksUntilIdle();
}

}  // namespace presentation
}  // namespace openscreen
