// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/payments/ios_payment_instrument_finder.h"

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

#include <map>
#include <set>

#include "base/bind.h"
#include "base/json/json_reader.h"
#include "base/strings/sys_string_conversions.h"
#include "base/values.h"
#include "ios/chrome/browser/payments/ios_payment_instrument.h"
#include "ios/chrome/browser/payments/payment_request.h"
#include "net/url_request/url_request_context_getter.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

// The maximum number of web app manifests that can be parsed from the payment
// method manifest.
const size_t kMaximumNumberOfWebAppManifests = 100U;

// The following constants are defined in one of the following two documents:
// https://w3c.github.io/payment-method-manifest/
// https://w3c.github.io/manifest/
static const char kDefaultApplications[] = "default_applications";
static const char kShortName[] = "short_name";
static const char kIcons[] = "icons";
static const char kIconsSource[] = "src";
static const char kIconsSizes[] = "sizes";
static const char kIconSizes32[] = "32x32";
static const char kRelatedApplications[] = "related_applications";
static const char kPlatform[] = "platform";
static const char kPlatformItunes[] = "itunes";
static const char kRelatedApplicationsUrl[] = "url";

}  // namespace

namespace payments {

IOSPaymentInstrumentFinder::IOSPaymentInstrumentFinder(
    net::URLRequestContextGetter* context_getter,
    id<PaymentRequestUIDelegate> payment_request_ui_delegate)
    : downloader_(context_getter),
      image_fetcher_(context_getter),
      payment_request_ui_delegate_(payment_request_ui_delegate),
      num_instruments_to_find_(0),
      weak_factory_(this) {}

IOSPaymentInstrumentFinder::~IOSPaymentInstrumentFinder() {}

std::vector<GURL>
IOSPaymentInstrumentFinder::FilterUnsupportedURLPaymentMethods(
    const std::vector<GURL>& queried_url_payment_method_identifiers) {
  const std::map<std::string, std::string>& enum_map =
      payments::GetMethodNameToSchemeName();
  std::vector<GURL> filtered_url_payment_methods;
  for (const GURL& method : queried_url_payment_method_identifiers) {
    // Ensure that the payment method is recognized by looking for an
    // "app-name://" scheme to query for its presence.
    if (!base::ContainsKey(enum_map, method.spec()))
      continue;

    // If there is an app that can handle |scheme| on this device, this payment
    // method is supported.
    const std::string& scheme = enum_map.find(method.spec())->second;
    UIApplication* application = [UIApplication sharedApplication];
    NSURL* URL = [NSURL URLWithString:(base::SysUTF8ToNSString(scheme))];
    if (![application canOpenURL:URL])
      continue;

    filtered_url_payment_methods.push_back(method);
  }

  return filtered_url_payment_methods;
}

std::vector<GURL>
IOSPaymentInstrumentFinder::CreateIOSPaymentInstrumentsForMethods(
    const std::vector<GURL>& methods,
    IOSPaymentInstrumentsFoundCallback callback) {
  // If |callback_| is not null, there's already an active search for iOS
  // payment instruments, which shouldn't happen.
  if (!callback_.is_null()) {
    std::move(callback).Run(
        std::vector<std::unique_ptr<IOSPaymentInstrument>>());
    return std::vector<GURL>();
  }

  // The function should immediately return if there are 0 valid methods
  // supplied.
  const std::vector<GURL>& filtered_methods =
      FilterUnsupportedURLPaymentMethods(methods);
  if (filtered_methods.empty()) {
    std::move(callback).Run(
        std::vector<std::unique_ptr<IOSPaymentInstrument>>());
    return std::vector<GURL>();
  }

  callback_ = std::move(callback);
  // This is originally set to the number of valid payment methods, but may
  // change depending on how many payment apps on a user's device support a
  // particular payment method.
  num_instruments_to_find_ = filtered_methods.size();
  instruments_found_.clear();

  for (const GURL& method : filtered_methods) {
    downloader_.DownloadPaymentMethodManifest(
        method,
        base::BindOnce(&IOSPaymentInstrumentFinder::OnPaymentManifestDownloaded,
                       weak_factory_.GetWeakPtr(), method));
  }

  return filtered_methods;
}

void IOSPaymentInstrumentFinder::OnPaymentManifestDownloaded(
    const GURL& method,
    const std::string& content) {
  // If |content| is empty then the download failed.
  if (content.empty()) {
    OnPaymentInstrumentProcessed();
    return;
  }

  std::vector<GURL> web_app_manifest_urls;
  // If there are no web app manifests found for the payment method, stop
  // processing this payment method.
  if (!GetWebAppManifestURLsFromPaymentManifest(content,
                                                &web_app_manifest_urls)) {
    OnPaymentInstrumentProcessed();
    return;
  }

  // The payment method manifest can point to several web app manifests.
  // Adjust the expectation of how many instruments we are looking for if
  // this is the case.
  num_instruments_to_find_ += web_app_manifest_urls.size() - 1;

  for (const GURL& web_app_manifest_url : web_app_manifest_urls) {
    downloader_.DownloadWebAppManifest(
        web_app_manifest_url,
        base::BindOnce(&IOSPaymentInstrumentFinder::OnWebAppManifestDownloaded,
                       weak_factory_.GetWeakPtr(), method,
                       web_app_manifest_url));
  }
}

bool IOSPaymentInstrumentFinder::GetWebAppManifestURLsFromPaymentManifest(
    const std::string& input,
    std::vector<GURL>* out_web_app_manifest_urls) {
  DCHECK(out_web_app_manifest_urls->empty());

  std::set<GURL> web_app_manifest_urls;

  std::unique_ptr<base::Value> value = base::JSONReader::Read(input);
  if (!value) {
    LOG(ERROR) << "Payment method manifest must be in JSON format.";
    return false;
  }

  std::unique_ptr<base::DictionaryValue> dict =
      base::DictionaryValue::From(std::move(value));
  if (!dict) {
    LOG(ERROR) << "Payment method manifest must be a JSON dictionary.";
    return false;
  }

  base::ListValue* list = nullptr;
  if (!dict->GetList(kDefaultApplications, &list)) {
    LOG(ERROR) << "\"" << kDefaultApplications << "\" must be a list.";
    return false;
  }

  size_t apps_number = list->GetSize();
  if (apps_number > kMaximumNumberOfWebAppManifests) {
    LOG(ERROR) << "\"" << kDefaultApplications << "\" must contain at most "
               << kMaximumNumberOfWebAppManifests << " entries.";
    return false;
  }

  std::string item;
  for (size_t i = 0; i < apps_number; ++i) {
    if (!list->GetString(i, &item) || item.empty())
      continue;

    GURL url(item);
    if (!url.is_valid() || !url.SchemeIs(url::kHttpsScheme))
      continue;

    // Ensure that the json file does not contain any repeated web
    // app manifest URLs.
    const auto result = web_app_manifest_urls.insert(url);
    if (result.second)
      out_web_app_manifest_urls->push_back(url);
  }

  // If we weren't able to find a valid web app manifest URL then we return
  // false.
  return !out_web_app_manifest_urls->empty();
}

void IOSPaymentInstrumentFinder::OnWebAppManifestDownloaded(
    const GURL& method,
    const GURL& web_app_manifest_url,
    const std::string& content) {
  // If |content| is empty then the download failed.
  if (content.empty()) {
    OnPaymentInstrumentProcessed();
    return;
  }

  std::string app_name;
  GURL app_icon_url;
  GURL universal_link;
  if (!GetPaymentAppDetailsFromWebAppManifest(content, web_app_manifest_url,
                                              &app_name, &app_icon_url,
                                              &universal_link)) {
    OnPaymentInstrumentProcessed();
    return;
  }

  CreateIOSPaymentInstrument(method, app_name, app_icon_url, universal_link);
}

bool IOSPaymentInstrumentFinder::GetPaymentAppDetailsFromWebAppManifest(
    const std::string& input,
    const GURL& web_app_manifest_url,
    std::string* out_app_name,
    GURL* out_app_icon_url,
    GURL* out_universal_link) {
  std::unique_ptr<base::Value> value = base::JSONReader::Read(input);
  if (!value) {
    LOG(ERROR) << "Web app manifest must be in JSON format.";
    return false;
  }

  std::unique_ptr<base::DictionaryValue> dict =
      base::DictionaryValue::From(std::move(value));
  if (!dict) {
    LOG(ERROR) << "Web app manifest must be a JSON dictionary.";
    return false;
  }

  if (!dict->GetString(kShortName, out_app_name) || out_app_name->empty()) {
    LOG(ERROR) << "\"" << kShortName << "\" must be a non-empty ASCII string.";
    return false;
  }

  base::ListValue* icons = nullptr;
  if (!dict->GetList(kIcons, &icons)) {
    LOG(ERROR) << "\"" << kIcons << "\" must be a list.";
    return false;
  }

  size_t icons_size = icons->GetSize();
  for (size_t i = 0; i < icons_size; ++i) {
    base::DictionaryValue* icon = nullptr;
    if (!icons->GetDictionary(i, &icon))
      continue;

    std::string icon_sizes;
    // TODO(crbug.com/752546): Determine acceptable sizes for payment app icon.
    if (!icon->GetString(kIconsSizes, &icon_sizes) ||
        icon_sizes != kIconSizes32)
      continue;

    std::string icon_string;
    if (!icon->GetString(kIconsSource, &icon_string) || icon_string.empty())
      continue;

    // The parsed value at "src" may be a relative path such that the base URL
    // is the path to the manifest. If so we check that here.
    GURL complete_url = web_app_manifest_url.Resolve(icon_string);
    if (complete_url.is_valid() && complete_url.SchemeIs(url::kHttpsScheme)) {
      *out_app_icon_url = complete_url;
      break;
    }

    GURL icon_url(icon_string);
    if (icon_url.is_valid() && icon_url.SchemeIs(url::kHttpsScheme))
      *out_app_icon_url = icon_url;
  }

  if (out_app_icon_url->is_empty())
    return false;

  base::ListValue* apps = nullptr;
  if (!dict->GetList(kRelatedApplications, &apps)) {
    LOG(ERROR) << "\"" << kRelatedApplications << "\" must be a list.";
    return false;
  }

  size_t related_applications_size = apps->GetSize();
  for (size_t i = 0; i < related_applications_size; ++i) {
    base::DictionaryValue* related_application = nullptr;
    if (!apps->GetDictionary(i, &related_application))
      continue;

    std::string platform;
    if (!related_application->GetString(kPlatform, &platform) ||
        platform != kPlatformItunes)
      continue;

    std::string link;
    if (!related_application->GetString(kRelatedApplicationsUrl, &link) ||
        link.empty())
      continue;

    GURL url(link);
    if (!url.is_valid() || !url.SchemeIs(url::kHttpsScheme))
      continue;

    *out_universal_link = url;
    break;
  }

  return !out_universal_link->is_empty();
}

void IOSPaymentInstrumentFinder::CreateIOSPaymentInstrument(
    const GURL& method_name,
    std::string& app_name,
    GURL& app_icon_url,
    GURL& universal_link) {
  GURL local_method_name(method_name);
  std::string local_app_name(app_name);
  GURL local_universal_link(universal_link);

  image_fetcher::IOSImageDataFetcherCallback callback =
      ^(NSData* data, const image_fetcher::RequestMetadata& metadata) {
        if (data) {
          UIImage* icon =
              [UIImage imageWithData:data scale:[UIScreen mainScreen].scale];
          instruments_found_.push_back(std::make_unique<IOSPaymentInstrument>(
              local_method_name.spec(), local_universal_link, local_app_name,
              icon, payment_request_ui_delegate_));
        }
        OnPaymentInstrumentProcessed();
      };
  image_fetcher_.FetchImageDataWebpDecoded(app_icon_url, callback);
}

void IOSPaymentInstrumentFinder::OnPaymentInstrumentProcessed() {
  DCHECK(callback_);

  if (--num_instruments_to_find_ == 0)
    std::move(callback_).Run(std::move(instruments_found_));
}

}  // namespace payments
