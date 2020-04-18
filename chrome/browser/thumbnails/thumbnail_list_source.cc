// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/thumbnails/thumbnail_list_source.h"

#include <stddef.h>

#include <string>

#include "base/base64.h"
#include "base/bind.h"
#include "base/logging.h"
#include "base/memory/ref_counted_memory.h"
#include "base/strings/strcat.h"
#include "chrome/browser/history/top_sites_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search/instant_io_context.h"
#include "chrome/browser/thumbnails/thumbnail_service.h"
#include "chrome/browser/thumbnails/thumbnail_service_factory.h"
#include "chrome/common/url_constants.h"
#include "components/history/core/browser/top_sites.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/escape.h"
#include "net/url_request/url_request.h"

using content::BrowserThread;

namespace {

const char kHtmlHeader[] =
    "<!DOCTYPE html>\n<html>\n<head>\n<title>TopSites Thumbnails</title>\n"
    "<meta charset=\"utf-8\">\n"
    "<style type=\"text/css\">\nimg.thumb {border: 1px solid black;}\n"
    "li {white-space: nowrap;}\n</style>\n";
const char kHtmlBody[] = "</head>\n<body>\n";
const char kHtmlFooter[] = "</body>\n</html>\n";

// If |want_thumbnails| == true, then renders elements in |mvurl_list| that have
// thumbnails, with their thumbnails. Otherwise renders elements in |mvurl_list|
// that have no thumbnails.
void RenderMostVisitedURLList(
    const history::MostVisitedURLList& mvurl_list,
    const std::vector<std::string>& base64_encoded_pngs,
    bool want_thumbnails,
    std::vector<std::string>* out) {
  DCHECK_EQ(mvurl_list.size(), base64_encoded_pngs.size());
  bool doing_forced_urls = true;
  out->push_back("<div><b>Forced URLs:</b></div>\n"
                 "<div><ul>\n");
  for (size_t i = 0; i < mvurl_list.size(); ++i) {
    const history::MostVisitedURL& mvurl = mvurl_list[i];
    if (doing_forced_urls && mvurl.last_forced_time.is_null()) {
      out->push_back("</ul></div>\n"
                     "<div><b>Non-forced URLs:</b></div>\n"
                     "<div><ul>\n");
      doing_forced_urls = false;
    }
    bool has_thumbnail = !base64_encoded_pngs[i].empty();
    if (has_thumbnail == want_thumbnails) {
      out->push_back("<li>\n");
      out->push_back(net::EscapeForHTML(mvurl.url.spec()) + "\n");
      if (want_thumbnails) {
        out->push_back("<div><img class=\"thumb\" "
                       "src=\"data:image/png;base64," +
                       base64_encoded_pngs[i] + "\"/></div>\n");
      }
      if (!mvurl.redirects.empty()) {
        out->push_back("<ul>\n");
        history::RedirectList::const_iterator jt;
        for (jt = mvurl.redirects.begin();
             jt != mvurl.redirects.end(); ++jt) {
          out->push_back("<li>" + net::EscapeForHTML(jt->spec()) + "</li>\n");
        }
        out->push_back("</ul>\n");
      }
      out->push_back("</li>\n");
    }
  }
  out->push_back("</ul></div>\n");
}

}  // namespace

ThumbnailListSource::ThumbnailListSource(Profile* profile)
    : thumbnail_service_(ThumbnailServiceFactory::GetForProfile(profile)),
      top_sites_(TopSitesFactory::GetForProfile(profile)),
      weak_ptr_factory_(this) {
}

ThumbnailListSource::~ThumbnailListSource() {
}

std::string ThumbnailListSource::GetSource() const {
  return chrome::kChromeUIThumbnailListHost;
}

void ThumbnailListSource::StartDataRequest(
    const std::string& path,
    const content::ResourceRequestInfo::WebContentsGetter& wc_getter,
    const content::URLDataSource::GotDataCallback& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (!top_sites_) {
    callback.Run(nullptr);
    return;
  }

  top_sites_->GetMostVisitedURLs(
      base::Bind(&ThumbnailListSource::OnMostVisitedURLsAvailable,
                 weak_ptr_factory_.GetWeakPtr(), callback),
      true);
}

std::string ThumbnailListSource::GetMimeType(const std::string& path) const {
  return "text/html";
}

scoped_refptr<base::SingleThreadTaskRunner>
ThumbnailListSource::TaskRunnerForRequestPath(const std::string& path) const {
  // TopSites can be accessed from the IO thread.
  return thumbnail_service_.get()
             ? nullptr
             : content::URLDataSource::TaskRunnerForRequestPath(path);
}

bool ThumbnailListSource::ShouldServiceRequest(
    const GURL& url,
    content::ResourceContext* resource_context,
    int render_process_id) const {
  if (url.SchemeIs(chrome::kChromeSearchScheme)) {
    return InstantIOContext::ShouldServiceRequest(url, resource_context,
                                                  render_process_id);
  }
  return URLDataSource::ShouldServiceRequest(url, resource_context,
                                             render_process_id);
}

bool ThumbnailListSource::ShouldReplaceExistingSource() const {
  return false;
}

void ThumbnailListSource::OnMostVisitedURLsAvailable(
    const content::URLDataSource::GotDataCallback& callback,
    const history::MostVisitedURLList& mvurl_list) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  const size_t num_mv = mvurl_list.size();
  size_t num_mv_with_thumb = 0;

  // Encode all available thumbnails and store into |base64_encoded_pngs|.
  std::vector<std::string> base64_encoded_pngs(num_mv);
  for (size_t i = 0; i < num_mv; ++i) {
    scoped_refptr<base::RefCountedMemory> data;
    if (thumbnail_service_->GetPageThumbnail(mvurl_list[i].url, false, &data)) {
      base::Base64Encode(
          base::StringPiece(data->front_as<char>(), data->size()),
          &base64_encoded_pngs[i]);
      ++num_mv_with_thumb;
    }
  }

  // Render HTML to embed URLs and thumbnails.
  std::vector<std::string> out;
  out.push_back(kHtmlHeader);
  out.push_back(kHtmlBody);
  if (num_mv_with_thumb > 0) {
    out.push_back("<h2>TopSites URLs with Thumbnails</h2>\n");
    RenderMostVisitedURLList(mvurl_list, base64_encoded_pngs, true, &out);
  }
  if (num_mv_with_thumb < num_mv) {
    out.push_back("<h2>TopSites URLs without Thumbnails</h2>\n");
    RenderMostVisitedURLList(mvurl_list, base64_encoded_pngs, false, &out);
  }
  out.push_back(kHtmlFooter);

  std::string out_html = base::StrCat(out);
  callback.Run(base::RefCountedString::TakeString(&out_html));
}
