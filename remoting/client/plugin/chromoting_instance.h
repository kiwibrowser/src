// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_CLIENT_PLUGIN_CHROMOTING_INSTANCE_H_
#define REMOTING_CLIENT_PLUGIN_CHROMOTING_INSTANCE_H_

#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <string>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/threading/thread_task_runner_handle.h"
#include "ppapi/c/pp_instance.h"
#include "ppapi/c/pp_rect.h"
#include "ppapi/c/pp_resource.h"
#include "ppapi/cpp/instance.h"
#include "ppapi/cpp/text_input_controller.h"
#include "ppapi/cpp/var.h"
#include "remoting/client/client_context.h"
#include "remoting/client/client_user_interface.h"
#include "remoting/client/empty_cursor_filter.h"
#include "remoting/client/input/key_event_mapper.h"
#include "remoting/client/input/touch_input_scaler.h"
#include "remoting/client/plugin/pepper_cursor_setter.h"
#include "remoting/client/plugin/pepper_input_handler.h"
#include "remoting/client/plugin/pepper_video_renderer.h"
#include "remoting/proto/event.pb.h"
#include "remoting/protocol/client_authentication_config.h"
#include "remoting/protocol/client_stub.h"
#include "remoting/protocol/clipboard_stub.h"
#include "remoting/protocol/connection_to_host.h"
#include "remoting/protocol/cursor_shape_stub.h"
#include "remoting/protocol/input_event_tracker.h"
#include "remoting/protocol/mouse_input_filter.h"
#include "remoting/protocol/performance_tracker.h"

namespace base {
class DictionaryValue;
}  // namespace base

namespace pp {
class InputEvent;
class VarDictionary;
}  // namespace pp

namespace jingle_glue {
class JingleThreadWrapper;
}  // namespace jingle_glue

namespace webrtc {
class DesktopRegion;
class DesktopSize;
class DesktopVector;
}  // namespace webrtc

