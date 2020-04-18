// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_NTP_SNIPPETS_BREAKING_NEWS_BREAKING_NEWS_LISTENER_H_
#define COMPONENTS_NTP_SNIPPETS_BREAKING_NEWS_BREAKING_NEWS_LISTENER_H_

#include <vector>

#include "base/callback_forward.h"
#include "components/ntp_snippets/remote/json_to_categories.h"

namespace ntp_snippets {

// Listens to pushed breaking news and refresh requests from the content
// suggestion servers.
class BreakingNewsListener {
 public:
  using OnNewRemoteSuggestionCallback =
      base::Callback<void(std::unique_ptr<RemoteSuggestion> remote_suggestion)>;
  using OnRefreshRequestedCallback = base::Callback<void()>;

  virtual ~BreakingNewsListener() = default;

  // Subscribe to the breaking news service and start listening for pushed
  // messages from the content suggestion server. Must not be called if already
  // listening.
  virtual void StartListening(
      OnNewRemoteSuggestionCallback on_new_remote_suggestion_callback,
      OnRefreshRequestedCallback on_refresh_requested_callback) = 0;

  // Stop listening for pushed messages from the content suggestion server. Any
  // further pushed messages will be ignored. Must be called while listening.
  virtual void StopListening() = 0;

  virtual bool IsListening() const = 0;
};

}  // namespace ntp_snippets

#endif  // COMPONENTS_NTP_SNIPPETS_BREAKING_NEWS_BREAKING_NEWS_LISTENER_H_
