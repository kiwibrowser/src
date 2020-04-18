// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/* DO NOT EDIT. Generated from components/cronet/native/generated/cronet.idl */

#include "components/cronet/native/generated/cronet.idl_impl_struct.h"

#include <utility>

#include "base/logging.h"

// Struct Cronet_Error.
Cronet_Error::Cronet_Error() = default;

Cronet_Error::Cronet_Error(const Cronet_Error& from) = default;

Cronet_Error::Cronet_Error(Cronet_Error&& from) = default;

Cronet_Error::~Cronet_Error() = default;

Cronet_ErrorPtr Cronet_Error_Create() {
  return new Cronet_Error();
}

void Cronet_Error_Destroy(Cronet_ErrorPtr self) {
  delete self;
}

// Struct Cronet_Error setters.
void Cronet_Error_error_code_set(Cronet_ErrorPtr self,
                                 Cronet_Error_ERROR_CODE error_code) {
  DCHECK(self);
  self->error_code = error_code;
}

void Cronet_Error_message_set(Cronet_ErrorPtr self, Cronet_String message) {
  DCHECK(self);
  self->message = message;
}

void Cronet_Error_internal_error_code_set(Cronet_ErrorPtr self,
                                          int32_t internal_error_code) {
  DCHECK(self);
  self->internal_error_code = internal_error_code;
}

void Cronet_Error_immediately_retryable_set(Cronet_ErrorPtr self,
                                            bool immediately_retryable) {
  DCHECK(self);
  self->immediately_retryable = immediately_retryable;
}

void Cronet_Error_quic_detailed_error_code_set(
    Cronet_ErrorPtr self,
    int32_t quic_detailed_error_code) {
  DCHECK(self);
  self->quic_detailed_error_code = quic_detailed_error_code;
}

// Struct Cronet_Error getters.
Cronet_Error_ERROR_CODE Cronet_Error_error_code_get(Cronet_ErrorPtr self) {
  DCHECK(self);
  return self->error_code;
}

Cronet_String Cronet_Error_message_get(Cronet_ErrorPtr self) {
  DCHECK(self);
  return self->message.c_str();
}

int32_t Cronet_Error_internal_error_code_get(Cronet_ErrorPtr self) {
  DCHECK(self);
  return self->internal_error_code;
}

bool Cronet_Error_immediately_retryable_get(Cronet_ErrorPtr self) {
  DCHECK(self);
  return self->immediately_retryable;
}

int32_t Cronet_Error_quic_detailed_error_code_get(Cronet_ErrorPtr self) {
  DCHECK(self);
  return self->quic_detailed_error_code;
}

// Struct Cronet_QuicHint.
Cronet_QuicHint::Cronet_QuicHint() = default;

Cronet_QuicHint::Cronet_QuicHint(const Cronet_QuicHint& from) = default;

Cronet_QuicHint::Cronet_QuicHint(Cronet_QuicHint&& from) = default;

Cronet_QuicHint::~Cronet_QuicHint() = default;

Cronet_QuicHintPtr Cronet_QuicHint_Create() {
  return new Cronet_QuicHint();
}

void Cronet_QuicHint_Destroy(Cronet_QuicHintPtr self) {
  delete self;
}

// Struct Cronet_QuicHint setters.
void Cronet_QuicHint_host_set(Cronet_QuicHintPtr self, Cronet_String host) {
  DCHECK(self);
  self->host = host;
}

void Cronet_QuicHint_port_set(Cronet_QuicHintPtr self, int32_t port) {
  DCHECK(self);
  self->port = port;
}

void Cronet_QuicHint_alternate_port_set(Cronet_QuicHintPtr self,
                                        int32_t alternate_port) {
  DCHECK(self);
  self->alternate_port = alternate_port;
}

// Struct Cronet_QuicHint getters.
Cronet_String Cronet_QuicHint_host_get(Cronet_QuicHintPtr self) {
  DCHECK(self);
  return self->host.c_str();
}

int32_t Cronet_QuicHint_port_get(Cronet_QuicHintPtr self) {
  DCHECK(self);
  return self->port;
}

int32_t Cronet_QuicHint_alternate_port_get(Cronet_QuicHintPtr self) {
  DCHECK(self);
  return self->alternate_port;
}

// Struct Cronet_PublicKeyPins.
Cronet_PublicKeyPins::Cronet_PublicKeyPins() = default;

Cronet_PublicKeyPins::Cronet_PublicKeyPins(const Cronet_PublicKeyPins& from) =
    default;

