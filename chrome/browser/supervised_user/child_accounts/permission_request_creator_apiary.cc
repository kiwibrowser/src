// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/supervised_user/child_accounts/permission_request_creator_apiary.h"

#include "base/callback.h"
#include "base/command_line.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/strings/stringprintf.h"
#include "base/values.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/signin/profile_oauth2_token_service_factory.h"
#include "chrome/browser/signin/signin_manager_factory.h"
#include "chrome/browser/supervised_user/child_accounts/kids_management_api.h"
#include "chrome/browser/supervised_user/supervised_user_constants.h"
#include "chrome/common/chrome_switches.h"
#include "components/data_use_measurement/core/data_use_user_data.h"
#include "components/signin/core/browser/profile_oauth2_token_service.h"
#include "components/signin/core/browser/signin_manager.h"
#include "components/signin/core/browser/signin_manager_base.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/storage_partition.h"
#include "google_apis/gaia/google_service_auth_error.h"
#include "net/base/load_flags.h"
#include "net/base/net_errors.h"
#include "net/http/http_status_code.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "url/gurl.h"

const char kPermissionRequestApiPath[] = "people/me/permissionRequests";
const char kPermissionRequestApiScope[] =
    "https://www.googleapis.com/auth/kid.permission";

const int kNumPermissionRequestRetries = 1;

// Request keys.
const char kEventTypeKey[] = "eventType";
const char kObjectRefKey[] = "objectRef";
const char kStateKey[] = "state";

// Request values.
const char kEventTypeURLRequest[] = "PERMISSION_CHROME_URL";
const char kEventTypeInstallRequest[] = "PERMISSION_CHROME_CWS_ITEM_INSTALL";
const char kEventTypeUpdateRequest[] = "PERMISSION_CHROME_CWS_ITEM_UPDATE";
const char kState[] = "PENDING";

// Response keys.
const char kPermissionRequestKey[] = "permissionRequest";
const char kIdKey[] = "id";

struct PermissionRequestCreatorApiary::Request {
  Request(const std::string& request_type,
          const std::string& object_ref,
          SuccessCallback callback);
  ~Request();

  std::string request_type;
  std::string object_ref;
  SuccessCallback callback;
  std::unique_ptr<OAuth2TokenService::Request> access_token_request;
  std::string access_token;
  bool access_token_expired;
  std::unique_ptr<network::SimpleURLLoader> simple_url_loader;
};

PermissionRequestCreatorApiary::Request::Request(
    const std::string& request_type,
    const std::string& object_ref,
    SuccessCallback callback)
    : request_type(request_type),
      object_ref(object_ref),
      callback(std::move(callback)),
      access_token_expired(false) {}

PermissionRequestCreatorApiary::Request::~Request() {}

PermissionRequestCreatorApiary::PermissionRequestCreatorApiary(
    OAuth2TokenService* oauth2_token_service,
    const std::string& account_id,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : OAuth2TokenService::Consumer("permissions_creator"),
      oauth2_token_service_(oauth2_token_service),
      account_id_(account_id),
      url_loader_factory_(std::move(url_loader_factory)),
      retry_on_network_change_(true) {}

PermissionRequestCreatorApiary::~PermissionRequestCreatorApiary() {}

// static
std::unique_ptr<PermissionRequestCreator>
PermissionRequestCreatorApiary::CreateWithProfile(Profile* profile) {
  ProfileOAuth2TokenService* token_service =
      ProfileOAuth2TokenServiceFactory::GetForProfile(profile);
  SigninManagerBase* signin = SigninManagerFactory::GetForProfile(profile);
  return base::WrapUnique(new PermissionRequestCreatorApiary(
      token_service, signin->GetAuthenticatedAccountId(),
      content::BrowserContext::GetDefaultStoragePartition(profile)
          ->GetURLLoaderFactoryForBrowserProcess()));
}

bool PermissionRequestCreatorApiary::IsEnabled() const {
  return true;
}

void PermissionRequestCreatorApiary::CreateURLAccessRequest(
    const GURL& url_requested,
    SuccessCallback callback) {
  CreateRequest(kEventTypeURLRequest, url_requested.spec(),
                std::move(callback));
}

void PermissionRequestCreatorApiary::CreateExtensionInstallRequest(
    const std::string& id,
    SuccessCallback callback) {
  CreateRequest(kEventTypeInstallRequest, id, std::move(callback));
}

void PermissionRequestCreatorApiary::CreateExtensionUpdateRequest(
    const std::string& id,
    SuccessCallback callback) {
  CreateRequest(kEventTypeUpdateRequest, id, std::move(callback));
}

GURL PermissionRequestCreatorApiary::GetApiUrl() const {
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kPermissionRequestApiUrl)) {
    GURL url(base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
        switches::kPermissionRequestApiUrl));
    LOG_IF(WARNING, !url.is_valid())
        << "Got invalid URL for " << switches::kPermissionRequestApiUrl;
    return url;
  }

  return kids_management_api::GetURL(kPermissionRequestApiPath);
}

std::string PermissionRequestCreatorApiary::GetApiScope() const {
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kPermissionRequestApiScope)) {
    return base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
        switches::kPermissionRequestApiScope);
  } else {
    return kPermissionRequestApiScope;
  }
}

void PermissionRequestCreatorApiary::CreateRequest(
    const std::string& request_type,
    const std::string& object_ref,
    SuccessCallback callback) {
  requests_.push_back(
      std::make_unique<Request>(request_type, object_ref, std::move(callback)));
  StartFetching(requests_.back().get());
}

void PermissionRequestCreatorApiary::StartFetching(Request* request) {
  OAuth2TokenService::ScopeSet scopes;
  scopes.insert(GetApiScope());
  request->access_token_request = oauth2_token_service_->StartRequest(
      account_id_, scopes, this);
}

