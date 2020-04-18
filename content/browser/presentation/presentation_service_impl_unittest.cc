// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/presentation/presentation_service_impl.h"

#include <stddef.h>
#include <stdint.h>

#include <iterator>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/run_loop.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/presentation_request.h"
#include "content/public/browser/presentation_service_delegate.h"
#include "content/public/common/presentation_connection_message.h"
#include "content/test/test_render_frame_host.h"
#include "content/test/test_render_view_host.h"
#include "content/test/test_web_contents.h"
#include "mojo/public/cpp/bindings/interface_ptr.h"
#include "testing/gmock/include/gmock/gmock.h"

using blink::mojom::PresentationConnectionCloseReason;
using blink::mojom::PresentationConnectionState;
using blink::mojom::PresentationError;
using blink::mojom::PresentationErrorType;
using blink::mojom::PresentationInfo;
using blink::mojom::PresentationInfoPtr;
using blink::mojom::ScreenAvailability;
using ::testing::_;
using ::testing::Eq;
using ::testing::Mock;
using ::testing::Return;
using ::testing::SaveArg;
using NewPresentationCallback =
    content::PresentationServiceImpl::NewPresentationCallback;

namespace content {

namespace {

MATCHER_P(PresentationUrlsAre, expected_urls, "") {
  return arg.presentation_urls == expected_urls;
}

// Matches blink::mojom::PresentationInfo.
MATCHER_P(InfoEquals, expected, "") {
  return expected.url == arg.url && expected.id == arg.id;
}

ACTION_TEMPLATE(SaveArgByMove,
                HAS_1_TEMPLATE_PARAMS(int, k),
                AND_1_VALUE_PARAMS(pointer)) {
  *pointer = std::move(::testing::get<k>(args));
}

const char kPresentationId[] = "presentationId";
const char kPresentationUrl1[] = "http://foo.com/index.html";
const char kPresentationUrl2[] = "http://example.com/index.html";
const char kPresentationUrl3[] = "http://example.net/index.html";

}  // namespace

class MockPresentationServiceDelegate
    : public ControllerPresentationServiceDelegate {
 public:
  MOCK_METHOD3(AddObserver,
               void(int render_process_id,
                    int render_frame_id,
                    PresentationServiceDelegate::Observer* observer));
  MOCK_METHOD2(RemoveObserver,
      void(int render_process_id, int render_frame_id));

  bool AddScreenAvailabilityListener(
      int render_process_id,
      int routing_id,
      PresentationScreenAvailabilityListener* listener) override {
    if (!screen_availability_listening_supported_) {
      listener->OnScreenAvailabilityChanged(ScreenAvailability::DISABLED);
    }

    return AddScreenAvailabilityListener();
  }
  MOCK_METHOD0(AddScreenAvailabilityListener, bool());

  MOCK_METHOD3(RemoveScreenAvailabilityListener,
      void(int render_process_id,
           int routing_id,
           PresentationScreenAvailabilityListener* listener));
  MOCK_METHOD2(Reset,
      void(int render_process_id,
           int routing_id));
  MOCK_METHOD2(SetDefaultPresentationUrls,
               void(const PresentationRequest& request,
                    DefaultPresentationConnectionCallback callback));

  // TODO(crbug.com/729950): Use MOCK_METHOD directly once GMock gets the
  // move-only type support.
  void StartPresentation(
      const PresentationRequest& request,
      PresentationConnectionCallback success_cb,
      PresentationConnectionErrorCallback error_cb) override {
    StartPresentationInternal(request, success_cb, error_cb);
  }
  MOCK_METHOD3(StartPresentationInternal,
               void(const PresentationRequest& request,
                    PresentationConnectionCallback& success_cb,
                    PresentationConnectionErrorCallback& error_cb));
  void ReconnectPresentation(
      const PresentationRequest& request,
      const std::string& presentation_id,
      PresentationConnectionCallback success_cb,
      PresentationConnectionErrorCallback error_cb) override {
    ReconnectPresentationInternal(request, presentation_id, success_cb,
                                  error_cb);
  }
  MOCK_METHOD4(ReconnectPresentationInternal,
               void(const PresentationRequest& request,
                    const std::string& presentation_id,
                    PresentationConnectionCallback& success_cb,
                    PresentationConnectionErrorCallback& error_cb));
  MOCK_METHOD3(CloseConnection,
               void(int render_process_id,
                    int render_frame_id,
                    const std::string& presentation_id));
  MOCK_METHOD3(Terminate,
               void(int render_process_id,
                    int render_frame_id,
                    const std::string& presentation_id));
  MOCK_METHOD3(GetMediaController,
               std::unique_ptr<content::MediaController>(
                   int render_process_id,
                   int render_frame_id,
                   const std::string& presentation_id));

  // PresentationConnectionMessage is move-only.
  // TODO(crbug.com/729950): Use MOCK_METHOD directly once GMock gets the
  // move-only type support.
  void SendMessage(int render_process_id,
                   int render_frame_id,
                   const PresentationInfo& presentation_info,
                   PresentationConnectionMessage message,
                   SendMessageCallback send_message_cb) {
    SendMessageInternal(render_process_id, render_frame_id, presentation_info,
                        message, send_message_cb);
  }
  MOCK_METHOD5(SendMessageInternal,
               void(int render_process_id,
                    int render_frame_id,
                    const PresentationInfo& presentation_info,
                    const PresentationConnectionMessage& message,
                    const SendMessageCallback& send_message_cb));

  MOCK_METHOD4(
      ListenForConnectionStateChange,
      void(int render_process_id,
           int render_frame_id,
           const PresentationInfo& connection,
           const PresentationConnectionStateChangedCallback& state_changed_cb));

  void ConnectToPresentation(
      int render_process_id,
      int render_frame_id,
      const PresentationInfo& presentation_info,
      PresentationConnectionPtr controller_conn_ptr,
      PresentationConnectionRequest receiver_conn_request) override {
    RegisterLocalPresentationConnectionRaw(render_process_id, render_frame_id,
                                           presentation_info,
                                           controller_conn_ptr.get());
  }

  MOCK_METHOD4(RegisterLocalPresentationConnectionRaw,
               void(int render_process_id,
                    int render_frame_id,
                    const PresentationInfo& presentation_info,
                    blink::mojom::PresentationConnection* connection));

  void set_screen_availability_listening_supported(bool value) {
    screen_availability_listening_supported_ = value;
  }

 private:
  bool screen_availability_listening_supported_ = true;
};

class MockPresentationReceiver : public blink::mojom::PresentationReceiver {
 public:
  void OnReceiverConnectionAvailable(
      PresentationInfoPtr info,
      blink::mojom::PresentationConnectionPtr controller_connection,
      blink::mojom::PresentationConnectionRequest receiver_connection_request)
      override {
    OnReceiverConnectionAvailable(*info);
  }

