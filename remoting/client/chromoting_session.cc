// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/client/chromoting_session.h"

#include <stdint.h>

#include <algorithm>
#include <utility>

#include "base/bind.h"
#include "base/callback.h"
#include "base/callback_helpers.h"
#include "base/format_macros.h"
#include "base/logging.h"
#include "base/single_thread_task_runner.h"
#include "base/timer/timer.h"
#include "jingle/glue/thread_wrapper.h"
#include "net/socket/client_socket_factory.h"
#include "remoting/base/chromium_url_request.h"
#include "remoting/base/chromoting_event.h"
#include "remoting/base/service_urls.h"
#include "remoting/client/audio/audio_player.h"
#include "remoting/client/chromoting_client_runtime.h"
#include "remoting/client/client_telemetry_logger.h"
#include "remoting/client/input/native_device_keymap.h"
#include "remoting/protocol/chromium_port_allocator_factory.h"
#include "remoting/protocol/chromium_socket_factory.h"
#include "remoting/protocol/client_authentication_config.h"
#include "remoting/protocol/frame_consumer.h"
#include "remoting/protocol/host_stub.h"
#include "remoting/protocol/network_settings.h"
#include "remoting/protocol/performance_tracker.h"
#include "remoting/protocol/transport_context.h"
#include "remoting/protocol/video_renderer.h"
#include "remoting/signaling/server_log_entry.h"
#include "ui/events/keycodes/dom/keycode_converter.h"

namespace remoting {

namespace {

const char* const kXmppServer = "talk.google.com";
const int kXmppPort = 5222;
const bool kXmppUseTls = true;

// Default DPI to assume for old clients that use notifyClientResolution.
const int kDefaultDPI = 96;

// Used by NormalizeclientResolution. See comment below.
const int kMinDimension = 640;

// Interval at which to log performance statistics, if enabled.
constexpr base::TimeDelta kPerfStatsInterval = base::TimeDelta::FromMinutes(1);

// Normalizes the resolution so that both dimensions are not smaller than
// kMinDimension.
void NormalizeClientResolution(protocol::ClientResolution* resolution) {
  int min_dimension =
      std::min(resolution->dips_width(), resolution->dips_height());
  if (min_dimension >= kMinDimension) {
    return;
  }

  // Always scale by integer to prevent blurry interpolation.
  int scale = std::ceil(((float)kMinDimension) / min_dimension);
  resolution->set_dips_width(resolution->dips_width() * scale);
  resolution->set_dips_height(resolution->dips_height() * scale);
}

void GetFeedbackDataOnNetworkThread(
    ChromotingClientRuntime* runtime,
    base::WeakPtr<ClientTelemetryLogger> logger,
    ChromotingSession::GetFeedbackDataCallback callback,
    scoped_refptr<base::SingleThreadTaskRunner> response_thread) {
  DCHECK(runtime->network_task_runner()->BelongsToCurrentThread());
  auto data = std::make_unique<FeedbackData>();
  if (logger) {
    data->FillWithChromotingEvent(logger->current_session_state_event());
  }
  if (response_thread->BelongsToCurrentThread()) {
    std::move(callback).Run(std::move(data));
  } else {
    response_thread->PostTask(
        FROM_HERE, base::BindOnce(std::move(callback), base::Passed(&data)));
  }
}

}  // namespace

struct ChromotingSession::SessionContext {
  ChromotingClientRuntime* runtime;
  base::WeakPtr<ChromotingSession::Delegate> delegate;
  base::WeakPtr<ClientTelemetryLogger> logger;
  base::WeakPtr<protocol::AudioStub> audio_player;
  std::unique_ptr<protocol::CursorShapeStub> cursor_shape_stub;
  std::unique_ptr<protocol::VideoRenderer> video_renderer;

  ConnectToHostInfo info;
};

class ChromotingSession::Core : public ClientUserInterface,
                                public protocol::ClipboardStub {
 public:
  explicit Core(std::unique_ptr<SessionContext> session_context);
  ~Core() override;

