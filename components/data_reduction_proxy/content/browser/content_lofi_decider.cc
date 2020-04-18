// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/data_reduction_proxy/content/browser/content_lofi_decider.h"

#include <string>

#include "base/feature_list.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "components/data_reduction_proxy/core/common/data_reduction_proxy_features.h"
#include "components/data_reduction_proxy/core/common/data_reduction_proxy_headers.h"
#include "components/data_reduction_proxy/core/common/data_reduction_proxy_params.h"
#include "components/previews/core/previews_decider.h"
#include "content/public/browser/resource_request_info.h"
#include "content/public/common/resource_type.h"
#include "net/base/load_flags.h"
#include "net/http/http_request_headers.h"
#include "net/url_request/url_request.h"
#include "url/gurl.h"

namespace data_reduction_proxy {

ContentLoFiDecider::ContentLoFiDecider() {}

ContentLoFiDecider::~ContentLoFiDecider() {}

// Static
content::PreviewsState
ContentLoFiDecider::DetermineCommittedServerPreviewsState(
    const net::URLRequest& request,
    content::PreviewsState initial_state) {
  DCHECK_EQ(
      content::RESOURCE_TYPE_MAIN_FRAME,
      content::ResourceRequestInfo::ForRequest(&request)->GetResourceType())
      << "Request was not for main frame";
  data_reduction_proxy::DataReductionProxyData* drp_data =
      data_reduction_proxy::DataReductionProxyData::GetData(request);
  if (!drp_data) {
    return initial_state &=
           ~(content::SERVER_LITE_PAGE_ON | content::SERVER_LOFI_ON);
  }
  content::PreviewsState updated_state = initial_state;
  if (!drp_data->lite_page_received()) {
    // Turn off LitePage bit.
    updated_state &= ~(content::SERVER_LITE_PAGE_ON);
  }
  if (!drp_data->lofi_policy_received()) {
    // Turn off LoFi bit(s).
    updated_state &= ~(content::SERVER_LOFI_ON);
    if (drp_data->used_data_reduction_proxy()) {
      // Turn off Client LoFi bit also if using proxy but proxy did not
      // request LoFi.
      updated_state &= ~(content::CLIENT_LOFI_ON);
    }
  }
  return updated_state;
}

void ContentLoFiDecider::MaybeSetAcceptTransformHeader(
    const net::URLRequest& request,
    net::HttpRequestHeaders* headers) const {
  const content::ResourceRequestInfo* request_info =
      content::ResourceRequestInfo::ForRequest(&request);

  if (!request_info)
    return;

  // Previews only operate on HTTP.
  if (!request.url().SchemeIs("http"))
    return;

  // Chrome-Proxy-Accept-Transform takes at most one token.
  if (headers->HasHeader(chrome_proxy_accept_transform_header()))
    return;

  content::ResourceType resource_type = request_info->GetResourceType();

  if (resource_type == content::RESOURCE_TYPE_MEDIA) {
    headers->SetHeader(chrome_proxy_accept_transform_header(),
                       compressed_video_directive());
    return;
  }

  content::PreviewsState previews_state = request_info->GetPreviewsState();

  // Do not add the Chrome-Proxy-Accept-Transform header when the page load
  // explicitly forbids previews transformations.
  if (previews_state & content::PREVIEWS_NO_TRANSFORM ||
      previews_state & content::PREVIEWS_OFF) {
    return;
  }

  std::string accept_transform_value;
  if ((previews_state & content::SERVER_LITE_PAGE_ON) &&
      resource_type == content::RESOURCE_TYPE_MAIN_FRAME) {
    accept_transform_value = lite_page_directive();
  } else if ((previews_state & content::SERVER_LOFI_ON)) {
    // Note that for subresource requests, the Lo-Fi bit should only be set
    // if the main frame response provided the "empty-image" directive (for
    // the client to echo back to the server here for any image resources).
    // Also, it should only be set for subresource requests that might be
    // image requests.
    bool resource_type_supports_empty_image =
        !(resource_type == content::RESOURCE_TYPE_MAIN_FRAME ||
          resource_type == content::RESOURCE_TYPE_STYLESHEET ||
          resource_type == content::RESOURCE_TYPE_SCRIPT ||
          resource_type == content::RESOURCE_TYPE_FONT_RESOURCE ||
          resource_type == content::RESOURCE_TYPE_MEDIA ||
          resource_type == content::RESOURCE_TYPE_CSP_REPORT);
    if (resource_type_supports_empty_image) {
      accept_transform_value = empty_image_directive();
    }
  }

  if (accept_transform_value.empty())
    return;

  headers->SetHeader(chrome_proxy_accept_transform_header(),
                     accept_transform_value);
}

bool ContentLoFiDecider::IsSlowPagePreviewRequested(
    const net::HttpRequestHeaders& headers) const {
  std::string accept_transform_header_value;
  if (!headers.GetHeader(chrome_proxy_accept_transform_header(),
                         &accept_transform_header_value)) {
    return false;
  }

  std::vector<std::string> tokens =
      base::SplitString(base::ToLowerASCII(accept_transform_header_value), ";",
                        base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL);
  // A slow page preview is a request for any unqualified transform type.
  if (tokens.size() != 1)
    return false;
  std::string transform_type;
  base::TrimWhitespaceASCII(tokens[0], base::TRIM_ALL, &transform_type);
  return (transform_type == lite_page_directive() ||
          transform_type == empty_image_directive());
}

bool ContentLoFiDecider::IsLitePagePreviewRequested(
    const net::HttpRequestHeaders& headers) const {
  std::string accept_transform_header_value;
  if (!headers.GetHeader(chrome_proxy_accept_transform_header(),
                         &accept_transform_header_value)) {
    return false;
  }
  std::vector<std::string> tokens =
      base::SplitString(base::ToLowerASCII(accept_transform_header_value), ";",
                        base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL);
  if (tokens.empty())
    return false;
  std::string transform_type;
  base::TrimWhitespaceASCII(tokens[0], base::TRIM_ALL, &transform_type);
  return transform_type == lite_page_directive();
}

void ContentLoFiDecider::RemoveAcceptTransformHeader(
    net::HttpRequestHeaders* headers) const {
  headers->RemoveHeader(chrome_proxy_accept_transform_header());
}

bool ContentLoFiDecider::ShouldRecordLoFiUMA(
    const net::URLRequest& request) const {
  const content::ResourceRequestInfo* request_info =
      content::ResourceRequestInfo::ForRequest(&request);

  if (!request_info)
    return false;

  return request_info->GetPreviewsState() & content::SERVER_LOFI_ON ||
         request_info->GetPreviewsState() & content::SERVER_LITE_PAGE_ON;
}

bool ContentLoFiDecider::IsClientLoFiImageRequest(
    const net::URLRequest& request) const {
  const content::ResourceRequestInfo* request_info =
      content::ResourceRequestInfo::ForRequest(&request);
  return request_info &&
         request_info->GetResourceType() == content::RESOURCE_TYPE_IMAGE &&
         (request_info->GetPreviewsState() & content::CLIENT_LOFI_ON);
}

bool ContentLoFiDecider::IsClientLoFiAutoReloadRequest(
    const net::URLRequest& request) const {
  const content::ResourceRequestInfo* request_info =
      content::ResourceRequestInfo::ForRequest(&request);
  return request_info &&
         (request_info->GetPreviewsState() & content::CLIENT_LOFI_AUTO_RELOAD);
}

void ContentLoFiDecider::MaybeApplyAMPPreview(
    net::URLRequest* request,
    GURL* new_url,
    previews::PreviewsDecider* previews_decider) const {
  const content::ResourceRequestInfo* request_info =
      content::ResourceRequestInfo::ForRequest(request);
  if (!request_info ||
      request_info->GetResourceType() != content::RESOURCE_TYPE_MAIN_FRAME ||
      !previews::params::IsAMPRedirectionPreviewEnabled() ||
      !request->url().has_host() ||
      !previews_decider->ShouldAllowPreview(
          *request, previews::PreviewsType::AMP_REDIRECTION)) {
    return;
  }

  // TODO(rajendrant): Apply the matching logic for |request| and update
  // |new_url| to its AMP version.
}

}  // namespace data_reduction_proxy
