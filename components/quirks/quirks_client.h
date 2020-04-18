// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_QUIRKS_QUIRKS_CLIENT_H_
#define COMPONENTS_QUIRKS_QUIRKS_CLIENT_H_

#include <memory>

#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/threading/thread_checker.h"
#include "base/timer/timer.h"
#include "net/base/backoff_entry.h"
#include "net/url_request/url_fetcher_delegate.h"

namespace quirks {

class QuirksManager;

// See declaration in quirks_manager.h.
using RequestFinishedCallback =
    base::Callback<void(const base::FilePath&, bool)>;

// Handles downloading icc and other display data files from Quirks Server.
class QuirksClient : public net::URLFetcherDelegate {
 public:
  QuirksClient(int64_t product_id,
               const std::string& display_name,
               const RequestFinishedCallback& on_request_finished,
               QuirksManager* manager);
  ~QuirksClient() override;

  void StartDownload();

  int64_t product_id() const { return product_id_; }

 private:
  // net::URLFetcherDelegate:
  void OnURLFetchComplete(const net::URLFetcher* source) override;

  // Send callback and tell manager to delete |this|.
  void Shutdown(bool success);

  // Schedules a retry.
  void Retry();

  // Translates json with base64-encoded data (|result|) into raw |data|.
  bool ParseResult(const std::string& result, std::string* data);

  // ID of display to request from Quirks Server.
  const int64_t product_id_;

  // Human-readable name to send to Quirks Server.
  const std::string display_name_;

  // Callback supplied by caller.
  const RequestFinishedCallback on_request_finished_;

  // Weak pointer owned by manager, guaranteed to outlive this client object.
  QuirksManager* manager_;

  // Full path to icc file.
  const base::FilePath icc_path_;

  // The class is expected to run on UI thread.
  base::ThreadChecker thread_checker_;

  // This fetcher is used to download icc file.
  std::unique_ptr<net::URLFetcher> url_fetcher_;

  // Pending retry.
  base::OneShotTimer request_scheduled_;

  // Controls exponential backoff of time between server checks.
  net::BackoffEntry backoff_entry_;

  // Factory for callbacks.
  base::WeakPtrFactory<QuirksClient> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(QuirksClient);
};

}  // namespace quirks

#endif  // COMPONENTS_QUIRKS_QUIRKS_CLIENT_H_