  void RequestPairing(const std::string& device_name);
  void SendMouseEvent(int x,
                      int y,
                      protocol::MouseEvent_MouseButton button,
                      bool button_down);
  void SendMouseWheelEvent(int delta_x, int delta_y);
  void SendKeyEvent(int usb_key_code, bool key_down);
  void SendTextEvent(const std::string& text);
  void SendTouchEvent(const protocol::TouchEvent& touch_event);
  void SendClientResolution(int dips_width, int dips_height, int scale);
  void EnableVideoChannel(bool enable);
  void SendClientMessage(const std::string& type, const std::string& data);

  // Logs the disconnect event and invalidates weak pointers.
  void Disconnect();

  // ClientUserInterface implementation.
  void OnConnectionState(protocol::ConnectionToHost::State state,
                         protocol::ErrorCode error) override;
  void OnConnectionReady(bool ready) override;
  void OnRouteChanged(const std::string& channel_name,
                      const protocol::TransportRoute& route) override;
  void SetCapabilities(const std::string& capabilities) override;
  void SetPairingResponse(const protocol::PairingResponse& response) override;
  void DeliverHostMessage(const protocol::ExtensionMessage& message) override;
  void SetDesktopSize(const webrtc::DesktopSize& size,
                      const webrtc::DesktopVector& dpi) override;
  protocol::ClipboardStub* GetClipboardStub() override;
  protocol::CursorShapeStub* GetCursorShapeStub() override;

  // CursorShapeStub implementation.
  void InjectClipboardEvent(const protocol::ClipboardEvent& event) override;

  base::WeakPtr<Core> GetWeakPtr();

 private:
  void ConnectOnNetworkThread();
  void LogPerfStats();

  // Pops up a UI to fetch the PIN.
  void FetchSecret(
      bool pairing_supported,
      const protocol::SecretFetchedCallback& secret_fetched_callback);
  void HandleOnSecretFetched(const protocol::SecretFetchedCallback& callback,
                             const std::string secret);

  // Pops up a UI to fetch the third party token.
  void FetchThirdPartyToken(
      const std::string& host_public_key,
      const std::string& token_url,
      const std::string& scopes,
      const protocol::ThirdPartyTokenFetchedCallback& token_fetched_callback);
  void HandleOnThirdPartyTokenFetched(
      const protocol::ThirdPartyTokenFetchedCallback& callback,
      const std::string& token,
      const std::string& shared_secret);

  scoped_refptr<AutoThreadTaskRunner> ui_task_runner() {
    return session_context_->runtime->ui_task_runner();
  }

  scoped_refptr<AutoThreadTaskRunner> network_task_runner() {
    return session_context_->runtime->network_task_runner();
  }

  std::unique_ptr<SessionContext> session_context_;

  std::unique_ptr<ClientContext> client_context_;
  std::unique_ptr<protocol::PerformanceTracker> perf_tracker_;

  // |signaling_| must outlive |client_|, so it must be declared above
  // |client_|.
  std::unique_ptr<XmppSignalStrategy> signaling_;
  std::unique_ptr<OAuthTokenGetter> token_getter_;
  std::unique_ptr<ChromotingClient> client_;

  // Empty string if client doesn't request for pairing.
  std::string device_name_for_pairing_;

  // The current session state.
  protocol::ConnectionToHost::State session_state_ =
      protocol::ConnectionToHost::INITIALIZING;

  base::RepeatingTimer perf_stats_logging_timer_;