void PermissionRequestCreatorApiary::OnGetTokenSuccess(
    const OAuth2TokenService::Request* request,
    const std::string& access_token,
    const base::Time& expiration_time) {
  auto it = requests_.begin();
  while (it != requests_.end()) {
    if (request == (*it)->access_token_request.get())
      break;
    ++it;
  }
  DCHECK(it != requests_.end());
  (*it)->access_token = access_token;

  net::NetworkTrafficAnnotationTag traffic_annotation =
      net::DefineNetworkTrafficAnnotation("permission_request_creator", R"(
        semantics {
          sender: "Supervised Users"
          description:
            "Requests permission for the user to access a blocked site."
          trigger: "Initiated by the user."
          data:
            "The request is authenticated with an OAuth2 access token "
            "identifying the Google account and contains the URL that the user "
            "requests access to."
          destination: GOOGLE_OWNED_SERVICE
        }
        policy {
          cookies_allowed: NO
          setting:
            "This feature cannot be disabled in settings and is only enabled "
            "for child accounts. If sign-in is restricted to accounts from a "
            "managed domain, those accounts are not going to be child accounts."
          chrome_policy {
            RestrictSigninToPattern {
              policy_options {mode: MANDATORY}
              RestrictSigninToPattern: "*@manageddomain.com"
            }
          }
        })");

  base::DictionaryValue dict;
  dict.SetKey(kEventTypeKey, base::Value((*it)->request_type));
  dict.SetKey(kObjectRefKey, base::Value((*it)->object_ref));
  dict.SetKey(kStateKey, base::Value(kState));
  std::string body;
  base::JSONWriter::Write(dict, &body);

  auto resource_request = std::make_unique<network::ResourceRequest>();
  resource_request->url = GetApiUrl();
  resource_request->load_flags =
      net::LOAD_DO_NOT_SEND_COOKIES | net::LOAD_DO_NOT_SAVE_COOKIES;
  resource_request->method = "POST";
  resource_request->headers.AddHeaderFromString(base::StringPrintf(
      supervised_users::kAuthorizationHeaderFormat, access_token.c_str()));
  (*it)->simple_url_loader = network::SimpleURLLoader::Create(
      std::move(resource_request), traffic_annotation);
  (*it)->simple_url_loader->AttachStringForUpload(body, "application/json");
  if (retry_on_network_change_) {
    (*it)->simple_url_loader->SetRetryOptions(
        kNumPermissionRequestRetries,
        network::SimpleURLLoader::RETRY_ON_NETWORK_CHANGE);
  }
  // TODO(https://crbug.com/808498): Re-add data use measurement once
  // SimpleURLLoader supports it.
  // ID=data_use_measurement::DataUseUserData::SUPERVISED_USER
  (*it)->simple_url_loader->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      url_loader_factory_.get(),
      base::BindOnce(&PermissionRequestCreatorApiary::OnSimpleLoaderComplete,
                     base::Unretained(this), std::move(it)));
}

void PermissionRequestCreatorApiary::OnGetTokenFailure(
    const OAuth2TokenService::Request* request,
    const GoogleServiceAuthError& error) {
  auto it = requests_.begin();
  while (it != requests_.end()) {
    if (request == (*it)->access_token_request.get())
      break;
    ++it;
  }
  DCHECK(it != requests_.end());
  LOG(WARNING) << "Token error: " << error.ToString();
  DispatchResult(it, false);
}

void PermissionRequestCreatorApiary::OnSimpleLoaderComplete(
    RequestList::iterator it,
    std::unique_ptr<std::string> response_body) {
  Request* request = it->get();
  std::unique_ptr<network::SimpleURLLoader> simple_url_loader =
      std::move(request->simple_url_loader);
  int net_error = simple_url_loader->NetError();
  int response_code = -1;
  if (simple_url_loader->ResponseInfo() &&
      simple_url_loader->ResponseInfo()->headers) {
    response_code = simple_url_loader->ResponseInfo()->headers->response_code();
  }

  if (response_code == net::HTTP_UNAUTHORIZED &&
      !request->access_token_expired) {
    request->access_token_expired = true;
    OAuth2TokenService::ScopeSet scopes;
    scopes.insert(GetApiScope());
    oauth2_token_service_->InvalidateAccessToken(account_id_, scopes,
                                                 request->access_token);
    StartFetching(request);
    return;
  }

  if (response_code != net::HTTP_OK) {
    LOG(WARNING) << "HTTP error " << response_code;
    DispatchResult(std::move(it), false);
    return;
  }

  if (net_error != net::OK) {
    LOG(WARNING) << "Network error " << net_error;
    DispatchResult(std::move(it), false);
    return;
  }

  std::string body;
  if (response_body)
    body = std::move(*response_body);

  std::unique_ptr<base::Value> value = base::JSONReader::Read(body);
  base::DictionaryValue* dict = NULL;
  if (!value || !value->GetAsDictionary(&dict)) {
    LOG(WARNING) << "Invalid top-level dictionary";
    DispatchResult(std::move(it), false);
    return;
  }
  base::DictionaryValue* permission_dict = NULL;
  if (!dict->GetDictionary(kPermissionRequestKey, &permission_dict)) {
    LOG(WARNING) << "Permission request not found";
    DispatchResult(std::move(it), false);
    return;
  }
  std::string id;
  if (!permission_dict->GetString(kIdKey, &id)) {
    LOG(WARNING) << "ID not found";
    DispatchResult(std::move(it), false);
    return;
  }
  DispatchResult(std::move(it), true);
}

void PermissionRequestCreatorApiary::DispatchResult(RequestList::iterator it,
                                                    bool success) {
  std::move((*it)->callback).Run(success);
  requests_.erase(it);
}
