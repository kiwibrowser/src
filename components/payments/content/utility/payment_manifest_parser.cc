// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/payments/content/utility/payment_manifest_parser.h"

#include <algorithm>
#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/logging.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_refptr.h"
#include "base/strings/string_number_conversions.h"
#include "components/payments/content/utility/fingerprint_parser.h"
#include "content/public/common/service_manager_connection.h"
#include "net/base/url_util.h"
#include "services/data_decoder/public/cpp/safe_json_parser.h"
#include "url/url_constants.h"

namespace payments {
namespace {

const size_t kMaximumNumberOfItems = 100U;

const char* const kDefaultApplications = "default_applications";
const char* const kHttpPrefix = "http://";
const char* const kHttpsPrefix = "https://";
const char* const kSupportedOrigins = "supported_origins";
const char* const kServiceWorker = "serviceworker";
const char* const kServiceWorkerSrc = "src";
const char* const kServiceWorkerScope = "scope";
const char* const kServiceWorkerUseCache = "use_cache";
const char* const kWebAppName = "name";
const char* const kWebAppIcons = "icons";
const char* const kWebAppIconSrc = "src";
const char* const kWebAppIconSizes = "sizes";
const char* const kWebAppIconType = "type";

// Parses the "default_applications": ["https://some/url"] from |dict| into
// |web_app_manifest_urls|. Returns 'false' for invalid data.
bool ParseDefaultApplications(base::DictionaryValue* dict,
                              std::vector<GURL>* web_app_manifest_urls) {
  DCHECK(dict);
  DCHECK(web_app_manifest_urls);

  base::ListValue* list = nullptr;
  if (!dict->GetList(kDefaultApplications, &list)) {
    LOG(ERROR) << "\"" << kDefaultApplications << "\" must be a list.";
    return false;
  }

  size_t apps_number = list->GetSize();
  if (apps_number > kMaximumNumberOfItems) {
    LOG(ERROR) << "\"" << kDefaultApplications << "\" must contain at most "
               << kMaximumNumberOfItems << " entries.";
    return false;
  }

  for (size_t i = 0; i < apps_number; ++i) {
    std::string item;
    if (!list->GetString(i, &item) || item.empty() ||
        !base::IsStringUTF8(item) ||
        !(base::StartsWith(item, kHttpsPrefix, base::CompareCase::SENSITIVE) ||
          base::StartsWith(item, kHttpPrefix, base::CompareCase::SENSITIVE))) {
      LOG(ERROR) << "Each entry in \"" << kDefaultApplications
                 << "\" must be UTF8 string that starts with \"" << kHttpsPrefix
                 << "\" or \"" << kHttpPrefix << "\" (for localhost).";
      web_app_manifest_urls->clear();
      return false;
    }

    GURL url(item);
    if (!url.is_valid() ||
        !(url.SchemeIs(url::kHttpsScheme) ||
          (url.SchemeIs(url::kHttpScheme) && net::IsLocalhost(url)))) {
      LOG(ERROR) << "\"" << item << "\" entry in \"" << kDefaultApplications
                 << "\" is not a valid URL with HTTPS scheme and is not a "
                    "valid localhost URL with HTTP scheme.";
      web_app_manifest_urls->clear();
      return false;
    }

    web_app_manifest_urls->push_back(url);
  }

  return true;
}

// Parses the "supported_origins": "*" (or ["https://some.origin"]) from |dict|
// into |supported_origins| and |all_origins_supported|. Returns 'false' for
// invalid data.
bool ParseSupportedOrigins(base::DictionaryValue* dict,
                           std::vector<url::Origin>* supported_origins,
                           bool* all_origins_supported) {
  DCHECK(dict);
  DCHECK(supported_origins);
  DCHECK(all_origins_supported);

  *all_origins_supported = false;

  {
    std::string item;
    if (dict->GetString(kSupportedOrigins, &item)) {
      if (item != "*") {
        LOG(ERROR) << "\"" << item << "\" is not a valid value for \""
                   << kSupportedOrigins
                   << "\". Must be either \"*\" or a list of RFC6454 origins.";
        return false;
      }

      *all_origins_supported = true;
      return true;
    }
  }

  base::ListValue* list = nullptr;
  if (!dict->GetList(kSupportedOrigins, &list)) {
    LOG(ERROR) << "\"" << kSupportedOrigins
               << "\" must be either \"*\" or a list of origins.";
    return false;
  }

  size_t supported_origins_number = list->GetSize();
  const size_t kMaximumNumberOfSupportedOrigins = 100000;
  if (supported_origins_number > kMaximumNumberOfSupportedOrigins) {
    LOG(ERROR) << "\"" << kSupportedOrigins << "\" must contain at most "
               << kMaximumNumberOfSupportedOrigins << " entires.";
    return false;
  }

  for (size_t i = 0; i < supported_origins_number; ++i) {
    std::string item;
    if (!list->GetString(i, &item) || item.empty() ||
        !base::IsStringUTF8(item) ||
        !(base::StartsWith(item, kHttpsPrefix, base::CompareCase::SENSITIVE) ||
          base::StartsWith(item, kHttpPrefix, base::CompareCase::SENSITIVE))) {
      LOG(ERROR) << "Each entry in \"" << kSupportedOrigins
                 << "\" must be UTF8 string that starts with \"" << kHttpsPrefix
                 << "\" or \"" << kHttpPrefix << "\" (for localhost).";
      supported_origins->clear();
      return false;
    }

    GURL url(item);
    if (!url.is_valid() ||
        !(url.SchemeIs(url::kHttpsScheme) ||
          (url.SchemeIs(url::kHttpScheme) && net::IsLocalhost(url))) ||
        url.path() != "/" || url.has_query() || url.has_ref() ||
        url.has_username() || url.has_password()) {
      LOG(ERROR) << "\"" << item << "\" entry in \"" << kSupportedOrigins
                 << "\" is not a valid origin with HTTPS scheme and is not a "
                    "valid localhost origin with HTTP scheme.";
      supported_origins->clear();
      return false;
    }

    supported_origins->push_back(url::Origin::Create(url));
  }

  return true;
}

// An object that allows both SafeJsonParser's callbacks (error/success) to run
// the same callback provided to ParsePaymentMethodManifest/ParseWebAppManifest.
// (since Callbacks are movable type, that callback has to be shared)
template <typename Callback>
class JsonParserCallback
    : public base::RefCounted<JsonParserCallback<Callback>> {
 public:
  JsonParserCallback(
      base::Callback<void(Callback, std::unique_ptr<base::Value>)>
          parser_callback,
      Callback client_callback)
      : parser_callback_(std::move(parser_callback)),
        client_callback_(std::move(client_callback)) {}

  void OnSuccess(std::unique_ptr<base::Value> value) {
    std::move(parser_callback_)
        .Run(std::move(client_callback_), std::move(value));
  }

  void OnError(const std::string& error_message) {
    std::move(parser_callback_)
        .Run(std::move(client_callback_), /*value=*/nullptr);
  }

 private:
  friend class base::RefCounted<JsonParserCallback>;
  ~JsonParserCallback() = default;

  base::Callback<void(Callback, std::unique_ptr<base::Value>)> parser_callback_;
  Callback client_callback_;
};

}  // namespace

PaymentManifestParser::WebAppIcon::WebAppIcon() = default;

PaymentManifestParser::WebAppIcon::~WebAppIcon() = default;

PaymentManifestParser::PaymentManifestParser() : weak_factory_(this) {}

PaymentManifestParser::~PaymentManifestParser() = default;

void PaymentManifestParser::ParsePaymentMethodManifest(
    const std::string& content,
    PaymentMethodCallback callback) {
  parse_payment_callback_counter_++;
  DCHECK_GE(10U, parse_payment_callback_counter_);

  scoped_refptr<JsonParserCallback<PaymentMethodCallback>> json_callback =
      new JsonParserCallback<PaymentMethodCallback>(
          base::Bind(&PaymentManifestParser::OnPaymentMethodParse,
                     weak_factory_.GetWeakPtr()),
          std::move(callback));

  data_decoder::SafeJsonParser::Parse(
      content::ServiceManagerConnection::GetForProcess()->GetConnector(),
      content,
      base::Bind(&JsonParserCallback<PaymentMethodCallback>::OnSuccess,
                 json_callback),
      base::Bind(&JsonParserCallback<PaymentMethodCallback>::OnError,
                 json_callback));
}

void PaymentManifestParser::ParseWebAppManifest(const std::string& content,
                                                WebAppCallback callback) {
  parse_webapp_callback_counter_++;
  DCHECK_GE(10U, parse_webapp_callback_counter_);

  scoped_refptr<JsonParserCallback<WebAppCallback>> parser_callback =
      new JsonParserCallback<WebAppCallback>(
          base::Bind(&PaymentManifestParser::OnWebAppParse,
                     weak_factory_.GetWeakPtr()),
          std::move(callback));

  data_decoder::SafeJsonParser::Parse(
      content::ServiceManagerConnection::GetForProcess()->GetConnector(),
      content,
      base::Bind(&JsonParserCallback<WebAppCallback>::OnSuccess,
                 parser_callback),
      base::Bind(&JsonParserCallback<WebAppCallback>::OnError,
                 parser_callback));
}

void PaymentManifestParser::ParseWebAppInstallationInfo(
    const std::string& content,
    WebAppInstallationInfoCallback callback) {
  scoped_refptr<JsonParserCallback<WebAppInstallationInfoCallback>>
      sw_parser_callback =
          new JsonParserCallback<WebAppInstallationInfoCallback>(
              base::Bind(&PaymentManifestParser::OnWebAppParseInstallationInfo,
                         weak_factory_.GetWeakPtr()),
              std::move(callback));

  data_decoder::SafeJsonParser::Parse(
      content::ServiceManagerConnection::GetForProcess()->GetConnector(),
      content,
      base::Bind(&JsonParserCallback<WebAppInstallationInfoCallback>::OnSuccess,
                 sw_parser_callback),
      base::Bind(&JsonParserCallback<WebAppInstallationInfoCallback>::OnError,
                 sw_parser_callback));
}

// static
void PaymentManifestParser::ParsePaymentMethodManifestIntoVectors(
    std::unique_ptr<base::Value> value,
    std::vector<GURL>* web_app_manifest_urls,
    std::vector<url::Origin>* supported_origins,
    bool* all_origins_supported) {
  DCHECK(web_app_manifest_urls);
  DCHECK(supported_origins);
  DCHECK(all_origins_supported);

  *all_origins_supported = false;

  std::unique_ptr<base::DictionaryValue> dict =
      base::DictionaryValue::From(std::move(value));
  if (!dict) {
    LOG(ERROR) << "Payment method manifest must be a JSON dictionary.";
    return;
  }

  if (dict->HasKey(kDefaultApplications) &&
      !ParseDefaultApplications(dict.get(), web_app_manifest_urls)) {
    return;
  }

  if (dict->HasKey(kSupportedOrigins) &&
      !ParseSupportedOrigins(dict.get(), supported_origins,
                             all_origins_supported)) {
    web_app_manifest_urls->clear();
  }
}

// static
bool PaymentManifestParser::ParseWebAppManifestIntoVector(
    std::unique_ptr<base::Value> value,
    std::vector<WebAppManifestSection>* output) {
  std::unique_ptr<base::DictionaryValue> dict =
      base::DictionaryValue::From(std::move(value));
  if (!dict) {
    LOG(ERROR) << "Web app manifest must be a JSON dictionary.";
    return false;
  }

  base::ListValue* list = nullptr;
  if (!dict->GetList("related_applications", &list)) {
    LOG(ERROR) << "\"related_applications\" must be a list.";
    return false;
  }

  size_t related_applications_size = list->GetSize();
  for (size_t i = 0; i < related_applications_size; ++i) {
    base::DictionaryValue* related_application = nullptr;
    if (!list->GetDictionary(i, &related_application) || !related_application) {
      LOG(ERROR) << "\"related_applications\" must be a list of dictionaries.";
      output->clear();
      return false;
    }

    std::string platform;
    if (!related_application->GetString("platform", &platform) ||
        platform != "play") {
      continue;
    }

    if (output->size() >= kMaximumNumberOfItems) {
      LOG(ERROR) << "\"related_applications\" must contain at most "
                 << kMaximumNumberOfItems
                 << " entries with \"platform\": \"play\".";
      output->clear();
      return false;
    }

    const char* const kId = "id";
    const char* const kMinVersion = "min_version";
    const char* const kFingerprints = "fingerprints";
    if (!related_application->HasKey(kId) ||
        !related_application->HasKey(kMinVersion) ||
        !related_application->HasKey(kFingerprints)) {
      LOG(ERROR) << "Each \"platform\": \"play\" entry in "
                    "\"related_applications\" must contain \""
                 << kId << "\", \"" << kMinVersion << "\", and \""
                 << kFingerprints << "\".";
      return false;
    }

    WebAppManifestSection section;
    section.min_version = 0;

    if (!related_application->GetString(kId, &section.id) ||
        section.id.empty() || !base::IsStringASCII(section.id)) {
      LOG(ERROR) << "\"" << kId << "\" must be a non-empty ASCII string.";
      output->clear();
      return false;
    }

    std::string min_version;
    if (!related_application->GetString(kMinVersion, &min_version) ||
        min_version.empty() || !base::IsStringASCII(min_version) ||
        !base::StringToInt64(min_version, &section.min_version)) {
      LOG(ERROR) << "\"" << kMinVersion
                 << "\" must be a string convertible into a number.";
      output->clear();
      return false;
    }

    base::ListValue* fingerprints_list = nullptr;
    if (!related_application->GetList(kFingerprints, &fingerprints_list) ||
        fingerprints_list->empty() ||
        fingerprints_list->GetSize() > kMaximumNumberOfItems) {
      LOG(ERROR) << "\"" << kFingerprints
                 << "\" must be a non-empty list of at most "
                 << kMaximumNumberOfItems << " items.";
      output->clear();
      return false;
    }

    size_t fingerprints_size = fingerprints_list->GetSize();
    for (size_t j = 0; j < fingerprints_size; ++j) {
      base::DictionaryValue* fingerprint_dict = nullptr;
      std::string fingerprint_type;
      std::string fingerprint_value;
      if (!fingerprints_list->GetDictionary(j, &fingerprint_dict) ||
          !fingerprint_dict ||
          !fingerprint_dict->GetString("type", &fingerprint_type) ||
          fingerprint_type != "sha256_cert" ||
          !fingerprint_dict->GetString("value", &fingerprint_value) ||
          fingerprint_value.empty() ||
          !base::IsStringASCII(fingerprint_value)) {
        LOG(ERROR) << "Each entry in \"" << kFingerprints
                   << "\" must be a dictionary with \"type\": "
                      "\"sha256_cert\" and a non-empty ASCII string \"value\".";
        output->clear();
        return false;
      }

      std::vector<uint8_t> hash =
          FingerprintStringToByteArray(fingerprint_value);
      if (hash.empty()) {
        output->clear();
        return false;
      }

      section.fingerprints.push_back(hash);
    }

    output->push_back(section);
  }

  return true;
}

void PaymentManifestParser::OnPaymentMethodParse(
    PaymentMethodCallback callback,
    std::unique_ptr<base::Value> value) {
  parse_payment_callback_counter_--;

  std::vector<GURL> web_app_manifest_urls;
  std::vector<url::Origin> supported_origins;
  bool all_origins_supported = false;
  ParsePaymentMethodManifestIntoVectors(
      std::move(value), &web_app_manifest_urls, &supported_origins,
      &all_origins_supported);

  // Can trigger synchronous deletion of this object, so can't access any of the
  // member variables after this block.
  std::move(callback).Run(web_app_manifest_urls, supported_origins,
                          all_origins_supported);
}

void PaymentManifestParser::OnWebAppParse(WebAppCallback callback,
                                          std::unique_ptr<base::Value> value) {
  parse_webapp_callback_counter_--;

  std::vector<WebAppManifestSection> manifest;
  ParseWebAppManifestIntoVector(std::move(value), &manifest);

  // Can trigger synchronous deletion of this object, so can't access any of the
  // member variables after this block.
  std::move(callback).Run(manifest);
}

void PaymentManifestParser::OnWebAppParseInstallationInfo(
    WebAppInstallationInfoCallback callback,
    std::unique_ptr<base::Value> value) {
  // TODO(crbug.com/782270): Move this function into a static function for unit
  // test.
  if (value->FindKey({kServiceWorker}) == nullptr) {
    return std::move(callback).Run(nullptr, nullptr);
  }

  std::unique_ptr<WebAppInstallationInfo> sw =
      std::make_unique<WebAppInstallationInfo>();
  auto* sw_path = value->FindPath({kServiceWorker, kServiceWorkerSrc});
  if (sw_path == nullptr) {
    LOG(ERROR) << "Service Worker js src cannot be empty.";
    return std::move(callback).Run(nullptr, nullptr);
  }
  sw->sw_js_url = sw_path->GetString();

  sw_path = value->FindPath({kServiceWorker, kServiceWorkerScope});
  if (sw_path != nullptr) {
    sw->sw_scope = sw_path->GetString();
  }

  sw_path = value->FindPath({kServiceWorker, kServiceWorkerUseCache});
  if (sw_path != nullptr) {
    sw->sw_use_cache = sw_path->GetBool();
  }

  auto* name_key = value->FindKey({kWebAppName});
  if (name_key != nullptr) {
    sw->name = name_key->GetString();
  }

  // Extract icons.
  std::unique_ptr<std::vector<WebAppIcon>> icons;
  auto* icons_key = value->FindKey({kWebAppIcons});
  if (icons_key != nullptr) {
    icons = std::make_unique<std::vector<WebAppIcon>>();
    for (const auto& icon : icons_key->GetList()) {
      if (!icon.is_dict())
        continue;

      WebAppIcon web_app_icon;
      const base::Value* icon_src =
          icon.FindKeyOfType(kWebAppIconSrc, base::Value::Type::STRING);
      if (!icon_src || icon_src->GetString().empty())
        continue;
      web_app_icon.src = icon_src->GetString();

      const base::Value* icon_sizes =
          icon.FindKeyOfType(kWebAppIconSizes, base::Value::Type::STRING);
      if (!icon_sizes || icon_sizes->GetString().empty())
        continue;
      web_app_icon.sizes = icon_sizes->GetString();

      const base::Value* icon_type =
          icon.FindKeyOfType(kWebAppIconType, base::Value::Type::STRING);
      if (icon_type)
        web_app_icon.type = icon_type->GetString();

      icons->emplace_back(web_app_icon);
    }
  }

  return std::move(callback).Run(std::move(sw), std::move(icons));
}

}  // namespace payments
