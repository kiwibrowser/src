// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/host/client_session.h"

#include <algorithm>
#include <utility>

#include "base/command_line.h"
#include "base/optional.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "build/build_config.h"
#include "remoting/base/capabilities.h"
#include "remoting/base/constants.h"
#include "remoting/base/session_options.h"
#include "remoting/base/logging.h"
#include "remoting/host/audio_capturer.h"
#include "remoting/host/desktop_environment.h"
#include "remoting/host/file_transfer_message_handler.h"
#include "remoting/host/host_extension_session.h"
#include "remoting/host/input_injector.h"
#include "remoting/host/mouse_shape_pump.h"
#include "remoting/host/screen_controls.h"
#include "remoting/host/screen_resolution.h"
#include "remoting/proto/control.pb.h"
#include "remoting/proto/event.pb.h"
#include "remoting/protocol/audio_stream.h"
#include "remoting/protocol/capability_names.h"
#include "remoting/protocol/client_stub.h"
#include "remoting/protocol/clipboard_thread_proxy.h"
#include "remoting/protocol/pairing_registry.h"
#include "remoting/protocol/session.h"
#include "remoting/protocol/session_config.h"
#include "remoting/protocol/video_frame_pump.h"
#include "third_party/webrtc/modules/desktop_capture/desktop_capturer.h"

namespace remoting {

namespace {

// Name of command-line flag to disable use of I444 by default.
const char kDisableI444SwitchName[] = "disable-i444";

}  // namespace

ClientSession::ClientSession(
    EventHandler* event_handler,
    std::unique_ptr<protocol::ConnectionToClient> connection,
    DesktopEnvironmentFactory* desktop_environment_factory,
    const DesktopEnvironmentOptions& desktop_environment_options,
    const base::TimeDelta& max_duration,
    scoped_refptr<protocol::PairingRegistry> pairing_registry,
    const std::vector<HostExtension*>& extensions)
    : event_handler_(event_handler),
      connection_(std::move(connection)),
      client_jid_(connection_->session()->jid()),
      desktop_environment_factory_(desktop_environment_factory),
      desktop_environment_options_(desktop_environment_options),
      input_tracker_(&host_input_filter_),
      remote_input_filter_(&input_tracker_),
      mouse_clamping_filter_(&remote_input_filter_),
      disable_input_filter_(&mouse_clamping_filter_),
      disable_clipboard_filter_(clipboard_echo_filter_.host_filter()),
      client_clipboard_factory_(clipboard_echo_filter_.client_filter()),
      max_duration_(max_duration),
      pairing_registry_(pairing_registry),
      // Note that |lossless_video_color_| defaults to true, but actually only
      // controls VP9 video stream color quality.
      lossless_video_color_(!base::CommandLine::ForCurrentProcess()->HasSwitch(
          kDisableI444SwitchName)),
      weak_factory_(this) {
  connection_->session()->AddPlugin(&host_experiment_session_plugin_);
  connection_->SetEventHandler(this);

  // Create a manager for the configured extensions, if any.
  extension_manager_.reset(new HostExtensionSessionManager(extensions, this));

#if defined(OS_WIN)
  // LocalInputMonitorWin filters out an echo of the injected input before it
  // reaches |remote_input_filter_|.
  remote_input_filter_.SetExpectLocalEcho(false);
#endif  // defined(OS_WIN)
}

ClientSession::~ClientSession() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(!audio_stream_);
  DCHECK(!desktop_environment_);
  DCHECK(!input_injector_);
  DCHECK(!screen_controls_);
  DCHECK(!video_stream_);

  connection_.reset();
}

void ClientSession::NotifyClientResolution(
    const protocol::ClientResolution& resolution) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(resolution.dips_width() >= 0 && resolution.dips_height() >= 0);
  VLOG(1) << "Received ClientResolution (dips_width="
          << resolution.dips_width() << ", dips_height="
          << resolution.dips_height() << ")";

  if (!screen_controls_)
    return;

  webrtc::DesktopSize client_size(resolution.dips_width(),
                                  resolution.dips_height());
  if (connection_->session()->config().protocol() ==
      protocol::SessionConfig::Protocol::WEBRTC) {
    // When using WebRTC round down the dimensions to multiple of 2. Otherwise
    // the dimensions will be rounded on the receiver, which will cause blurring
    // due to scaling. The resulting size is still close to the client size and
    // will fit on the client's screen without scaling.
    // TODO(sergeyu): Make WebRTC handle odd dimensions properly.
    // crbug.com/636071
    client_size.set(client_size.width() & (~1), client_size.height() & (~1));
  }

  // Try to match the client's resolution.
  // TODO(sergeyu): Pass clients DPI to the resizer.
  screen_controls_->SetScreenResolution(ScreenResolution(
      client_size, webrtc::DesktopVector(kDefaultDpi, kDefaultDpi)));
}

