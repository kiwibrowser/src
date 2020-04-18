// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/quirks/quirks_manager.h"

#include <utility>

#include "base/files/file_util.h"
#include "base/format_macros.h"
#include "base/memory/ptr_util.h"
#include "base/path_service.h"
#include "base/strings/stringprintf.h"
#include "base/task_runner.h"
#include "base/task_runner_util.h"
#include "base/task_scheduler/post_task.h"
#include "base/task_scheduler/task_traits.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "components/quirks/pref_names.h"
#include "components/quirks/quirks_client.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_request_context_getter.h"
#include "url/gurl.h"

namespace quirks {

namespace {

QuirksManager* manager_ = nullptr;

const char kIccExtension[] = ".icc";

// How often we query Quirks Server.
const int kDaysBetweenServerChecks = 30;

// Check if QuirksClient has already downloaded icc file from server.
base::FilePath CheckForIccFile(const base::FilePath& path) {
  const bool exists = base::PathExists(path);
  VLOG(1) << (exists ? "File" : "No File") << " found at " << path.value();
  // TODO(glevin): If file exists, do we want to implement a hash to verify that
  // the file hasn't been corrupted or tampered with?
  return exists ? path : base::FilePath();
}

}  // namespace

std::string IdToHexString(int64_t product_id) {
  return base::StringPrintf("%08" PRIx64, product_id);
}

std::string IdToFileName(int64_t product_id) {
  return IdToHexString(product_id).append(kIccExtension);
}

////////////////////////////////////////////////////////////////////////////////
// QuirksManager

QuirksManager::QuirksManager(
    std::unique_ptr<Delegate> delegate,
    PrefService* local_state,
    scoped_refptr<net::URLRequestContextGetter> url_context_getter)
    : waiting_for_login_(true),
      delegate_(std::move(delegate)),
      task_runner_(base::CreateTaskRunnerWithTraits({base::MayBlock()})),
      local_state_(local_state),
      url_context_getter_(url_context_getter),
      weak_ptr_factory_(this) {}

QuirksManager::~QuirksManager() {
  clients_.clear();
  manager_ = nullptr;
}

// static
void QuirksManager::Initialize(
    std::unique_ptr<Delegate> delegate,
    PrefService* local_state,
    scoped_refptr<net::URLRequestContextGetter> url_context_getter) {
  manager_ =
      new QuirksManager(std::move(delegate), local_state, url_context_getter);
}

// static
void QuirksManager::Shutdown() {
  delete manager_;
}

// static
QuirksManager* QuirksManager::Get() {
  DCHECK(manager_);
  return manager_;
}

// static
void QuirksManager::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterDictionaryPref(prefs::kQuirksClientLastServerCheck);
}

// Delay downloads until after login, to ensure that device policy has been set.
void QuirksManager::OnLoginCompleted() {
  if (!waiting_for_login_)
    return;

  waiting_for_login_ = false;
  if (!clients_.empty() && !QuirksEnabled()) {
    VLOG(2) << clients_.size() << " client(s) deleted.";
    clients_.clear();
  }

  for (const std::unique_ptr<QuirksClient>& client : clients_)
    client->StartDownload();
}

void QuirksManager::RequestIccProfilePath(
    int64_t product_id,
    const std::string& display_name,
    const RequestFinishedCallback& on_request_finished) {
  DCHECK(thread_checker_.CalledOnValidThread());

  if (!QuirksEnabled()) {
    VLOG(1) << "Quirks Client disabled.";
    on_request_finished.Run(base::FilePath(), false);
    return;
  }

  if (!product_id) {
    VLOG(1) << "Could not determine display information (product id = 0)";
    on_request_finished.Run(base::FilePath(), false);
    return;
  }

  std::string name = IdToFileName(product_id);
  base::PostTaskAndReplyWithResult(
      task_runner_.get(), FROM_HERE,
      base::Bind(&CheckForIccFile,
                 delegate_->GetDisplayProfileDirectory().Append(name)),
      base::Bind(&QuirksManager::OnIccFilePathRequestCompleted,
                 weak_ptr_factory_.GetWeakPtr(), product_id, display_name,
                 on_request_finished));
}

void QuirksManager::ClientFinished(QuirksClient* client) {
  DCHECK(thread_checker_.CalledOnValidThread());
  SetLastServerCheck(client->product_id(), base::Time::Now());
  auto it = std::find_if(clients_.begin(), clients_.end(),
                         [client](const std::unique_ptr<QuirksClient>& c) {
                           return c.get() == client;
                         });
  CHECK(it != clients_.end());
  clients_.erase(it);
}

std::unique_ptr<net::URLFetcher> QuirksManager::CreateURLFetcher(
    const GURL& url,
    net::URLFetcherDelegate* delegate) {
  if (!fake_quirks_fetcher_creator_.is_null())
    return fake_quirks_fetcher_creator_.Run(url, delegate);

  net::NetworkTrafficAnnotationTag traffic_annotation =
      net::DefineNetworkTrafficAnnotation("quirks_display_fetcher", R"(
          semantics {
            sender: "Quirks"
            description: "Download custom display calibration file."
            trigger:
                "Chrome OS attempts to download monitor calibration files on"
                "first device login, and then once every 30 days."
            data: "ICC files to calibrate and improve the quality of a display."
            destination: GOOGLE_OWNED_SERVICE
          }
          policy {
            cookies_allowed: NO
            chrome_policy {
              DeviceQuirksDownloadEnabled {
                  DeviceQuirksDownloadEnabled: false
              }
            }
          }
        )");

  return net::URLFetcher::Create(url, net::URLFetcher::GET, delegate,
                                 traffic_annotation);
}

void QuirksManager::OnIccFilePathRequestCompleted(
    int64_t product_id,
    const std::string& display_name,
    const RequestFinishedCallback& on_request_finished,
    base::FilePath path) {
  DCHECK(thread_checker_.CalledOnValidThread());

  // If we found a file, just inform requester.
  if (!path.empty()) {
    on_request_finished.Run(path, false);
    // TODO(glevin): If Quirks files are ever modified on the server, we'll need
    // to modify this logic to check for updates. See crbug.com/595024.
    return;
  }

  double last_check = 0.0;
  local_state_->GetDictionary(prefs::kQuirksClientLastServerCheck)
      ->GetDouble(IdToHexString(product_id), &last_check);

  const base::TimeDelta time_since =
      base::Time::Now() - base::Time::FromDoubleT(last_check);

  // Don't need server check if we've checked within last 30 days.
  if (time_since < base::TimeDelta::FromDays(kDaysBetweenServerChecks)) {
    VLOG(2) << time_since.InDays()
            << " days since last Quirks Server check for display "
            << IdToHexString(product_id);
    on_request_finished.Run(base::FilePath(), false);
    return;
  }

  // Create and start a client to download file.
  QuirksClient* client =
      new QuirksClient(product_id, display_name, on_request_finished, this);
  clients_.insert(base::WrapUnique(client));
  if (!waiting_for_login_)
    client->StartDownload();
  else
    VLOG(2) << "Quirks Client created; waiting for login to begin download.";
}

bool QuirksManager::QuirksEnabled() {
  if (!delegate_->DevicePolicyEnabled()) {
    VLOG(2) << "Quirks Client disabled by device policy.";
    return false;
  }
  return true;
}

void QuirksManager::SetLastServerCheck(int64_t product_id,
                                       const base::Time& last_check) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DictionaryPrefUpdate dict(local_state_, prefs::kQuirksClientLastServerCheck);
  dict->SetDouble(IdToHexString(product_id), last_check.ToDoubleT());
}

}  // namespace quirks