Cronet_PublicKeyPins::Cronet_PublicKeyPins(Cronet_PublicKeyPins&& from) =
    default;

Cronet_PublicKeyPins::~Cronet_PublicKeyPins() = default;

Cronet_PublicKeyPinsPtr Cronet_PublicKeyPins_Create() {
  return new Cronet_PublicKeyPins();
}

void Cronet_PublicKeyPins_Destroy(Cronet_PublicKeyPinsPtr self) {
  delete self;
}

// Struct Cronet_PublicKeyPins setters.
void Cronet_PublicKeyPins_host_set(Cronet_PublicKeyPinsPtr self,
                                   Cronet_String host) {
  DCHECK(self);
  self->host = host;
}

void Cronet_PublicKeyPins_pins_sha256_add(Cronet_PublicKeyPinsPtr self,
                                          Cronet_String element) {
  DCHECK(self);
  self->pins_sha256.push_back(element);
}

void Cronet_PublicKeyPins_include_subdomains_set(Cronet_PublicKeyPinsPtr self,
                                                 bool include_subdomains) {
  DCHECK(self);
  self->include_subdomains = include_subdomains;
}

void Cronet_PublicKeyPins_expiration_date_set(Cronet_PublicKeyPinsPtr self,
                                              int64_t expiration_date) {
  DCHECK(self);
  self->expiration_date = expiration_date;
}

// Struct Cronet_PublicKeyPins getters.
Cronet_String Cronet_PublicKeyPins_host_get(Cronet_PublicKeyPinsPtr self) {
  DCHECK(self);
  return self->host.c_str();
}

uint32_t Cronet_PublicKeyPins_pins_sha256_size(Cronet_PublicKeyPinsPtr self) {
  DCHECK(self);
  return self->pins_sha256.size();
}
Cronet_String Cronet_PublicKeyPins_pins_sha256_at(Cronet_PublicKeyPinsPtr self,
                                                  uint32_t index) {
  DCHECK(self);
  DCHECK(index < self->pins_sha256.size());
  return self->pins_sha256[index].c_str();
}
void Cronet_PublicKeyPins_pins_sha256_clear(Cronet_PublicKeyPinsPtr self) {
  DCHECK(self);
  self->pins_sha256.clear();
}

bool Cronet_PublicKeyPins_include_subdomains_get(Cronet_PublicKeyPinsPtr self) {
  DCHECK(self);
  return self->include_subdomains;
}

int64_t Cronet_PublicKeyPins_expiration_date_get(Cronet_PublicKeyPinsPtr self) {
  DCHECK(self);
  return self->expiration_date;
}

// Struct Cronet_EngineParams.
Cronet_EngineParams::Cronet_EngineParams() = default;

Cronet_EngineParams::Cronet_EngineParams(const Cronet_EngineParams& from) =
    default;

Cronet_EngineParams::Cronet_EngineParams(Cronet_EngineParams&& from) = default;

Cronet_EngineParams::~Cronet_EngineParams() = default;

Cronet_EngineParamsPtr Cronet_EngineParams_Create() {
  return new Cronet_EngineParams();
}

void Cronet_EngineParams_Destroy(Cronet_EngineParamsPtr self) {
  delete self;
}

// Struct Cronet_EngineParams setters.
void Cronet_EngineParams_enable_check_result_set(Cronet_EngineParamsPtr self,
                                                 bool enable_check_result) {
  DCHECK(self);
  self->enable_check_result = enable_check_result;
}

void Cronet_EngineParams_user_agent_set(Cronet_EngineParamsPtr self,
                                        Cronet_String user_agent) {
  DCHECK(self);
  self->user_agent = user_agent;
}

void Cronet_EngineParams_accept_language_set(Cronet_EngineParamsPtr self,
                                             Cronet_String accept_language) {
  DCHECK(self);
  self->accept_language = accept_language;
}

void Cronet_EngineParams_storage_path_set(Cronet_EngineParamsPtr self,
                                          Cronet_String storage_path) {
  DCHECK(self);
  self->storage_path = storage_path;
}

void Cronet_EngineParams_enable_quic_set(Cronet_EngineParamsPtr self,
                                         bool enable_quic) {
  DCHECK(self);
  self->enable_quic = enable_quic;
}

void Cronet_EngineParams_enable_http2_set(Cronet_EngineParamsPtr self,
                                          bool enable_http2) {
  DCHECK(self);
  self->enable_http2 = enable_http2;
}

