// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "headless/lib/browser/headless_network_transaction_factory.h"

#include "base/callback_helpers.h"
#include "base/logging.h"
#include "headless/lib/browser/headless_browser_context_impl.h"
#include "net/http/http_cache_writers.h"
#include "net/http/http_transaction.h"

namespace headless {

class HeadlessCacheBackendFactory : public net::HttpCache::BackendFactory {
 public:
  HeadlessCacheBackendFactory() {}
  ~HeadlessCacheBackendFactory() override {}

  int CreateBackend(net::NetLog* net_log,
                    std::unique_ptr<disk_cache::Backend>* backend,
                    const net::CompletionCallback& callback) override {
    return net::OK;
  }
};

class HeadlessHttpCache : public net::HttpCache {
 public:
  HeadlessHttpCache(net::HttpNetworkSession* session,
                    HeadlessBrowserContextImpl* headless_browser_context)
      : net::HttpCache(session,
                       std::make_unique<HeadlessCacheBackendFactory>(),
                       true /* is_main_cache */),
        headless_browser_context_(headless_browser_context) {}

  ~HeadlessHttpCache() override {}

  void WriteMetadata(const GURL& url,
                     net::RequestPriority priority,
                     base::Time expected_response_time,
                     net::IOBuffer* buf,
                     int buf_len) override {
    headless_browser_context_->NotifyMetadataForResource(url, buf, buf_len);
  }

 private:
  HeadlessBrowserContextImpl* headless_browser_context_;  // NOT OWNED
};

HeadlessNetworkTransactionFactory::HeadlessNetworkTransactionFactory(
    net::HttpNetworkSession* session,
    HeadlessBrowserContextImpl* headless_browser_context)
    : session_(session),
      http_cache_(new HeadlessHttpCache(session, headless_browser_context)) {}

HeadlessNetworkTransactionFactory::~HeadlessNetworkTransactionFactory() {}

// static
std::unique_ptr<net::HttpTransactionFactory>
HeadlessNetworkTransactionFactory::Create(
    HeadlessBrowserContextImpl* headless_browser_context,
    net::HttpNetworkSession* session) {
  return std::make_unique<HeadlessNetworkTransactionFactory>(
      session, headless_browser_context);
}

int HeadlessNetworkTransactionFactory::CreateTransaction(
    net::RequestPriority priority,
    std::unique_ptr<net::HttpTransaction>* trans) {
  return http_cache_->CreateTransaction(priority, trans);
}

net::HttpCache* HeadlessNetworkTransactionFactory::GetCache() {
  return http_cache_.get();
}

net::HttpNetworkSession* HeadlessNetworkTransactionFactory::GetSession() {
  return session_;
}

}  // namespace headless
