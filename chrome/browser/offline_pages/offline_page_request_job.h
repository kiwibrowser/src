// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_OFFLINE_PAGES_OFFLINE_PAGE_REQUEST_JOB_H_
#define CHROME_BROWSER_OFFLINE_PAGES_OFFLINE_PAGE_REQUEST_JOB_H_

#include <memory>
#include <vector>

#include "base/memory/scoped_refptr.h"
#include "chrome/browser/offline_pages/offline_page_request_handler.h"
#include "net/url_request/url_request_job.h"

namespace previews {
class PreviewsDecider;
}

namespace offline_pages {

class OfflinePageRequestHandler;

// A request job that serves offline contents without network service enabled.
class OfflinePageRequestJob : public net::URLRequestJob,
                              public OfflinePageRequestHandler::Delegate {
 public:
  // Creates and returns a job to serve the offline page. Nullptr is returned if
  // offline page cannot or should not be served. Embedder must gaurantee that
  // |previews_decider| outlives the returned instance.
  static OfflinePageRequestJob* Create(
      net::URLRequest* request,
      net::NetworkDelegate* network_delegate,
      previews::PreviewsDecider* previews_decider);

  ~OfflinePageRequestJob() override;

  void SetWebContentsGetterForTesting(
      OfflinePageRequestHandler::Delegate::WebContentsGetter
          web_contents_getter);
  void SetTabIdGetterForTesting(
      OfflinePageRequestHandler::Delegate::TabIdGetter tab_id_getter);

 private:
  OfflinePageRequestJob(net::URLRequest* request,
                        net::NetworkDelegate* network_delegate,
                        previews::PreviewsDecider* previews_decider);

  // net::URLRequestJob overrides:
  void Start() override;
  void Kill() override;
  int ReadRawData(net::IOBuffer* dest, int dest_size) override;
  void GetResponseInfo(net::HttpResponseInfo* info) override;
  void GetLoadTimingInfo(net::LoadTimingInfo* load_timing_info) const override;
  bool CopyFragmentOnRedirect(const GURL& location) const override;
  bool GetMimeType(std::string* mime_type) const override;
  void SetExtraRequestHeaders(const net::HttpRequestHeaders& headers) override;

  // OfflinePageRequestHandler::Delegate overrides:
  void FallbackToDefault() override;
  void NotifyStartError(int error) override;
  void NotifyHeadersComplete(int64_t file_size) override;
  void NotifyReadRawDataComplete(int bytes_read) override;
  void SetOfflinePageNavigationUIData(bool is_offline_page) override;
  bool ShouldAllowPreview() const override;
  int GetPageTransition() const override;
  OfflinePageRequestHandler::Delegate::WebContentsGetter GetWebContentsGetter()
      const override;
  OfflinePageRequestHandler::Delegate::TabIdGetter GetTabIdGetter()
      const override;

  // Used to determine if an URLRequest is eligible for offline previews.
  previews::PreviewsDecider* previews_decider_;

  std::unique_ptr<OfflinePageRequestHandler> request_handler_;

  OfflinePageRequestHandler::Delegate::WebContentsGetter web_contents_getter_;
  OfflinePageRequestHandler::Delegate::TabIdGetter tab_id_getter_;

  DISALLOW_COPY_AND_ASSIGN(OfflinePageRequestJob);
};

}  // namespace offline_pages

#endif  // CHROME_BROWSER_OFFLINE_PAGES_OFFLINE_PAGE_REQUEST_JOB_H_