void Cronet_EngineParams_enable_brotli_set(Cronet_EngineParamsPtr self,
                                           bool enable_brotli) {
  DCHECK(self);
  self->enable_brotli = enable_brotli;
}

void Cronet_EngineParams_http_cache_mode_set(
    Cronet_EngineParamsPtr self,
    Cronet_EngineParams_HTTP_CACHE_MODE http_cache_mode) {
  DCHECK(self);
  self->http_cache_mode = http_cache_mode;
}

void Cronet_EngineParams_http_cache_max_size_set(Cronet_EngineParamsPtr self,
                                                 int64_t http_cache_max_size) {
  DCHECK(self);
  self->http_cache_max_size = http_cache_max_size;
}

void Cronet_EngineParams_quic_hints_add(Cronet_EngineParamsPtr self,
                                        Cronet_QuicHintPtr element) {
  DCHECK(self);
  self->quic_hints.push_back(*element);
}

void Cronet_EngineParams_public_key_pins_add(Cronet_EngineParamsPtr self,
                                             Cronet_PublicKeyPinsPtr element) {
  DCHECK(self);
  self->public_key_pins.push_back(*element);
}

void Cronet_EngineParams_enable_public_key_pinning_bypass_for_local_trust_anchors_set(
    Cronet_EngineParamsPtr self,
    bool enable_public_key_pinning_bypass_for_local_trust_anchors) {
  DCHECK(self);
  self->enable_public_key_pinning_bypass_for_local_trust_anchors =
      enable_public_key_pinning_bypass_for_local_trust_anchors;
}

void Cronet_EngineParams_experimental_options_set(
    Cronet_EngineParamsPtr self,
    Cronet_String experimental_options) {
  DCHECK(self);
  self->experimental_options = experimental_options;
}

// Struct Cronet_EngineParams getters.
bool Cronet_EngineParams_enable_check_result_get(Cronet_EngineParamsPtr self) {
  DCHECK(self);
  return self->enable_check_result;
}

Cronet_String Cronet_EngineParams_user_agent_get(Cronet_EngineParamsPtr self) {
  DCHECK(self);
  return self->user_agent.c_str();
}

Cronet_String Cronet_EngineParams_accept_language_get(
    Cronet_EngineParamsPtr self) {
  DCHECK(self);
  return self->accept_language.c_str();
}

Cronet_String Cronet_EngineParams_storage_path_get(
    Cronet_EngineParamsPtr self) {
  DCHECK(self);
  return self->storage_path.c_str();
}

bool Cronet_EngineParams_enable_quic_get(Cronet_EngineParamsPtr self) {
  DCHECK(self);
  return self->enable_quic;
}

bool Cronet_EngineParams_enable_http2_get(Cronet_EngineParamsPtr self) {
  DCHECK(self);
  return self->enable_http2;
}

bool Cronet_EngineParams_enable_brotli_get(Cronet_EngineParamsPtr self) {
  DCHECK(self);
  return self->enable_brotli;
}

Cronet_EngineParams_HTTP_CACHE_MODE Cronet_EngineParams_http_cache_mode_get(
    Cronet_EngineParamsPtr self) {
  DCHECK(self);
  return self->http_cache_mode;
}

int64_t Cronet_EngineParams_http_cache_max_size_get(
    Cronet_EngineParamsPtr self) {
  DCHECK(self);
  return self->http_cache_max_size;
}

uint32_t Cronet_EngineParams_quic_hints_size(Cronet_EngineParamsPtr self) {
  DCHECK(self);
  return self->quic_hints.size();
}
Cronet_QuicHintPtr Cronet_EngineParams_quic_hints_at(
    Cronet_EngineParamsPtr self,
    uint32_t index) {
  DCHECK(self);
  DCHECK(index < self->quic_hints.size());
  return &(self->quic_hints[index]);
}
void Cronet_EngineParams_quic_hints_clear(Cronet_EngineParamsPtr self) {
  DCHECK(self);
  self->quic_hints.clear();
}

uint32_t Cronet_EngineParams_public_key_pins_size(Cronet_EngineParamsPtr self) {
  DCHECK(self);
  return self->public_key_pins.size();
}
Cronet_PublicKeyPinsPtr Cronet_EngineParams_public_key_pins_at(
    Cronet_EngineParamsPtr self,
    uint32_t index) {
  DCHECK(self);
  DCHECK(index < self->public_key_pins.size());
  return &(self->public_key_pins[index]);
}
void Cronet_EngineParams_public_key_pins_clear(Cronet_EngineParamsPtr self) {
  DCHECK(self);
  self->public_key_pins.clear();
}

