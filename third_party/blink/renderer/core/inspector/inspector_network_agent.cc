/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/core/inspector/inspector_network_agent.h"

#include <memory>
#include <utility>

#include "base/macros.h"
#include "base/memory/scoped_refptr.h"
#include "build/build_config.h"
#include "services/network/public/mojom/request_context_frame_type.mojom-shared.h"
#include "third_party/blink/public/platform/task_type.h"
#include "third_party/blink/public/platform/web_effective_connection_type.h"
#include "third_party/blink/public/platform/web_mixed_content_context_type.h"
#include "third_party/blink/public/platform/web_url_loader_client.h"
#include "third_party/blink/public/platform/web_url_request.h"
#include "third_party/blink/renderer/bindings/core/v8/exception_state.h"
#include "third_party/blink/renderer/bindings/core/v8/source_location.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/scriptable_document_parser.h"
#include "third_party/blink/renderer/core/fileapi/file_reader_loader.h"
#include "third_party/blink/renderer/core/fileapi/file_reader_loader_client.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/html/html_frame_owner_element.h"
#include "third_party/blink/renderer/core/inspector/identifiers_factory.h"
#include "third_party/blink/renderer/core/inspector/inspected_frames.h"
#include "third_party/blink/renderer/core/inspector/network_resources_data.h"
#include "third_party/blink/renderer/core/loader/document_loader.h"
#include "third_party/blink/renderer/core/loader/frame_loader.h"
#include "third_party/blink/renderer/core/loader/mixed_content_checker.h"
#include "third_party/blink/renderer/core/loader/threadable_loader_client.h"
#include "third_party/blink/renderer/core/page/page.h"
#include "third_party/blink/renderer/core/probe/core_probes.h"
#include "third_party/blink/renderer/core/workers/worker_global_scope.h"
#include "third_party/blink/renderer/core/xmlhttprequest/xml_http_request.h"
#include "third_party/blink/renderer/platform/blob/blob_data.h"
#include "third_party/blink/renderer/platform/loader/fetch/fetch_initiator_info.h"
#include "third_party/blink/renderer/platform/loader/fetch/fetch_initiator_type_names.h"
#include "third_party/blink/renderer/platform/loader/fetch/memory_cache.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_error.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_fetcher.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_load_timing.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_request.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_response.h"
#include "third_party/blink/renderer/platform/loader/fetch/unique_identifier.h"
#include "third_party/blink/renderer/platform/network/http_header_map.h"
#include "third_party/blink/renderer/platform/network/network_state_notifier.h"
#include "third_party/blink/renderer/platform/network/web_socket_handshake_request.h"
#include "third_party/blink/renderer/platform/network/web_socket_handshake_response.h"
#include "third_party/blink/renderer/platform/runtime_enabled_features.h"
#include "third_party/blink/renderer/platform/weborigin/kurl.h"
#include "third_party/blink/renderer/platform/weborigin/referrer_policy.h"
#include "third_party/blink/renderer/platform/weborigin/security_origin.h"
#include "third_party/blink/renderer/platform/wtf/text/base64.h"
#include "third_party/blink/renderer/platform/wtf/time.h"

namespace blink {

using GetRequestPostDataCallback =
    protocol::Network::Backend::GetRequestPostDataCallback;
using GetResponseBodyCallback =
    protocol::Network::Backend::GetResponseBodyCallback;
using protocol::Response;

namespace NetworkAgentState {
static const char kNetworkAgentEnabled[] = "networkAgentEnabled";
static const char kExtraRequestHeaders[] = "extraRequestHeaders";
static const char kCacheDisabled[] = "cacheDisabled";
static const char kBypassServiceWorker[] = "bypassServiceWorker";
static const char kUserAgentOverride[] = "userAgentOverride";
static const char kBlockedURLs[] = "blockedURLs";
static const char kTotalBufferSize[] = "totalBufferSize";
static const char kResourceBufferSize[] = "resourceBufferSize";
static const char kMaxPostDataSize[] = "maxPostBodySize";
}  // namespace NetworkAgentState

namespace {

#if defined(OS_ANDROID)
// 10MB
static size_t g_maximum_total_buffer_size = 10 * 1000 * 1000;
// 5MB
static size_t g_maximum_resource_buffer_size = 5 * 1000 * 1000;
#else
// 100MB
static size_t g_maximum_total_buffer_size = 100 * 1000 * 1000;
// 10MB
static size_t g_maximum_resource_buffer_size = 10 * 1000 * 1000;
#endif

// Pattern may contain stars ('*') which match to any (possibly empty) string.
// Stars implicitly assumed at the begin/end of pattern.
bool Matches(const String& url, const String& pattern) {
  Vector<String> parts;
  pattern.Split("*", parts);
  size_t pos = 0;
  for (const String& part : parts) {
    pos = url.Find(part, pos);
    if (pos == kNotFound)
      return false;
    pos += part.length();
  }
  return true;
}

bool LoadsFromCacheOnly(const ResourceRequest& request) {
  switch (request.GetCacheMode()) {
    case mojom::FetchCacheMode::kDefault:
    case mojom::FetchCacheMode::kNoStore:
    case mojom::FetchCacheMode::kValidateCache:
    case mojom::FetchCacheMode::kBypassCache:
    case mojom::FetchCacheMode::kForceCache:
      return false;
    case mojom::FetchCacheMode::kOnlyIfCached:
    case mojom::FetchCacheMode::kUnspecifiedOnlyIfCachedStrict:
    case mojom::FetchCacheMode::kUnspecifiedForceCacheMiss:
      return true;
  }
  NOTREACHED();
  return false;
}

protocol::Network::CertificateTransparencyCompliance
SerializeCTPolicyCompliance(
    ResourceResponse::CTPolicyCompliance ct_compliance) {
  switch (ct_compliance) {
    case ResourceResponse::kCTPolicyComplianceDetailsNotAvailable:
      return protocol::Network::CertificateTransparencyComplianceEnum::Unknown;
    case ResourceResponse::kCTPolicyComplies:
      return protocol::Network::CertificateTransparencyComplianceEnum::
          Compliant;
    case ResourceResponse::kCTPolicyDoesNotComply:
      return protocol::Network::CertificateTransparencyComplianceEnum::
          NotCompliant;
  }
  NOTREACHED();
  return protocol::Network::CertificateTransparencyComplianceEnum::Unknown;
}

static std::unique_ptr<protocol::Network::Headers> BuildObjectForHeaders(
    const HTTPHeaderMap& headers) {
  std::unique_ptr<protocol::DictionaryValue> headers_object =
      protocol::DictionaryValue::create();
  for (const auto& header : headers)
    headers_object->setString(header.key.GetString(), header.value);
  protocol::ErrorSupport errors;
  return protocol::Network::Headers::fromValue(headers_object.get(), &errors);
}

class InspectorFileReaderLoaderClient final : public FileReaderLoaderClient {
 public:
  InspectorFileReaderLoaderClient(
      scoped_refptr<BlobDataHandle> blob,
      base::OnceCallback<void(scoped_refptr<SharedBuffer>)> callback)
      : blob_(std::move(blob)), callback_(std::move(callback)) {
    loader_ = FileReaderLoader::Create(FileReaderLoader::kReadByClient, this);
  }

  ~InspectorFileReaderLoaderClient() override = default;

  void Start() {
    raw_data_ = SharedBuffer::Create();
    loader_->Start(blob_);
  }

  void DidStartLoading() override {}

  void DidReceiveDataForClient(const char* data,
                               unsigned data_length) override {
    if (!data_length)
      return;
    raw_data_->Append(data, data_length);
  }

  void DidFinishLoading() override { Done(raw_data_); }

  void DidFail(FileError::ErrorCode) override { Done(nullptr); }

 private:
  void Done(scoped_refptr<SharedBuffer> output) {
    std::move(callback_).Run(output);
    delete this;
  }

