// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_TEST_TEST_CHROMOTING_CLIENT_H_
#define REMOTING_TEST_TEST_CHROMOTING_CLIENT_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/observer_list.h"
#include "remoting/client/chromoting_client.h"
#include "remoting/client/client_user_interface.h"
#include "remoting/protocol/clipboard_filter.h"
#include "remoting/protocol/cursor_shape_stub.h"
#include "remoting/test/remote_connection_observer.h"

namespace remoting {

class ClientContext;

namespace protocol {
class ClipboardStub;
class HostStub;
class InputStub;
class VideoRenderer;
}  // namespace protocol

namespace test {

struct ConnectionSetupInfo;

// Manages a chromoting connection to a remote host.  Destroying a
// TestChromotingClient object with an active connection will close it.
// Must be used from a thread running an IO message loop.
// RemoteConnectionObserver objects must not destroy this class within a
// callback.
// A VideoRenderer can be passed in to customize the connection.
class TestChromotingClient : public ClientUserInterface,
                             public protocol::ClipboardStub,
                             public protocol::CursorShapeStub {
 public:
  TestChromotingClient();
  explicit TestChromotingClient(
      std::unique_ptr<protocol::VideoRenderer> video_renderer);
  ~TestChromotingClient() override;

  // Starts a Chromoting connection using the specified connection setup info.
  void StartConnection(bool use_test_api_values,
                       const ConnectionSetupInfo& connection_setup_info);

  // Ends the current remote connection and updates the connection state.
  void EndConnection();

  // Stubs used to send messages to the remote host.
  protocol::ClipboardStub* clipboard_forwarder() {
    return chromoting_client_->clipboard_forwarder();
  }
  protocol::HostStub* host_stub() { return chromoting_client_->host_stub(); }
  protocol::InputStub* input_stub() { return chromoting_client_->input_stub(); }

  // Registers an observer which will be notfied when remote connection events
  // occur.  Registered Observers must not tear-down this object on receipt of
  // these callbacks. The callbacks should be used for informational purposes.
  void AddRemoteConnectionObserver(RemoteConnectionObserver* observer);

  // Unregisters an observerer from notifications for remote connection events.
  void RemoveRemoteConnectionObserver(RemoteConnectionObserver* observer);

  // Used to set a fake/mock dependencies for tests.
  void SetSignalStrategyForTests(
      std::unique_ptr<SignalStrategy> signal_strategy);
  void SetConnectionToHostForTests(
      std::unique_ptr<protocol::ConnectionToHost> connection_to_host);

 private:
  // ClientUserInterface interface.
  void OnConnectionState(protocol::ConnectionToHost::State state,
                         protocol::ErrorCode error_code) override;
  void OnConnectionReady(bool ready) override;
  void OnRouteChanged(const std::string& channel_name,
                      const protocol::TransportRoute& route) override;
  void SetCapabilities(const std::string& capabilities) override;
  void SetPairingResponse(
      const protocol::PairingResponse& pairing_response) override;
  void DeliverHostMessage(const protocol::ExtensionMessage& message) override;
  void SetDesktopSize(const webrtc::DesktopSize& size,
                      const webrtc::DesktopVector& dpi) override;
  protocol::ClipboardStub* GetClipboardStub() override;
  protocol::CursorShapeStub* GetCursorShapeStub() override;

  // protocol::ClipboardStub interface.
  void InjectClipboardEvent(const protocol::ClipboardEvent& event) override;

  // protocol::CursorShapeStub interface.
  void SetCursorShape(const protocol::CursorShapeInfo& cursor_shape) override;

  // Tracks the current connection state.
  protocol::ConnectionToHost::State connection_to_host_state_;

  // Tracks the most recent connection error code seen.
  protocol::ErrorCode connection_error_code_;

  // List of observers which are notified when remote connection events occur.
  // We specify true below for the 'check_empty' flag so the list will check to
  // see if all observers have been unregistered when it is destroyed.
  base::ObserverList<RemoteConnectionObserver, true> connection_observers_;

  // ConnectionToHost used by TestChromotingClient tests.
  std::unique_ptr<protocol::ConnectionToHost> test_connection_to_host_;

  // Creates and manages the connection to the remote host.
  std::unique_ptr<ChromotingClient> chromoting_client_;

  // Manages the threads and task runners for |chromoting_client_|.
  std::unique_ptr<ClientContext> client_context_;

  // Processes video packets from the host.
  std::unique_ptr<protocol::VideoRenderer> video_renderer_;

  // SignalStrategy used for connection signaling.
  std::unique_ptr<SignalStrategy> signal_strategy_;

  DISALLOW_COPY_AND_ASSIGN(TestChromotingClient);
};

}  // namespace test
}  // namespace remoting

#endif  // REMOTING_TEST_TEST_CHROMOTING_CLIENT_H_