  base::WeakPtrFactory<Core> weak_factory_;
  DISALLOW_COPY_AND_ASSIGN(Core);
};

ChromotingSession::Core::Core(std::unique_ptr<SessionContext> session_context)
    : session_context_(std::move(session_context)), weak_factory_(this) {
  DCHECK(ui_task_runner()->BelongsToCurrentThread());

  network_task_runner()->PostTask(
      FROM_HERE, base::BindOnce(&Core::ConnectOnNetworkThread, GetWeakPtr()));
}

ChromotingSession::Core::~Core() {
  DCHECK(network_task_runner()->BelongsToCurrentThread());

  // Make sure we log a close event if the session has not been disconnected
  // yet.
  Disconnect();
}

void ChromotingSession::Core::RequestPairing(const std::string& device_name) {
  DCHECK(!device_name.empty());
  DCHECK(network_task_runner()->BelongsToCurrentThread());
  device_name_for_pairing_ = device_name;
}

void ChromotingSession::Core::SendMouseEvent(
    int x,
    int y,
    protocol::MouseEvent_MouseButton button,
    bool button_down) {
  DCHECK(network_task_runner()->BelongsToCurrentThread());
  protocol::MouseEvent event;
  event.set_x(x);
  event.set_y(y);
  event.set_button(button);
  if (button != protocol::MouseEvent::BUTTON_UNDEFINED)
    event.set_button_down(button_down);

  client_->input_stub()->InjectMouseEvent(event);
}

void ChromotingSession::Core::SendMouseWheelEvent(int delta_x, int delta_y) {
  DCHECK(network_task_runner()->BelongsToCurrentThread());
  protocol::MouseEvent event;
  event.set_wheel_delta_x(delta_x);
  event.set_wheel_delta_y(delta_y);
  client_->input_stub()->InjectMouseEvent(event);
}

void ChromotingSession::Core::SendKeyEvent(int usb_key_code, bool key_down) {
  DCHECK(network_task_runner()->BelongsToCurrentThread());
  protocol::KeyEvent event;
  event.set_usb_keycode(usb_key_code);
  event.set_pressed(key_down);
  client_->input_stub()->InjectKeyEvent(event);
}

void ChromotingSession::Core::SendTextEvent(const std::string& text) {
  DCHECK(network_task_runner()->BelongsToCurrentThread());
  protocol::TextEvent event;
  event.set_text(text);
  client_->input_stub()->InjectTextEvent(event);
}

void ChromotingSession::Core::SendTouchEvent(
    const protocol::TouchEvent& touch_event) {
  DCHECK(network_task_runner()->BelongsToCurrentThread());
  client_->input_stub()->InjectTouchEvent(touch_event);
}

void ChromotingSession::Core::SendClientResolution(int dips_width,
                                                   int dips_height,
                                                   int scale) {
  DCHECK(network_task_runner()->BelongsToCurrentThread());
  protocol::ClientResolution client_resolution;
  client_resolution.set_dips_width(dips_width);
  client_resolution.set_dips_height(dips_height);
  client_resolution.set_x_dpi(scale * kDefaultDPI);
  client_resolution.set_y_dpi(scale * kDefaultDPI);
  NormalizeClientResolution(&client_resolution);

  // Include the legacy width & height in physical pixels for use by older
  // hosts.
  client_resolution.set_width_deprecated(dips_width * scale);
  client_resolution.set_height_deprecated(dips_height * scale);

  client_->host_stub()->NotifyClientResolution(client_resolution);
}

void ChromotingSession::Core::EnableVideoChannel(bool enable) {
  DCHECK(network_task_runner()->BelongsToCurrentThread());
  protocol::VideoControl video_control;
  video_control.set_enable(enable);
  client_->host_stub()->ControlVideo(video_control);
}

void ChromotingSession::Core::SendClientMessage(const std::string& type,
                                                const std::string& data) {
  DCHECK(network_task_runner()->BelongsToCurrentThread());
  protocol::ExtensionMessage extension_message;
  extension_message.set_type(type);
  extension_message.set_data(data);
  client_->host_stub()->DeliverClientMessage(extension_message);
}

void ChromotingSession::Core::Disconnect() {
  DCHECK(network_task_runner()->BelongsToCurrentThread());

  // Do not log session state change if the connection is already closed.
  if (session_state_ != protocol::ConnectionToHost::INITIALIZING &&
      session_state_ != protocol::ConnectionToHost::FAILED &&
      session_state_ != protocol::ConnectionToHost::CLOSED) {
    ChromotingEvent::SessionState session_state_to_log;
    if (session_state_ == protocol::ConnectionToHost::CONNECTED) {
      session_state_to_log = ChromotingEvent::SessionState::CLOSED;
    } else {
      session_state_to_log = ChromotingEvent::SessionState::CONNECTION_CANCELED;
    }
    session_context_->logger->LogSessionStateChange(
        session_state_to_log, ChromotingEvent::ConnectionError::NONE);
    session_state_ = protocol::ConnectionToHost::CLOSED;
    // Prevent all pending and future calls from ChromotingSession.
    weak_factory_.InvalidateWeakPtrs();
  }
}

void ChromotingSession::Core::OnConnectionState(
    protocol::ConnectionToHost::State state,
    protocol::ErrorCode error) {
  DCHECK(network_task_runner()->BelongsToCurrentThread());