void ClientSession::ControlVideo(const protocol::VideoControl& video_control) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // Note that |video_stream_| may be null, depending upon whether
  // extensions choose to wrap or "steal" the video capturer or encoder.
  if (video_control.has_enable()) {
    VLOG(1) << "Received VideoControl (enable="
            << video_control.enable() << ")";
    pause_video_ = !video_control.enable();
    if (video_stream_)
      video_stream_->Pause(pause_video_);
  }
  if (video_control.has_lossless_encode()) {
    VLOG(1) << "Received VideoControl (lossless_encode="
            << video_control.lossless_encode() << ")";
    lossless_video_encode_ = video_control.lossless_encode();
    if (video_stream_)
      video_stream_->SetLosslessEncode(lossless_video_encode_);
  }
  if (video_control.has_lossless_color()) {
    VLOG(1) << "Received VideoControl (lossless_color="
            << video_control.lossless_color() << ")";
    lossless_video_color_ = video_control.lossless_color();
    if (video_stream_)
      video_stream_->SetLosslessColor(lossless_video_color_);
  }
}

void ClientSession::ControlAudio(const protocol::AudioControl& audio_control) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (audio_control.has_enable()) {
    VLOG(1) << "Received AudioControl (enable="
            << audio_control.enable() << ")";
    if (audio_stream_)
      audio_stream_->Pause(!audio_control.enable());
  }
}

void ClientSession::SetCapabilities(
    const protocol::Capabilities& capabilities) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // Ignore all the messages but the 1st one.
  if (client_capabilities_) {
    LOG(WARNING) << "protocol::Capabilities has been received already.";
    return;
  }

  // Compute the set of capabilities supported by both client and host.
  client_capabilities_ = std::make_unique<std::string>();
  if (capabilities.has_capabilities())
    *client_capabilities_ = capabilities.capabilities();
  capabilities_ = IntersectCapabilities(*client_capabilities_,
                                        host_capabilities_);
  extension_manager_->OnNegotiatedCapabilities(
      connection_->client_stub(), capabilities_);

  if (HasCapability(capabilities_, protocol::kFileTransferCapability)) {
    data_channel_manager_.RegisterCreateHandlerCallback(
        kFileTransferDataChannelPrefix,
        base::Bind(&ClientSession::CreateFileTransferMessageHandler,
                   base::Unretained(this)));
  }

  VLOG(1) << "Client capabilities: " << *client_capabilities_;

  desktop_environment_->SetCapabilities(capabilities_);
}

void ClientSession::RequestPairing(
    const protocol::PairingRequest& pairing_request) {
  if (pairing_registry_.get() && pairing_request.has_client_name()) {
    protocol::PairingRegistry::Pairing pairing =
        pairing_registry_->CreatePairing(pairing_request.client_name());
    protocol::PairingResponse pairing_response;
    pairing_response.set_client_id(pairing.client_id());
    pairing_response.set_shared_secret(pairing.shared_secret());
    connection_->client_stub()->SetPairingResponse(pairing_response);
  }
}

void ClientSession::DeliverClientMessage(
    const protocol::ExtensionMessage& message) {
  if (message.has_type()) {
    if (extension_manager_->OnExtensionMessage(message))
      return;

    DLOG(INFO) << "Unexpected message received: " << message.type() << ": "
               << message.data();
  }
}

void ClientSession::OnConnectionAuthenticating() {
  event_handler_->OnSessionAuthenticating(this);
}

