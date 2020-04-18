// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_TEST_FAKE_SERVER_FAKE_SERVER_NETWORK_RESOURCES_H_
#define COMPONENTS_SYNC_TEST_FAKE_SERVER_FAKE_SERVER_NETWORK_RESOURCES_H_

#include <memory>

#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/single_thread_task_runner.h"
#include "components/sync/engine/net/network_resources.h"
#include "components/sync/engine/net/network_time_update_callback.h"

namespace net {
class URLRequestContextGetter;
}  // namespace net

namespace fake_server {

class FakeServer;
class HttpPostProviderFactory;

class FakeServerNetworkResources : public syncer::NetworkResources {
 public:
  explicit FakeServerNetworkResources(
      const base::WeakPtr<FakeServer>& fake_server);
  ~FakeServerNetworkResources() override;

  // NetworkResources
  std::unique_ptr<syncer::HttpPostProviderFactory> GetHttpPostProviderFactory(
      const scoped_refptr<net::URLRequestContextGetter>&
          baseline_context_getter,
      const syncer::NetworkTimeUpdateCallback& network_time_update_callback,
      syncer::CancelationSignal* cancelation_signal) override;

 private:
  base::WeakPtr<FakeServer> fake_server_;
  scoped_refptr<base::SingleThreadTaskRunner> fake_server_task_runner_;
};

}  // namespace fake_server

#endif  // COMPONENTS_SYNC_TEST_FAKE_SERVER_FAKE_SERVER_NETWORK_RESOURCES_H_
