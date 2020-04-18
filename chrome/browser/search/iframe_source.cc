// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/search/iframe_source.h"

#include "base/memory/ref_counted_memory.h"
#include "base/strings/string_piece.h"
#include "base/strings/string_util.h"
#include "chrome/browser/search/instant_io_context.h"
#include "chrome/common/url_constants.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/resource/resource_bundle.h"
#include "url/gurl.h"

IframeSource::IframeSource() = default;

IframeSource::~IframeSource() = default;

std::string IframeSource::GetMimeType(
    const std::string& path_and_query) const {
  std::string path(GURL("chrome-search://host/" + path_and_query).path());
  if (base::EndsWith(path, ".js", base::CompareCase::INSENSITIVE_ASCII))
    return "application/javascript";
  if (base::EndsWith(path, ".png", base::CompareCase::INSENSITIVE_ASCII))
    return "image/png";
  if (base::EndsWith(path, ".css", base::CompareCase::INSENSITIVE_ASCII))
    return "text/css";
  if (base::EndsWith(path, ".html", base::CompareCase::INSENSITIVE_ASCII))
    return "text/html";
  return std::string();
}

bool IframeSource::AllowCaching() const {
  return false;
}

bool IframeSource::ShouldServiceRequest(
    const GURL& url,
    content::ResourceContext* resource_context,
    int render_process_id) const {
  return InstantIOContext::ShouldServiceRequest(url, resource_context,
                                                render_process_id) &&
         url.SchemeIs(chrome::kChromeSearchScheme) &&
         url.host_piece() == GetSource() && ServesPath(url.path());
}

bool IframeSource::ShouldDenyXFrameOptions() const {
  return false;
}

bool IframeSource::GetOrigin(
    const content::ResourceRequestInfo::WebContentsGetter& wc_getter,
    std::string* origin) const {
  if (wc_getter.is_null())
    return false;
  const content::WebContents* contents = wc_getter.Run();
  if (!contents)
    return false;
  const content::NavigationEntry* entry =
      contents->GetController().GetVisibleEntry();
  if (!entry)
    return false;

  *origin = entry->GetURL().GetOrigin().spec();
  // Origin should not include a trailing slash. That is part of the path.
  base::TrimString(*origin, "/", origin);
  return true;
}

void IframeSource::SendResource(
    int resource_id,
    const content::URLDataSource::GotDataCallback& callback) {
  scoped_refptr<base::RefCountedMemory> response(
      ui::ResourceBundle::GetSharedInstance().LoadDataResourceBytes(
          resource_id));
  callback.Run(response.get());
}

void IframeSource::SendJSWithOrigin(
    int resource_id,
    const content::ResourceRequestInfo::WebContentsGetter& wc_getter,
    const content::URLDataSource::GotDataCallback& callback) {
  std::string origin;
  if (!GetOrigin(wc_getter, &origin)) {
    callback.Run(nullptr);
    return;
  }

  base::StringPiece template_js =
      ui::ResourceBundle::GetSharedInstance().GetRawDataResource(resource_id);
  std::string response(template_js.as_string());
  base::ReplaceFirstSubstringAfterOffset(&response, 0, "{{ORIGIN}}", origin);
  callback.Run(base::RefCountedString::TakeString(&response));
}