  MOCK_METHOD1(OnReceiverConnectionAvailable,
               void(const PresentationInfo& info));
};

class MockReceiverPresentationServiceDelegate
    : public ReceiverPresentationServiceDelegate {
 public:
  MOCK_METHOD3(AddObserver,
               void(int render_process_id,
                    int render_frame_id,
                    PresentationServiceDelegate::Observer* observer));
  MOCK_METHOD2(RemoveObserver,
               void(int render_process_id, int render_frame_id));
  MOCK_METHOD2(Reset, void(int render_process_id, int routing_id));
  MOCK_METHOD1(RegisterReceiverConnectionAvailableCallback,
               void(const ReceiverConnectionAvailableCallback&));
};

class MockPresentationConnection : public blink::mojom::PresentationConnection {
 public:
  // PresentationConnectionMessage is move-only.
  void OnMessage(PresentationConnectionMessage message,
                 base::OnceCallback<void(bool)> send_message_cb) override {
    OnMessageInternal(message, send_message_cb);
  }
  MOCK_METHOD2(OnMessageInternal,
               void(const PresentationConnectionMessage& message,
                    base::OnceCallback<void(bool)>& send_message_cb));
  MOCK_METHOD1(DidChangeState, void(PresentationConnectionState state));
  MOCK_METHOD0(RequestClose, void());
};

class MockPresentationController : public blink::mojom::PresentationController {
 public:
  MOCK_METHOD2(OnScreenAvailabilityUpdated,
               void(const GURL& url, ScreenAvailability availability));
  void OnConnectionStateChanged(PresentationInfoPtr connection,
                                PresentationConnectionState new_state) {
    OnConnectionStateChangedInternal(*connection, new_state);
  }
  MOCK_METHOD2(OnConnectionStateChangedInternal,
               void(const PresentationInfo& connection,
                    PresentationConnectionState new_state));
  void OnConnectionClosed(
      PresentationInfoPtr connection,
      blink::mojom::PresentationConnectionCloseReason reason,
      const std::string& message) {
    OnConnectionClosedInternal(*connection, reason, message);
  }
  MOCK_METHOD3(OnConnectionClosedInternal,
               void(const PresentationInfo& connection,
                    blink::mojom::PresentationConnectionCloseReason reason,
                    const std::string& message));
  // PresentationConnectionMessage is move-only.
  void OnConnectionMessagesReceived(
      PresentationInfoPtr presentation_info,
      std::vector<PresentationConnectionMessage> messages) {
    OnConnectionMessagesReceivedInternal(*presentation_info, messages);
  }
  MOCK_METHOD2(
      OnConnectionMessagesReceivedInternal,
      void(const PresentationInfo& presentation_info,
           const std::vector<PresentationConnectionMessage>& messages));
  void OnDefaultPresentationStarted(PresentationInfoPtr presentation_info) {
    OnDefaultPresentationStartedInternal(*presentation_info);
  }
  MOCK_METHOD1(OnDefaultPresentationStartedInternal,
               void(const PresentationInfo& presentation_info));
};

class PresentationServiceImplTest : public RenderViewHostImplTestHarness {
 public:
  PresentationServiceImplTest()
      : presentation_url1_(GURL(kPresentationUrl1)),
        presentation_url2_(GURL(kPresentationUrl2)),
        presentation_url3_(GURL(kPresentationUrl3)) {}

