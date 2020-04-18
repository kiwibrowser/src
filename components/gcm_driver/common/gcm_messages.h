// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_GCM_DRIVER_COMMON_GCM_MESSAGES_H_
#define COMPONENTS_GCM_DRIVER_COMMON_GCM_MESSAGES_H_

#include <map>
#include <string>

#include "components/gcm_driver/common/gcm_driver_export.h"

namespace gcm {

// Message data consisting of key-value pairs.
typedef std::map<std::string, std::string> MessageData;

// Message to be delivered to the other party.
struct GCM_DRIVER_EXPORT OutgoingMessage {
  OutgoingMessage();
  OutgoingMessage(const OutgoingMessage& other);
  ~OutgoingMessage();

  // Message ID.
  std::string id;
  // In seconds.
  int time_to_live;
  MessageData data;

  static const int kMaximumTTL;
};

// Message being received from the other party.
struct GCM_DRIVER_EXPORT IncomingMessage {
  IncomingMessage();
  IncomingMessage(const IncomingMessage& other);
  ~IncomingMessage();

  MessageData data;
  std::string collapse_key;
  std::string sender_id;
  std::string raw_data;

  // Whether the contents of the message have been decrypted, and are
  // available in |raw_data|.
  bool decrypted;
};

}  // namespace gcm

#endif  // COMPONENTS_GCM_DRIVER_COMMON_GCM_MESSAGES_H_
