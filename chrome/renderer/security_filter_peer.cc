// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/renderer/security_filter_peer.h"

#include <memory>
#include <string>
#include <utility>

#include "base/memory/ptr_util.h"
#include "base/strings/stringprintf.h"
#include "chrome/grit/generated_resources.h"
#include "content/public/renderer/fixed_received_data.h"
#include "net/base/escape.h"
#include "net/base/net_errors.h"
#include "net/http/http_response_headers.h"
#include "services/network/public/cpp/url_loader_completion_status.h"
#include "ui/base/l10n/l10n_util.h"

SecurityFilterPeer::SecurityFilterPeer(
    std::unique_ptr<content::RequestPeer> peer,
    const std::string& mime_type,
    const std::string& data)
    : original_peer_(std::move(peer)), mime_type_(mime_type), data_(data) {}
SecurityFilterPeer::~SecurityFilterPeer() {}

// static
std::unique_ptr<content::RequestPeer>
SecurityFilterPeer::CreateSecurityFilterPeerForDeniedRequest(
    content::ResourceType resource_type,
    std::unique_ptr<content::RequestPeer> peer,
    int os_error) {
  // Create a filter for SSL and CERT errors.
  switch (os_error) {
    case net::ERR_SSL_PROTOCOL_ERROR:
    case net::ERR_CERT_COMMON_NAME_INVALID:
    case net::ERR_CERT_DATE_INVALID:
    case net::ERR_CERT_AUTHORITY_INVALID:
    case net::ERR_CERT_CONTAINS_ERRORS:
    case net::ERR_CERT_NO_REVOCATION_MECHANISM:
    case net::ERR_CERT_UNABLE_TO_CHECK_REVOCATION:
    case net::ERR_CERT_REVOKED:
    case net::ERR_CERT_INVALID:
    case net::ERR_CERT_WEAK_SIGNATURE_ALGORITHM:
    case net::ERR_CERT_WEAK_KEY:
    case net::ERR_CERT_NAME_CONSTRAINT_VIOLATION:
    case net::ERR_INSECURE_RESPONSE:
    case net::ERR_SSL_PINNED_KEY_NOT_IN_CERT_CHAIN: {
      std::string mime_type;
      std::string data;
      if (content::IsResourceTypeFrame(resource_type)) {
        // TODO(jcampan): use a different message when getting a
        // phishing/malware error.
        data = base::StringPrintf(
            "<html><meta charset='UTF-8'>"
            "<body style='background-color:#990000;color:white;'>"
            "%s</body></html>",
            net::EscapeForHTML(
                l10n_util::GetStringUTF8(IDS_UNSAFE_FRAME_MESSAGE))
                .c_str());
        mime_type = "text/html";
      }
      return base::WrapUnique(
          new SecurityFilterPeer(std::move(peer), mime_type, data));
    }
    default:
      // For other errors, we use our normal error handling.
      return peer;
  }
}

void SecurityFilterPeer::OnUploadProgress(uint64_t position, uint64_t size) {
  NOTREACHED();
}

bool SecurityFilterPeer::OnReceivedRedirect(
    const net::RedirectInfo& redirect_info,
    const network::ResourceResponseInfo& info) {
  NOTREACHED();
  return false;
}

void SecurityFilterPeer::OnReceivedResponse(
    const network::ResourceResponseInfo& info) {
  NOTREACHED();
}

void SecurityFilterPeer::OnStartLoadingResponseBody(
    mojo::ScopedDataPipeConsumerHandle body) {
  NOTREACHED();
}

void SecurityFilterPeer::OnDownloadedData(int len, int encoded_data_length) {
  NOTREACHED();
}

void SecurityFilterPeer::OnReceivedData(std::unique_ptr<ReceivedData> data) {
  NOTREACHED();
}

void SecurityFilterPeer::OnTransferSizeUpdated(int transfer_size_diff) {
  NOTREACHED();
}

void SecurityFilterPeer::OnCompletedRequest(
    const network::URLLoaderCompletionStatus& status) {
  network::ResourceResponseInfo info;
  info.mime_type = mime_type_;
  info.headers = CreateHeaders(mime_type_);
  info.content_length = static_cast<int>(data_.size());
  original_peer_->OnReceivedResponse(info);
  if (!data_.empty()) {
    original_peer_->OnReceivedData(std::make_unique<content::FixedReceivedData>(
        data_.data(), data_.size()));
  }
  network::URLLoaderCompletionStatus ok_status(status);
  ok_status.error_code = net::OK;
  original_peer_->OnCompletedRequest(ok_status);
}

scoped_refptr<net::HttpResponseHeaders> SecurityFilterPeer::CreateHeaders(
    const std::string& mime_type) {
  std::string raw_headers;
  raw_headers.append("HTTP/1.1 200 OK");
  raw_headers.push_back('\0');
  // Don't cache the data we are serving, it is not the real data for that URL
  // (if the filtered resource were to make it into the WebCore cache, then the
  // same URL loaded in a safe scenario would still return the filtered
  // resource).
  raw_headers.append("cache-control: no-cache");
  raw_headers.push_back('\0');
  if (!mime_type.empty()) {
    raw_headers.append("content-type: ");
    raw_headers.append(mime_type);
    raw_headers.push_back('\0');
  }
  raw_headers.push_back('\0');
  return base::MakeRefCounted<net::HttpResponseHeaders>(raw_headers);
}
