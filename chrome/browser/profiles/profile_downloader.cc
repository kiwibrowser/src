// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/profiles/profile_downloader.h"

#include <stddef.h>

#include <string>
#include <vector>

#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_downloader_delegate.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/signin/account_fetcher_service_factory.h"
#include "chrome/browser/signin/account_tracker_service_factory.h"
#include "chrome/browser/signin/chrome_signin_client_factory.h"
#include "chrome/browser/signin/profile_oauth2_token_service_factory.h"
#include "chrome/browser/signin/signin_manager_factory.h"
#include "components/data_use_measurement/core/data_use_user_data.h"
#include "components/signin/core/browser/account_fetcher_service.h"
#include "components/signin/core/browser/avatar_icon_util.h"
#include "components/signin/core/browser/profile_oauth2_token_service.h"
#include "components/signin/core/browser/signin_client.h"
#include "components/signin/core/browser/signin_manager.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/storage_partition.h"
#include "google_apis/gaia/gaia_constants.h"
#include "net/base/load_flags.h"
#include "net/http/http_status_code.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "skia/ext/image_operations.h"
#include "url/gurl.h"

using content::BrowserThread;

namespace {

// Template for optional authorization header when using an OAuth access token.
constexpr char kAuthorizationHeader[] = "Authorization: Bearer %s";

}  // namespace

ProfileDownloader::ProfileDownloader(ProfileDownloaderDelegate* delegate)
    : OAuth2TokenService::Consumer("profile_downloader"),
      delegate_(delegate),
      picture_status_(PICTURE_FAILED),
      account_tracker_service_(
          AccountTrackerServiceFactory::GetForProfile(
              delegate_->GetBrowserProfile())),
      waiting_for_account_info_(false) {
  DCHECK(delegate_);
  account_tracker_service_->AddObserver(this);
}

void ProfileDownloader::Start() {
  StartForAccount(std::string());
}

void ProfileDownloader::StartForAccount(const std::string& account_id) {
  VLOG(1) << "Starting profile downloader...";
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  ProfileOAuth2TokenService* service =
      ProfileOAuth2TokenServiceFactory::GetForProfile(
          delegate_->GetBrowserProfile());
  if (!service) {
    // This can happen in some test paths.
    LOG(WARNING) << "User has no token service";
    delegate_->OnProfileDownloadFailure(
        this, ProfileDownloaderDelegate::TOKEN_ERROR);
    return;
  }

  SigninManagerBase* signin_manager =
      SigninManagerFactory::GetForProfile(delegate_->GetBrowserProfile());
  account_id_ =
      account_id.empty() ?
          signin_manager->GetAuthenticatedAccountId() : account_id;
  if (service->RefreshTokenIsAvailable(account_id_))
    StartFetchingOAuth2AccessToken();
  else
    service->AddObserver(this);
}

base::string16 ProfileDownloader::GetProfileHostedDomain() const {
  return base::UTF8ToUTF16(account_info_.hosted_domain);
}

base::string16 ProfileDownloader::GetProfileFullName() const {
  return base::UTF8ToUTF16(account_info_.full_name);
}

base::string16 ProfileDownloader::GetProfileGivenName() const {
  return base::UTF8ToUTF16(account_info_.given_name);
}

std::string ProfileDownloader::GetProfileLocale() const {
  return account_info_.locale;
}

SkBitmap ProfileDownloader::GetProfilePicture() const {
  return profile_picture_;
}

ProfileDownloader::PictureStatus ProfileDownloader::GetProfilePictureStatus()
    const {
  return picture_status_;
}

std::string ProfileDownloader::GetProfilePictureURL() const {
  GURL url(account_info_.picture_url);
  if (!url.is_valid())
    return std::string();
  return signin::GetAvatarImageURLWithOptions(
             GURL(account_info_.picture_url),
             delegate_->GetDesiredImageSideLength(), true /* no_silhouette */)
      .spec();
}

void ProfileDownloader::StartFetchingImage() {
  VLOG(1) << "Fetching user entry with token: " << auth_token_;
  account_info_ = account_tracker_service_->GetAccountInfo(account_id_);

  if (delegate_->IsPreSignin()) {
    AccountFetcherServiceFactory::GetForProfile(delegate_->GetBrowserProfile())
        ->FetchUserInfoBeforeSignin(account_id_);
  }

  if (account_info_.IsValid()) {
    // FetchImageData might call the delegate's OnProfileDownloadSuccess
    // synchronously, causing |this| to be deleted so there should not be more
    // code after it.
    FetchImageData();
  } else {
    waiting_for_account_info_ = true;
  }
}