bool Cronet_EngineParams_enable_public_key_pinning_bypass_for_local_trust_anchors_get(
    Cronet_EngineParamsPtr self) {
  DCHECK(self);
  return self->enable_public_key_pinning_bypass_for_local_trust_anchors;
}

Cronet_String Cronet_EngineParams_experimental_options_get(
    Cronet_EngineParamsPtr self) {
  DCHECK(self);
  return self->experimental_options.c_str();
}

// Struct Cronet_HttpHeader.
Cronet_HttpHeader::Cronet_HttpHeader() = default;

Cronet_HttpHeader::Cronet_HttpHeader(const Cronet_HttpHeader& from) = default;

Cronet_HttpHeader::Cronet_HttpHeader(Cronet_HttpHeader&& from) = default;

Cronet_HttpHeader::~Cronet_HttpHeader() = default;

Cronet_HttpHeaderPtr Cronet_HttpHeader_Create() {
  return new Cronet_HttpHeader();
}

void Cronet_HttpHeader_Destroy(Cronet_HttpHeaderPtr self) {
  delete self;
}

// Struct Cronet_HttpHeader setters.
void Cronet_HttpHeader_name_set(Cronet_HttpHeaderPtr self, Cronet_String name) {
  DCHECK(self);
  self->name = name;
}

void Cronet_HttpHeader_value_set(Cronet_HttpHeaderPtr self,
                                 Cronet_String value) {
  DCHECK(self);
  self->value = value;
}

// Struct Cronet_HttpHeader getters.
Cronet_String Cronet_HttpHeader_name_get(Cronet_HttpHeaderPtr self) {
  DCHECK(self);
  return self->name.c_str();
}

Cronet_String Cronet_HttpHeader_value_get(Cronet_HttpHeaderPtr self) {
  DCHECK(self);
  return self->value.c_str();
}

// Struct Cronet_UrlResponseInfo.
Cronet_UrlResponseInfo::Cronet_UrlResponseInfo() = default;

Cronet_UrlResponseInfo::Cronet_UrlResponseInfo(
    const Cronet_UrlResponseInfo& from) = default;

Cronet_UrlResponseInfo::Cronet_UrlResponseInfo(Cronet_UrlResponseInfo&& from) =
    default;

Cronet_UrlResponseInfo::~Cronet_UrlResponseInfo() = default;

Cronet_UrlResponseInfoPtr Cronet_UrlResponseInfo_Create() {
  return new Cronet_UrlResponseInfo();
}

void Cronet_UrlResponseInfo_Destroy(Cronet_UrlResponseInfoPtr self) {
  delete self;
}

// Struct Cronet_UrlResponseInfo setters.
void Cronet_UrlResponseInfo_url_set(Cronet_UrlResponseInfoPtr self,
                                    Cronet_String url) {
  DCHECK(self);
  self->url = url;
}

void Cronet_UrlResponseInfo_url_chain_add(Cronet_UrlResponseInfoPtr self,
                                          Cronet_String element) {
  DCHECK(self);
  self->url_chain.push_back(element);
}

void Cronet_UrlResponseInfo_http_status_code_set(Cronet_UrlResponseInfoPtr self,
                                                 int32_t http_status_code) {
  DCHECK(self);
  self->http_status_code = http_status_code;
}

void Cronet_UrlResponseInfo_http_status_text_set(
    Cronet_UrlResponseInfoPtr self,
    Cronet_String http_status_text) {
  DCHECK(self);
  self->http_status_text = http_status_text;
}

void Cronet_UrlResponseInfo_all_headers_list_add(Cronet_UrlResponseInfoPtr self,
                                                 Cronet_HttpHeaderPtr element) {
  DCHECK(self);
  self->all_headers_list.push_back(*element);
}

void Cronet_UrlResponseInfo_was_cached_set(Cronet_UrlResponseInfoPtr self,
                                           bool was_cached) {
  DCHECK(self);
  self->was_cached = was_cached;
}

void Cronet_UrlResponseInfo_negotiated_protocol_set(
    Cronet_UrlResponseInfoPtr self,
    Cronet_String negotiated_protocol) {
  DCHECK(self);
  self->negotiated_protocol = negotiated_protocol;
}

void Cronet_UrlResponseInfo_proxy_server_set(Cronet_UrlResponseInfoPtr self,
                                             Cronet_String proxy_server) {
  DCHECK(self);
  self->proxy_server = proxy_server;
}

