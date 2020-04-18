// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/download/download_controller_base.h"
#include "chrome/common/chrome_content_client.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/resource_request_info.h"
#include "content/public/browser/web_contents.h"
#include "net/url_request/url_request.h"

const char kOMADrmMessageMimeType[] = "application/vnd.oma.drm.message";
const char kOMADrmContentMimeType[] = "application/vnd.oma.drm.content";
const char kOMADrmRightsMimeType1[] = "application/vnd.oma.drm.rights+xml";
const char kOMADrmRightsMimeType2[] = "application/vnd.oma.drm.rights+wbxml";

content::WebContents* GetWebContents(int render_process_id,
                                     int render_view_id) {
  content::RenderViewHost* render_view_host =
      content::RenderViewHost::FromID(render_process_id, render_view_id);

  if (!render_view_host)
    return nullptr;

  return content::WebContents::FromRenderViewHost(render_view_host);
}

// static
DownloadControllerBase* DownloadControllerBase::download_controller_ = nullptr;

using content::ResourceRequestInfo;

DownloadInfo::DownloadInfo(const GURL& url,
                           const GURL& original_url,
                           const std::string& content_disposition,
                           const std::string& original_mime_type,
                           const std::string& user_agent,
                           const std::string& cookie,
                           const std::string& referer)
    : url(url),
      original_url(original_url),
      content_disposition(content_disposition),
      original_mime_type(original_mime_type),
      user_agent(user_agent),
      cookie(cookie),
      referer(referer) {}

DownloadInfo::DownloadInfo(const net::URLRequest* request) {
  request->GetResponseHeaderByName("content-disposition", &content_disposition);

  if (request->response_headers())
    request->response_headers()->GetMimeType(&original_mime_type);

  request->extra_request_headers().GetHeader(
    net::HttpRequestHeaders::kUserAgent, &user_agent);
  if (user_agent.empty())
    user_agent = GetUserAgent();
  GURL referer_url(request->referrer());
  if (referer_url.is_valid())
    referer = referer_url.spec();
  if (!request->url_chain().empty()) {
    original_url = request->url_chain().front();
    url = request->url_chain().back();
  }
}

DownloadInfo::DownloadInfo(const DownloadInfo& other) = default;

DownloadInfo::~DownloadInfo() {}