  void SetUp() override {
    RenderViewHostImplTestHarness::SetUp();
    // This needed to keep the WebContentsObserverSanityChecker checks happy for
    // when AppendChild is called.
    NavigateAndCommit(GURL("about:blank"));

    EXPECT_CALL(mock_delegate_, AddObserver(_, _, _)).Times(1);
    TestRenderFrameHost* render_frame_host = contents()->GetMainFrame();
    render_frame_host->InitializeRenderFrameIfNeeded();
    service_impl_.reset(new PresentationServiceImpl(
        render_frame_host, contents(), &mock_delegate_, nullptr));

    blink::mojom::PresentationControllerPtr controller_ptr;
    controller_binding_.reset(
        new mojo::Binding<blink::mojom::PresentationController>(
            &mock_controller_, mojo::MakeRequest(&controller_ptr)));
    service_impl_->SetController(std::move(controller_ptr));

    presentation_urls_.push_back(presentation_url1_);
    presentation_urls_.push_back(presentation_url2_);

    expect_presentation_success_cb_ =
        base::BindOnce(&PresentationServiceImplTest::ExpectPresentationSuccess,
                       base::Unretained(this));
    expect_presentation_error_cb_ =
        base::BindOnce(&PresentationServiceImplTest::ExpectPresentationError,
                       base::Unretained(this));
  }