void ClientSession::OnConnectionAuthenticated() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(!audio_stream_);
  DCHECK(!desktop_environment_);
  DCHECK(!input_injector_);
  DCHECK(!screen_controls_);
  DCHECK(!video_stream_);

  is_authenticated_ = true;

  if (max_duration_ > base::TimeDelta()) {
    max_duration_timer_.Start(
        FROM_HERE, max_duration_,
        base::Bind(&ClientSession::DisconnectSession, base::Unretained(this),
                   protocol::MAX_SESSION_LENGTH));
  }

  // Notify EventHandler.
  event_handler_->OnSessionAuthenticated(this);

  const SessionOptions session_options(
      host_experiment_session_plugin_.configuration());

  connection_->ApplySessionOptions(session_options);

  DesktopEnvironmentOptions options = desktop_environment_options_;
  options.ApplySessionOptions(session_options);
  // Create the desktop environment. Drop the connection if it could not be
  // created for any reason (for instance the curtain could not initialize).
  desktop_environment_ =
      desktop_environment_factory_->Create(weak_factory_.GetWeakPtr(), options);
  if (!desktop_environment_) {
    DisconnectSession(protocol::HOST_CONFIGURATION_ERROR);
    return;
  }

  // Connect host stub.
  connection_->set_host_stub(this);

  // Collate the set of capabilities to offer the client, if it supports them.
  host_capabilities_ = desktop_environment_->GetCapabilities();
  if (!host_capabilities_.empty())
    host_capabilities_.append(" ");
  host_capabilities_.append(extension_manager_->GetCapabilities());

  // Create the object that controls the screen resolution.
  screen_controls_ = desktop_environment_->CreateScreenControls();

  // Create the event executor.
  input_injector_ = desktop_environment_->CreateInputInjector();

  // Connect the host input stubs.
  connection_->set_input_stub(&disable_input_filter_);
  host_input_filter_.set_input_stub(input_injector_.get());

  // Connect the clipboard stubs.
  connection_->set_clipboard_stub(&disable_clipboard_filter_);
  clipboard_echo_filter_.set_host_stub(input_injector_.get());
  clipboard_echo_filter_.set_client_stub(connection_->client_stub());
}

void ClientSession::CreateMediaStreams() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // Create a VideoStream to pump frames from the capturer to the client.
  video_stream_ = connection_->StartVideoStream(
      desktop_environment_->CreateVideoCapturer());

  // Create a AudioStream to pump audio from the capturer to the client.
  std::unique_ptr<protocol::AudioSource> audio_capturer =
      desktop_environment_->CreateAudioCapturer();
  if (audio_capturer) {
    audio_stream_ = connection_->StartAudioStream(std::move(audio_capturer));
  }

  video_stream_->SetObserver(this);

  // Apply video-control parameters to the new stream.
  video_stream_->SetLosslessEncode(lossless_video_encode_);
  video_stream_->SetLosslessColor(lossless_video_color_);

  // Pause capturing if necessary.
  video_stream_->Pause(pause_video_);

  if (event_timestamp_source_for_tests_)
    video_stream_->SetEventTimestampsSource(event_timestamp_source_for_tests_);
}

void ClientSession::OnConnectionChannelsConnected() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  DCHECK(!channels_connected_);
  channels_connected_ = true;

  // Negotiate capabilities with the client.
  VLOG(1) << "Host capabilities: " << host_capabilities_;
  protocol::Capabilities capabilities;
  capabilities.set_capabilities(host_capabilities_);
  connection_->client_stub()->SetCapabilities(capabilities);

  // Start the event executor.
  input_injector_->Start(CreateClipboardProxy());
  SetDisableInputs(false);

  // Create MouseShapePump to send mouse cursor shape.
  mouse_shape_pump_.reset(
      new MouseShapePump(desktop_environment_->CreateMouseCursorMonitor(),
                         connection_->client_stub()));

  if (pending_video_layout_message_) {
    connection_->client_stub()->SetVideoLayout(*pending_video_layout_message_);
    pending_video_layout_message_.reset();
  }

  // Notify the event handler that all our channels are now connected.
  event_handler_->OnSessionChannelsConnected(this);
}

void ClientSession::OnConnectionClosed(protocol::ErrorCode error) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  HOST_LOG << "Client disconnected: " << client_jid_ << "; error = " << error;

  // Ignore any further callbacks.
  weak_factory_.InvalidateWeakPtrs();

  // If the client never authenticated then the session failed.
  if (!is_authenticated_)
    event_handler_->OnSessionAuthenticationFailed(this);

  // Ensure that any pressed keys or buttons are released.
  input_tracker_.ReleaseAll();

  // Stop components access the client, audio or video stubs, which are no
  // longer valid once ConnectionToClient calls OnConnectionClosed().
  audio_stream_.reset();
  video_stream_.reset();
  mouse_shape_pump_.reset();
  client_clipboard_factory_.InvalidateWeakPtrs();
  input_injector_.reset();
  screen_controls_.reset();
  desktop_environment_.reset();

  // Notify the ChromotingHost that this client is disconnected.
  event_handler_->OnSessionClosed(this);
}