void Cronet_UrlResponseInfo_received_byte_count_set(
    Cronet_UrlResponseInfoPtr self,
    int64_t received_byte_count) {
  DCHECK(self);
  self->received_byte_count = received_byte_count;
}

// Struct Cronet_UrlResponseInfo getters.
Cronet_String Cronet_UrlResponseInfo_url_get(Cronet_UrlResponseInfoPtr self) {
  DCHECK(self);
  return self->url.c_str();
}

uint32_t Cronet_UrlResponseInfo_url_chain_size(Cronet_UrlResponseInfoPtr self) {
  DCHECK(self);
  return self->url_chain.size();
}
Cronet_String Cronet_UrlResponseInfo_url_chain_at(
    Cronet_UrlResponseInfoPtr self,
    uint32_t index) {
  DCHECK(self);
  DCHECK(index < self->url_chain.size());
  return self->url_chain[index].c_str();
}
void Cronet_UrlResponseInfo_url_chain_clear(Cronet_UrlResponseInfoPtr self) {
  DCHECK(self);
  self->url_chain.clear();
}

int32_t Cronet_UrlResponseInfo_http_status_code_get(
    Cronet_UrlResponseInfoPtr self) {
  DCHECK(self);
  return self->http_status_code;
}

Cronet_String Cronet_UrlResponseInfo_http_status_text_get(
    Cronet_UrlResponseInfoPtr self) {
  DCHECK(self);
  return self->http_status_text.c_str();
}

uint32_t Cronet_UrlResponseInfo_all_headers_list_size(
    Cronet_UrlResponseInfoPtr self) {
  DCHECK(self);
  return self->all_headers_list.size();
}
Cronet_HttpHeaderPtr Cronet_UrlResponseInfo_all_headers_list_at(
    Cronet_UrlResponseInfoPtr self,
    uint32_t index) {
  DCHECK(self);
  DCHECK(index < self->all_headers_list.size());
  return &(self->all_headers_list[index]);
}
void Cronet_UrlResponseInfo_all_headers_list_clear(
    Cronet_UrlResponseInfoPtr self) {
  DCHECK(self);
  self->all_headers_list.clear();
}

bool Cronet_UrlResponseInfo_was_cached_get(Cronet_UrlResponseInfoPtr self) {
  DCHECK(self);
  return self->was_cached;
}

Cronet_String Cronet_UrlResponseInfo_negotiated_protocol_get(
    Cronet_UrlResponseInfoPtr self) {
  DCHECK(self);
  return self->negotiated_protocol.c_str();
}

Cronet_String Cronet_UrlResponseInfo_proxy_server_get(
    Cronet_UrlResponseInfoPtr self) {
  DCHECK(self);
  return self->proxy_server.c_str();
}

int64_t Cronet_UrlResponseInfo_received_byte_count_get(
    Cronet_UrlResponseInfoPtr self) {
  DCHECK(self);
  return self->received_byte_count;
}

// Struct Cronet_UrlRequestParams.
Cronet_UrlRequestParams::Cronet_UrlRequestParams() = default;

Cronet_UrlRequestParams::Cronet_UrlRequestParams(
    const Cronet_UrlRequestParams& from) = default;

Cronet_UrlRequestParams::Cronet_UrlRequestParams(
    Cronet_UrlRequestParams&& from) = default;

Cronet_UrlRequestParams::~Cronet_UrlRequestParams() = default;

Cronet_UrlRequestParamsPtr Cronet_UrlRequestParams_Create() {
  return new Cronet_UrlRequestParams();
}

void Cronet_UrlRequestParams_Destroy(Cronet_UrlRequestParamsPtr self) {
  delete self;
}

// Struct Cronet_UrlRequestParams setters.
void Cronet_UrlRequestParams_http_method_set(Cronet_UrlRequestParamsPtr self,
                                             Cronet_String http_method) {
  DCHECK(self);
  self->http_method = http_method;
}

void Cronet_UrlRequestParams_request_headers_add(
    Cronet_UrlRequestParamsPtr self,
    Cronet_HttpHeaderPtr element) {
  DCHECK(self);
  self->request_headers.push_back(*element);
}

void Cronet_UrlRequestParams_disable_cache_set(Cronet_UrlRequestParamsPtr self,
                                               bool disable_cache) {
  DCHECK(self);
  self->disable_cache = disable_cache;
}

void Cronet_UrlRequestParams_priority_set(
    Cronet_UrlRequestParamsPtr self,
    Cronet_UrlRequestParams_REQUEST_PRIORITY priority) {
  DCHECK(self);
  self->priority = priority;
}

