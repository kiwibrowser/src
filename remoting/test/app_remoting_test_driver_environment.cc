// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/test/app_remoting_test_driver_environment.h"

#include <map>
#include <string>
#include <vector>

#include "base/bind.h"
#include "base/callback_forward.h"
#include "base/logging.h"
#include "base/message_loop/message_loop.h"
#include "base/message_loop/message_loop_current.h"
#include "base/run_loop.h"
#include "base/strings/stringprintf.h"
#include "remoting/test/access_token_fetcher.h"
#include "remoting/test/app_remoting_report_issue_request.h"
#include "remoting/test/refresh_token_store.h"
#include "remoting/test/remote_host_info.h"

namespace remoting {
namespace test {

AppRemotingTestDriverEnvironment* AppRemotingSharedData;

AppRemotingTestDriverEnvironment::EnvironmentOptions::EnvironmentOptions()
    : refresh_token_file_path(base::FilePath()),
      service_environment(kUnknownEnvironment),
      release_hosts_when_done(false) {}

AppRemotingTestDriverEnvironment::EnvironmentOptions::~EnvironmentOptions() =
    default;

AppRemotingTestDriverEnvironment::AppRemotingTestDriverEnvironment(
    const EnvironmentOptions& options)
    : user_name_(options.user_name),
      service_environment_(options.service_environment),
      release_hosts_when_done_(options.release_hosts_when_done),
      refresh_token_file_path_(options.refresh_token_file_path),
      test_access_token_fetcher_(nullptr),
      test_app_remoting_report_issue_request_(nullptr),
      test_refresh_token_store_(nullptr),
      test_remote_host_info_fetcher_(nullptr) {
  DCHECK(!user_name_.empty());
  DCHECK(service_environment_ < kUnknownEnvironment);
}

AppRemotingTestDriverEnvironment::~AppRemotingTestDriverEnvironment() = default;

bool AppRemotingTestDriverEnvironment::Initialize(
    const std::string& auth_code) {
  if (!access_token_.empty()) {
    return true;
  }

  if (!base::MessageLoopCurrent::Get()) {
    message_loop_.reset(new base::MessageLoopForIO);
  }

  // If a unit test has set |test_refresh_token_store_| then we should use it
  // below.  Note that we do not want to destroy the test object.
  std::unique_ptr<RefreshTokenStore> temporary_refresh_token_store;
  RefreshTokenStore* refresh_token_store = test_refresh_token_store_;
  if (!refresh_token_store) {
    temporary_refresh_token_store =
        RefreshTokenStore::OnDisk(user_name_, refresh_token_file_path_);
    refresh_token_store = temporary_refresh_token_store.get();
  }

  // Check to see if we have a refresh token stored for this user.
  refresh_token_ = refresh_token_store->FetchRefreshToken();
  if (refresh_token_.empty()) {
    // This isn't necessarily an error as this might be a first run scenario.
    VLOG(2) << "No refresh token stored for " << user_name_;

    if (auth_code.empty()) {
      // No token and no Auth code means no service connectivity, bail!
      LOG(ERROR) << "Cannot retrieve an access token without a stored refresh"
                 << " token on disk or an auth_code passed into the tool";
      return false;
    }
  }

  if (!RetrieveAccessToken(auth_code)) {
    // If we cannot retrieve an access token, then nothing is going to work and
    // we should let the caller know that our object is not ready to be used.
    return false;
  }

  return true;
}

bool AppRemotingTestDriverEnvironment::RefreshAccessToken() {
  DCHECK(!refresh_token_.empty());

  // Empty auth code is used when refreshing.
  return RetrieveAccessToken(std::string());
}

bool AppRemotingTestDriverEnvironment::GetRemoteHostInfoForApplicationId(
    const std::string& application_id,
    RemoteHostInfo* remote_host_info) {
  DCHECK(!application_id.empty());
  DCHECK(remote_host_info);

  if (access_token_.empty()) {
    LOG(ERROR) << "RemoteHostInfo requested without a valid access token. "
               << "Ensure the environment object has been initialized.";
    return false;
  }

  base::RunLoop run_loop;

  RemoteHostInfoCallback remote_host_info_fetch_callback = base::Bind(
      &AppRemotingTestDriverEnvironment::OnRemoteHostInfoRetrieved,
      base::Unretained(this), run_loop.QuitClosure(), remote_host_info);

  // If a unit test has set |test_remote_host_info_fetcher_| then we should use
  // it below.  Note that we do not want to destroy the test object at the end
  // of the function which is why we have the dance below.
  std::unique_ptr<RemoteHostInfoFetcher> temporary_remote_host_info_fetcher;
  RemoteHostInfoFetcher* remote_host_info_fetcher =
      test_remote_host_info_fetcher_;
  if (!remote_host_info_fetcher) {
    temporary_remote_host_info_fetcher.reset(new RemoteHostInfoFetcher());
    remote_host_info_fetcher = temporary_remote_host_info_fetcher.get();
  }

  remote_host_info_fetcher->RetrieveRemoteHostInfo(
      application_id, access_token_, service_environment_,
      remote_host_info_fetch_callback);

  run_loop.Run();

  return remote_host_info->IsReadyForConnection();
}

void AppRemotingTestDriverEnvironment::AddHostToReleaseList(
    const std::string& application_id,
    const std::string& host_id) {
  if (!release_hosts_when_done_) {
    return;
  }

  auto map_iterator = host_ids_to_release_.find(application_id);
  if (map_iterator == host_ids_to_release_.end()) {
    std::vector<std::string> host_id_list(1, host_id);
    host_ids_to_release_.insert(std::make_pair(application_id, host_id_list));
  } else {
    std::vector<std::string>* host_ids = &map_iterator->second;
    if (std::find(host_ids->begin(), host_ids->end(), host_id) ==
        host_ids->end()) {
      host_ids->push_back(host_id);
    }
  }
}

void AppRemotingTestDriverEnvironment::ShowHostAvailability() {
  const char kHostAvailabilityFormatString[] = "%-25s%-35s%-10s";

  LOG(INFO) << base::StringPrintf(kHostAvailabilityFormatString,
                                  "Application Name", "Application ID",
                                  "Status");

  for (const auto& application_name : application_names_) {
    const RemoteApplicationDetails& application_details =
        GetDetailsFromAppName(application_name);

    RemoteHostInfo remote_host_info;
    GetRemoteHostInfoForApplicationId(application_details.application_id,
                                      &remote_host_info);

    std::string status;
    RemoteHostStatus remote_host_status = remote_host_info.remote_host_status;
    if (remote_host_status == kRemoteHostStatusReady) {
      status = "Ready :)";
    } else if (remote_host_status == kRemoteHostStatusPending) {
      status = "Pending :|";
    } else {
      status = "Unknown :(";
    }

    LOG(INFO) << base::StringPrintf(
        kHostAvailabilityFormatString, application_name.c_str(),
        application_details.application_id.c_str(), status.c_str());
  }
}

const RemoteApplicationDetails&
AppRemotingTestDriverEnvironment::GetDetailsFromAppName(
    const std::string& application_name) {
  const auto map_pair_iterator =
      application_details_map_.find(application_name);
  DCHECK(map_pair_iterator != application_details_map_.end());

  return map_pair_iterator->second;
}

void AppRemotingTestDriverEnvironment::SetAccessTokenFetcherForTest(
    AccessTokenFetcher* access_token_fetcher) {
  DCHECK(access_token_fetcher);

  test_access_token_fetcher_ = access_token_fetcher;
}

void AppRemotingTestDriverEnvironment::SetAppRemotingReportIssueRequestForTest(
    AppRemotingReportIssueRequest* app_remoting_report_issue_request) {
  DCHECK(app_remoting_report_issue_request);

  test_app_remoting_report_issue_request_ = app_remoting_report_issue_request;
}

void AppRemotingTestDriverEnvironment::SetRefreshTokenStoreForTest(
    RefreshTokenStore* refresh_token_store) {
  DCHECK(refresh_token_store);

  test_refresh_token_store_ = refresh_token_store;
}

void AppRemotingTestDriverEnvironment::SetRemoteHostInfoFetcherForTest(
    RemoteHostInfoFetcher* remote_host_info_fetcher) {
  DCHECK(remote_host_info_fetcher);

  test_remote_host_info_fetcher_ = remote_host_info_fetcher;
}

void AppRemotingTestDriverEnvironment::TearDown() {
  // If a unit test has set |test_app_remoting_report_issue_request_| then we
  // should use it below.  Note that we do not want to destroy the test object
  // at the end of the function which is why we have the dance below.
  std::unique_ptr<AppRemotingReportIssueRequest> temporary_report_issue_request;
  AppRemotingReportIssueRequest* report_issue_request =
      test_app_remoting_report_issue_request_;
  if (!report_issue_request) {
    temporary_report_issue_request.reset(new AppRemotingReportIssueRequest());
    report_issue_request = temporary_report_issue_request.get();
  }

  for (const auto& kvp : host_ids_to_release_) {
    std::string application_id = kvp.first;
    VLOG(1) << "Releasing hosts for application: " << application_id;

    for (const auto& host_id : kvp.second) {
      base::RunLoop run_loop;

      VLOG(1) << "    Releasing host: " << host_id;
      bool request_started = report_issue_request->Start(
          application_id, host_id, access_token_, service_environment_, true,
          run_loop.QuitClosure());

      if (request_started) {
        run_loop.Run();
      } else {
        LOG(ERROR) << "Failed to send ReportIssueRequest for: "
                   << application_id << ", " << host_id;
      }
    }
  }
  temporary_report_issue_request.reset();

  // Letting the MessageLoop tear down during the test destructor results in
  // errors after test completion, when the MessageLoop dtor touches the
  // registered AtExitManager. The AtExitManager is torn down before the test
  // destructor is executed, so we tear down the MessageLoop here, while it is
  // still valid.
  message_loop_.reset();
}

bool AppRemotingTestDriverEnvironment::RetrieveAccessToken(
    const std::string& auth_code) {
  base::RunLoop run_loop;

  access_token_.clear();

  AccessTokenCallback access_token_callback =
      base::Bind(&AppRemotingTestDriverEnvironment::OnAccessTokenRetrieved,
                 base::Unretained(this), run_loop.QuitClosure());

  // If a unit test has set |test_access_token_fetcher_| then we should use it
  // below.  Note that we do not want to destroy the test object at the end of
  // the function which is why we have the dance below.
  std::unique_ptr<AccessTokenFetcher> temporary_access_token_fetcher;
  AccessTokenFetcher* access_token_fetcher = test_access_token_fetcher_;
  if (!access_token_fetcher) {
    temporary_access_token_fetcher.reset(new AccessTokenFetcher());
    access_token_fetcher = temporary_access_token_fetcher.get();
  }

  if (!auth_code.empty()) {
    // If the user passed in an authcode, then use it to retrieve an
    // updated access/refresh token.
    access_token_fetcher->GetAccessTokenFromAuthCode(auth_code,
                                                     access_token_callback);
  } else {
    DCHECK(!refresh_token_.empty());

    access_token_fetcher->GetAccessTokenFromRefreshToken(refresh_token_,
                                                         access_token_callback);
  }

  run_loop.Run();

  // If we were using an auth_code and received a valid refresh token,
  // then we want to store it locally.  If we had an auth code and did not
  // receive a refresh token, then we should let the user know and exit.
  if (!auth_code.empty()) {
    if (!refresh_token_.empty()) {
      // If a unit test has set |test_refresh_token_store_| then we should use
      // it below.  Note that we do not want to destroy the test object.
      std::unique_ptr<RefreshTokenStore> temporary_refresh_token_store;
      RefreshTokenStore* refresh_token_store = test_refresh_token_store_;
      if (!refresh_token_store) {
        temporary_refresh_token_store =
            RefreshTokenStore::OnDisk(user_name_, refresh_token_file_path_);
        refresh_token_store = temporary_refresh_token_store.get();
      }

      if (!refresh_token_store->StoreRefreshToken(refresh_token_)) {
        // If we failed to persist the refresh token, then we should let the
        // user sort out the issue before continuing.
        return false;
      }
    } else {
      LOG(ERROR) << "Failed to use AUTH CODE to retrieve a refresh token.\n"
                 << "Was the one-time use AUTH CODE used more than once?";
      return false;
    }
  }

  if (access_token_.empty()) {
    LOG(ERROR) << "Failed to retrieve access token.";
    return false;
  }

  return true;
}

void AppRemotingTestDriverEnvironment::OnAccessTokenRetrieved(
    base::Closure done_closure,
    const std::string& access_token,
    const std::string& refresh_token) {
  access_token_ = access_token;
  refresh_token_ = refresh_token;

  done_closure.Run();
}

void AppRemotingTestDriverEnvironment::OnRemoteHostInfoRetrieved(
    base::Closure done_closure,
    RemoteHostInfo* remote_host_info,
    const RemoteHostInfo& retrieved_remote_host_info) {
  DCHECK(remote_host_info);

  *remote_host_info = retrieved_remote_host_info;

  done_closure.Run();
}

}  // namespace test
}  // namespace remoting
