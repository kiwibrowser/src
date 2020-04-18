// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef HEADLESS_LIB_BROWSER_HEADLESS_NETWORK_TRANSACTION_FACTORY_H_
#define HEADLESS_LIB_BROWSER_HEADLESS_NETWORK_TRANSACTION_FACTORY_H_

#include "base/macros.h"
#include "net/http/http_cache.h"
#include "net/http/http_transaction_factory.h"

namespace headless {

class HeadlessBrowserContextImpl;

// This class exists purely to let headless capture resource metadata.
// In the cases where this used, the headless embedder will have its own
// protocol handler.
class HeadlessNetworkTransactionFactory : public net::HttpTransactionFactory {
 public:
  HeadlessNetworkTransactionFactory(
      net::HttpNetworkSession* session,
      HeadlessBrowserContextImpl* headless_browser_context);

  ~HeadlessNetworkTransactionFactory() override;

  static std::unique_ptr<net::HttpTransactionFactory> Create(
      HeadlessBrowserContextImpl* headless_browser_context,
      net::HttpNetworkSession* session);

  // Creates a HttpTransaction object. On success, saves the new
  // transaction to |*trans| and returns OK.
  int CreateTransaction(net::RequestPriority priority,
                        std::unique_ptr<net::HttpTransaction>* trans) override;

  // Returns the associated cache if any (may be NULL).
  net::HttpCache* GetCache() override;

  // Returns the associated HttpNetworkSession used by new transactions.
  net::HttpNetworkSession* GetSession() override;

 private:
  net::HttpNetworkSession* const session_;  // NOT OWNED

  std::unique_ptr<net::HttpCache> http_cache_;
};

}  // namespace headless

#endif  // HEADLESS_LIB_BROWSER_HEADLESS_NETWORK_TRANSACTION_FACTORY_H_
