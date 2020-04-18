/*
 * Copyright (C) 2006, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2009 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/platform/loader/fetch/resource_response.h"

#include <algorithm>
#include <limits>
#include <memory>
#include <string>

#include "third_party/blink/public/platform/web_url_response.h"
#include "third_party/blink/renderer/platform/network/http_names.h"
#include "third_party/blink/renderer/platform/network/http_parsers.h"
#include "third_party/blink/renderer/platform/wtf/assertions.h"
#include "third_party/blink/renderer/platform/wtf/std_lib_extras.h"
#include "third_party/blink/renderer/platform/wtf/time.h"

namespace blink {

namespace {

template <typename Interface>
Vector<Interface> IsolatedCopy(const Vector<Interface>& src) {
  Vector<Interface> result;
  result.ReserveCapacity(src.size());
  for (const auto& timestamp : src) {
    result.push_back(timestamp.IsolatedCopy());
  }
  return result;
}

static const char kCacheControlHeader[] = "cache-control";
static const char kPragmaHeader[] = "pragma";

}  // namespace

ResourceResponse::SignedCertificateTimestamp::SignedCertificateTimestamp(
    const blink::WebURLResponse::SignedCertificateTimestamp& sct)
    : status_(sct.status),
      origin_(sct.origin),
      log_description_(sct.log_description),
      log_id_(sct.log_id),
      timestamp_(sct.timestamp),
      hash_algorithm_(sct.hash_algorithm),
      signature_algorithm_(sct.signature_algorithm),
      signature_data_(sct.signature_data) {}

ResourceResponse::SignedCertificateTimestamp
ResourceResponse::SignedCertificateTimestamp::IsolatedCopy() const {
  return SignedCertificateTimestamp(
      status_.IsolatedCopy(), origin_.IsolatedCopy(),
      log_description_.IsolatedCopy(), log_id_.IsolatedCopy(), timestamp_,
      hash_algorithm_.IsolatedCopy(), signature_algorithm_.IsolatedCopy(),
      signature_data_.IsolatedCopy());
}

ResourceResponse::ResourceResponse()
    : expected_content_length_(0), is_null_(true) {}

ResourceResponse::ResourceResponse(const KURL& url,
                                   const AtomicString& mime_type,
                                   long long expected_length,
                                   const AtomicString& text_encoding_name)
    : url_(url),
      mime_type_(mime_type),
      expected_content_length_(expected_length),
      text_encoding_name_(text_encoding_name),
      is_null_(false) {}

ResourceResponse::ResourceResponse(CrossThreadResourceResponseData* data)
    : ResourceResponse() {
  SetURL(data->url_);
  SetMimeType(AtomicString(data->mime_type_));
  SetExpectedContentLength(data->expected_content_length_);
  SetTextEncodingName(AtomicString(data->text_encoding_name_));

  SetHTTPStatusCode(data->http_status_code_);
  SetHTTPStatusText(AtomicString(data->http_status_text_));

  http_header_fields_.Adopt(std::move(data->http_headers_));
  SetResourceLoadTiming(std::move(data->resource_load_timing_));
  remote_ip_address_ = AtomicString(data->remote_ip_address_);
  remote_port_ = data->remote_port_;
  has_major_certificate_errors_ = data->has_major_certificate_errors_;
  ct_policy_compliance_ = data->ct_policy_compliance_;
  is_legacy_symantec_cert_ = data->is_legacy_symantec_cert_;
  cert_validity_start_ = data->cert_validity_start_;
  was_fetched_via_spdy_ = data->was_fetched_via_spdy_;
  was_fetched_via_proxy_ = data->was_fetched_via_proxy_;
  was_fetched_via_service_worker_ = data->was_fetched_via_service_worker_;
  was_fallback_required_by_service_worker_ =
      data->was_fallback_required_by_service_worker_;
  did_service_worker_navigation_preload_ =
      data->did_service_worker_navigation_preload_;
  response_type_via_service_worker_ = data->response_type_via_service_worker_;
  security_style_ = data->security_style_;
  security_details_.protocol = data->security_details_.protocol;
  security_details_.cipher = data->security_details_.cipher;
  security_details_.key_exchange = data->security_details_.key_exchange;
  security_details_.key_exchange_group =
      data->security_details_.key_exchange_group;
  security_details_.mac = data->security_details_.mac;
  security_details_.subject_name = data->security_details_.subject_name;
  security_details_.san_list = data->security_details_.san_list;
  security_details_.issuer = data->security_details_.issuer;
  security_details_.valid_from = data->security_details_.valid_from;
  security_details_.valid_to = data->security_details_.valid_to;
  for (auto& cert : data->certificate_)
    security_details_.certificate.push_back(AtomicString(cert));
  security_details_.sct_list = data->security_details_.sct_list;
  http_version_ = data->http_version_;
  app_cache_id_ = data->app_cache_id_;
  app_cache_manifest_url_ = data->app_cache_manifest_url_.Copy();
  multipart_boundary_ = data->multipart_boundary_;
  url_list_via_service_worker_ = data->url_list_via_service_worker_;
  cache_storage_cache_name_ = data->cache_storage_cache_name_;
  response_time_ = data->response_time_;
  encoded_data_length_ = data->encoded_data_length_;
  encoded_body_length_ = data->encoded_body_length_;
  decoded_body_length_ = data->decoded_body_length_;
  downloaded_file_path_ = data->downloaded_file_path_;
  downloaded_file_handle_ = data->downloaded_file_handle_;

  // Bug https://bugs.webkit.org/show_bug.cgi?id=60397 this doesn't support
  // whatever values may be present in the opaque m_extraData structure.
}

ResourceResponse::ResourceResponse(const ResourceResponse&) = default;
ResourceResponse& ResourceResponse::operator=(const ResourceResponse&) =
    default;

std::unique_ptr<CrossThreadResourceResponseData> ResourceResponse::CopyData()
    const {
  std::unique_ptr<CrossThreadResourceResponseData> data =
      std::make_unique<CrossThreadResourceResponseData>();
  data->url_ = Url().Copy();
  data->mime_type_ = MimeType().GetString().IsolatedCopy();
  data->expected_content_length_ = ExpectedContentLength();
  data->text_encoding_name_ = TextEncodingName().GetString().IsolatedCopy();
  data->http_status_code_ = HttpStatusCode();
  data->http_status_text_ = HttpStatusText().GetString().IsolatedCopy();
  data->http_headers_ = HttpHeaderFields().CopyData();
  if (resource_load_timing_)
    data->resource_load_timing_ = resource_load_timing_->DeepCopy();
  data->remote_ip_address_ = remote_ip_address_.GetString().IsolatedCopy();
  data->remote_port_ = remote_port_;
  data->has_major_certificate_errors_ = has_major_certificate_errors_;
  data->ct_policy_compliance_ = ct_policy_compliance_;
  data->is_legacy_symantec_cert_ = is_legacy_symantec_cert_;
  data->cert_validity_start_ = cert_validity_start_;
  data->was_fetched_via_spdy_ = was_fetched_via_spdy_;
  data->was_fetched_via_proxy_ = was_fetched_via_proxy_;
  data->was_fetched_via_service_worker_ = was_fetched_via_service_worker_;
  data->was_fallback_required_by_service_worker_ =
      was_fallback_required_by_service_worker_;
  data->did_service_worker_navigation_preload_ =
      did_service_worker_navigation_preload_;
  data->response_type_via_service_worker_ = response_type_via_service_worker_;
  data->security_style_ = security_style_;
  data->security_details_.protocol = security_details_.protocol.IsolatedCopy();
  data->security_details_.cipher = security_details_.cipher.IsolatedCopy();
  data->security_details_.key_exchange =
      security_details_.key_exchange.IsolatedCopy();
  data->security_details_.key_exchange_group =
      security_details_.key_exchange_group.IsolatedCopy();
  data->security_details_.mac = security_details_.mac.IsolatedCopy();
  data->security_details_.subject_name =
      security_details_.subject_name.IsolatedCopy();
  data->security_details_.san_list = IsolatedCopy(security_details_.san_list);
  data->security_details_.issuer = security_details_.issuer.IsolatedCopy();
  data->security_details_.valid_from = security_details_.valid_from;
  data->security_details_.valid_to = security_details_.valid_to;
  for (auto& cert : security_details_.certificate)
    data->certificate_.push_back(cert.GetString().IsolatedCopy());
  data->security_details_.sct_list = IsolatedCopy(security_details_.sct_list);
  data->http_version_ = http_version_;
  data->app_cache_id_ = app_cache_id_;
  data->app_cache_manifest_url_ = app_cache_manifest_url_.Copy();
  data->multipart_boundary_ = multipart_boundary_;
  data->url_list_via_service_worker_.resize(
      url_list_via_service_worker_.size());
  std::transform(url_list_via_service_worker_.begin(),
                 url_list_via_service_worker_.end(),
                 data->url_list_via_service_worker_.begin(),
                 [](const KURL& url) { return url.Copy(); });
  data->cache_storage_cache_name_ = CacheStorageCacheName().IsolatedCopy();
  data->response_time_ = response_time_;
  data->encoded_data_length_ = encoded_data_length_;
  data->encoded_body_length_ = encoded_body_length_;
  data->decoded_body_length_ = decoded_body_length_;
  data->downloaded_file_path_ = downloaded_file_path_.IsolatedCopy();
  data->downloaded_file_handle_ = downloaded_file_handle_;

  // Bug https://bugs.webkit.org/show_bug.cgi?id=60397 this doesn't support
  // whatever values may be present in the opaque m_extraData structure.

  return data;
}

bool ResourceResponse::IsHTTP() const {
  return url_.ProtocolIsInHTTPFamily();
}

const KURL& ResourceResponse::Url() const {
  return url_;
}

void ResourceResponse::SetURL(const KURL& url) {
  is_null_ = false;

  url_ = url;
}

const AtomicString& ResourceResponse::MimeType() const {
  return mime_type_;
}

void ResourceResponse::SetMimeType(const AtomicString& mime_type) {
  is_null_ = false;

  // FIXME: MIME type is determined by HTTP Content-Type header. We should
  // update the header, so that it doesn't disagree with m_mimeType.
  mime_type_ = mime_type;
}

long long ResourceResponse::ExpectedContentLength() const {
  return expected_content_length_;
}

void ResourceResponse::SetExpectedContentLength(
    long long expected_content_length) {
  is_null_ = false;

  // FIXME: Content length is determined by HTTP Content-Length header. We
  // should update the header, so that it doesn't disagree with
  // m_expectedContentLength.
  expected_content_length_ = expected_content_length;
}

const AtomicString& ResourceResponse::TextEncodingName() const {
  return text_encoding_name_;
}

void ResourceResponse::SetTextEncodingName(const AtomicString& encoding_name) {
  is_null_ = false;

  // FIXME: Text encoding is determined by HTTP Content-Type header. We should
  // update the header, so that it doesn't disagree with m_textEncodingName.
  text_encoding_name_ = encoding_name;
}

int ResourceResponse::HttpStatusCode() const {
  return http_status_code_;
}

void ResourceResponse::SetHTTPStatusCode(int status_code) {
  http_status_code_ = status_code;
}

const AtomicString& ResourceResponse::HttpStatusText() const {
  return http_status_text_;
}

void ResourceResponse::SetHTTPStatusText(const AtomicString& status_text) {
  http_status_text_ = status_text;
}

const AtomicString& ResourceResponse::HttpHeaderField(
    const AtomicString& name) const {
  return http_header_fields_.Get(name);
}

void ResourceResponse::UpdateHeaderParsedState(const AtomicString& name) {
  static const char kAgeHeader[] = "age";
  static const char kDateHeader[] = "date";
  static const char kExpiresHeader[] = "expires";
  static const char kLastModifiedHeader[] = "last-modified";

  if (DeprecatedEqualIgnoringCase(name, kAgeHeader))
    have_parsed_age_header_ = false;
  else if (DeprecatedEqualIgnoringCase(name, kCacheControlHeader) ||
           DeprecatedEqualIgnoringCase(name, kPragmaHeader))
    cache_control_header_ = CacheControlHeader();
  else if (DeprecatedEqualIgnoringCase(name, kDateHeader))
    have_parsed_date_header_ = false;
  else if (DeprecatedEqualIgnoringCase(name, kExpiresHeader))
    have_parsed_expires_header_ = false;
  else if (DeprecatedEqualIgnoringCase(name, kLastModifiedHeader))
    have_parsed_last_modified_header_ = false;
}

void ResourceResponse::SetSecurityDetails(
    const String& protocol,
    const String& key_exchange,
    const String& key_exchange_group,
    const String& cipher,
    const String& mac,
    const String& subject_name,
    const Vector<String>& san_list,
    const String& issuer,
    time_t valid_from,
    time_t valid_to,
    const Vector<AtomicString>& certificate,
    const SignedCertificateTimestampList& sct_list) {
  security_details_.protocol = protocol;
  security_details_.key_exchange = key_exchange;
  security_details_.key_exchange_group = key_exchange_group;
  security_details_.cipher = cipher;
  security_details_.mac = mac;
  security_details_.subject_name = subject_name;
  security_details_.san_list = san_list;
  security_details_.issuer = issuer;
  security_details_.valid_from = valid_from;
  security_details_.valid_to = valid_to;
  security_details_.certificate = certificate;
  security_details_.sct_list = sct_list;
}

void ResourceResponse::SetHTTPHeaderField(const AtomicString& name,
                                          const AtomicString& value) {
  UpdateHeaderParsedState(name);

  http_header_fields_.Set(name, value);
}

void ResourceResponse::AddHTTPHeaderField(const AtomicString& name,
                                          const AtomicString& value) {
  UpdateHeaderParsedState(name);

  HTTPHeaderMap::AddResult result = http_header_fields_.Add(name, value);
  if (!result.is_new_entry)
    result.stored_value->value = result.stored_value->value + ", " + value;
}

void ResourceResponse::ClearHTTPHeaderField(const AtomicString& name) {
  http_header_fields_.Remove(name);
}

const HTTPHeaderMap& ResourceResponse::HttpHeaderFields() const {
  return http_header_fields_;
}

bool ResourceResponse::CacheControlContainsNoCache() const {
  if (!cache_control_header_.parsed) {
    cache_control_header_ = ParseCacheControlDirectives(
        http_header_fields_.Get(kCacheControlHeader),
        http_header_fields_.Get(kPragmaHeader));
  }
  return cache_control_header_.contains_no_cache;
}

bool ResourceResponse::CacheControlContainsNoStore() const {
  if (!cache_control_header_.parsed) {
    cache_control_header_ = ParseCacheControlDirectives(
        http_header_fields_.Get(kCacheControlHeader),
        http_header_fields_.Get(kPragmaHeader));
  }
  return cache_control_header_.contains_no_store;
}

bool ResourceResponse::CacheControlContainsMustRevalidate() const {
  if (!cache_control_header_.parsed) {
    cache_control_header_ = ParseCacheControlDirectives(
        http_header_fields_.Get(kCacheControlHeader),
        http_header_fields_.Get(kPragmaHeader));
  }
  return cache_control_header_.contains_must_revalidate;
}

bool ResourceResponse::HasCacheValidatorFields() const {
  static const char kLastModifiedHeader[] = "last-modified";
  static const char kETagHeader[] = "etag";
  return !http_header_fields_.Get(kLastModifiedHeader).IsEmpty() ||
         !http_header_fields_.Get(kETagHeader).IsEmpty();
}

double ResourceResponse::CacheControlMaxAge() const {
  if (!cache_control_header_.parsed) {
    cache_control_header_ = ParseCacheControlDirectives(
        http_header_fields_.Get(kCacheControlHeader),
        http_header_fields_.Get(kPragmaHeader));
  }
  return cache_control_header_.max_age;
}

static double ParseDateValueInHeader(const HTTPHeaderMap& headers,
                                     const AtomicString& header_name) {
  const AtomicString& header_value = headers.Get(header_name);
  if (header_value.IsEmpty())
    return std::numeric_limits<double>::quiet_NaN();
  // This handles all date formats required by RFC2616:
  // Sun, 06 Nov 1994 08:49:37 GMT  ; RFC 822, updated by RFC 1123
  // Sunday, 06-Nov-94 08:49:37 GMT ; RFC 850, obsoleted by RFC 1036
  // Sun Nov  6 08:49:37 1994       ; ANSI C's asctime() format
  double date_in_milliseconds = ParseDate(header_value);
  if (!std::isfinite(date_in_milliseconds))
    return std::numeric_limits<double>::quiet_NaN();
  return date_in_milliseconds / 1000;
}

double ResourceResponse::Date() const {
  if (!have_parsed_date_header_) {
    static const char kHeaderName[] = "date";
    date_ = ParseDateValueInHeader(http_header_fields_, kHeaderName);
    have_parsed_date_header_ = true;
  }
  return date_;
}

double ResourceResponse::Age() const {
  if (!have_parsed_age_header_) {
    static const char kHeaderName[] = "age";
    const AtomicString& header_value = http_header_fields_.Get(kHeaderName);
    bool ok;
    age_ = header_value.ToDouble(&ok);
    if (!ok)
      age_ = std::numeric_limits<double>::quiet_NaN();
    have_parsed_age_header_ = true;
  }
  return age_;
}

double ResourceResponse::Expires() const {
  if (!have_parsed_expires_header_) {
    static const char kHeaderName[] = "expires";
    expires_ = ParseDateValueInHeader(http_header_fields_, kHeaderName);
    have_parsed_expires_header_ = true;
  }
  return expires_;
}

double ResourceResponse::LastModified() const {
  if (!have_parsed_last_modified_header_) {
    static const char kHeaderName[] = "last-modified";
    last_modified_ = ParseDateValueInHeader(http_header_fields_, kHeaderName);
    have_parsed_last_modified_header_ = true;
  }
  return last_modified_;
}

bool ResourceResponse::IsAttachment() const {
  static const char kAttachmentString[] = "attachment";
  String value = http_header_fields_.Get(HTTPNames::Content_Disposition);
  size_t loc = value.find(';');
  if (loc != kNotFound)
    value = value.Left(loc);
  value = value.StripWhiteSpace();
  return DeprecatedEqualIgnoringCase(value, kAttachmentString);
}

AtomicString ResourceResponse::HttpContentType() const {
  return ExtractMIMETypeFromMediaType(
      HttpHeaderField(HTTPNames::Content_Type).DeprecatedLower());
}

bool ResourceResponse::WasCached() const {
  return was_cached_;
}

void ResourceResponse::SetWasCached(bool value) {
  was_cached_ = value;
}

bool ResourceResponse::ConnectionReused() const {
  return connection_reused_;
}

void ResourceResponse::SetConnectionReused(bool connection_reused) {
  connection_reused_ = connection_reused;
}

unsigned ResourceResponse::ConnectionID() const {
  return connection_id_;
}

void ResourceResponse::SetConnectionID(unsigned connection_id) {
  connection_id_ = connection_id;
}

ResourceLoadTiming* ResourceResponse::GetResourceLoadTiming() const {
  return resource_load_timing_.get();
}

void ResourceResponse::SetResourceLoadTiming(
    scoped_refptr<ResourceLoadTiming> resource_load_timing) {
  resource_load_timing_ = std::move(resource_load_timing);
}

scoped_refptr<ResourceLoadInfo> ResourceResponse::GetResourceLoadInfo() const {
  return resource_load_info_.get();
}

void ResourceResponse::SetResourceLoadInfo(
    scoped_refptr<ResourceLoadInfo> load_info) {
  resource_load_info_ = std::move(load_info);
}

void ResourceResponse::SetCTPolicyCompliance(CTPolicyCompliance compliance) {
  ct_policy_compliance_ = compliance;
}

bool ResourceResponse::IsOpaqueResponseFromServiceWorker() const {
  switch (response_type_via_service_worker_) {
    case network::mojom::FetchResponseType::kBasic:
    case network::mojom::FetchResponseType::kCORS:
    case network::mojom::FetchResponseType::kDefault:
    case network::mojom::FetchResponseType::kError:
      return false;
    case network::mojom::FetchResponseType::kOpaque:
    case network::mojom::FetchResponseType::kOpaqueRedirect:
      return true;
  }
  NOTREACHED();
  return false;
}

KURL ResourceResponse::OriginalURLViaServiceWorker() const {
  if (url_list_via_service_worker_.IsEmpty())
    return KURL();
  return url_list_via_service_worker_.back();
}

AtomicString ResourceResponse::ConnectionInfoString() const {
  std::string connection_info_string =
      net::HttpResponseInfo::ConnectionInfoToString(connection_info_);
  return AtomicString(
      reinterpret_cast<const LChar*>(connection_info_string.data()),
      connection_info_string.length());
}

void ResourceResponse::SetEncodedDataLength(long long value) {
  encoded_data_length_ = value;
}

void ResourceResponse::SetEncodedBodyLength(long long value) {
  encoded_body_length_ = value;
}

void ResourceResponse::SetDecodedBodyLength(long long value) {
  decoded_body_length_ = value;
}

void ResourceResponse::SetDownloadedFilePath(
    const String& downloaded_file_path) {
  downloaded_file_path_ = downloaded_file_path;
  if (downloaded_file_path_.IsEmpty()) {
    downloaded_file_handle_ = nullptr;
    return;
  }
  // TODO(dmurph): Investigate whether we need the mimeType on this blob.
  std::unique_ptr<BlobData> blob_data =
      BlobData::CreateForFileWithUnknownSize(downloaded_file_path_);
  blob_data->DetachFromCurrentThread();
  downloaded_file_handle_ = BlobDataHandle::Create(std::move(blob_data), -1);
}

void ResourceResponse::AppendRedirectResponse(
    const ResourceResponse& response) {
  redirect_responses_.push_back(response);
}

bool ResourceResponse::Compare(const ResourceResponse& a,
                               const ResourceResponse& b) {
  if (a.IsNull() != b.IsNull())
    return false;
  if (a.Url() != b.Url())
    return false;
  if (a.MimeType() != b.MimeType())
    return false;
  if (a.ExpectedContentLength() != b.ExpectedContentLength())
    return false;
  if (a.TextEncodingName() != b.TextEncodingName())
    return false;
  if (a.HttpStatusCode() != b.HttpStatusCode())
    return false;
  if (a.HttpStatusText() != b.HttpStatusText())
    return false;
  if (a.HttpHeaderFields() != b.HttpHeaderFields())
    return false;
  if (a.GetResourceLoadTiming() && b.GetResourceLoadTiming() &&
      *a.GetResourceLoadTiming() == *b.GetResourceLoadTiming())
    return true;
  if (a.GetResourceLoadTiming() != b.GetResourceLoadTiming())
    return false;
  if (a.EncodedBodyLength() != b.EncodedBodyLength())
    return false;
  if (a.DecodedBodyLength() != b.DecodedBodyLength())
    return false;
  return true;
}

STATIC_ASSERT_ENUM(WebURLResponse::kHTTPVersionUnknown,
                   ResourceResponse::kHTTPVersionUnknown);
STATIC_ASSERT_ENUM(WebURLResponse::kHTTPVersion_0_9,
                   ResourceResponse::kHTTPVersion_0_9);
STATIC_ASSERT_ENUM(WebURLResponse::kHTTPVersion_1_0,
                   ResourceResponse::kHTTPVersion_1_0);
STATIC_ASSERT_ENUM(WebURLResponse::kHTTPVersion_1_1,
                   ResourceResponse::kHTTPVersion_1_1);
STATIC_ASSERT_ENUM(WebURLResponse::kHTTPVersion_2_0,
                   ResourceResponse::kHTTPVersion_2_0);
}  // namespace blink