void ClientSession::OnRouteChange(const std::string& channel_name,
                                  const protocol::TransportRoute& route) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  event_handler_->OnSessionRouteChange(this, channel_name, route);
}

void ClientSession::OnIncomingDataChannel(
    const std::string& channel_name,
    std::unique_ptr<protocol::MessagePipe> pipe) {
  data_channel_manager_.OnIncomingDataChannel(channel_name, std::move(pipe));
}

const std::string& ClientSession::client_jid() const {
  return client_jid_;
}

void ClientSession::DisconnectSession(protocol::ErrorCode error) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(connection_.get());

  max_duration_timer_.Stop();

  // This triggers OnConnectionClosed(), and the session may be destroyed
  // as the result, so this call must be the last in this method.
  connection_->Disconnect(error);
}

void ClientSession::OnLocalMouseMoved(const webrtc::DesktopVector& position) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  remote_input_filter_.LocalMouseMoved(position);
}

void ClientSession::SetDisableInputs(bool disable_inputs) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (disable_inputs)
    input_tracker_.ReleaseAll();

  disable_input_filter_.set_enabled(!disable_inputs);
  disable_clipboard_filter_.set_enabled(!disable_inputs);
}

uint32_t ClientSession::desktop_session_id() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(desktop_environment_);
  return desktop_environment_->GetDesktopSessionId();
}

ClientSessionControl* ClientSession::session_control() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return this;
}

void ClientSession::RegisterCreateHandlerCallbackForTesting(
    const std::string& prefix,
    protocol::DataChannelManager::CreateHandlerCallback constructor) {
  data_channel_manager_.RegisterCreateHandlerCallback(
      prefix, std::move(constructor));
}

void ClientSession::SetEventTimestampsSourceForTests(
    scoped_refptr<protocol::InputEventTimestampsSource>
        event_timestamp_source) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  event_timestamp_source_for_tests_ = event_timestamp_source;
  if (video_stream_)
    video_stream_->SetEventTimestampsSource(event_timestamp_source_for_tests_);
}

std::unique_ptr<protocol::ClipboardStub> ClientSession::CreateClipboardProxy() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return std::make_unique<protocol::ClipboardThreadProxy>(
      client_clipboard_factory_.GetWeakPtr(),
      base::ThreadTaskRunnerHandle::Get());
}

void ClientSession::OnVideoSizeChanged(protocol::VideoStream* video_stream,
                                       const webrtc::DesktopSize& size,
                                       const webrtc::DesktopVector& dpi) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  mouse_clamping_filter_.set_output_size(size);

  switch (connection_->session()->config().protocol()) {
    case protocol::SessionConfig::Protocol::ICE:
      mouse_clamping_filter_.set_input_size(size);
      break;

    case protocol::SessionConfig::Protocol::WEBRTC: {
      // When using WebRTC protocol the client sends mouse coordinates in DIPs,
      // while InputInjector expects them in physical pixels.
      // TODO(sergeyu): Fix InputInjector implementations to use DIPs as well.
      webrtc::DesktopSize size_dips(size.width() * kDefaultDpi / dpi.x(),
                                    size.height() * kDefaultDpi / dpi.y());
      mouse_clamping_filter_.set_input_size(size_dips);

      // Generate and send VideoLayout message.
      protocol::VideoLayout layout;
      protocol::VideoTrackLayout* video_track = layout.add_video_track();
      video_track->set_position_x(0);
      video_track->set_position_y(0);
      video_track->set_width(size_dips.width());
      video_track->set_height(size_dips.height());
      video_track->set_x_dpi(dpi.x());
      video_track->set_y_dpi(dpi.y());

      // VideoLayout can be sent only after the control channel is connected.
      // TODO(sergeyu): Change client_stub() implementation to allow queuing
      // while connection is being established.
      if (channels_connected_) {
        connection_->client_stub()->SetVideoLayout(layout);
      } else {
        pending_video_layout_message_.reset(new protocol::VideoLayout(layout));
      }
      break;
    }
  }
}

void ClientSession::CreateFileTransferMessageHandler(
    const std::string& channel_name,
    std::unique_ptr<protocol::MessagePipe> pipe) {
  // FileTransferMessageHandler manages its own lifetime and is tied to the
  // lifetime of |pipe|. Once |pipe| is closed, this instance will be cleaned
  // up.
  new FileTransferMessageHandler(
      channel_name, std::move(pipe),
      desktop_environment_->CreateFileProxyWrapper());
}

}  // namespace remoting
