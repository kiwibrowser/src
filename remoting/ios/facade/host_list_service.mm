// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

#import "remoting/ios/facade/host_list_service.h"

#import <CoreFoundation/CoreFoundation.h>

#import "remoting/ios/domain/host_info.h"
#import "remoting/ios/domain/user_info.h"
#import "remoting/ios/facade/host_info.h"
#import "remoting/ios/facade/host_list_fetcher.h"
#import "remoting/ios/facade/remoting_authentication.h"
#import "remoting/ios/facade/remoting_service.h"

#include "base/i18n/time_formatting.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "net/url_request/url_fetcher.h"
#include "remoting/base/string_resources.h"
#include "ui/base/l10n/l10n_util.h"

namespace remoting {

namespace {

bool IsValidErrorCode(int error_code) {
#define HTTP_STATUS(label, code, reason) \
  if (error_code == code)                \
    return true;
#include "net/http/http_status_code_list.h"
#undef HTTP_STATUS
  return false;
}

std::string GetRequestErrorMessage(int error_code) {
  if (IsValidErrorCode(error_code)) {
    std::string error_phrase =
        net::GetHttpReasonPhrase(static_cast<net::HttpStatusCode>(error_code));
    return l10n_util::GetStringFUTF8(IDS_SERVER_COMMUNICATION_ERROR,
                                     base::UTF8ToUTF16(error_phrase));
  }

  switch (error_code) {
    case net::URLFetcher::RESPONSE_CODE_INVALID:
      return l10n_util::GetStringUTF8(IDS_ERROR_NETWORK_ERROR);
    default:
      return l10n_util::GetStringFUTF8(IDS_SERVER_COMMUNICATION_ERROR,
                                       base::IntToString16(error_code));
  }
}

}  // namespace

HostListService* HostListService::GetInstance() {
  static base::NoDestructor<HostListService> instance;
  return instance.get();
}

HostListService::HostListService() : weak_factory_(this) {
  auto weak_this = weak_factory_.GetWeakPtr();
  user_update_observer_ = [NSNotificationCenter.defaultCenter
      addObserverForName:kUserDidUpdate
                  object:nil
                   queue:nil
              usingBlock:^(NSNotification* notification) {
                UserInfo* user = notification.userInfo[kUserInfo];
                if (weak_this) {
                  weak_this->OnUserUpdated(user != nil);
                }
              }];
}

HostListService::~HostListService() {
  [NSNotificationCenter.defaultCenter removeObserver:user_update_observer_];
}

std::unique_ptr<HostListService::CallbackSubscription>
HostListService::RegisterHostListStateCallback(
    const base::RepeatingClosure& callback) {
  return host_list_state_callbacks_.Add(callback);
}

std::unique_ptr<HostListService::CallbackSubscription>
HostListService::RegisterFetchFailureCallback(
    const base::RepeatingClosure& callback) {
  return fetch_failure_callbacks_.Add(callback);
}

void HostListService::RequestFetch() {
  auto weak_this = weak_factory_.GetWeakPtr();
  [RemotingService.instance.authentication
      callbackWithAccessToken:^(RemotingAuthenticationStatus status,
                                NSString* userEmail, NSString* accessToken) {
        if (status == RemotingAuthenticationStatusSuccess) {
          if (weak_this) {
            weak_this->StartHostListFetch(base::SysNSStringToUTF8(accessToken));
          }
          return;
        }

        FetchFailureReason failureReason;
        switch (status) {
          case RemotingAuthenticationStatusNetworkError:
            failureReason = FetchFailureReason::NETWORK_ERROR;
            break;
          case RemotingAuthenticationStatusAuthError:
            failureReason = FetchFailureReason::AUTH_ERROR;
            break;
          default:
            NOTREACHED();
            failureReason = FetchFailureReason::NETWORK_ERROR;
        }
        if (weak_this) {
          weak_this->HandleFetchFailure(failureReason, 0);
        }
      }];
}

void HostListService::SetHostListFetcherForTesting(
    std::unique_ptr<HostListFetcher> fetcher) {
  host_list_fetcher_ = std::move(fetcher);
}

// static
std::unique_ptr<HostListService> HostListService::CreateInstanceForTesting() {
  return std::make_unique<HostListService>();
}

void HostListService::SetState(State state) {
  if (state == state_) {
    return;
  }
  if (state == State::NOT_FETCHED) {
    hosts_ = {};
  } else if (state == State::FETCHING || state == State::FETCHED) {
    last_fetch_failure_.reset();
  }
  state_ = state;
  host_list_state_callbacks_.Notify();
}

void HostListService::StartHostListFetch(const std::string& access_token) {
  if (state_ == State::FETCHING) {
    return;
  }
  SetState(State::FETCHING);
  if (!host_list_fetcher_) {
    host_list_fetcher_.reset(new HostListFetcher(
        ChromotingClientRuntime::GetInstance()->url_requester()));
  }
  host_list_fetcher_->RetrieveHostlist(
      access_token, base::BindOnce(&HostListService::HandleHostListResult,
                                   weak_factory_.GetWeakPtr()));
}

void HostListService::HandleHostListResult(
    int responseCode,
    const std::vector<remoting::HostInfo>& hostlist) {
  if (responseCode == net::HTTP_OK) {
    hosts_ = hostlist;
    SetState(State::FETCHED);
    return;
  }

  if (responseCode != HostListFetcher::RESPONSE_CODE_CANCELLED) {
    if (responseCode == net::HTTP_UNAUTHORIZED) {
      [RemotingService.instance.authentication logout];
    } else {
      HandleFetchFailure(FetchFailureReason::REQUEST_ERROR, responseCode);
    }
  }
  SetState(State::NOT_FETCHED);
}

void HostListService::HandleFetchFailure(FetchFailureReason reason,
                                         int error_code) {
  last_fetch_failure_ = std::make_unique<FetchFailureInfo>();
  last_fetch_failure_->reason = reason;
  last_fetch_failure_->error_code = error_code;

  switch (reason) {
    case FetchFailureReason::NETWORK_ERROR:
      last_fetch_failure_->localized_description =
          l10n_util::GetStringUTF8(IDS_ERROR_NETWORK_ERROR);
      break;
    case FetchFailureReason::AUTH_ERROR:
      last_fetch_failure_->localized_description =
          l10n_util::GetStringUTF8(IDS_ERROR_OAUTH_TOKEN_INVALID);
      break;
    case FetchFailureReason::REQUEST_ERROR:
      last_fetch_failure_->localized_description =
          GetRequestErrorMessage(error_code);
      break;
    default:
      NOTREACHED();
  }
  LOG(WARNING) << "Failed to fetch host list: "
               << last_fetch_failure_->localized_description
               << " reason: " << static_cast<int>(reason)
               << ", error_code: " << error_code;
  fetch_failure_callbacks_.Notify();
}

void HostListService::OnUserUpdated(bool is_user_signed_in) {
  if (host_list_fetcher_) {
    host_list_fetcher_->CancelFetch();
  }
  SetState(State::NOT_FETCHED);
  if (is_user_signed_in) {
    RequestFetch();
  }
}

}  // namespace remoting
