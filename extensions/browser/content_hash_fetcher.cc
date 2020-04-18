// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/content_hash_fetcher.h"

#include <stddef.h>

#include <algorithm>
#include <memory>
#include <vector>

#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/macros.h"
#include "base/sequenced_task_runner.h"
#include "base/task_scheduler/post_task.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "content/public/browser/browser_thread.h"
#include "extensions/browser/content_verifier/content_hash.h"
#include "extensions/browser/content_verifier_delegate.h"
#include "extensions/browser/extension_file_task_runner.h"
#include "extensions/browser/verified_contents.h"
#include "extensions/common/extension.h"
#include "extensions/common/file_util.h"
#include "net/base/load_flags.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_request_status.h"

namespace extensions {

namespace internals {

ContentHashFetcher::ContentHashFetcher(
    const ContentHash::ExtensionKey& key,
    const ContentHash::FetchParams& fetch_params)
    : extension_key_(key),
      fetch_params_(fetch_params),
      response_task_runner_(base::SequencedTaskRunnerHandle::Get()) {}

void ContentHashFetcher::OnURLFetchComplete(const net::URLFetcher* source) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  VLOG(1) << "URLFetchComplete for " << extension_key_.extension_id
          << " is_success:" << url_fetcher_->GetStatus().is_success() << " "
          << fetch_params_.fetch_url.possibly_invalid_spec();
  DCHECK(hash_fetcher_callback_);
  std::unique_ptr<net::URLFetcher> url_fetcher = std::move(url_fetcher_);
  std::unique_ptr<std::string> response(new std::string);
  if (!url_fetcher->GetStatus().is_success() ||
      !url_fetcher->GetResponseAsString(response.get())) {
    // Failed.
    response = nullptr;
  }
  response_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(std::move(hash_fetcher_callback_), extension_key_,
                     fetch_params_, std::move(response)));
  delete this;
}

void ContentHashFetcher::Start(HashFetcherCallback hash_fetcher_callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  hash_fetcher_callback_ = std::move(hash_fetcher_callback);

  net::NetworkTrafficAnnotationTag traffic_annotation =
      net::DefineNetworkTrafficAnnotation("content_hash_verification_job", R"(
        semantics {
          sender: "Web Store Content Verification"
          description:
            "The request sent to retrieve the file required for content "
            "verification for an extension from the Web Store."
          trigger:
            "An extension from the Web Store is missing the "
            "verified_contents.json file required for extension content "
            "verification."
          data: "The extension id and extension version."
          destination: GOOGLE_OWNED_SERVICE
        }
        policy {
          cookies_allowed: NO
          setting:
            "This feature cannot be directly disabled; it is enabled if any "
            "extension from the webstore is installed in the browser."
          policy_exception_justification:
            "Not implemented, not required. If the user has extensions from "
            "the Web Store, this feature is required to ensure the "
            "extensions match what is distributed by the store."
        })");
  url_fetcher_ = net::URLFetcher::Create(
      fetch_params_.fetch_url, net::URLFetcher::GET, this, traffic_annotation);
  url_fetcher_->SetRequestContext(fetch_params_.request_context);
  url_fetcher_->SetLoadFlags(net::LOAD_DO_NOT_SEND_COOKIES |
                             net::LOAD_DO_NOT_SAVE_COOKIES |
                             net::LOAD_DISABLE_CACHE);
  url_fetcher_->SetAutomaticallyRetryOnNetworkChanges(3);
  url_fetcher_->Start();
}

ContentHashFetcher::~ContentHashFetcher() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

}  // namespace internals
}  // namespace extensions
