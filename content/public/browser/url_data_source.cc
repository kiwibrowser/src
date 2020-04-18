// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/browser/url_data_source.h"

#include "content/browser/webui/url_data_manager.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/common/url_constants.h"
#include "net/url_request/url_request.h"

namespace content {

void URLDataSource::Add(BrowserContext* browser_context,
                        URLDataSource* source) {
  URLDataManager::AddDataSource(browser_context, source);
}

scoped_refptr<base::SingleThreadTaskRunner>
URLDataSource::TaskRunnerForRequestPath(const std::string& path) const {
  return BrowserThread::GetTaskRunnerForThread(BrowserThread::UI);
}

bool URLDataSource::ShouldReplaceExistingSource() const {
  return true;
}

bool URLDataSource::AllowCaching() const {
  return true;
}

bool URLDataSource::ShouldAddContentSecurityPolicy() const {
  return true;
}

std::string URLDataSource::GetContentSecurityPolicyScriptSrc() const {
  // Specific resources require unsafe-eval in the Content Security Policy.
  // TODO(tsepez,mfoltz): Remove 'unsafe-eval' when tests have been fixed to
  // not use eval()/new Function().  http://crbug.com/525224
  return "script-src chrome://resources 'self' 'unsafe-eval';";
}

std::string URLDataSource::GetContentSecurityPolicyObjectSrc() const {
  return "object-src 'none';";
}

std::string URLDataSource::GetContentSecurityPolicyChildSrc() const {
  return "child-src 'none';";
}

std::string URLDataSource::GetContentSecurityPolicyStyleSrc() const {
  return std::string();
}

std::string URLDataSource::GetContentSecurityPolicyImgSrc() const {
  return std::string();
}

bool URLDataSource::ShouldDenyXFrameOptions() const {
  return true;
}

bool URLDataSource::ShouldServiceRequest(const GURL& url,
                                         ResourceContext* resource_context,
                                         int render_process_id) const {
  return url.SchemeIs(kChromeDevToolsScheme) || url.SchemeIs(kChromeUIScheme);
}

bool URLDataSource::ShouldServeMimeTypeAsContentTypeHeader() const {
  return false;
}

std::string URLDataSource::GetAccessControlAllowOriginForOrigin(
    const std::string& origin) const {
  return std::string();
}

bool URLDataSource::IsGzipped(const std::string& path) const {
  return false;
}

}  // namespace content