  void TearDown() override {
    if (service_impl_.get()) {
      ExpectDelegateReset();
      EXPECT_CALL(mock_delegate_, RemoveObserver(_, _)).Times(1);
      service_impl_.reset();
    }
    RenderViewHostImplTestHarness::TearDown();
  }

  void Navigate(bool main_frame) {
    RenderFrameHost* rfh = main_rfh();
    RenderFrameHostTester* rfh_tester = RenderFrameHostTester::For(rfh);
    if (!main_frame)
      rfh = rfh_tester->AppendChild("subframe");
    std::unique_ptr<NavigationHandle> navigation_handle =
        NavigationHandle::CreateNavigationHandleForTesting(
            GURL(), rfh, true);
    // Destructor calls DidFinishNavigation.
  }

  void ListenForScreenAvailabilityAndWait(const GURL& url,
                                          bool delegate_success) {
    EXPECT_CALL(mock_delegate_, AddScreenAvailabilityListener())
        .WillOnce(Return(delegate_success));
    service_impl_->ListenForScreenAvailability(url);

    EXPECT_TRUE(Mock::VerifyAndClearExpectations(&mock_delegate_));
  }

  void SimulateScreenAvailabilityChangeAndWait(
      const GURL& url,
      ScreenAvailability availability) {
    auto listener_it = service_impl_->screen_availability_listeners_.find(url);
    ASSERT_TRUE(listener_it->second);

    EXPECT_CALL(mock_controller_,
                OnScreenAvailabilityUpdated(url, availability));
    listener_it->second->OnScreenAvailabilityChanged(availability);
    base::RunLoop().RunUntilIdle();
  }

  void ExpectDelegateReset() {
    EXPECT_CALL(mock_delegate_, Reset(_, _)).Times(1);
  }

  void ExpectCleanState() {
    EXPECT_TRUE(service_impl_->default_presentation_urls_.empty());
    EXPECT_EQ(
        service_impl_->screen_availability_listeners_.find(presentation_url1_),
        service_impl_->screen_availability_listeners_.end());
  }

  void ExpectPresentationSuccess(PresentationInfoPtr info,
                                 blink::mojom::PresentationErrorPtr error) {
    EXPECT_FALSE(info.is_null());
    EXPECT_TRUE(error.is_null());
    presentation_cb_was_run_ = true;
  }

  void ExpectPresentationError(PresentationInfoPtr info,
                               blink::mojom::PresentationErrorPtr error) {
    EXPECT_TRUE(info.is_null());
    EXPECT_FALSE(error.is_null());
    presentation_cb_was_run_ = true;
  }

  void ExpectPresentationCallbackWasRun() const {
    EXPECT_TRUE(presentation_cb_was_run_)
        << "ExpectPresentationSuccess or ExpectPresentationError was called";
  }

  MockPresentationServiceDelegate mock_delegate_;
  MockReceiverPresentationServiceDelegate mock_receiver_delegate_;

  std::unique_ptr<PresentationServiceImpl> service_impl_;

  MockPresentationController mock_controller_;
  std::unique_ptr<mojo::Binding<blink::mojom::PresentationController>>
      controller_binding_;

  GURL presentation_url1_;
  GURL presentation_url2_;
  GURL presentation_url3_;
  std::vector<GURL> presentation_urls_;