  scoped_refptr<BlobDataHandle> blob_;
  String mime_type_;
  String text_encoding_name_;
  base::OnceCallback<void(scoped_refptr<SharedBuffer>)> callback_;
  std::unique_ptr<FileReaderLoader> loader_;
  scoped_refptr<SharedBuffer> raw_data_;
  DISALLOW_COPY_AND_ASSIGN(InspectorFileReaderLoaderClient);
};

static void ResponseBodyFileReaderLoaderDone(
    const String& mime_type,
    const String& text_encoding_name,
    std::unique_ptr<GetResponseBodyCallback> callback,
    scoped_refptr<SharedBuffer> raw_data) {
  if (!raw_data) {
    callback->sendFailure(Response::Error("Couldn't read BLOB"));
    return;
  }
  String result;
  bool base64_encoded;
  if (InspectorPageAgent::SharedBufferContent(
          raw_data, mime_type, text_encoding_name, &result, &base64_encoded)) {
    callback->sendSuccess(result, base64_encoded);
  } else {
    callback->sendFailure(Response::Error("Couldn't encode data"));
  }
}

class InspectorPostBodyParser
    : public WTF::RefCounted<InspectorPostBodyParser> {
 public:
  explicit InspectorPostBodyParser(
      std::unique_ptr<GetRequestPostDataCallback> callback)
      : callback_(std::move(callback)), error_(false) {}

  void Parse(EncodedFormData* request_body) {
    if (!request_body || request_body->IsEmpty())
      return;

    parts_.Grow(request_body->Elements().size());
    for (size_t i = 0; i < request_body->Elements().size(); i++) {
      const FormDataElement& data = request_body->Elements()[i];
      switch (data.type_) {
        case FormDataElement::kData:
          parts_[i] = String::FromUTF8WithLatin1Fallback(data.data_.data(),
                                                         data.data_.size());
          break;
        case FormDataElement::kEncodedBlob:
          ReadDataBlob(data.optional_blob_data_handle_, &parts_[i]);
          break;
        case FormDataElement::kEncodedFile:
        case FormDataElement::kDataPipe:
          // Do nothing, not supported
          break;
      }
    }
  }

 private:
  friend class WTF::RefCounted<InspectorPostBodyParser>;

  ~InspectorPostBodyParser() {
    if (error_)
      return;
    String result;
    for (const auto& part : parts_)
      result.append(part);
    callback_->sendSuccess(result);
  }

  void BlobReadCallback(String* destination,
                        scoped_refptr<SharedBuffer> raw_data) {
    if (raw_data) {
      *destination = String::FromUTF8WithLatin1Fallback(raw_data->Data(),
                                                        raw_data->size());
    } else {
      error_ = true;
    }
  }

  void ReadDataBlob(scoped_refptr<blink::BlobDataHandle> blob_handle,
                    String* destination) {
    if (!blob_handle)
      return;
    auto* reader = new InspectorFileReaderLoaderClient(
        blob_handle,
        WTF::Bind(&InspectorPostBodyParser::BlobReadCallback,
                  WTF::RetainedRef(this), WTF::Unretained(destination)));
    reader->Start();
  }

  std::unique_ptr<GetRequestPostDataCallback> callback_;
  bool error_;
  Vector<String> parts_;
  DISALLOW_COPY_AND_ASSIGN(InspectorPostBodyParser);
};

KURL UrlWithoutFragment(const KURL& url) {
  KURL result = url;
  result.RemoveFragmentIdentifier();
  return result;
}

String MixedContentTypeForContextType(WebMixedContentContextType context_type) {
  switch (context_type) {
    case WebMixedContentContextType::kNotMixedContent:
      return protocol::Security::MixedContentTypeEnum::None;
    case WebMixedContentContextType::kBlockable:
      return protocol::Security::MixedContentTypeEnum::Blockable;
    case WebMixedContentContextType::kOptionallyBlockable:
    case WebMixedContentContextType::kShouldBeBlockable:
      return protocol::Security::MixedContentTypeEnum::OptionallyBlockable;
  }

  return protocol::Security::MixedContentTypeEnum::None;
}

String ResourcePriorityJSON(ResourceLoadPriority priority) {
  switch (priority) {
    case ResourceLoadPriority::kVeryLow:
      return protocol::Network::ResourcePriorityEnum::VeryLow;
    case ResourceLoadPriority::kLow:
      return protocol::Network::ResourcePriorityEnum::Low;
    case ResourceLoadPriority::kMedium:
      return protocol::Network::ResourcePriorityEnum::Medium;
    case ResourceLoadPriority::kHigh:
      return protocol::Network::ResourcePriorityEnum::High;
    case ResourceLoadPriority::kVeryHigh:
      return protocol::Network::ResourcePriorityEnum::VeryHigh;
    case ResourceLoadPriority::kUnresolved:
      break;
  }
  NOTREACHED();
  return protocol::Network::ResourcePriorityEnum::Medium;
}

String BuildBlockedReason(ResourceRequestBlockedReason reason) {
  switch (reason) {
    case ResourceRequestBlockedReason::kCSP:
      return protocol::Network::BlockedReasonEnum::Csp;
    case ResourceRequestBlockedReason::kMixedContent:
      return protocol::Network::BlockedReasonEnum::MixedContent;
    case ResourceRequestBlockedReason::kOrigin:
      return protocol::Network::BlockedReasonEnum::Origin;
    case ResourceRequestBlockedReason::kInspector:
      return protocol::Network::BlockedReasonEnum::Inspector;
    case ResourceRequestBlockedReason::kSubresourceFilter:
      return protocol::Network::BlockedReasonEnum::SubresourceFilter;
    case ResourceRequestBlockedReason::kContentType:
      return protocol::Network::BlockedReasonEnum::ContentType;
    case ResourceRequestBlockedReason::kOther:
      return protocol::Network::BlockedReasonEnum::Other;
  }
  NOTREACHED();
  return protocol::Network::BlockedReasonEnum::Other;
}

WebConnectionType ToWebConnectionType(const String& connection_type) {
  if (connection_type == protocol::Network::ConnectionTypeEnum::None)
    return kWebConnectionTypeNone;
  if (connection_type == protocol::Network::ConnectionTypeEnum::Cellular2g)
    return kWebConnectionTypeCellular2G;
  if (connection_type == protocol::Network::ConnectionTypeEnum::Cellular3g)
    return kWebConnectionTypeCellular3G;
  if (connection_type == protocol::Network::ConnectionTypeEnum::Cellular4g)
    return kWebConnectionTypeCellular4G;
  if (connection_type == protocol::Network::ConnectionTypeEnum::Bluetooth)
    return kWebConnectionTypeBluetooth;
  if (connection_type == protocol::Network::ConnectionTypeEnum::Ethernet)
    return kWebConnectionTypeEthernet;
  if (connection_type == protocol::Network::ConnectionTypeEnum::Wifi)
    return kWebConnectionTypeWifi;
  if (connection_type == protocol::Network::ConnectionTypeEnum::Wimax)
    return kWebConnectionTypeWimax;
  if (connection_type == protocol::Network::ConnectionTypeEnum::Other)
    return kWebConnectionTypeOther;
  return kWebConnectionTypeUnknown;
}

String GetReferrerPolicy(ReferrerPolicy policy) {
  switch (policy) {
    case kReferrerPolicyAlways:
      return protocol::Network::Request::ReferrerPolicyEnum::UnsafeUrl;
    case kReferrerPolicyDefault:
      if (RuntimeEnabledFeatures::ReducedReferrerGranularityEnabled()) {
        return protocol::Network::Request::ReferrerPolicyEnum::
            StrictOriginWhenCrossOrigin;
      } else {
        return protocol::Network::Request::ReferrerPolicyEnum::
            NoReferrerWhenDowngrade;
      }
    case kReferrerPolicyNoReferrerWhenDowngrade:
      return protocol::Network::Request::ReferrerPolicyEnum::
          NoReferrerWhenDowngrade;
    case kReferrerPolicyNever:
      return protocol::Network::Request::ReferrerPolicyEnum::NoReferrer;
    case kReferrerPolicyOrigin:
      return protocol::Network::Request::ReferrerPolicyEnum::Origin;
    case kReferrerPolicyOriginWhenCrossOrigin:
      return protocol::Network::Request::ReferrerPolicyEnum::
          OriginWhenCrossOrigin;
    case kReferrerPolicySameOrigin:
      return protocol::Network::Request::ReferrerPolicyEnum::SameOrigin;
    case kReferrerPolicyStrictOrigin:
      return protocol::Network::Request::ReferrerPolicyEnum::StrictOrigin;
    case kReferrerPolicyStrictOriginWhenCrossOrigin:
      return protocol::Network::Request::ReferrerPolicyEnum::
          StrictOriginWhenCrossOrigin;
  }

  return protocol::Network::Request::ReferrerPolicyEnum::
      NoReferrerWhenDowngrade;
}
}  // namespace

void InspectorNetworkAgent::Restore() {
  if (state_->booleanProperty(NetworkAgentState::kNetworkAgentEnabled, false)) {
    Enable(state_->integerProperty(NetworkAgentState::kTotalBufferSize,
                                   g_maximum_total_buffer_size),
           state_->integerProperty(NetworkAgentState::kResourceBufferSize,
                                   g_maximum_resource_buffer_size),
           state_->integerProperty(NetworkAgentState::kMaxPostDataSize, 0));
  }
}

static std::unique_ptr<protocol::Network::ResourceTiming> BuildObjectForTiming(
    const ResourceLoadTiming& timing) {
  return protocol::Network::ResourceTiming::create()
      .setRequestTime(TimeTicksInSeconds(timing.RequestTime()))
      .setProxyStart(timing.CalculateMillisecondDelta(timing.ProxyStart()))
      .setProxyEnd(timing.CalculateMillisecondDelta(timing.ProxyEnd()))
      .setDnsStart(timing.CalculateMillisecondDelta(timing.DnsStart()))
      .setDnsEnd(timing.CalculateMillisecondDelta(timing.DnsEnd()))
      .setConnectStart(timing.CalculateMillisecondDelta(timing.ConnectStart()))
      .setConnectEnd(timing.CalculateMillisecondDelta(timing.ConnectEnd()))
      .setSslStart(timing.CalculateMillisecondDelta(timing.SslStart()))
      .setSslEnd(timing.CalculateMillisecondDelta(timing.SslEnd()))
      .setWorkerStart(timing.CalculateMillisecondDelta(timing.WorkerStart()))
      .setWorkerReady(timing.CalculateMillisecondDelta(timing.WorkerReady()))
      .setSendStart(timing.CalculateMillisecondDelta(timing.SendStart()))
      .setSendEnd(timing.CalculateMillisecondDelta(timing.SendEnd()))
      .setReceiveHeadersEnd(
          timing.CalculateMillisecondDelta(timing.ReceiveHeadersEnd()))
      .setPushStart(TimeTicksInSeconds(timing.PushStart()))
      .setPushEnd(TimeTicksInSeconds(timing.PushEnd()))
      .build();
}

static bool FormDataToString(scoped_refptr<EncodedFormData> body,
                             size_t max_body_size,
                             String* content) {
  *content = "";
  if (!body || body->IsEmpty())
    return false;

  // SizeInBytes below doesn't support all element types, so first check if all
  // the body elements are of the right type.
  for (const auto& element : body->Elements()) {
    if (element.type_ != FormDataElement::kData)
      return true;
  }

  if (max_body_size != 0 && body->SizeInBytes() > max_body_size)
    return true;

  Vector<char> bytes;
  body->Flatten(bytes);
  *content = String::FromUTF8WithLatin1Fallback(bytes.data(), bytes.size());
  return true;
}

static std::unique_ptr<protocol::Network::Request>
BuildObjectForResourceRequest(const ResourceRequest& request,
                              size_t max_body_size) {
  String postData;
  bool hasPostData =
      FormDataToString(request.HttpBody(), max_body_size, &postData);
  std::unique_ptr<protocol::Network::Request> result =
      protocol::Network::Request::create()
          .setUrl(UrlWithoutFragment(request.Url()).GetString())
          .setMethod(request.HttpMethod())
          .setHeaders(BuildObjectForHeaders(request.HttpHeaderFields()))
          .setInitialPriority(ResourcePriorityJSON(request.Priority()))
          .setReferrerPolicy(GetReferrerPolicy(request.GetReferrerPolicy()))
          .build();
  if (!postData.IsEmpty())
    result->setPostData(postData);
  if (hasPostData)
    result->setHasPostData(true);
  return result;
}

static std::unique_ptr<protocol::Network::Response>
BuildObjectForResourceResponse(const ResourceResponse& response,
                               Resource* cached_resource = nullptr,
                               bool* is_empty = nullptr) {
  if (response.IsNull())
    return nullptr;

  int status;
  String status_text;
  if (response.GetResourceLoadInfo() &&
      response.GetResourceLoadInfo()->http_status_code) {
    status = response.GetResourceLoadInfo()->http_status_code;
    status_text = response.GetResourceLoadInfo()->http_status_text;
  } else {
    status = response.HttpStatusCode();
    status_text = response.HttpStatusText();
  }
  HTTPHeaderMap headers_map;
  if (response.GetResourceLoadInfo() &&
      response.GetResourceLoadInfo()->response_headers.size())
    headers_map = response.GetResourceLoadInfo()->response_headers;
  else
    headers_map = response.HttpHeaderFields();

  int64_t encoded_data_length = response.EncodedDataLength();

  String security_state = protocol::Security::SecurityStateEnum::Unknown;
  switch (response.GetSecurityStyle()) {
    case ResourceResponse::kSecurityStyleUnknown:
      security_state = protocol::Security::SecurityStateEnum::Unknown;
      break;
    case ResourceResponse::kSecurityStyleUnauthenticated:
      security_state = protocol::Security::SecurityStateEnum::Neutral;
      break;
    case ResourceResponse::kSecurityStyleAuthenticationBroken:
      security_state = protocol::Security::SecurityStateEnum::Insecure;
      break;
    case ResourceResponse::kSecurityStyleAuthenticated:
      security_state = protocol::Security::SecurityStateEnum::Secure;
      break;
  }

  // Use mime type from cached resource in case the one in response is empty.
  String mime_type = response.MimeType();
  if (mime_type.IsEmpty() && cached_resource)
    mime_type = cached_resource->GetResponse().MimeType();

  if (is_empty)
    *is_empty = !status && mime_type.IsEmpty() && !headers_map.size();

  std::unique_ptr<protocol::Network::Response> response_object =
      protocol::Network::Response::create()
          .setUrl(UrlWithoutFragment(response.Url()).GetString())
          .setStatus(status)
          .setStatusText(status_text)
          .setHeaders(BuildObjectForHeaders(headers_map))
          .setMimeType(mime_type)
          .setConnectionReused(response.ConnectionReused())
          .setConnectionId(response.ConnectionID())
          .setEncodedDataLength(encoded_data_length)
          .setSecurityState(security_state)
          .build();

  response_object->setFromDiskCache(response.WasCached());
  response_object->setFromServiceWorker(response.WasFetchedViaServiceWorker());
  if (response.GetResourceLoadTiming())
    response_object->setTiming(
        BuildObjectForTiming(*response.GetResourceLoadTiming()));

  if (response.GetResourceLoadInfo()) {
    if (!response.GetResourceLoadInfo()->response_headers_text.IsEmpty()) {
      response_object->setHeadersText(
          response.GetResourceLoadInfo()->response_headers_text);
    }
    if (response.GetResourceLoadInfo()->request_headers.size()) {
      response_object->setRequestHeaders(BuildObjectForHeaders(
          response.GetResourceLoadInfo()->request_headers));
    }
    if (!response.GetResourceLoadInfo()->request_headers_text.IsEmpty()) {
      response_object->setRequestHeadersText(
          response.GetResourceLoadInfo()->request_headers_text);
    }
  }

  String remote_ip_address = response.RemoteIPAddress();
  if (!remote_ip_address.IsEmpty()) {
    response_object->setRemoteIPAddress(remote_ip_address);
    response_object->setRemotePort(response.RemotePort());
  }

  String protocol;
  if (response.GetResourceLoadInfo())
    protocol = response.GetResourceLoadInfo()->npn_negotiated_protocol;
  if (protocol.IsEmpty() || protocol == "unknown") {
    if (response.WasFetchedViaSPDY()) {
      protocol = "spdy";
    } else if (response.IsHTTP()) {
      protocol = "http";
      if (response.HttpVersion() ==
          ResourceResponse::HTTPVersion::kHTTPVersion_0_9)
        protocol = "http/0.9";
      else if (response.HttpVersion() ==
               ResourceResponse::HTTPVersion::kHTTPVersion_1_0)
        protocol = "http/1.0";
      else if (response.HttpVersion() ==
               ResourceResponse::HTTPVersion::kHTTPVersion_1_1)
        protocol = "http/1.1";
    } else {
      protocol = response.Url().Protocol();
    }
  }
  response_object->setProtocol(protocol);

  if (response.GetSecurityStyle() != ResourceResponse::kSecurityStyleUnknown &&
      response.GetSecurityStyle() !=
          ResourceResponse::kSecurityStyleUnauthenticated) {
    const ResourceResponse::SecurityDetails* response_security_details =
        response.GetSecurityDetails();

    std::unique_ptr<protocol::Array<String>> san_list =
        protocol::Array<String>::create();
    for (auto const& san : response_security_details->san_list)
      san_list->addItem(san);

    std::unique_ptr<
        protocol::Array<protocol::Network::SignedCertificateTimestamp>>
        signed_certificate_timestamp_list = protocol::Array<
            protocol::Network::SignedCertificateTimestamp>::create();
    for (auto const& sct : response_security_details->sct_list) {
      std::unique_ptr<protocol::Network::SignedCertificateTimestamp>
          signed_certificate_timestamp =
              protocol::Network::SignedCertificateTimestamp::create()
                  .setStatus(sct.status_)
                  .setOrigin(sct.origin_)
                  .setLogDescription(sct.log_description_)
                  .setLogId(sct.log_id_)
                  .setTimestamp(sct.timestamp_)
                  .setHashAlgorithm(sct.hash_algorithm_)
                  .setSignatureAlgorithm(sct.signature_algorithm_)
                  .setSignatureData(sct.signature_data_)
                  .build();
      signed_certificate_timestamp_list->addItem(
          std::move(signed_certificate_timestamp));
    }

    std::unique_ptr<protocol::Network::SecurityDetails> security_details =
        protocol::Network::SecurityDetails::create()
            .setProtocol(response_security_details->protocol)
            .setKeyExchange(response_security_details->key_exchange)
            .setCipher(response_security_details->cipher)
            .setSubjectName(response_security_details->subject_name)
            .setSanList(std::move(san_list))
            .setIssuer(response_security_details->issuer)
            .setValidFrom(response_security_details->valid_from)
            .setValidTo(response_security_details->valid_to)
            .setCertificateId(0)  // Keep this in protocol for compatability.
            .setSignedCertificateTimestampList(
                std::move(signed_certificate_timestamp_list))
            .setCertificateTransparencyCompliance(
                SerializeCTPolicyCompliance(response.GetCTPolicyCompliance()))
            .build();
    if (response_security_details->key_exchange_group.length() > 0)
      security_details->setKeyExchangeGroup(
          response_security_details->key_exchange_group);
    if (response_security_details->mac.length() > 0)
      security_details->setMac(response_security_details->mac);

    response_object->setSecurityDetails(std::move(security_details));
  }

  return response_object;
}

InspectorNetworkAgent::~InspectorNetworkAgent() = default;

void InspectorNetworkAgent::Trace(blink::Visitor* visitor) {
  visitor->Trace(inspected_frames_);
  visitor->Trace(worker_global_scope_);
  visitor->Trace(resources_data_);
  visitor->Trace(replay_xhrs_);
  visitor->Trace(replay_xhrs_to_be_deleted_);
  visitor->Trace(pending_xhr_replay_data_);
  InspectorBaseAgent::Trace(visitor);
}

void InspectorNetworkAgent::ShouldBlockRequest(const KURL& url, bool* result) {
  protocol::DictionaryValue* blocked_urls =
      state_->getObject(NetworkAgentState::kBlockedURLs);
  if (!blocked_urls)
    return;

  String url_string = url.GetString();
  for (size_t i = 0; i < blocked_urls->size(); ++i) {
    auto entry = blocked_urls->at(i);
    if (Matches(url_string, entry.first)) {
      *result = true;
      return;
    }
  }
  return;
}

void InspectorNetworkAgent::ShouldBypassServiceWorker(bool* result) {
  *result =
      state_->booleanProperty(NetworkAgentState::kBypassServiceWorker, false);
}

void InspectorNetworkAgent::DidBlockRequest(
    ExecutionContext* execution_context,
    const ResourceRequest& request,
    DocumentLoader* loader,
    const FetchInitiatorInfo& initiator_info,
    ResourceRequestBlockedReason reason,
    Resource::Type resource_type) {
  unsigned long identifier = CreateUniqueIdentifier();
  InspectorPageAgent::ResourceType type =
      InspectorPageAgent::ToResourceType(resource_type);

  WillSendRequestInternal(execution_context, identifier, loader, request,
                          ResourceResponse(), initiator_info, type);

  String request_id = IdentifiersFactory::RequestId(loader, identifier);
  String protocol_reason = BuildBlockedReason(reason);
  GetFrontend()->loadingFailed(
      request_id, CurrentTimeTicksInSeconds(),
      InspectorPageAgent::ResourceTypeJson(
          resources_data_->GetResourceType(request_id)),
      String(), false, protocol_reason);
}

void InspectorNetworkAgent::DidChangeResourcePriority(
    DocumentLoader* loader,
    unsigned long identifier,
    ResourceLoadPriority load_priority) {
  String request_id = IdentifiersFactory::RequestId(loader, identifier);
  GetFrontend()->resourceChangedPriority(request_id,
                                         ResourcePriorityJSON(load_priority),
                                         CurrentTimeTicksInSeconds());
}

void InspectorNetworkAgent::WillSendRequestInternal(
    ExecutionContext* execution_context,
    unsigned long identifier,
    DocumentLoader* loader,
    const ResourceRequest& request,
    const ResourceResponse& redirect_response,
    const FetchInitiatorInfo& initiator_info,
    InspectorPageAgent::ResourceType type) {
  String loader_id = IdentifiersFactory::LoaderId(loader);
  // DocumentLoader doesn't have main resource set at the point, so RequestId()
  // won't properly detect main resource. Workaround this by checking the
  // frame type and manually setting request id to loader id.
  String request_id = IdentifiersFactory::RequestId(loader, identifier);
  bool is_navigation =
      request.GetFrameType() != network::mojom::RequestContextFrameType::kNone;
  if (is_navigation)
    request_id = loader_id;
  NetworkResourcesData::ResourceData const* data =
      resources_data_->Data(request_id);
  // Support for POST request redirect
  scoped_refptr<EncodedFormData> post_data;
  if (data)
    post_data = data->PostData();
  else if (request.HttpBody())
    post_data = request.HttpBody()->DeepCopy();

  resources_data_->ResourceCreated(execution_context, request_id, loader_id,
                                   request.Url(), post_data);
  if (initiator_info.name == FetchInitiatorTypeNames::xmlhttprequest)
    type = InspectorPageAgent::kXHRResource;

  resources_data_->SetResourceType(request_id, type);

  if (is_navigation)
    return;

  String frame_id = loader && loader->GetFrame()
                        ? IdentifiersFactory::FrameId(loader->GetFrame())
                        : "";
  std::unique_ptr<protocol::Network::Initiator> initiator_object =
      BuildInitiatorObject(loader && loader->GetFrame()
                               ? loader->GetFrame()->GetDocument()
                               : nullptr,
                           initiator_info);

  std::unique_ptr<protocol::Network::Request> request_info(
      BuildObjectForResourceRequest(request, max_post_data_size_));

  // |loader| is null while inspecting worker.
  // TODO(horo): Refactor MixedContentChecker and set mixed content type even if
  // |loader| is null.
  if (loader) {
    request_info->setMixedContentType(MixedContentTypeForContextType(
        MixedContentChecker::ContextTypeForInspector(loader->GetFrame(),
                                                     request)));
  }

  request_info->setReferrerPolicy(
      GetReferrerPolicy(request.GetReferrerPolicy()));
  if (initiator_info.is_link_preload)
    request_info->setIsLinkPreload(true);

  String resource_type = InspectorPageAgent::ResourceTypeJson(type);
  String documentURL =
      loader ? UrlWithoutFragment(loader->Url()).GetString()
             : UrlWithoutFragment(execution_context->Url()).GetString();
  Maybe<String> maybe_frame_id;
  if (!frame_id.IsEmpty())
    maybe_frame_id = frame_id;
  GetFrontend()->requestWillBeSent(
      request_id, loader_id, documentURL, std::move(request_info),
      CurrentTimeTicksInSeconds(), CurrentTime(), std::move(initiator_object),
      BuildObjectForResourceResponse(redirect_response), resource_type,
      std::move(maybe_frame_id), request.HasUserGesture());
  if (pending_xhr_replay_data_ && !pending_xhr_replay_data_->Async())
    GetFrontend()->flush();
}

void InspectorNetworkAgent::WillSendRequest(
    ExecutionContext* execution_context,
    unsigned long identifier,
    DocumentLoader* loader,
    ResourceRequest& request,
    const ResourceResponse& redirect_response,
    const FetchInitiatorInfo& initiator_info,
    Resource::Type resource_type) {
  // Ignore the request initiated internally.
  if (initiator_info.name == FetchInitiatorTypeNames::internal)
    return;

  if (initiator_info.name == FetchInitiatorTypeNames::document &&
      loader->GetSubstituteData().IsValid())
    return;

  protocol::DictionaryValue* headers =
      state_->getObject(NetworkAgentState::kExtraRequestHeaders);
  if (headers) {
    for (size_t i = 0; i < headers->size(); ++i) {
      auto header = headers->at(i);
      AtomicString header_name = AtomicString(header.first);
      String value;
      if (!header.second->asString(&value))
        continue;
      // When overriding referer, also override referrer policy
      // for this request to assure the request will be allowed.
      if (header_name.LowerASCII() == HTTPNames::Referer.LowerASCII())
        request.SetHTTPReferrer(Referrer(value, kReferrerPolicyAlways));
      else
        request.SetHTTPHeaderField(header_name, AtomicString(value));
    }
  }

  request.SetReportRawHeaders(true);

  if (state_->booleanProperty(NetworkAgentState::kCacheDisabled, false)) {
    if (LoadsFromCacheOnly(request) &&
        request.GetRequestContext() != WebURLRequest::kRequestContextInternal) {
      request.SetCacheMode(mojom::FetchCacheMode::kUnspecifiedForceCacheMiss);
    } else {
      request.SetCacheMode(mojom::FetchCacheMode::kBypassCache);
    }
    request.SetShouldResetAppCache(true);
  }
  if (state_->booleanProperty(NetworkAgentState::kBypassServiceWorker, false))
    request.SetSkipServiceWorker(true);

  InspectorPageAgent::ResourceType type =
      InspectorPageAgent::ToResourceType(resource_type);

  WillSendRequestInternal(execution_context, identifier, loader, request,
                          redirect_response, initiator_info, type);

  if (!conditions_token_.IsEmpty()) {
    request.AddHTTPHeaderField(
        HTTPNames::X_DevTools_Emulate_Network_Conditions_Client_Id,
        AtomicString(conditions_token_));
  }
}

void InspectorNetworkAgent::MarkResourceAsCached(DocumentLoader* loader,
                                                 unsigned long identifier) {
  GetFrontend()->requestServedFromCache(
      IdentifiersFactory::RequestId(loader, identifier));
}

void InspectorNetworkAgent::DidReceiveResourceResponse(
    unsigned long identifier,
    DocumentLoader* loader,
    const ResourceResponse& response,
    Resource* cached_resource) {
  String request_id = IdentifiersFactory::RequestId(loader, identifier);
  bool is_not_modified = response.HttpStatusCode() == 304;

  bool resource_is_empty = true;
  std::unique_ptr<protocol::Network::Response> resource_response =
      BuildObjectForResourceResponse(response, cached_resource,
                                     &resource_is_empty);

  InspectorPageAgent::ResourceType type =
      cached_resource
          ? InspectorPageAgent::ToResourceType(cached_resource->GetType())
          : InspectorPageAgent::kOtherResource;
  // Override with already discovered resource type.
  InspectorPageAgent::ResourceType saved_type =
      resources_data_->GetResourceType(request_id);
  if (saved_type == InspectorPageAgent::kScriptResource ||
      saved_type == InspectorPageAgent::kXHRResource ||
      saved_type == InspectorPageAgent::kDocumentResource ||
      saved_type == InspectorPageAgent::kFetchResource ||
      saved_type == InspectorPageAgent::kEventSourceResource) {
    type = saved_type;
  }
  if (type == InspectorPageAgent::kDocumentResource && loader &&
      loader->GetSubstituteData().IsValid())
    return;

  // Resources are added to NetworkResourcesData as a WeakMember here and
  // removed in willDestroyResource() called in the prefinalizer of Resource.
  // Because NetworkResourceData retains weak references only, it
  // doesn't affect Resource lifetime.
  if (cached_resource)
    resources_data_->AddResource(request_id, cached_resource);
  String frame_id = loader && loader->GetFrame()
                        ? IdentifiersFactory::FrameId(loader->GetFrame())
                        : "";
  String loader_id = IdentifiersFactory::LoaderId(loader);
  resources_data_->ResponseReceived(request_id, frame_id, response);
  resources_data_->SetResourceType(request_id, type);

  if (response.GetSecurityStyle() != ResourceResponse::kSecurityStyleUnknown &&
      response.GetSecurityStyle() !=
          ResourceResponse::kSecurityStyleUnauthenticated) {
    const ResourceResponse::SecurityDetails* response_security_details =
        response.GetSecurityDetails();
    resources_data_->SetCertificate(request_id,
                                    response_security_details->certificate);
  }

  if (IsNavigation(loader, identifier))
    return;
  if (resource_response && !resource_is_empty) {
    Maybe<String> maybe_frame_id;
    if (!frame_id.IsEmpty())
      maybe_frame_id = frame_id;
    GetFrontend()->responseReceived(
        request_id, loader_id, CurrentTimeTicksInSeconds(),
        InspectorPageAgent::ResourceTypeJson(type),
        std::move(resource_response), std::move(maybe_frame_id));
  }
  // If we revalidated the resource and got Not modified, send content length
  // following didReceiveResponse as there will be no calls to didReceiveData
  // from the network stack.
  if (is_not_modified && cached_resource && cached_resource->EncodedSize())
    DidReceiveData(identifier, loader, nullptr, cached_resource->EncodedSize());
}

static bool IsErrorStatusCode(int status_code) {
  return status_code >= 400;
}

void InspectorNetworkAgent::DidReceiveData(unsigned long identifier,
                                           DocumentLoader* loader,
                                           const char* data,
                                           int data_length) {
  String request_id = IdentifiersFactory::RequestId(loader, identifier);

  if (data) {
    NetworkResourcesData::ResourceData const* resource_data =
        resources_data_->Data(request_id);
    if (resource_data &&
        (!resource_data->CachedResource() ||
         resource_data->CachedResource()->GetDataBufferingPolicy() ==
             kDoNotBufferData ||
         IsErrorStatusCode(resource_data->HttpStatusCode())))
      resources_data_->MaybeAddResourceData(request_id, data, data_length);
  }

  GetFrontend()->dataReceived(
      request_id, CurrentTimeTicksInSeconds(), data_length,
      resources_data_->GetAndClearPendingEncodedDataLength(request_id));
}

void InspectorNetworkAgent::DidReceiveBlob(unsigned long identifier,
                                           DocumentLoader* loader,
                                           scoped_refptr<BlobDataHandle> blob) {
  String request_id = IdentifiersFactory::RequestId(loader, identifier);
  resources_data_->BlobReceived(request_id, std::move(blob));
}

void InspectorNetworkAgent::DidReceiveEncodedDataLength(
    DocumentLoader* loader,
    unsigned long identifier,
    int encoded_data_length) {
  String request_id = IdentifiersFactory::RequestId(loader, identifier);
  resources_data_->AddPendingEncodedDataLength(request_id, encoded_data_length);
}

void InspectorNetworkAgent::DidFinishLoading(unsigned long identifier,
                                             DocumentLoader* loader,
                                             TimeTicks monotonic_finish_time,
                                             int64_t encoded_data_length,
                                             int64_t decoded_body_length,
                                             bool blocked_cross_site_document) {
  String request_id = IdentifiersFactory::RequestId(loader, identifier);
  NetworkResourcesData::ResourceData const* resource_data =
      resources_data_->Data(request_id);

  int pending_encoded_data_length =
      resources_data_->GetAndClearPendingEncodedDataLength(request_id);
  if (pending_encoded_data_length > 0) {
    GetFrontend()->dataReceived(request_id, CurrentTimeTicksInSeconds(), 0,
                                pending_encoded_data_length);
  }

  if (resource_data &&
      (!resource_data->CachedResource() ||
       resource_data->CachedResource()->GetDataBufferingPolicy() ==
           kDoNotBufferData ||
       IsErrorStatusCode(resource_data->HttpStatusCode()))) {
    resources_data_->MaybeAddResourceData(request_id, "", 0);
  }

  resources_data_->MaybeDecodeDataToContent(request_id);
  if (monotonic_finish_time.is_null())
    monotonic_finish_time = CurrentTimeTicks();

  // TODO(npm): Use TimeTicks in Network.h.
  GetFrontend()->loadingFinished(
      request_id, TimeTicksInSeconds(monotonic_finish_time),
      encoded_data_length, blocked_cross_site_document);
}

void InspectorNetworkAgent::DidReceiveCORSRedirectResponse(
    unsigned long identifier,
    DocumentLoader* loader,
    const ResourceResponse& response,
    Resource* resource) {
  // Update the response and finish loading
  DidReceiveResourceResponse(identifier, loader, response, resource);
  DidFinishLoading(identifier, loader, TimeTicks(),
                   WebURLLoaderClient::kUnknownEncodedDataLength, 0, false);
}

void InspectorNetworkAgent::DidFailLoading(unsigned long identifier,
                                           DocumentLoader* loader,
                                           const ResourceError& error) {
  String request_id = IdentifiersFactory::RequestId(loader, identifier);
  bool canceled = error.IsCancellation();
  base::Optional<ResourceRequestBlockedReason> resource_request_blocked_reason =
      error.GetResourceRequestBlockedReason();
  blink::protocol::Maybe<String> blocked_reason;
  if (resource_request_blocked_reason) {
    blocked_reason =
        BuildBlockedReason(resource_request_blocked_reason.value());
  }
  GetFrontend()->loadingFailed(
      request_id, CurrentTimeTicksInSeconds(),
      InspectorPageAgent::ResourceTypeJson(
          resources_data_->GetResourceType(request_id)),
      error.LocalizedDescription(), canceled, std::move(blocked_reason));
}

void InspectorNetworkAgent::ScriptImported(unsigned long identifier,
                                           const String& source_string) {
  resources_data_->SetResourceContent(
      IdentifiersFactory::SubresourceRequestId(identifier), source_string);
}

void InspectorNetworkAgent::DidReceiveScriptResponse(unsigned long identifier) {
  resources_data_->SetResourceType(
      IdentifiersFactory::SubresourceRequestId(identifier),
      InspectorPageAgent::kScriptResource);
}

void InspectorNetworkAgent::ClearPendingRequestData() {
  if (pending_request_type_ == InspectorPageAgent::kXHRResource)
    pending_xhr_replay_data_.Clear();
  pending_request_ = nullptr;
}

// static
bool InspectorNetworkAgent::IsNavigation(DocumentLoader* loader,
                                         unsigned long identifier) {
  return loader && loader->MainResourceIdentifier() == identifier;
}

void InspectorNetworkAgent::DocumentThreadableLoaderStartedLoadingForClient(
    unsigned long identifier,
    ThreadableLoaderClient* client) {
  if (!client)
    return;
  if (client != pending_request_) {
    DCHECK(!pending_request_);
    return;
  }

  String request_id = IdentifiersFactory::SubresourceRequestId(identifier);
  resources_data_->SetResourceType(request_id, pending_request_type_);
  if (pending_request_type_ == InspectorPageAgent::kXHRResource) {
    resources_data_->SetXHRReplayData(request_id,
                                      pending_xhr_replay_data_.Get());
  }

  ClearPendingRequestData();
}

void InspectorNetworkAgent::
    DocumentThreadableLoaderFailedToStartLoadingForClient(
        ThreadableLoaderClient* client) {
  if (!client)
    return;
  if (client != pending_request_) {
    DCHECK(!pending_request_);
    return;
  }

  ClearPendingRequestData();
}

void InspectorNetworkAgent::WillLoadXHR(XMLHttpRequest* xhr,
                                        ThreadableLoaderClient* client,
                                        const AtomicString& method,
                                        const KURL& url,
                                        bool async,
                                        const HTTPHeaderMap& headers,
                                        bool include_credentials) {
  DCHECK(xhr);
  DCHECK(!pending_request_);
  pending_request_ = client;
  pending_request_type_ = InspectorPageAgent::kXHRResource;
  pending_xhr_replay_data_ = XHRReplayData::Create(
      method, UrlWithoutFragment(url), async, include_credentials);
  for (const auto& header : headers)
    pending_xhr_replay_data_->AddHeader(header.key, header.value);
}

void InspectorNetworkAgent::DelayedRemoveReplayXHR(XMLHttpRequest* xhr) {
  if (!replay_xhrs_.Contains(xhr))
    return;
  replay_xhrs_to_be_deleted_.insert(xhr);
  replay_xhrs_.erase(xhr);
  remove_finished_replay_xhr_timer_.StartOneShot(TimeDelta(), FROM_HERE);
}

void InspectorNetworkAgent::DidFailXHRLoading(ExecutionContext* context,
                                              XMLHttpRequest* xhr,
                                              ThreadableLoaderClient* client,
                                              const AtomicString& method,
                                              const String& url) {
  DidFinishXHRInternal(context, xhr, client, method, url, false);
}

void InspectorNetworkAgent::DidFinishXHRLoading(ExecutionContext* context,
                                                XMLHttpRequest* xhr,
                                                ThreadableLoaderClient* client,
                                                const AtomicString& method,
                                                const String& url) {
  DidFinishXHRInternal(context, xhr, client, method, url, true);
}

void InspectorNetworkAgent::DidFinishXHRInternal(ExecutionContext* context,
                                                 XMLHttpRequest* xhr,
                                                 ThreadableLoaderClient* client,
                                                 const AtomicString& method,
                                                 const String& url,
                                                 bool success) {
  ClearPendingRequestData();

  // This method will be called from the XHR.
  // We delay deleting the replay XHR, as deleting here may delete the caller.
  DelayedRemoveReplayXHR(xhr);
}

void InspectorNetworkAgent::WillStartFetch(ThreadableLoaderClient* client) {
  DCHECK(!pending_request_);
  pending_request_ = client;
  pending_request_type_ = InspectorPageAgent::kFetchResource;
}

void InspectorNetworkAgent::WillSendEventSourceRequest(
    ThreadableLoaderClient* event_source) {
  DCHECK(!pending_request_);
  pending_request_ = event_source;
  pending_request_type_ = InspectorPageAgent::kEventSourceResource;
}

void InspectorNetworkAgent::WillDispatchEventSourceEvent(
    unsigned long identifier,
    const AtomicString& event_name,
    const AtomicString& event_id,
    const String& data) {
  GetFrontend()->eventSourceMessageReceived(
      IdentifiersFactory::SubresourceRequestId(identifier),
      CurrentTimeTicksInSeconds(), event_name.GetString(), event_id.GetString(),
      data);
}

void InspectorNetworkAgent::DidFinishEventSourceRequest(
    ThreadableLoaderClient* event_source) {
  ClearPendingRequestData();
}

void InspectorNetworkAgent::DetachClientRequest(
    ThreadableLoaderClient* client) {
  // This method is called by loader clients when finalizing
  // (i.e., from their "prefinalizers".) The client reference must
  // no longer be held onto upon completion.
  if (pending_request_ == client) {
    pending_request_ = nullptr;
    if (pending_request_type_ == InspectorPageAgent::kXHRResource) {
      pending_xhr_replay_data_.Clear();
    }
  }
}

void InspectorNetworkAgent::ApplyUserAgentOverride(String* user_agent) {
  String user_agent_override;
  state_->getString(NetworkAgentState::kUserAgentOverride,
                    &user_agent_override);
  if (!user_agent_override.IsEmpty())
    *user_agent = user_agent_override;
}

std::unique_ptr<protocol::Network::Initiator>
InspectorNetworkAgent::BuildInitiatorObject(
    Document* document,
    const FetchInitiatorInfo& initiator_info) {
  if (!initiator_info.imported_module_referrer.IsEmpty()) {
    std::unique_ptr<protocol::Network::Initiator> initiator_object =
        protocol::Network::Initiator::create()
            .setType(protocol::Network::Initiator::TypeEnum::Script)
            .build();
    initiator_object->setUrl(initiator_info.imported_module_referrer);
    initiator_object->setLineNumber(
        initiator_info.position.line_.ZeroBasedInt());
    return initiator_object;
  }

  std::unique_ptr<v8_inspector::protocol::Runtime::API::StackTrace>
      current_stack_trace =
          SourceLocation::Capture(document)->BuildInspectorObject();
  if (current_stack_trace) {
    std::unique_ptr<protocol::Network::Initiator> initiator_object =
        protocol::Network::Initiator::create()
            .setType(protocol::Network::Initiator::TypeEnum::Script)
            .build();
    initiator_object->setStack(std::move(current_stack_trace));
    return initiator_object;
  }

  while (document && !document->GetScriptableDocumentParser())
    document = document->LocalOwner() ? document->LocalOwner()->ownerDocument()
                                      : nullptr;
  if (document && document->GetScriptableDocumentParser()) {
    std::unique_ptr<protocol::Network::Initiator> initiator_object =
        protocol::Network::Initiator::create()
            .setType(protocol::Network::Initiator::TypeEnum::Parser)
            .build();
    initiator_object->setUrl(UrlWithoutFragment(document->Url()).GetString());
    if (TextPosition::BelowRangePosition() != initiator_info.position)
      initiator_object->setLineNumber(
          initiator_info.position.line_.ZeroBasedInt());
    else
      initiator_object->setLineNumber(
          document->GetScriptableDocumentParser()->LineNumber().ZeroBasedInt());
    return initiator_object;
  }

  return protocol::Network::Initiator::create()
      .setType(protocol::Network::Initiator::TypeEnum::Other)
      .build();
}

void InspectorNetworkAgent::DidCreateWebSocket(
    ExecutionContext* execution_context,
    unsigned long identifier,
    const KURL& request_url,
    const String&) {
  std::unique_ptr<v8_inspector::protocol::Runtime::API::StackTrace>
      current_stack_trace =
          SourceLocation::Capture(execution_context)->BuildInspectorObject();
  if (!current_stack_trace) {
    GetFrontend()->webSocketCreated(
        IdentifiersFactory::SubresourceRequestId(identifier),
        UrlWithoutFragment(request_url).GetString());
    return;
  }

  std::unique_ptr<protocol::Network::Initiator> initiator_object =
      protocol::Network::Initiator::create()
          .setType(protocol::Network::Initiator::TypeEnum::Script)
          .build();
  initiator_object->setStack(std::move(current_stack_trace));
  GetFrontend()->webSocketCreated(
      IdentifiersFactory::SubresourceRequestId(identifier),
      UrlWithoutFragment(request_url).GetString(), std::move(initiator_object));
}

void InspectorNetworkAgent::WillSendWebSocketHandshakeRequest(
    ExecutionContext*,
    unsigned long identifier,
    const WebSocketHandshakeRequest* request) {
  DCHECK(request);
  std::unique_ptr<protocol::Network::WebSocketRequest> request_object =
      protocol::Network::WebSocketRequest::create()
          .setHeaders(BuildObjectForHeaders(request->HeaderFields()))
          .build();
  GetFrontend()->webSocketWillSendHandshakeRequest(
      IdentifiersFactory::SubresourceRequestId(identifier),
      CurrentTimeTicksInSeconds(), CurrentTime(), std::move(request_object));
}

void InspectorNetworkAgent::DidReceiveWebSocketHandshakeResponse(
    ExecutionContext*,
    unsigned long identifier,
    const WebSocketHandshakeRequest* request,
    const WebSocketHandshakeResponse* response) {
  DCHECK(response);
  std::unique_ptr<protocol::Network::WebSocketResponse> response_object =
      protocol::Network::WebSocketResponse::create()
          .setStatus(response->StatusCode())
          .setStatusText(response->StatusText())
          .setHeaders(BuildObjectForHeaders(response->HeaderFields()))
          .build();

  if (!response->HeadersText().IsEmpty())
    response_object->setHeadersText(response->HeadersText());
  if (request) {
    response_object->setRequestHeaders(
        BuildObjectForHeaders(request->HeaderFields()));
    if (!request->HeadersText().IsEmpty())
      response_object->setRequestHeadersText(request->HeadersText());
  }
  GetFrontend()->webSocketHandshakeResponseReceived(
      IdentifiersFactory::SubresourceRequestId(identifier),
      CurrentTimeTicksInSeconds(), std::move(response_object));
}

void InspectorNetworkAgent::DidCloseWebSocket(ExecutionContext*,
                                              unsigned long identifier) {
  GetFrontend()->webSocketClosed(
      IdentifiersFactory::SubresourceRequestId(identifier),
      CurrentTimeTicksInSeconds());
}

void InspectorNetworkAgent::DidReceiveWebSocketFrame(unsigned long identifier,
                                                     int op_code,
                                                     bool masked,
                                                     const char* payload,
                                                     size_t payload_length) {
  std::unique_ptr<protocol::Network::WebSocketFrame> frame_object =
      protocol::Network::WebSocketFrame::create()
          .setOpcode(op_code)
          .setMask(masked)
          .setPayloadData(
              String::FromUTF8WithLatin1Fallback(payload, payload_length))
          .build();
  GetFrontend()->webSocketFrameReceived(
      IdentifiersFactory::SubresourceRequestId(identifier),
      CurrentTimeTicksInSeconds(), std::move(frame_object));
}

void InspectorNetworkAgent::DidSendWebSocketFrame(unsigned long identifier,
                                                  int op_code,
                                                  bool masked,
                                                  const char* payload,
                                                  size_t payload_length) {
  std::unique_ptr<protocol::Network::WebSocketFrame> frame_object =
      protocol::Network::WebSocketFrame::create()
          .setOpcode(op_code)
          .setMask(masked)
          .setPayloadData(
              String::FromUTF8WithLatin1Fallback(payload, payload_length))
          .build();
  GetFrontend()->webSocketFrameSent(
      IdentifiersFactory::RequestId(nullptr, identifier),
      CurrentTimeTicksInSeconds(), std::move(frame_object));
}

void InspectorNetworkAgent::DidReceiveWebSocketFrameError(
    unsigned long identifier,
    const String& error_message) {
  GetFrontend()->webSocketFrameError(
      IdentifiersFactory::RequestId(nullptr, identifier),
      CurrentTimeTicksInSeconds(), error_message);
}

Response InspectorNetworkAgent::enable(Maybe<int> total_buffer_size,
                                       Maybe<int> resource_buffer_size,
                                       Maybe<int> max_post_data_size) {
  Enable(total_buffer_size.fromMaybe(g_maximum_total_buffer_size),
         resource_buffer_size.fromMaybe(g_maximum_resource_buffer_size),
         max_post_data_size.fromMaybe(0));
  return Response::OK();
}

void InspectorNetworkAgent::Enable(int total_buffer_size,
                                   int resource_buffer_size,
                                   int max_post_data_size) {
  if (!GetFrontend())
    return;
  resources_data_->SetResourcesDataSizeLimits(total_buffer_size,
                                              resource_buffer_size);
  state_->setBoolean(NetworkAgentState::kNetworkAgentEnabled, true);
  state_->setInteger(NetworkAgentState::kTotalBufferSize, total_buffer_size);
  state_->setInteger(NetworkAgentState::kResourceBufferSize,
                     resource_buffer_size);
  state_->setInteger(NetworkAgentState::kMaxPostDataSize, max_post_data_size);
  max_post_data_size_ = max_post_data_size;
  instrumenting_agents_->addInspectorNetworkAgent(this);
}

Response InspectorNetworkAgent::disable() {
  DCHECK(!pending_request_);
  state_->setBoolean(NetworkAgentState::kNetworkAgentEnabled, false);
  state_->setString(NetworkAgentState::kUserAgentOverride, "");
  instrumenting_agents_->removeInspectorNetworkAgent(this);
  resources_data_->Clear();
  return Response::OK();
}

Response InspectorNetworkAgent::setUserAgentOverride(const String& user_agent) {
  if (user_agent.Contains('\n') || user_agent.Contains('\r') ||
      user_agent.Contains('\0')) {
    return Response::Error("Invalid characters found in userAgent");
  }
  state_->setString(NetworkAgentState::kUserAgentOverride, user_agent);
  return Response::OK();
}

Response InspectorNetworkAgent::setExtraHTTPHeaders(
    std::unique_ptr<protocol::Network::Headers> headers) {
  state_->setObject(NetworkAgentState::kExtraRequestHeaders,
                    headers->toValue());
  return Response::OK();
}

bool InspectorNetworkAgent::CanGetResponseBodyBlob(const String& request_id) {
  NetworkResourcesData::ResourceData const* resource_data =
      resources_data_->Data(request_id);
  BlobDataHandle* blob =
      resource_data ? resource_data->DownloadedFileBlob() : nullptr;
  if (!blob)
    return false;
  if (worker_global_scope_)
    return true;
  LocalFrame* frame = IdentifiersFactory::FrameById(inspected_frames_,
                                                    resource_data->FrameId());
  return frame && frame->GetDocument();
}

void InspectorNetworkAgent::GetResponseBodyBlob(
    const String& request_id,
    std::unique_ptr<GetResponseBodyCallback> callback) {
  NetworkResourcesData::ResourceData const* resource_data =
      resources_data_->Data(request_id);
  BlobDataHandle* blob = resource_data->DownloadedFileBlob();
  InspectorFileReaderLoaderClient* client = new InspectorFileReaderLoaderClient(
      blob,
      WTF::Bind(ResponseBodyFileReaderLoaderDone, resource_data->MimeType(),
                resource_data->TextEncodingName(),
                WTF::Passed(std::move(callback))));
  client->Start();
}

void InspectorNetworkAgent::getResponseBody(
    const String& request_id,
    std::unique_ptr<GetResponseBodyCallback> callback) {
  if (CanGetResponseBodyBlob(request_id)) {
    GetResponseBodyBlob(request_id, std::move(callback));
    return;
  }

  String content;
  bool base64_encoded;
  Response response = GetResponseBody(request_id, &content, &base64_encoded);
  if (response.isSuccess()) {
    callback->sendSuccess(content, base64_encoded);
  } else {
    callback->sendFailure(response);
  }
}

Response InspectorNetworkAgent::setBlockedURLs(
    std::unique_ptr<protocol::Array<String>> urls) {
  std::unique_ptr<protocol::DictionaryValue> new_list =
      protocol::DictionaryValue::create();
  for (size_t i = 0; i < urls->length(); i++)
    new_list->setBoolean(urls->get(i), true);
  state_->setObject(NetworkAgentState::kBlockedURLs, std::move(new_list));
  return Response::OK();
}

Response InspectorNetworkAgent::replayXHR(const String& request_id) {
  String actual_request_id = request_id;

  XHRReplayData* xhr_replay_data = resources_data_->XhrReplayData(request_id);
  auto* data = resources_data_->Data(request_id);
  if (!xhr_replay_data || !data)
    return Response::Error("Given id does not correspond to XHR");

  ExecutionContext* execution_context = data->GetExecutionContext();
  if (execution_context->IsContextDestroyed()) {
    resources_data_->SetXHRReplayData(request_id, nullptr);
    return Response::Error("Document is already detached");
  }

  XMLHttpRequest* xhr = XMLHttpRequest::Create(execution_context);

  execution_context->RemoveURLFromMemoryCache(xhr_replay_data->Url());

  xhr->open(xhr_replay_data->Method(), xhr_replay_data->Url(),
            xhr_replay_data->Async(), IGNORE_EXCEPTION_FOR_TESTING);
  if (xhr_replay_data->IncludeCredentials())
    xhr->setWithCredentials(true, IGNORE_EXCEPTION_FOR_TESTING);
  for (const auto& header : xhr_replay_data->Headers()) {
    xhr->setRequestHeader(header.key, header.value,
                          IGNORE_EXCEPTION_FOR_TESTING);
  }
  xhr->SendForInspectorXHRReplay(data ? data->PostData() : nullptr,
                                 IGNORE_EXCEPTION_FOR_TESTING);

  replay_xhrs_.insert(xhr);
  return Response::OK();
}

Response InspectorNetworkAgent::canClearBrowserCache(bool* result) {
  *result = true;
  return Response::OK();
}

Response InspectorNetworkAgent::canClearBrowserCookies(bool* result) {
  *result = true;
  return Response::OK();
}

Response InspectorNetworkAgent::emulateNetworkConditions(
    bool offline,
    double latency,
    double download_throughput,
    double upload_throughput,
    Maybe<String> connection_type) {
  if (!IsMainThread())
    return Response::Error("Not supported");

  WebConnectionType type = kWebConnectionTypeUnknown;
  if (connection_type.isJust()) {
    type = ToWebConnectionType(connection_type.fromJust());
    if (type == kWebConnectionTypeUnknown)
      return Response::Error("Unknown connection type");
  }
  // TODO(dgozman): networkStateNotifier is per-process. It would be nice to
  // have per-frame override instead.
  if (offline || latency || download_throughput || upload_throughput) {
    GetNetworkStateNotifier().SetNetworkConnectionInfoOverride(
        !offline, type, base::nullopt, latency,
        download_throughput / (1024 * 1024 / 8));
  } else {
    GetNetworkStateNotifier().ClearOverride();
  }
  return Response::OK();
}

Response InspectorNetworkAgent::setCacheDisabled(bool cache_disabled) {
  // TODO(ananta)
  // We should extract network cache state into a global entity which can be
  // queried from FrameLoader and other places.
  state_->setBoolean(NetworkAgentState::kCacheDisabled, cache_disabled);
  if (cache_disabled && IsMainThread())
    GetMemoryCache()->EvictResources();
  return Response::OK();
}

Response InspectorNetworkAgent::setBypassServiceWorker(bool bypass) {
  state_->setBoolean(NetworkAgentState::kBypassServiceWorker, bypass);
  return Response::OK();
}

Response InspectorNetworkAgent::setDataSizeLimitsForTest(int max_total,
                                                         int max_resource) {
  resources_data_->SetResourcesDataSizeLimits(max_total, max_resource);
  return Response::OK();
}

Response InspectorNetworkAgent::getCertificate(
    const String& origin,
    std::unique_ptr<protocol::Array<String>>* certificate) {
  *certificate = protocol::Array<String>::create();
  scoped_refptr<const SecurityOrigin> security_origin =
      SecurityOrigin::CreateFromString(origin);
  for (auto& resource : resources_data_->Resources()) {
    scoped_refptr<const SecurityOrigin> resource_origin =
        SecurityOrigin::Create(resource->RequestedURL());
    if (resource_origin->IsSameSchemeHostPort(security_origin.get()) &&
        resource->Certificate().size()) {
      for (auto& cert : resource->Certificate())
        certificate->get()->addItem(Base64Encode(cert.Latin1()));
      return Response::OK();
    }
  }
  return Response::OK();
}

void InspectorNetworkAgent::DidCommitLoad(LocalFrame* frame,
                                          DocumentLoader* loader) {
  DCHECK(IsMainThread());
  if (loader->GetFrame() != inspected_frames_->Root())
    return;

  if (state_->booleanProperty(NetworkAgentState::kCacheDisabled, false))
    GetMemoryCache()->EvictResources(MemoryCache::kDoNotEvictUnusedPreloads);

  resources_data_->Clear(IdentifiersFactory::LoaderId(loader));
}

void InspectorNetworkAgent::FrameScheduledNavigation(LocalFrame* frame,
                                                     ScheduledNavigation*) {
  String frame_id = IdentifiersFactory::FrameId(frame);
  frames_with_scheduled_navigation_.insert(frame_id);
  if (!frames_with_scheduled_client_navigation_.Contains(frame_id)) {
    frame_navigation_initiator_map_.Set(
        frame_id,
        BuildInitiatorObject(frame->GetDocument(), FetchInitiatorInfo()));
  }
}

void InspectorNetworkAgent::FrameClearedScheduledNavigation(LocalFrame* frame) {
  String frame_id = IdentifiersFactory::FrameId(frame);
  frames_with_scheduled_navigation_.erase(frame_id);
  if (!frames_with_scheduled_client_navigation_.Contains(frame_id))
    frame_navigation_initiator_map_.erase(frame_id);
}

void InspectorNetworkAgent::FrameScheduledClientNavigation(LocalFrame* frame) {
  String frame_id = IdentifiersFactory::FrameId(frame);
  frames_with_scheduled_client_navigation_.insert(frame_id);
  if (!frames_with_scheduled_navigation_.Contains(frame_id)) {
    frame_navigation_initiator_map_.Set(
        frame_id,
        BuildInitiatorObject(frame->GetDocument(), FetchInitiatorInfo()));
  }
}

void InspectorNetworkAgent::FrameClearedScheduledClientNavigation(
    LocalFrame* frame) {
  String frame_id = IdentifiersFactory::FrameId(frame);
  frames_with_scheduled_client_navigation_.erase(frame_id);
  if (!frames_with_scheduled_navigation_.Contains(frame_id))
    frame_navigation_initiator_map_.erase(frame_id);
}

Response InspectorNetworkAgent::GetResponseBody(const String& request_id,
                                                String* content,
                                                bool* base64_encoded) {
  NetworkResourcesData::ResourceData const* resource_data =
      resources_data_->Data(request_id);
  if (!resource_data) {
    return Response::Error("No resource with given identifier found");
  }

  if (resource_data->HasContent()) {
    *content = resource_data->Content();
    *base64_encoded = resource_data->Base64Encoded();
    return Response::OK();
  }

  if (resource_data->IsContentEvicted()) {
    return Response::Error("Request content was evicted from inspector cache");
  }

  if (resource_data->Buffer() && !resource_data->TextEncodingName().IsNull()) {
    bool success = InspectorPageAgent::SharedBufferContent(
        resource_data->Buffer(), resource_data->MimeType(),
        resource_data->TextEncodingName(), content, base64_encoded);
    DCHECK(success);
    return Response::OK();
  }

  if (resource_data->CachedResource() &&
      InspectorPageAgent::CachedResourceContent(resource_data->CachedResource(),
                                                content, base64_encoded)) {
    return Response::OK();
  }

  return Response::Error("No data found for resource with given identifier");
}

Response InspectorNetworkAgent::searchInResponseBody(
    const String& request_id,
    const String& query,
    Maybe<bool> case_sensitive,
    Maybe<bool> is_regex,
    std::unique_ptr<
        protocol::Array<v8_inspector::protocol::Debugger::API::SearchMatch>>*
        matches) {
  String content;
  bool base64_encoded;
  Response response = GetResponseBody(request_id, &content, &base64_encoded);
  if (!response.isSuccess())
    return response;

  auto results = v8_session_->searchInTextByLines(
      ToV8InspectorStringView(content), ToV8InspectorStringView(query),
      case_sensitive.fromMaybe(false), is_regex.fromMaybe(false));
  *matches = protocol::Array<
      v8_inspector::protocol::Debugger::API::SearchMatch>::create();
  for (size_t i = 0; i < results.size(); ++i)
    matches->get()->addItem(std::move(results[i]));
  return Response::OK();
}

bool InspectorNetworkAgent::FetchResourceContent(Document* document,
                                                 const KURL& url,
                                                 String* content,
                                                 bool* base64_encoded) {
  DCHECK(document);
  DCHECK(IsMainThread());
  // First try to fetch content from the cached resource.
  Resource* cached_resource = document->Fetcher()->CachedResource(url);
  if (!cached_resource) {
    cached_resource = GetMemoryCache()->ResourceForURL(
        url, document->Fetcher()->GetCacheIdentifier());
  }
  if (cached_resource && InspectorPageAgent::CachedResourceContent(
                             cached_resource, content, base64_encoded))
    return true;

  // Then fall back to resource data.
  for (auto& resource : resources_data_->Resources()) {
    if (resource->RequestedURL() == url) {
      *content = resource->Content();
      *base64_encoded = resource->Base64Encoded();
      return true;
    }
  }
  return false;
}

String InspectorNetworkAgent::NavigationInitiatorInfo(LocalFrame* frame) {
  if (!state_->booleanProperty(NetworkAgentState::kNetworkAgentEnabled, false))
    return String();
  FrameNavigationInitiatorMap::iterator it =
      frame_navigation_initiator_map_.find(IdentifiersFactory::FrameId(frame));
  if (it != frame_navigation_initiator_map_.end())
    return it->value->serialize();
  return BuildInitiatorObject(frame->GetDocument(), FetchInitiatorInfo())
      ->serialize();
}

void InspectorNetworkAgent::RemoveFinishedReplayXHRFired(TimerBase*) {
  replay_xhrs_to_be_deleted_.clear();
}

InspectorNetworkAgent::InspectorNetworkAgent(
    InspectedFrames* inspected_frames,
    WorkerGlobalScope* worker_global_scope,
    v8_inspector::V8InspectorSession* v8_session)
    : inspected_frames_(inspected_frames),
      worker_global_scope_(worker_global_scope),
      v8_session_(v8_session),
      resources_data_(
          NetworkResourcesData::Create(g_maximum_total_buffer_size,
                                       g_maximum_resource_buffer_size)),
      pending_request_(nullptr),
      remove_finished_replay_xhr_timer_(
          worker_global_scope_
              ? worker_global_scope->GetTaskRunner(TaskType::kInternalLoading)
              : inspected_frames->Root()->GetTaskRunner(
                    TaskType::kInternalLoading),
          this,
          &InspectorNetworkAgent::RemoveFinishedReplayXHRFired),
      max_post_data_size_(0) {
  DCHECK((IsMainThread() && !worker_global_scope_) ||
         (!IsMainThread() && worker_global_scope_));
  conditions_token_ = IdentifiersFactory::IdFromToken(
      worker_global_scope_ ? worker_global_scope_->GetParentDevToolsToken()
                           : inspected_frames->Root()->GetDevToolsFrameToken());
}

void InspectorNetworkAgent::ShouldForceCORSPreflight(bool* result) {
  if (state_->booleanProperty(NetworkAgentState::kCacheDisabled, false))
    *result = true;
}

void InspectorNetworkAgent::getRequestPostData(
    const String& request_id,
    std::unique_ptr<GetRequestPostDataCallback> callback) {
  NetworkResourcesData::ResourceData const* resource_data =
      resources_data_->Data(request_id);
  if (!resource_data) {
    callback->sendFailure(
        Response::Error("No resource with given id was found"));
    return;
  }
  scoped_refptr<EncodedFormData> post_data = resource_data->PostData();
  if (!post_data || post_data->IsEmpty()) {
    callback->sendFailure(
        Response::Error("No post data available for the request"));
    return;
  }

  scoped_refptr<InspectorPostBodyParser> parser =
      base::MakeRefCounted<InspectorPostBodyParser>(std::move(callback));
  // TODO(crbug.com/810554): Extend protocol to fetch body parts separately
  parser->Parse(post_data.get());
}

}  // namespace blink
