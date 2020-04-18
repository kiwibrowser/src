// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_RENDERER_API_DISPLAY_SOURCE_WIFI_DISPLAY_WIFI_DISPLAY_SESSION_H_
#define EXTENSIONS_RENDERER_API_DISPLAY_SOURCE_WIFI_DISPLAY_WIFI_DISPLAY_SESSION_H_

#include <map>
#include <string>

#include "extensions/common/mojo/wifi_display_session_service.mojom.h"
#include "extensions/renderer/api/display_source/display_source_session.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "third_party/wds/src/libwds/public/source.h"

namespace base {
class Timer;
}  // namespace base

namespace extensions {

class WiFiDisplayMediaManager;

// This class represents a single Wi-Fi Display session.
// It manages life-cycle of the session and it is also responsible for
// exchange of session controlling (RTSP) messages with the sink.
class WiFiDisplaySession : public DisplaySourceSession,
                           public mojom::WiFiDisplaySessionServiceClient,
                           public wds::Peer::Delegate,
                           public wds::Peer::Observer {
 public:
  explicit WiFiDisplaySession(
      const DisplaySourceSessionParams& params);
  ~WiFiDisplaySession() override;

 private:
  using DisplaySourceSession::CompletionCallback;
  // DisplaySourceSession overrides.
  void Start(const CompletionCallback& callback) override;
  void Terminate(const CompletionCallback& callback) override;

  // WiFiDisplaySessionServiceClient overrides.
  void OnConnected(const std::string& local_ip_address,
                   const std::string& sink_ip_address) override;
  void OnConnectRequestHandled(bool success, const std::string& error) override;
  void OnTerminated() override;
  void OnDisconnectRequestHandled(bool success,
                                  const std::string& error) override;
  void OnError(int32_t type, const std::string& description) override;
  void OnMessage(const std::string& data) override;

  // wds::Peer::Delegate overrides.
  unsigned CreateTimer(int seconds) override;
  void ReleaseTimer(unsigned timer_id) override;
  void SendRTSPData(const std::string& message) override;
  std::string GetLocalIPAddress() const override;
  int GetNextCSeq(int* initial_peer_cseq = nullptr) const override;

  // wds::Peer::Observer overrides.
  void ErrorOccurred(wds::ErrorType error) override;
  void SessionCompleted() override;

  // A connection error handler for the mojo objects used in this class.
  void OnIPCConnectionError();

  // An error handler for media pipeline error.
  void OnMediaError(const std::string& error);

  void Terminate();

  void RunStartCallback(bool success, const std::string& error = "");
  void RunTerminateCallback(bool success, const std::string& error = "");

 private:
  std::unique_ptr<wds::Source> wfd_source_;
  std::unique_ptr<WiFiDisplayMediaManager> media_manager_;
  mojom::WiFiDisplaySessionServicePtr service_;
  mojo::Binding<WiFiDisplaySessionServiceClient> binding_;
  std::string local_ip_address_;
  std::map<int, std::unique_ptr<base::Timer>> timers_;

  DisplaySourceSessionParams params_;
  CompletionCallback start_completion_callback_;
  CompletionCallback teminate_completion_callback_;
  // Holds sequence number for the following RTSP request-response pair.
  mutable int cseq_;
  int timer_id_;
  base::WeakPtrFactory<WiFiDisplaySession> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(WiFiDisplaySession);
};

}  // namespace extensions

#endif  // EXTENSIONS_RENDERER_API_DISPLAY_SOURCE_WIFI_DISPLAY_WIFI_DISPLAY_SESSION_H_