  NewPresentationCallback expect_presentation_success_cb_;
  NewPresentationCallback expect_presentation_error_cb_;
  bool presentation_cb_was_run_ = false;
};

TEST_F(PresentationServiceImplTest, ListenForScreenAvailability) {
  ListenForScreenAvailabilityAndWait(presentation_url1_, true);

  SimulateScreenAvailabilityChangeAndWait(presentation_url1_,
                                          ScreenAvailability::AVAILABLE);
  SimulateScreenAvailabilityChangeAndWait(presentation_url1_,
                                          ScreenAvailability::UNAVAILABLE);
  SimulateScreenAvailabilityChangeAndWait(presentation_url1_,
                                          ScreenAvailability::AVAILABLE);
}

TEST_F(PresentationServiceImplTest, ScreenAvailabilityNotSupported) {
  mock_delegate_.set_screen_availability_listening_supported(false);
  EXPECT_CALL(mock_controller_,
              OnScreenAvailabilityUpdated(presentation_url1_,
                                          ScreenAvailability::DISABLED));
  ListenForScreenAvailabilityAndWait(presentation_url1_, false);
  base::RunLoop().RunUntilIdle();
}

TEST_F(PresentationServiceImplTest, OnDelegateDestroyed) {
  ListenForScreenAvailabilityAndWait(presentation_url1_, true);

  service_impl_->OnDelegateDestroyed();

  // TearDown() expects |mock_delegate_| to have been notified when
  // |service_impl_| is destroyed; this does not apply here since the delegate
  // is destroyed first.
  service_impl_.reset();
}

TEST_F(PresentationServiceImplTest, DidNavigateThisFrame) {
  ListenForScreenAvailabilityAndWait(presentation_url1_, true);

  ExpectDelegateReset();
  Navigate(true);
  ExpectCleanState();
}

TEST_F(PresentationServiceImplTest, DidNavigateOtherFrame) {
  ListenForScreenAvailabilityAndWait(presentation_url1_, true);

  Navigate(false);

  // Availability is reported and callback is invoked since it was not
  // removed.
  SimulateScreenAvailabilityChangeAndWait(presentation_url1_,
                                          ScreenAvailability::AVAILABLE);
}

TEST_F(PresentationServiceImplTest, DelegateFails) {
  ListenForScreenAvailabilityAndWait(presentation_url1_, false);
  ASSERT_EQ(
      service_impl_->screen_availability_listeners_.end(),
      service_impl_->screen_availability_listeners_.find(presentation_url1_));
}

TEST_F(PresentationServiceImplTest, SetDefaultPresentationUrls) {
  EXPECT_CALL(mock_delegate_, SetDefaultPresentationUrls(
                                  PresentationUrlsAre(presentation_urls_), _))
      .Times(1);

  service_impl_->SetDefaultPresentationUrls(presentation_urls_);

  // Sets different DPUs.
  std::vector<GURL> more_urls = presentation_urls_;
  more_urls.push_back(presentation_url3_);

  PresentationConnectionCallback callback;
  EXPECT_CALL(mock_delegate_,
              SetDefaultPresentationUrls(PresentationUrlsAre(more_urls), _))
      .WillOnce(SaveArgByMove<1>(&callback));
  service_impl_->SetDefaultPresentationUrls(more_urls);

  PresentationInfo presentation_info(presentation_url2_, kPresentationId);

  EXPECT_CALL(mock_controller_, OnDefaultPresentationStartedInternal(
                                    InfoEquals(presentation_info)));
  EXPECT_CALL(mock_delegate_, ListenForConnectionStateChange(_, _, _, _));
  std::move(callback).Run(
      PresentationInfo(presentation_url2_, kPresentationId));
  base::RunLoop().RunUntilIdle();
}

TEST_F(PresentationServiceImplTest,
       SetDefaultPresentationUrlsNoopsOnNonMainFrame) {
  RenderFrameHost* rfh = main_rfh();
  RenderFrameHostTester* rfh_tester = RenderFrameHostTester::For(rfh);
  rfh = rfh_tester->AppendChild("subframe");

  EXPECT_CALL(mock_delegate_, RemoveObserver(_, _)).Times(1);
  EXPECT_CALL(mock_delegate_, AddObserver(_, _, _)).Times(1);
  service_impl_.reset(
      new PresentationServiceImpl(rfh, contents(), &mock_delegate_, nullptr));

  EXPECT_CALL(mock_delegate_, SetDefaultPresentationUrls(_, _)).Times(0);
  service_impl_->SetDefaultPresentationUrls(presentation_urls_);
}

TEST_F(PresentationServiceImplTest, ListenForConnectionStateChange) {
  PresentationInfo connection(presentation_url1_, kPresentationId);
  PresentationConnectionStateChangedCallback state_changed_cb;
  // Trigger state change. It should be propagated back up to
  // |mock_controller_|.
  PresentationInfo presentation_connection(presentation_url1_, kPresentationId);

  EXPECT_CALL(mock_delegate_, ListenForConnectionStateChange(_, _, _, _))
      .WillOnce(SaveArg<3>(&state_changed_cb));
  service_impl_->ListenForConnectionStateChange(connection);

  EXPECT_CALL(mock_controller_, OnConnectionStateChangedInternal(
                                    InfoEquals(presentation_connection),
                                    PresentationConnectionState::TERMINATED));
  state_changed_cb.Run(PresentationConnectionStateChangeInfo(
      PresentationConnectionState::TERMINATED));
  base::RunLoop().RunUntilIdle();
}

TEST_F(PresentationServiceImplTest, ListenForConnectionClose) {
  PresentationInfo connection(presentation_url1_, kPresentationId);
  PresentationConnectionStateChangedCallback state_changed_cb;
  EXPECT_CALL(mock_delegate_, ListenForConnectionStateChange(_, _, _, _))
      .WillOnce(SaveArg<3>(&state_changed_cb));
  service_impl_->ListenForConnectionStateChange(connection);

  // Trigger connection close. It should be propagated back up to
  // |mock_controller_|.
  PresentationInfo presentation_connection(presentation_url1_, kPresentationId);
  PresentationConnectionStateChangeInfo closed_info(
      PresentationConnectionState::CLOSED);
  closed_info.close_reason = PresentationConnectionCloseReason::WENT_AWAY;
  closed_info.message = "Foo";

  EXPECT_CALL(mock_controller_,
              OnConnectionClosedInternal(
                  InfoEquals(presentation_connection),
                  PresentationConnectionCloseReason::WENT_AWAY, "Foo"));
  state_changed_cb.Run(closed_info);
  base::RunLoop().RunUntilIdle();
}

TEST_F(PresentationServiceImplTest, SetSameDefaultPresentationUrls) {
  EXPECT_CALL(mock_delegate_, SetDefaultPresentationUrls(_, _)).Times(1);
  service_impl_->SetDefaultPresentationUrls(presentation_urls_);
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(&mock_delegate_));

