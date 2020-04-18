// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_ROUTER_PROVIDERS_DIAL_DIAL_INTERNAL_MESSAGE_UTIL_H_
#define CHROME_BROWSER_MEDIA_ROUTER_PROVIDERS_DIAL_DIAL_INTERNAL_MESSAGE_UTIL_H_

#include <memory>
#include <string>

#include "base/values.h"
#include "chrome/browser/media/router/discovery/dial/parsed_dial_app_info.h"
#include "content/public/common/presentation_connection_message.h"

namespace media_router {

struct DialLaunchInfo;
class MediaSinkInternal;

// Types of internal messages that are used in a custom DIAL launch workflow.
enum class DialInternalMessageType {
  // Cast SDK -> MR
  kClientConnect,
  kV2Message,

  // MR -> Cast SDK
  kNewSession,
  kReceiverAction,

  // MR <-> Cast SDK
  kCustomDialLaunch,

  kOther
};

// Possible types of ReceiverAction taken by the user on a receiver.
enum class DialReceiverAction {
  // The user selected a receiver with the intent of casting to it with the
  // sender application.
  kCast,

  // The user requested to stop the session running on a receiver.
  kStop
};

// Parsed custom DIAL launch internal message coming from a Cast SDK client.
struct DialInternalMessage {
  // Returns a DialInternalMessage for |message|, or nullptr is |message| is not
  // a valid custom DIAL launch internal message.
  static std::unique_ptr<DialInternalMessage> From(const std::string& message);

  DialInternalMessage(DialInternalMessageType type,
                      base::Optional<base::Value> body,
                      const std::string& client_id,
                      int sequence_number);
  ~DialInternalMessage();

  DialInternalMessageType type;
  base::Optional<base::Value> body;
  std::string client_id;
  int sequence_number;

  DISALLOW_COPY_AND_ASSIGN(DialInternalMessage);
};

// Parsed CUSTOM_DIAL_LAUNCH response from the Cast SDK client.
struct CustomDialLaunchMessageBody {
  // Returns a CustomDialLaunchMessageBody for |message|.
  // This method is only valid to call if |message.type| == |kCustomDialLaunch|.
  static CustomDialLaunchMessageBody From(const DialInternalMessage& message);

  CustomDialLaunchMessageBody();
  CustomDialLaunchMessageBody(
      bool do_launch,
      const base::Optional<std::string>& launch_parameter);
  CustomDialLaunchMessageBody(const CustomDialLaunchMessageBody& other);
  ~CustomDialLaunchMessageBody();

  // If |true|, the DialMediaRouteProvider should handle the app launch.
  bool do_launch = true;

  // If |do_launch| is |true|, optional launch parameter to include with the
  // launch (POST) request. This overrides the launch parameter that was
  // specified in the MediaSource (if any).
  base::Optional<std::string> launch_parameter;
};

class DialInternalMessageUtil {
 public:
  // Returns |true| if |message| is a valid STOP_SESSION message.
  static bool IsStopSessionMessage(const DialInternalMessage& message);

  // Returns a NEW_SESSION message to be sent to the page when the user requests
  // an app launch.
  static content::PresentationConnectionMessage CreateNewSessionMessage(
      const DialLaunchInfo& launch_info,
      const MediaSinkInternal& sink);

  // Returns a RECEIVER_ACTION / CAST message to be sent to the page when the
  // user requests an app launch.
  static content::PresentationConnectionMessage CreateReceiverActionCastMessage(
      const DialLaunchInfo& launch_info,
      const MediaSinkInternal& sink);

  // Returns a RECEIVER_ACTION / STOP message to be sent to the page when an app
  // is stopped by DialMediaRouteProvider.
  static content::PresentationConnectionMessage CreateReceiverActionStopMessage(
      const DialLaunchInfo& launch_info,
      const MediaSinkInternal& sink);

  // Returns a CUSTOM_DIAL_LAUNCH request message to be sent to the page.
  // Generates and returns the next number to associate a DIAL launch sequence
  // with.
  static std::pair<content::PresentationConnectionMessage, int>
  CreateCustomDialLaunchMessage(const DialLaunchInfo& launch_info,
                                const MediaSinkInternal& sink,
                                const ParsedDialAppInfo& app_info);
};

}  // namespace media_router

#endif  // CHROME_BROWSER_MEDIA_ROUTER_PROVIDERS_DIAL_DIAL_INTERNAL_MESSAGE_UTIL_H_
