// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_COMMON_PUSH_EVENT_PAYLOAD_H_
#define CONTENT_PUBLIC_COMMON_PUSH_EVENT_PAYLOAD_H_

#include <string>

#include "content/common/content_export.h"

namespace content {

// Structure representing the payload delivered as part of a push message.
// This struct contains the decrypted information sent from the push
// service as part of a PushEvent as well as metadata about the information.
struct CONTENT_EXPORT PushEventPayload {
  PushEventPayload() : is_null(true) {}
  ~PushEventPayload() {}

  // Method to both set the data string and update the null status.
  void setData(const std::string& data_in) {
    data = data_in;
    is_null = false;
  }

  // Data contained in the payload.
  std::string data;

  // Whether the payload is null or not. Payloads can be valid with non-empty
  // content, valid with empty content, or null.
  bool is_null;
};

}  // namespace content

#endif  // CONTENT_PUBLIC_COMMON_PUSH_EVENT_PAYLOAD_H_