void ProfileDownloader::StartFetchingOAuth2AccessToken() {
  Profile* profile = delegate_->GetBrowserProfile();
  OAuth2TokenService::ScopeSet scopes;
  scopes.insert(GaiaConstants::kGoogleUserInfoProfile);
  // Required to determine if lock should be enabled.
  scopes.insert(GaiaConstants::kGoogleUserInfoEmail);
  ProfileOAuth2TokenService* token_service =
      ProfileOAuth2TokenServiceFactory::GetForProfile(profile);
  oauth2_access_token_request_ = token_service->StartRequest(
      account_id_, scopes, this);
}

ProfileDownloader::~ProfileDownloader() {
  // Ensures PO2TS observation is cleared when ProfileDownloader is destructed
  // before refresh token is available.
  ProfileOAuth2TokenService* service =
      ProfileOAuth2TokenServiceFactory::GetForProfile(
          delegate_->GetBrowserProfile());
  if (service)
    service->RemoveObserver(this);

  account_tracker_service_->RemoveObserver(this);
}

void ProfileDownloader::FetchImageData() {
  DCHECK(account_info_.IsValid());

  if (!delegate_->NeedsProfilePicture()) {
    VLOG(1) << "Skipping profile picture download";
    delegate_->OnProfileDownloadSuccess(this);
    return;
  }

  if (account_info_.picture_url == AccountTrackerService::kNoPictureURLFound) {
    VLOG(1) << "No picture URL for account " << account_info_.email
            << ". Using the default profile picture.";
    picture_status_ = PICTURE_DEFAULT;
    delegate_->OnProfileDownloadSuccess(this);
    return;
  }

  std::string image_url_with_size = GetProfilePictureURL();
  if (!image_url_with_size.empty() &&
      image_url_with_size == delegate_->GetCachedPictureURL()) {
    VLOG(1) << "Picture URL matches cached picture URL";
    picture_status_ = PICTURE_CACHED;
    delegate_->OnProfileDownloadSuccess(this);
    return;
  }

  GURL image_url_to_fetch(image_url_with_size);
  if (!image_url_to_fetch.is_valid()) {
    VLOG(1) << "Profile picture URL with size |" << image_url_to_fetch << "| "
            << "is not valid (the account picture URL is "
            << "|" << account_info_.picture_url << "|)";
    delegate_->OnProfileDownloadFailure(
        this,
        ProfileDownloaderDelegate::FailureReason::INVALID_PROFILE_PICTURE_URL);
    return;
  }

  // Create traffic annotation tag.
  net::NetworkTrafficAnnotationTag traffic_annotation =
      net::DefineNetworkTrafficAnnotation("signed_in_profile_avatar", R"(
        semantics {
          sender: "Profile G+ Image Downloader"
          description:
            "Signed in users use their G+ profile image as their Chrome "
            "profile image, unless they explicitly select otherwise. This "
            "fetcher uses the sign-in token and the image URL provided by GAIA "
            "to fetch the image."
          trigger: "User signs into a Profile."
          data: "Filename of the png to download and Google OAuth bearer token."
          destination: GOOGLE_OWNED_SERVICE
        }
        policy {
          cookies_allowed: NO
          setting: "This feature cannot be disabled by settings."
          policy_exception_justification:
            "Not implemented, considered not useful as no content is being "
            "uploaded or saved; this request merely downloads the user's G+ "
            "profile image."
        })");

  VLOG(1) << "Loading profile image from " << image_url_to_fetch;

  auto resource_request = std::make_unique<network::ResourceRequest>();
  resource_request->url = image_url_to_fetch;
  resource_request->load_flags =
      net::LOAD_DO_NOT_SEND_COOKIES | net::LOAD_DO_NOT_SAVE_COOKIES;
  if (!auth_token_.empty()) {
    resource_request->headers.AddHeadersFromString(
        base::StringPrintf(kAuthorizationHeader, auth_token_.c_str()));
  }

  network::mojom::URLLoaderFactory* loader_factory =
      content::BrowserContext::GetDefaultStoragePartition(
          delegate_->GetBrowserProfile())
          ->GetURLLoaderFactoryForBrowserProcess()
          .get();

  simple_loader_ = network::SimpleURLLoader::Create(std::move(resource_request),
                                                    traffic_annotation);
  simple_loader_->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      loader_factory, base::BindOnce(&ProfileDownloader::OnURLLoaderComplete,
                                     base::Unretained(this)));
}