  // Same URLs as before; no-ops.
  service_impl_->SetDefaultPresentationUrls(presentation_urls_);
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(&mock_delegate_));
}

TEST_F(PresentationServiceImplTest, StartPresentationSuccess) {
  base::OnceCallback<void(const PresentationInfo&)> success_cb;
  EXPECT_CALL(mock_delegate_, StartPresentationInternal(_, _, _))
      .WillOnce(SaveArgByMove<1>(&success_cb));
  service_impl_->StartPresentation(presentation_urls_,
                                   std::move(expect_presentation_success_cb_));
  EXPECT_FALSE(success_cb.is_null());
  EXPECT_CALL(mock_delegate_, ListenForConnectionStateChange(_, _, _, _))
      .Times(1);
  std::move(success_cb)
      .Run(PresentationInfo(presentation_url1_, kPresentationId));
  ExpectPresentationCallbackWasRun();
}

TEST_F(PresentationServiceImplTest, StartPresentationError) {
  base::OnceCallback<void(const PresentationError&)> error_cb;
  EXPECT_CALL(mock_delegate_, StartPresentationInternal(_, _, _))
      .WillOnce(SaveArgByMove<2>(&error_cb));
  service_impl_->StartPresentation(presentation_urls_,
                                   std::move(expect_presentation_error_cb_));
  EXPECT_FALSE(error_cb.is_null());
  std::move(error_cb).Run(
      PresentationError(PresentationErrorType::UNKNOWN, "Error message"));
  ExpectPresentationCallbackWasRun();
}

