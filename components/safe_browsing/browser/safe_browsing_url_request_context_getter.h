// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SAFE_BROWSING_BROWSER_SAFE_BROWSING_URL_REQUEST_CONTEXT_GETTER_H_
#define COMPONENTS_SAFE_BROWSING_BROWSER_SAFE_BROWSING_URL_REQUEST_CONTEXT_GETTER_H_

#include "base/files/file_path.h"
#include "net/url_request/url_request_context_getter.h"

namespace base {
class SingleThreadTaskRunner;
}

namespace net {
class ChannelIDService;
class CookieStore;
class HttpNetworkSession;
class HttpTransactionFactory;
class URLRequestContext;
}

namespace safe_browsing {

class SafeBrowsingURLRequestContextGetter
    : public net::URLRequestContextGetter {
 public:
  explicit SafeBrowsingURLRequestContextGetter(
      scoped_refptr<net::URLRequestContextGetter> system_context_getter,
      const base::FilePath& user_data_dir);

  // Implementation for net::UrlRequestContextGetter.
  net::URLRequestContext* GetURLRequestContext() override;
  scoped_refptr<base::SingleThreadTaskRunner> GetNetworkTaskRunner()
      const override;

  // Shuts down any pending requests using the getter, and sets |shut_down_| to
  // true.
  void ServiceShuttingDown();

  // Disables QUIC. This should not be necessary anymore when
  // http://crbug.com/678653 is implemented.
  void DisableQuicOnIOThread();

 protected:
  ~SafeBrowsingURLRequestContextGetter() override;

 private:
  base::FilePath GetBaseFilename();
  base::FilePath CookieFilePath();
  base::FilePath ChannelIDFilePath();

  bool shut_down_;
  base::FilePath user_data_dir_;

  scoped_refptr<net::URLRequestContextGetter> system_context_getter_;
  scoped_refptr<base::SingleThreadTaskRunner> network_task_runner_;
  std::unique_ptr<net::URLRequestContext> safe_browsing_request_context_;
  std::unique_ptr<net::CookieStore> safe_browsing_cookie_store_;
  std::unique_ptr<net::ChannelIDService> channel_id_service_;
  std::unique_ptr<net::HttpNetworkSession> http_network_session_;
  std::unique_ptr<net::HttpTransactionFactory> http_transaction_factory_;
};

}  // namespace safe_browsing

#endif  // COMPONENTS_SAFE_BROWSING_BROWSER_SAFE_BROWSING_URL_REQUEST_CONTEXT_GETTER_H_
