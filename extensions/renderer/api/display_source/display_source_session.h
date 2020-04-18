// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_RENDERER_API_DISPLAY_SOURCE_DISPLAY_SOURCE_SESSION_H_
#define EXTENSIONS_RENDERER_API_DISPLAY_SOURCE_DISPLAY_SOURCE_SESSION_H_

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/macros.h"
#include "extensions/common/api/display_source.h"
#include "third_party/blink/public/web/web_dom_media_stream_track.h"

namespace content {
class RenderFrame;
}

namespace extensions {

using DisplaySourceAuthInfo = api::display_source::AuthenticationInfo;
using DisplaySourceAuthMethod = api::display_source::AuthenticationMethod;
using DisplaySourceErrorType = api::display_source::ErrorType;

// This class represents a generic display source session interface.
class DisplaySourceSession {
 public:
  using CompletionCallback =
      base::Callback<void(bool success, const std::string& error_description)>;
  using ErrorCallback =
      base::Callback<void(DisplaySourceErrorType error_type,
                          const std::string& error_description)>;

  // State flow is ether:
  // 'Idle' -> 'Establishing' -> 'Established' -> 'Terminating' -> 'Idle'
  // (terminated by Terminate() call)
  //  or
  // 'Idle' -> 'Establishing' -> 'Established' -> 'Idle'
  // (terminated from sink device or due to an error)
  enum State {
    Idle,
    Establishing,
    Established,
    Terminating
  };

  virtual ~DisplaySourceSession();

  // Starts the session.
  // The session state should be set to 'Establishing' immediately after this
  // method is called.
  // |callback| : Called with 'success' flag set to 'true' if the session is
  //              successfully started (state should be set to 'Established')
  //
  //              Called with 'success' flag set to 'false' if the session
  //              has failed to start (state should be set back to 'Idle').
  //              The 'error_description' argument contains description of
  //              an error that had caused the call falure.
  virtual void Start(const CompletionCallback& callback) = 0;

  // Terminates the session.
  // The session state should be set to 'Terminating' immediately after this
  // method is called.
  // |callback| : Called with 'success' flag set to 'true' if the session is
  //              successfully terminated (state should be set to 'Idle')
  //
  //              Called with 'success' flag set to 'false' if the session
  //              could not terminate (state should not be modified).
  //              The 'error_description' argument contains description of
  //              an error that had caused the call falure.
  virtual void Terminate(const CompletionCallback& callback) = 0;

  State state() const { return state_; }

  // Sets the callbacks invoked to inform about the session's state changes.
  // It is required to set the callbacks before the session is started.
  // |terminated_callback| : Called when session was terminated (state
  //                         should be set to 'Idle')
  // |error_callback| : Called if a fatal error has occured and the session
  //                    will be terminated soon for emergency reasons.
  void SetNotificationCallbacks(const base::Closure& terminated_callback,
                                const ErrorCallback& error_callback);

 protected:
  DisplaySourceSession();

  State state_;
  base::Closure terminated_callback_;
  ErrorCallback error_callback_;

 private:
  DISALLOW_COPY_AND_ASSIGN(DisplaySourceSession);
};

struct DisplaySourceSessionParams {
  DisplaySourceSessionParams();
  DisplaySourceSessionParams(const DisplaySourceSessionParams&);
  ~DisplaySourceSessionParams();

  int sink_id;
  blink::WebMediaStreamTrack video_track;
  blink::WebMediaStreamTrack audio_track;
  DisplaySourceAuthMethod auth_method;
  std::string auth_data;
  content::RenderFrame* render_frame;
};

class DisplaySourceSessionFactory {
 public:
  static std::unique_ptr<DisplaySourceSession> CreateSession(
      const DisplaySourceSessionParams& params);

 private:
  DISALLOW_COPY_AND_ASSIGN(DisplaySourceSessionFactory);
};

}  // namespace extensions

#endif  // EXTENSIONS_RENDERER_API_DISPLAY_SOURCE_DISPLAY_SOURCE_SESSION_H_