TEST_F(PresentationServiceImplTest, StartPresentationInProgress) {
  EXPECT_CALL(mock_delegate_, StartPresentationInternal(_, _, _)).Times(1);
  // Uninvoked callbacks must outlive |service_impl_| since they get invoked
  // at |service_impl_|'s destruction.
  service_impl_->StartPresentation(presentation_urls_, base::DoNothing());

  // This request should fail immediately, since there is already a
  // StartPresentation in progress.
  service_impl_->StartPresentation(presentation_urls_,
                                   std::move(expect_presentation_error_cb_));
  ExpectPresentationCallbackWasRun();
}

TEST_F(PresentationServiceImplTest, ReconnectPresentationSuccess) {
  base::OnceCallback<void(const PresentationInfo&)> success_cb;
  EXPECT_CALL(mock_delegate_,
              ReconnectPresentationInternal(_, kPresentationId, _, _))
      .WillOnce(SaveArgByMove<2>(&success_cb));
  service_impl_->ReconnectPresentation(
      presentation_urls_, kPresentationId,
      std::move(expect_presentation_success_cb_));
  EXPECT_FALSE(success_cb.is_null());
  EXPECT_CALL(mock_delegate_, ListenForConnectionStateChange(_, _, _, _))
      .Times(1);
  std::move(success_cb)
      .Run(PresentationInfo(presentation_url1_, kPresentationId));
  ExpectPresentationCallbackWasRun();
}

TEST_F(PresentationServiceImplTest, ReconnectPresentationError) {
  base::OnceCallback<void(const PresentationError&)> error_cb;
  EXPECT_CALL(mock_delegate_,
              ReconnectPresentationInternal(_, kPresentationId, _, _))
      .WillOnce(SaveArgByMove<3>(&error_cb));
  service_impl_->ReconnectPresentation(
      presentation_urls_, kPresentationId,
      std::move(expect_presentation_error_cb_));
  EXPECT_FALSE(error_cb.is_null());
  std::move(error_cb).Run(
      PresentationError(PresentationErrorType::UNKNOWN, "Error message"));
  ExpectPresentationCallbackWasRun();
}

TEST_F(PresentationServiceImplTest, MaxPendingReconnectPresentationRequests) {
  const char* presentation_url = "http://fooUrl%d";
  const char* presentation_id = "presentationId%d";
  int num_requests = PresentationServiceImpl::kMaxQueuedRequests;
  int i = 0;
  EXPECT_CALL(mock_delegate_, ReconnectPresentationInternal(_, _, _, _))
      .Times(num_requests);
  for (; i < num_requests; ++i) {
    std::vector<GURL> urls = {GURL(base::StringPrintf(presentation_url, i))};
    // Uninvoked callbacks must outlive |service_impl_| since they get invoked
    // at |service_impl_|'s destruction.
    service_impl_->ReconnectPresentation(
        urls, base::StringPrintf(presentation_id, i), base::DoNothing());
  }

  std::vector<GURL> urls = {GURL(base::StringPrintf(presentation_url, i))};
  // Exceeded maximum queue size, should invoke mojo callback with error.
  service_impl_->ReconnectPresentation(
      urls, base::StringPrintf(presentation_id, i),
      std::move(expect_presentation_error_cb_));
  ExpectPresentationCallbackWasRun();
}

TEST_F(PresentationServiceImplTest, CloseConnection) {
  EXPECT_CALL(mock_delegate_, CloseConnection(_, _, Eq(kPresentationId)));
  service_impl_->CloseConnection(presentation_url1_, kPresentationId);
}

TEST_F(PresentationServiceImplTest, Terminate) {
  EXPECT_CALL(mock_delegate_, Terminate(_, _, Eq(kPresentationId)));
  service_impl_->Terminate(presentation_url1_, kPresentationId);
}

