// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/browser/devtools_network_transaction_factory.h"

#include "services/network/throttling/throttling_network_transaction_factory.h"

namespace content {

std::unique_ptr<net::HttpTransactionFactory>
CreateDevToolsNetworkTransactionFactory(net::HttpNetworkSession* session) {
  return std::make_unique<network::ThrottlingNetworkTransactionFactory>(
      session);
}

}  // namespace content
