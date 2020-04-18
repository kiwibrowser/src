// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_TEST_CHROMEDRIVER_CHROME_DEVTOOLS_HTTP_CLIENT_H_
#define CHROME_TEST_CHROMEDRIVER_CHROME_DEVTOOLS_HTTP_CLIENT_H_

#include <stddef.h>

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "chrome/test/chromedriver/chrome/browser_info.h"
#include "chrome/test/chromedriver/net/sync_websocket_factory.h"

namespace base {
class TimeDelta;
}

struct DeviceMetrics;
class DevToolsClient;
class NetAddress;
class Status;
class URLRequestContextGetter;

struct WebViewInfo {
  enum Type {
    kApp,
    kBackgroundPage,
    kPage,
    kWorker,
    kWebView,
    kIFrame,
    kOther,
    kServiceWorker,
    kSharedWorker,
    kExternal,
    kBrowser,
  };

  WebViewInfo(const std::string& id,
              const std::string& debugger_url,
              const std::string& url,
              Type type);
  WebViewInfo(const WebViewInfo& other);
  ~WebViewInfo();

  bool IsFrontend() const;
  bool IsInactiveBackgroundPage() const;

  std::string id;
  std::string debugger_url;
  std::string url;
  Type type;
};

class WebViewsInfo {
 public:
  WebViewsInfo();
  explicit WebViewsInfo(const std::vector<WebViewInfo>& info);
  ~WebViewsInfo();

  const WebViewInfo& Get(int index) const;
  size_t GetSize() const;
  const WebViewInfo* GetForId(const std::string& id) const;

 private:
  std::vector<WebViewInfo> views_info;
};

class DevToolsHttpClient {
 public:
  DevToolsHttpClient(const NetAddress& address,
                     scoped_refptr<URLRequestContextGetter> context_getter,
                     const SyncWebSocketFactory& socket_factory,
                     std::unique_ptr<DeviceMetrics> device_metrics,
                     std::unique_ptr<std::set<WebViewInfo::Type>> window_types,
                     std::string page_load_strategy);
  ~DevToolsHttpClient();

  Status Init(const base::TimeDelta& timeout);

  Status GetWebViewsInfo(WebViewsInfo* views_info);

  std::unique_ptr<DevToolsClient> CreateClient(const std::string& id);

  Status CloseWebView(const std::string& id);

  Status ActivateWebView(const std::string& id);

  const BrowserInfo* browser_info();
  const DeviceMetrics* device_metrics();
  bool IsBrowserWindow(const WebViewInfo& view) const;

 private:
  Status CloseFrontends(const std::string& for_client_id);
  bool FetchUrlAndLog(const std::string& url,
                      URLRequestContextGetter* getter,
                      std::string* response);

  scoped_refptr<URLRequestContextGetter> context_getter_;
  SyncWebSocketFactory socket_factory_;
  std::string server_url_;
  std::string web_socket_url_prefix_;
  BrowserInfo browser_info_;
  std::unique_ptr<DeviceMetrics> device_metrics_;
  std::unique_ptr<std::set<WebViewInfo::Type>> window_types_;
  std::string page_load_strategy_;

  DISALLOW_COPY_AND_ASSIGN(DevToolsHttpClient);
};

Status ParseType(const std::string& data, WebViewInfo::Type* type);

namespace internal {
Status ParseWebViewsInfo(const std::string& data, WebViewsInfo* views_info);
}  // namespace internal

#endif  // CHROME_TEST_CHROMEDRIVER_CHROME_DEVTOOLS_HTTP_CLIENT_H_