TEST_F(PresentationServiceImplTest, SetPresentationConnection) {
  PresentationInfoPtr presentation_info =
      PresentationInfo::New(presentation_url1_, kPresentationId);

  blink::mojom::PresentationConnectionPtr connection;
  MockPresentationConnection mock_presentation_connection;
  mojo::Binding<blink::mojom::PresentationConnection> connection_binding(
      &mock_presentation_connection, mojo::MakeRequest(&connection));
  blink::mojom::PresentationConnectionPtr receiver_connection;
  auto request = mojo::MakeRequest(&receiver_connection);

  PresentationInfo expected(presentation_url1_, kPresentationId);
  EXPECT_CALL(mock_delegate_, RegisterLocalPresentationConnectionRaw(
                                  _, _, InfoEquals(expected), _));

  service_impl_->SetPresentationConnection(
      std::move(presentation_info), std::move(connection), std::move(request));
}

TEST_F(PresentationServiceImplTest, ReceiverPresentationServiceDelegate) {
  EXPECT_CALL(mock_receiver_delegate_, AddObserver(_, _, _)).Times(1);

  PresentationServiceImpl service_impl(main_rfh(), contents(), nullptr,
                                       &mock_receiver_delegate_);

  ReceiverConnectionAvailableCallback callback;
  EXPECT_CALL(mock_receiver_delegate_,
              RegisterReceiverConnectionAvailableCallback(_))
      .WillOnce(SaveArg<0>(&callback));

  MockPresentationReceiver mock_receiver;
  blink::mojom::PresentationReceiverPtr receiver_ptr;
  mojo::Binding<blink::mojom::PresentationReceiver> receiver_binding(
      &mock_receiver, mojo::MakeRequest(&receiver_ptr));
  service_impl.controller_delegate_ = nullptr;
  service_impl.SetReceiver(std::move(receiver_ptr));
  EXPECT_FALSE(callback.is_null());

  PresentationInfo expected(presentation_url1_, kPresentationId);

  // Client gets notified of receiver connections.
  blink::mojom::PresentationConnectionPtr controller_connection;
  MockPresentationConnection mock_presentation_connection;
  mojo::Binding<blink::mojom::PresentationConnection> connection_binding(
      &mock_presentation_connection, mojo::MakeRequest(&controller_connection));
  blink::mojom::PresentationConnectionPtr receiver_connection;

  EXPECT_CALL(mock_receiver,
              OnReceiverConnectionAvailable(InfoEquals(expected)))
      .Times(1);
  callback.Run(PresentationInfo::New(expected),
               std::move(controller_connection),
               mojo::MakeRequest(&receiver_connection));
  base::RunLoop().RunUntilIdle();

  EXPECT_CALL(mock_receiver_delegate_, RemoveObserver(_, _)).Times(1);
}

TEST_F(PresentationServiceImplTest, ReceiverDelegateOnSubFrame) {
  EXPECT_CALL(mock_receiver_delegate_, AddObserver(_, _, _)).Times(1);

  PresentationServiceImpl service_impl(main_rfh(), contents(), nullptr,
                                       &mock_receiver_delegate_);
  service_impl.is_main_frame_ = false;

  ReceiverConnectionAvailableCallback callback;
  EXPECT_CALL(mock_receiver_delegate_,
              RegisterReceiverConnectionAvailableCallback(_))
      .Times(0);

  blink::mojom::PresentationControllerPtr controller_ptr;
  controller_binding_.reset(
      new mojo::Binding<blink::mojom::PresentationController>(
          &mock_controller_, mojo::MakeRequest(&controller_ptr)));
  service_impl.controller_delegate_ = nullptr;
  service_impl.SetController(std::move(controller_ptr));

  EXPECT_CALL(mock_receiver_delegate_, Reset(_, _)).Times(0);
  service_impl.Reset();

  EXPECT_CALL(mock_receiver_delegate_, RemoveObserver(_, _)).Times(1);
}

}  // namespace content