void Cronet_UrlRequestParams_upload_data_provider_set(
    Cronet_UrlRequestParamsPtr self,
    Cronet_UploadDataProviderPtr upload_data_provider) {
  DCHECK(self);
  self->upload_data_provider = upload_data_provider;
}

void Cronet_UrlRequestParams_upload_data_provider_executor_set(
    Cronet_UrlRequestParamsPtr self,
    Cronet_ExecutorPtr upload_data_provider_executor) {
  DCHECK(self);
  self->upload_data_provider_executor = upload_data_provider_executor;
}

void Cronet_UrlRequestParams_allow_direct_executor_set(
    Cronet_UrlRequestParamsPtr self,
    bool allow_direct_executor) {
  DCHECK(self);
  self->allow_direct_executor = allow_direct_executor;
}

void Cronet_UrlRequestParams_annotations_add(Cronet_UrlRequestParamsPtr self,
                                             Cronet_RawDataPtr element) {
  DCHECK(self);
  self->annotations.push_back(element);
}

// Struct Cronet_UrlRequestParams getters.
Cronet_String Cronet_UrlRequestParams_http_method_get(
    Cronet_UrlRequestParamsPtr self) {
  DCHECK(self);
  return self->http_method.c_str();
}

uint32_t Cronet_UrlRequestParams_request_headers_size(
    Cronet_UrlRequestParamsPtr self) {
  DCHECK(self);
  return self->request_headers.size();
}
Cronet_HttpHeaderPtr Cronet_UrlRequestParams_request_headers_at(
    Cronet_UrlRequestParamsPtr self,
    uint32_t index) {
  DCHECK(self);
  DCHECK(index < self->request_headers.size());
  return &(self->request_headers[index]);
}
void Cronet_UrlRequestParams_request_headers_clear(
    Cronet_UrlRequestParamsPtr self) {
  DCHECK(self);
  self->request_headers.clear();
}

bool Cronet_UrlRequestParams_disable_cache_get(
    Cronet_UrlRequestParamsPtr self) {
  DCHECK(self);
  return self->disable_cache;
}

Cronet_UrlRequestParams_REQUEST_PRIORITY Cronet_UrlRequestParams_priority_get(
    Cronet_UrlRequestParamsPtr self) {
  DCHECK(self);
  return self->priority;
}

Cronet_UploadDataProviderPtr Cronet_UrlRequestParams_upload_data_provider_get(
    Cronet_UrlRequestParamsPtr self) {
  DCHECK(self);
  return self->upload_data_provider;
}

Cronet_ExecutorPtr Cronet_UrlRequestParams_upload_data_provider_executor_get(
    Cronet_UrlRequestParamsPtr self) {
  DCHECK(self);
  return self->upload_data_provider_executor;
}

bool Cronet_UrlRequestParams_allow_direct_executor_get(
    Cronet_UrlRequestParamsPtr self) {
  DCHECK(self);
  return self->allow_direct_executor;
}

uint32_t Cronet_UrlRequestParams_annotations_size(
    Cronet_UrlRequestParamsPtr self) {
  DCHECK(self);
  return self->annotations.size();
}
Cronet_RawDataPtr Cronet_UrlRequestParams_annotations_at(
    Cronet_UrlRequestParamsPtr self,
    uint32_t index) {
  DCHECK(self);
  DCHECK(index < self->annotations.size());
  return self->annotations[index];
}
void Cronet_UrlRequestParams_annotations_clear(
    Cronet_UrlRequestParamsPtr self) {
  DCHECK(self);
  self->annotations.clear();
}

// Struct Cronet_RequestFinishedInfo.
Cronet_RequestFinishedInfo::Cronet_RequestFinishedInfo() = default;

Cronet_RequestFinishedInfo::Cronet_RequestFinishedInfo(
    const Cronet_RequestFinishedInfo& from) = default;

Cronet_RequestFinishedInfo::Cronet_RequestFinishedInfo(
    Cronet_RequestFinishedInfo&& from) = default;

Cronet_RequestFinishedInfo::~Cronet_RequestFinishedInfo() = default;

Cronet_RequestFinishedInfoPtr Cronet_RequestFinishedInfo_Create() {
  return new Cronet_RequestFinishedInfo();
}

void Cronet_RequestFinishedInfo_Destroy(Cronet_RequestFinishedInfoPtr self) {
  delete self;
}

// Struct Cronet_RequestFinishedInfo setters.

// Struct Cronet_RequestFinishedInfo getters.
