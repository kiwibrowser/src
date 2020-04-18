// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_COMMON_PRESENTATION_CONNECTION_MESSAGE_H_
#define CONTENT_PUBLIC_COMMON_PRESENTATION_CONNECTION_MESSAGE_H_

#include <stddef.h>  // For size_t
#include <stdint.h>

#include <string>
#include <vector>

#include "base/optional.h"
#include "content/common/content_export.h"

namespace content {

// Represents a presentation connection message.  If this is a text message,
// |data| is null; otherwise, |message| is null.  Empty messages are allowed.
struct CONTENT_EXPORT PresentationConnectionMessage {
 public:
  // Constructs a new, untyped message (for Mojo).  These messages are not valid
  // and exactly one of |message| or |data| must be set.
  PresentationConnectionMessage();
  // Copy constructor / assignment are necessary due to MediaRouter allowing
  // multiple RouteMessageObserver instances per MediaRoute.
  PresentationConnectionMessage(const PresentationConnectionMessage& other);
  PresentationConnectionMessage(PresentationConnectionMessage&& other) noexcept;
  // Constructs a text message from |message|.
  explicit PresentationConnectionMessage(std::string message);
  // Constructs a binary message from |data|.
  explicit PresentationConnectionMessage(std::vector<uint8_t> data);

  ~PresentationConnectionMessage();

  bool is_binary() const;

  bool operator==(const PresentationConnectionMessage& other) const;
  PresentationConnectionMessage& operator=(
      const PresentationConnectionMessage& other);
  PresentationConnectionMessage& operator=(
      PresentationConnectionMessage&& other);

  base::Optional<std::string> message;
  base::Optional<std::vector<uint8_t>> data;
};

}  // namespace content

#endif  // CONTENT_PUBLIC_COMMON_PRESENTATION_CONNECTION_MESSAGE_H_