namespace remoting {

class PepperAudioPlayer;
class ChromotingClient;
class ClientContext;
class DelegatingSignalStrategy;
class PepperAudioPlayer;
class PepperMouseLocker;

class ChromotingInstance : public ClientUserInterface,
                           public PepperVideoRenderer::EventHandler,
                           public protocol::ClipboardStub,
                           public protocol::CursorShapeStub,
                           public pp::Instance {
 public:
  // Plugin API version. This should be incremented whenever the API
  // interface changes.
  static const int kApiVersion = 7;

  // Plugin API features. This allows orthogonal features to be supported
  // without bumping the API version.
  static const char kApiFeatures[];

  // Capabilities supported by the plugin that should also be supported by the
  // webapp to be enabled.
  static const char kRequestedCapabilities[];

  // Capabilities supported by the plugin that do not need to be supported by
  // the webapp to be enabled.
  static const char kSupportedCapabilities[];

  // Backward-compatibility version used by for the messaging
  // interface. Should be updated whenever we remove support for
  // an older version of the API.
  static const int kApiMinMessagingVersion = 5;

  // Backward-compatibility version used by for the ScriptableObject
  // interface. Should be updated whenever we remove support for
  // an older version of the API.
  static const int kApiMinScriptableVersion = 5;

  explicit ChromotingInstance(PP_Instance instance);
  ~ChromotingInstance() override;

  // pp::Instance interface.
  void DidChangeFocus(bool has_focus) override;
  void DidChangeView(const pp::View& view) override;
  bool Init(uint32_t argc, const char* argn[], const char* argv[]) override;
  void HandleMessage(const pp::Var& message) override;
  bool HandleInputEvent(const pp::InputEvent& event) override;

  // ClientUserInterface interface.
  void OnConnectionState(protocol::ConnectionToHost::State state,
                         protocol::ErrorCode error) override;
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

  // PepperVideoRenderer::EventHandler interface.
  void OnVideoDecodeError() override;
  void OnVideoFirstFrameReceived() override;
  void OnVideoFrameDirtyRegion(
      const webrtc::DesktopRegion& dirty_region) override;

  // Registers a global log message handler that redirects the log output to
  // our plugin instance.
  // This is called by the plugin's PPP_InitializeModule.
  // Note that no logging will be processed unless a ChromotingInstance has been
  // registered for logging (see RegisterLoggingInstance).
  static void RegisterLogMessageHandler();

  // Registers this instance so it processes messages sent by the global log
  // message handler. This overwrites any previously registered instance.
  void RegisterLoggingInstance();

  // Unregisters this instance so that debug log messages will no longer be sent
  // to it. If this instance is not the currently registered logging instance,
  // then the currently registered instance will stay in effect.
  void UnregisterLoggingInstance();

  // A Log Message Handler that is called after each LOG message has been
  // processed. This must be of type LogMessageHandlerFunction defined in
  // base/logging.h.
  static bool LogToUI(int severity, const char* file, int line,
                      size_t message_start, const std::string& str);

  // Requests the webapp to fetch a third-party token.
  void FetchThirdPartyToken(
      const std::string& host_public_key,
      const std::string& token_url,
      const std::string& scope,
      const protocol::ThirdPartyTokenFetchedCallback& token_fetched_callback);

  // Updates the specified UMA enumeration histogram with the input value.
  void UpdateUmaEnumHistogram(const std::string& histogram_name,
                              int64_t value,
                              int histogram_max);

  // Updates the specified UMA custom counts or custom times histogram with the
  // input value.
  void UpdateUmaCustomHistogram(bool is_custom_counts_histogram,
                                const std::string& histogram_name,
                                int64_t value,
                                int histogram_min,
                                int histogram_max,
                                int histogram_buckets);

 private:
  // Used as the |FetchSecretCallback| for IT2Me (or Me2Me from old webapps).
  // Immediately calls |secret_fetched_callback| with |shared_secret|.
  static void FetchSecretFromString(
      const std::string& shared_secret,
      bool pairing_supported,
      const protocol::SecretFetchedCallback& secret_fetched_callback);

  // Message handlers for messages that come from JavaScript. Called
  // from HandleMessage().
  void HandleConnect(const base::DictionaryValue& data);
  void HandleDisconnect(const base::DictionaryValue& data);
  void HandleOnIncomingIq(const base::DictionaryValue& data);
  void HandleReleaseAllKeys(const base::DictionaryValue& data);
  void HandleInjectKeyEvent(const base::DictionaryValue& data);
  void HandleRemapKey(const base::DictionaryValue& data);
  void HandleTrapKey(const base::DictionaryValue& data);
  void HandleSendClipboardItem(const base::DictionaryValue& data);
  void HandleNotifyClientResolution(const base::DictionaryValue& data);
  void HandleVideoControl(const base::DictionaryValue& data);
  void HandlePauseAudio(const base::DictionaryValue& data);
  void HandleOnPinFetched(const base::DictionaryValue& data);
  void HandleOnThirdPartyTokenFetched(const base::DictionaryValue& data);
  void HandleRequestPairing(const base::DictionaryValue& data);
  void HandleExtensionMessage(const base::DictionaryValue& data);
  void HandleAllowMouseLockMessage();
  void HandleSendMouseInputWhenUnfocused();
  void HandleDelegateLargeCursors();
  void HandleEnableDebugRegion(const base::DictionaryValue& data);
  void HandleEnableTouchEvents(const base::DictionaryValue& data);
  void HandleEnableStuckModifierKeyDetection(const base::DictionaryValue& data);

  void Disconnect();

  // Helper method to post messages to the webapp.
  void PostChromotingMessage(const std::string& method,
                             const pp::VarDictionary& data);

  // Same as above, but serializes messages to JSON before sending them.  This
  // method is used for backward compatibility with older version of the webapp
  // that expect to received most messages formatted using JSON.
  //
  // TODO(sergeyu): When all current versions of the webapp support raw messages
  // remove this method and use PostChromotingMessage() instead.
  void PostLegacyJsonMessage(const std::string& method,
                             std::unique_ptr<base::DictionaryValue> data);

  // Posts trapped keys to the web-app to handle.
  void SendTrappedKey(uint32_t usb_keycode, bool pressed);

  // Callback for DelegatingSignalStrategy.
  void SendOutgoingIq(const std::string& iq);

  void UpdatePerfStatsInUI();

  // Returns true if there is a ConnectionToHost and it is connected.
  bool IsConnected();

  // Used as the |FetchSecretCallback| for Me2Me connections.
  // Uses the PIN request dialog in the webapp to obtain the shared secret.
  void FetchSecretFromDialog(
      bool pairing_supported,
      const protocol::SecretFetchedCallback& secret_fetched_callback);

  void SendNetworkInfo();

  bool initialized_;

  scoped_refptr<base::SingleThreadTaskRunner> plugin_task_runner_;
  std::unique_ptr<base::ThreadTaskRunnerHandle> thread_task_runner_handle_;
  std::unique_ptr<jingle_glue::JingleThreadWrapper> thread_wrapper_;
  ClientContext context_;
  protocol::PerformanceTracker perf_tracker_;
  std::unique_ptr<PepperAudioPlayer> audio_player_;
  std::unique_ptr<PepperVideoRenderer> video_renderer_;
  pp::View plugin_view_;

  // Contains the most-recently-reported desktop shape, if any.
  std::unique_ptr<webrtc::DesktopRegion> desktop_shape_;

  std::unique_ptr<DelegatingSignalStrategy> signal_strategy_;

  std::unique_ptr<ChromotingClient> client_;

  scoped_refptr<protocol::TransportContext> transport_context_;

  // Input pipeline components, in reverse order of distance from input source.
  protocol::MouseInputFilter mouse_input_filter_;
  TouchInputScaler touch_input_scaler_;
  KeyEventMapper key_mapper_;
  std::unique_ptr<protocol::InputFilter> normalizing_input_filter_;
  protocol::InputEventTracker input_tracker_;
  PepperInputHandler input_handler_;

  // Cursor shape handling components, in reverse order to that in which they
  // process cursor shape events. Note that |mouse_locker_| appears in the
  // cursor pipeline since it is triggered by receipt of an empty cursor.
  PepperCursorSetter cursor_setter_;
  std::unique_ptr<PepperMouseLocker> mouse_locker_;
  EmptyCursorFilter empty_cursor_filter_;

  // Used to control text input settings, such as whether to show the IME.
  pp::TextInputController text_input_controller_;

  // PIN Fetcher.
  bool use_async_pin_dialog_;
  protocol::SecretFetchedCallback secret_fetched_callback_;

  protocol::ThirdPartyTokenFetchedCallback third_party_token_fetched_callback_;

  base::RepeatingTimer stats_update_timer_;

  base::TimeTicks connection_started_time;
  base::TimeTicks connection_authenticated_time_;
  base::TimeTicks connection_connected_time_;

  // Weak reference to this instance, used for global logging and task posting.
  base::WeakPtrFactory<ChromotingInstance> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(ChromotingInstance);
};

}  // namespace remoting

#endif  // REMOTING_CLIENT_PLUGIN_CHROMOTING_INSTANCE_H_