void ProfileDownloader::OnURLLoaderComplete(
    std::unique_ptr<std::string> response_body) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  int response_code = -1;
  if (simple_loader_->ResponseInfo() && simple_loader_->ResponseInfo()->headers)
    response_code = simple_loader_->ResponseInfo()->headers->response_code();

  if (response_body) {
    simple_loader_.reset();
    DVLOG(1) << "Decoding the image...";
    ImageDecoder::Start(this, *response_body);
  } else if (response_code == net::HTTP_NOT_FOUND) {
    simple_loader_.reset();
    VLOG(1) << "Got 404, using default picture...";
    picture_status_ = PICTURE_DEFAULT;
    delegate_->OnProfileDownloadSuccess(this);
  } else {
    LOG(WARNING) << "Loading profile data failed";
    DVLOG(1) << "  Error: " << simple_loader_->NetError();
    DVLOG(1) << "  Response code: " << response_code;
    DVLOG(1) << "  Url: " << simple_loader_->GetFinalURL().spec();
    // Handle miscellaneous 400/500 errors.
    bool network_error =
        response_code == -1 || (response_code >= 400 && response_code < 600);
    simple_loader_.reset();
    delegate_->OnProfileDownloadFailure(this, network_error ?
        ProfileDownloaderDelegate::NETWORK_ERROR :
        ProfileDownloaderDelegate::SERVICE_ERROR);
  }
}

void ProfileDownloader::OnImageDecoded(const SkBitmap& decoded_image) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  int image_size = delegate_->GetDesiredImageSideLength();
  profile_picture_ = skia::ImageOperations::Resize(
      decoded_image,
      skia::ImageOperations::RESIZE_BEST,
      image_size,
      image_size);
  picture_status_ = PICTURE_SUCCESS;
  delegate_->OnProfileDownloadSuccess(this);
}

void ProfileDownloader::OnDecodeImageFailed() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  delegate_->OnProfileDownloadFailure(
      this, ProfileDownloaderDelegate::IMAGE_DECODE_FAILED);
}

void ProfileDownloader::OnRefreshTokenAvailable(const std::string& account_id) {
  ProfileOAuth2TokenService* service =
      ProfileOAuth2TokenServiceFactory::GetForProfile(
          delegate_->GetBrowserProfile());
  if (account_id != account_id_)
    return;

  service->RemoveObserver(this);
  StartFetchingOAuth2AccessToken();
}

// Callback for OAuth2TokenService::Request on success. |access_token| is the
// token used to start fetching user data.
void ProfileDownloader::OnGetTokenSuccess(
    const OAuth2TokenService::Request* request,
    const std::string& access_token,
    const base::Time& expiration_time) {
  DCHECK_EQ(request, oauth2_access_token_request_.get());
  oauth2_access_token_request_.reset();
  auth_token_ = access_token;
  StartFetchingImage();
}

// Callback for OAuth2TokenService::Request on failure.
void ProfileDownloader::OnGetTokenFailure(
    const OAuth2TokenService::Request* request,
    const GoogleServiceAuthError& error) {
  DCHECK_EQ(request, oauth2_access_token_request_.get());
  oauth2_access_token_request_.reset();
  LOG(WARNING) << "ProfileDownloader: token request using refresh token failed:"
               << error.ToString();
  delegate_->OnProfileDownloadFailure(
      this, ProfileDownloaderDelegate::TOKEN_ERROR);
}

void ProfileDownloader::OnAccountUpdated(const AccountInfo& info) {
  if (info.account_id == account_id_ && info.IsValid()) {
    account_info_ = info;

    // If the StartFetchingImage was called before we had valid info, the
    // downloader has been waiting so we need to fetch the image data now.
    if (waiting_for_account_info_) {
      waiting_for_account_info_ = false;
      // FetchImageData might call the delegate's OnProfileDownloadSuccess
      // synchronously, causing |this| to be deleted so there should not be more
      // code after it.
      FetchImageData();
    }
  }
}