  if (state == protocol::ConnectionToHost::CONNECTED) {
    perf_stats_logging_timer_.Start(
        FROM_HERE, kPerfStatsInterval,
        base::BindRepeating(&Core::LogPerfStats, GetWeakPtr()));

    if (!device_name_for_pairing_.empty()) {
      protocol::PairingRequest request;
      request.set_client_name(device_name_for_pairing_);
      client_->host_stub()->RequestPairing(request);
    }
  } else if (perf_stats_logging_timer_.IsRunning()) {
    perf_stats_logging_timer_.Stop();
  }

  session_context_->logger->LogSessionStateChange(
      ClientTelemetryLogger::TranslateState(state, session_state_),
      ClientTelemetryLogger::TranslateError(error));

  session_state_ = state;

  ui_task_runner()->PostTask(
      FROM_HERE, base::BindOnce(&ChromotingSession::Delegate::OnConnectionState,
                                session_context_->delegate, state, error));

  if (state == protocol::ConnectionToHost::CLOSED ||
      state == protocol::ConnectionToHost::FAILED) {
    weak_factory_.InvalidateWeakPtrs();
  }
}

void ChromotingSession::Core::OnConnectionReady(bool ready) {
  // We ignore this message, since OnConnectionState tells us the same thing.
}

void ChromotingSession::Core::OnRouteChanged(
    const std::string& channel_name,
    const protocol::TransportRoute& route) {
  DCHECK(network_task_runner()->BelongsToCurrentThread());
  std::string message = "Channel " + channel_name + " using " +
                        protocol::TransportRoute::GetTypeString(route.type) +
                        " connection.";
  VLOG(1) << "Route: " << message;
  session_context_->logger->SetTransportRoute(route);
}

void ChromotingSession::Core::SetCapabilities(const std::string& capabilities) {
  DCHECK(network_task_runner()->BelongsToCurrentThread());
  ui_task_runner()->PostTask(
      FROM_HERE, base::BindOnce(&ChromotingSession::Delegate::SetCapabilities,
                                session_context_->delegate, capabilities));
}

void ChromotingSession::Core::SetPairingResponse(
    const protocol::PairingResponse& response) {
  DCHECK(network_task_runner()->BelongsToCurrentThread());
  ui_task_runner()->PostTask(
      FROM_HERE,
      base::BindOnce(&ChromotingSession::Delegate::CommitPairingCredentials,
                     session_context_->delegate, session_context_->info.host_id,
                     response.client_id(), response.shared_secret()));
}

void ChromotingSession::Core::DeliverHostMessage(
    const protocol::ExtensionMessage& message) {
  DCHECK(network_task_runner()->BelongsToCurrentThread());
  ui_task_runner()->PostTask(
      FROM_HERE,
      base::BindOnce(&ChromotingSession::Delegate::HandleExtensionMessage,
                     session_context_->delegate, message.type(),
                     message.data()));
}

void ChromotingSession::Core::SetDesktopSize(const webrtc::DesktopSize& size,
                                             const webrtc::DesktopVector& dpi) {
  // ChromotingSession's VideoRenderer gets size from the frames and it doesn't
  // use DPI, so this call can be ignored.
}

protocol::ClipboardStub* ChromotingSession::Core::GetClipboardStub() {
  return this;
}

protocol::CursorShapeStub* ChromotingSession::Core::GetCursorShapeStub() {
  return session_context_->cursor_shape_stub.get();
}

void ChromotingSession::Core::InjectClipboardEvent(
    const protocol::ClipboardEvent& event) {
  NOTIMPLEMENTED();
}

base::WeakPtr<ChromotingSession::Core> ChromotingSession::Core::GetWeakPtr() {
  return weak_factory_.GetWeakPtr();
}

void ChromotingSession::Core::ConnectOnNetworkThread() {
  DCHECK(network_task_runner()->BelongsToCurrentThread());

  jingle_glue::JingleThreadWrapper::EnsureForCurrentMessageLoop();

  client_context_.reset(new ClientContext(network_task_runner()));
  client_context_->Start();

  perf_tracker_.reset(new protocol::PerformanceTracker());

  session_context_->video_renderer->Initialize(*client_context_,
                                               perf_tracker_.get());
  session_context_->logger->SetHostInfo(
      session_context_->info.host_version,
      ChromotingEvent::ParseOsFromString(session_context_->info.host_os),
      session_context_->info.host_os_version);

  client_.reset(new ChromotingClient(client_context_.get(), this,
                                     session_context_->video_renderer.get(),
                                     session_context_->audio_player));

  XmppSignalStrategy::XmppServerConfig xmpp_config;
  xmpp_config.host = kXmppServer;
  xmpp_config.port = kXmppPort;
  xmpp_config.use_tls = kXmppUseTls;
  xmpp_config.username = session_context_->info.username;
  xmpp_config.auth_token = session_context_->info.auth_token;

  signaling_.reset(new XmppSignalStrategy(
      net::ClientSocketFactory::GetDefaultFactory(),
      session_context_->runtime->url_requester(), xmpp_config));

  token_getter_ = session_context_->runtime->CreateOAuthTokenGetter();

  scoped_refptr<protocol::TransportContext> transport_context =
      new protocol::TransportContext(
          signaling_.get(),
          std::make_unique<protocol::ChromiumPortAllocatorFactory>(),
          std::make_unique<ChromiumUrlRequestFactory>(
              session_context_->runtime->url_requester()),
          protocol::NetworkSettings(
              protocol::NetworkSettings::NAT_TRAVERSAL_FULL),
          protocol::TransportRole::CLIENT);
  transport_context->set_ice_config_url(
      ServiceUrls::GetInstance()->ice_config_url(), token_getter_.get());

#if defined(ENABLE_WEBRTC_REMOTING_CLIENT)
  if (session_context_->info.flags.find("useWebrtc") != std::string::npos) {
    VLOG(0) << "Attempting to connect using WebRTC.";
    std::unique_ptr<protocol::CandidateSessionConfig> protocol_config =
        protocol::CandidateSessionConfig::CreateEmpty();
    protocol_config->set_webrtc_supported(true);
    protocol_config->set_ice_supported(false);
    client_->set_protocol_config(std::move(protocol_config));
  }
#endif  // defined(ENABLE_WEBRTC_REMOTING_CLIENT)
  if (session_context_->info.pairing_id.length() &&
      session_context_->info.pairing_secret.length()) {
    session_context_->logger->SetAuthMethod(
        ChromotingEvent::AuthMethod::PINLESS);
  }

  protocol::ClientAuthenticationConfig client_auth_config;
  client_auth_config.host_id = session_context_->info.host_id;
  client_auth_config.pairing_client_id = session_context_->info.pairing_id;
  client_auth_config.pairing_secret = session_context_->info.pairing_secret;
  client_auth_config.fetch_third_party_token_callback =
      base::BindRepeating(&Core::FetchThirdPartyToken, GetWeakPtr(),
                          session_context_->info.host_pubkey);
  client_auth_config.fetch_secret_callback =
      base::BindRepeating(&Core::FetchSecret, GetWeakPtr());

  client_->Start(signaling_.get(), client_auth_config, transport_context,
                 session_context_->info.host_jid,
                 session_context_->info.capabilities);
}

void ChromotingSession::Core::LogPerfStats() {
  DCHECK(network_task_runner()->BelongsToCurrentThread());

  session_context_->logger->LogStatistics(*perf_tracker_);
}

void ChromotingSession::Core::FetchSecret(
    bool pairing_supported,
    const protocol::SecretFetchedCallback& secret_fetched_callback) {
  DCHECK(network_task_runner()->BelongsToCurrentThread());

  // TODO(yuweih): Use bindOnce once SecretFetchedCallback becomes OnceCallback.
  auto secret_fetched_callback_for_ui_thread = base::BindRepeating(
      [](scoped_refptr<AutoThreadTaskRunner> network_task_runner,
         base::WeakPtr<ChromotingSession::Core> core,
         const protocol::SecretFetchedCallback& callback,
         const std::string& secret) {
        DCHECK(!network_task_runner->BelongsToCurrentThread());
        network_task_runner->PostTask(
            FROM_HERE,
            base::BindOnce(&ChromotingSession::Core::HandleOnSecretFetched,
                           core, callback, secret));
      },
      network_task_runner(), GetWeakPtr(), secret_fetched_callback);
  ui_task_runner()->PostTask(
      FROM_HERE, base::BindOnce(&ChromotingSession::Delegate::FetchSecret,
                                session_context_->delegate, pairing_supported,
                                secret_fetched_callback_for_ui_thread));
}

void ChromotingSession::Core::HandleOnSecretFetched(
    const protocol::SecretFetchedCallback& callback,
    const std::string secret) {
  DCHECK(network_task_runner()->BelongsToCurrentThread());

  session_context_->logger->SetAuthMethod(ChromotingEvent::AuthMethod::PIN);

  callback.Run(secret);
}

void ChromotingSession::Core::FetchThirdPartyToken(
    const std::string& host_public_key,
    const std::string& token_url,
    const std::string& scopes,
    const protocol::ThirdPartyTokenFetchedCallback& token_fetched_callback) {
  DCHECK(network_task_runner()->BelongsToCurrentThread());

  // TODO(yuweih): Use bindOnce once SecretFetchedCallback becomes OnceCallback.
  auto token_fetched_callback_for_ui_thread = base::BindRepeating(
      [](scoped_refptr<AutoThreadTaskRunner> network_task_runner,
         base::WeakPtr<ChromotingSession::Core> core,
         const protocol::ThirdPartyTokenFetchedCallback& callback,
         const std::string& token, const std::string& shared_secret) {
        DCHECK(!network_task_runner->BelongsToCurrentThread());
        network_task_runner->PostTask(
            FROM_HERE,
            base::BindOnce(
                &ChromotingSession::Core::HandleOnThirdPartyTokenFetched, core,
                callback, token, shared_secret));
      },
      network_task_runner(), GetWeakPtr(), token_fetched_callback);

  ui_task_runner()->PostTask(
      FROM_HERE,
      base::BindOnce(&ChromotingSession::Delegate::FetchThirdPartyToken,
                     session_context_->delegate, token_url, host_public_key,
                     scopes, token_fetched_callback_for_ui_thread));
}

void ChromotingSession::Core::HandleOnThirdPartyTokenFetched(
    const protocol::ThirdPartyTokenFetchedCallback& callback,
    const std::string& token,
    const std::string& shared_secret) {
  DCHECK(network_task_runner()->BelongsToCurrentThread());

  session_context_->logger->SetAuthMethod(
      ChromotingEvent::AuthMethod::THIRD_PARTY);

  callback.Run(token, shared_secret);
}

// ChromotingSession implementation.

ChromotingSession::ChromotingSession(
    base::WeakPtr<ChromotingSession::Delegate> delegate,
    std::unique_ptr<protocol::CursorShapeStub> cursor_shape_stub,
    std::unique_ptr<protocol::VideoRenderer> video_renderer,
    base::WeakPtr<protocol::AudioStub> audio_player,
    const ConnectToHostInfo& info) {
  DCHECK(delegate);
  DCHECK(cursor_shape_stub);
  DCHECK(video_renderer);
  // Don't DCHECK audio_player since it will bind audio_player to the ui thread.

  runtime_ = ChromotingClientRuntime::GetInstance();
  DCHECK(runtime_->ui_task_runner()->BelongsToCurrentThread());

  logger_ = std::make_unique<ClientTelemetryLogger>(
      runtime_->log_writer(), ChromotingEvent::Mode::ME2ME);

  // logger is set when connection is started.
  session_context_ = std::make_unique<SessionContext>();
  session_context_->runtime = runtime_;
  session_context_->delegate = delegate;
  session_context_->logger = logger_->GetWeakPtr();
  session_context_->audio_player = audio_player;
  session_context_->cursor_shape_stub = std::move(cursor_shape_stub);
  session_context_->video_renderer = std::move(video_renderer);
  session_context_->info = info;
}

ChromotingSession::~ChromotingSession() {
  DCHECK(runtime_->ui_task_runner()->BelongsToCurrentThread());

  if (core_) {
    runtime_->network_task_runner()->DeleteSoon(FROM_HERE, core_.release());
  }
  runtime_->network_task_runner()->DeleteSoon(FROM_HERE, logger_.release());
}

void ChromotingSession::Connect() {
  DCHECK(runtime_->ui_task_runner()->BelongsToCurrentThread());
  DCHECK(session_context_) << "Session has already been connected before.";
  core_ = std::make_unique<Core>(std::move(session_context_));
}

void ChromotingSession::Disconnect() {
  RunCoreTaskOnNetworkThread(FROM_HERE, &Core::Disconnect);
}

void ChromotingSession::GetFeedbackData(
    GetFeedbackDataCallback callback) const {
  DCHECK(runtime_->ui_task_runner()->BelongsToCurrentThread());

  runtime_->network_task_runner()->PostTask(
      FROM_HERE, base::BindOnce(&GetFeedbackDataOnNetworkThread, runtime_,
                                logger_->GetWeakPtr(), base::Passed(&callback),
                                runtime_->ui_task_runner()));
}

void ChromotingSession::RequestPairing(const std::string& device_name) {
  RunCoreTaskOnNetworkThread(FROM_HERE, &Core::RequestPairing, device_name);
}

void ChromotingSession::SendMouseEvent(int x,
                                       int y,
                                       protocol::MouseEvent_MouseButton button,
                                       bool button_down) {
  RunCoreTaskOnNetworkThread(FROM_HERE, &Core::SendMouseEvent, x, y, button,
                             button_down);
}

void ChromotingSession::SendMouseWheelEvent(int delta_x, int delta_y) {
  RunCoreTaskOnNetworkThread(FROM_HERE, &Core::SendMouseWheelEvent, delta_x,
                             delta_y);
}

bool ChromotingSession::SendKeyEvent(int scan_code,
                                     int key_code,
                                     bool key_down) {
  DCHECK(runtime_->ui_task_runner()->BelongsToCurrentThread());

  // For software keyboards |scan_code| is set to 0, in which case the
  // |key_code| is used instead.
  uint32_t usb_key_code =
      scan_code ? ui::KeycodeConverter::NativeKeycodeToUsbKeycode(scan_code)
                : NativeDeviceKeycodeToUsbKeycode(key_code);
  if (!usb_key_code) {
    LOG(WARNING) << "Ignoring unknown key code: " << key_code
                 << " scan code: " << scan_code;
    return false;
  }
  RunCoreTaskOnNetworkThread(FROM_HERE, &Core::SendKeyEvent, usb_key_code,
                             key_down);

  return true;
}

void ChromotingSession::SendTextEvent(const std::string& text) {
  RunCoreTaskOnNetworkThread(FROM_HERE, &Core::SendTextEvent, text);
}

void ChromotingSession::SendTouchEvent(
    const protocol::TouchEvent& touch_event) {
  RunCoreTaskOnNetworkThread(FROM_HERE, &Core::SendTouchEvent, touch_event);
}

void ChromotingSession::SendClientResolution(int dips_width,
                                             int dips_height,
                                             int scale) {
  RunCoreTaskOnNetworkThread(FROM_HERE, &Core::SendClientResolution, dips_width,
                             dips_height, scale);
}

void ChromotingSession::EnableVideoChannel(bool enable) {
  RunCoreTaskOnNetworkThread(FROM_HERE, &Core::EnableVideoChannel, enable);
}

void ChromotingSession::SendClientMessage(const std::string& type,
                                          const std::string& data) {
  RunCoreTaskOnNetworkThread(FROM_HERE, &Core::SendClientMessage, type, data);
}

template <typename Functor, typename... Args>
void ChromotingSession::RunCoreTaskOnNetworkThread(
    const base::Location& from_here,
    Functor&& core_functor,
    Args&&... args) {
  DCHECK(runtime_->ui_task_runner()->BelongsToCurrentThread());

  if (!core_) {
    LOG(WARNING) << "Session is not connected.";
    return;
  }

  runtime_->network_task_runner()->PostTask(
      FROM_HERE,
      base::BindOnce(std::forward<Functor>(core_functor), core_->GetWeakPtr(),
                     std::forward<Args>(args)...));
}

}  // namespace remoting
