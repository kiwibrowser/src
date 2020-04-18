// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/test_blacklist_state_fetcher.h"

#include "base/stl_util.h"
#include "base/threading/thread_task_runner_handle.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace extensions {
namespace {

static const char kUrlPrefix[] = "https://prefix.com/foo";
static const char kBackupConnectUrlPrefix[] = "https://alt1-prefix.com/foo";
static const char kBackupHttpUrlPrefix[] = "https://alt2-prefix.com/foo";
static const char kBackupNetworkUrlPrefix[] = "https://alt3-prefix.com/foo";
static const char kClient[] = "unittest";
static const char kAppVer[] = "1.0";

safe_browsing::SafeBrowsingProtocolConfig CreateSafeBrowsingProtocolConfig() {
  safe_browsing::SafeBrowsingProtocolConfig config;
  config.client_name = kClient;
  config.url_prefix = kUrlPrefix;
  config.backup_connect_error_url_prefix = kBackupConnectUrlPrefix;
  config.backup_http_error_url_prefix = kBackupHttpUrlPrefix;
  config.backup_network_error_url_prefix = kBackupNetworkUrlPrefix;
  config.version = kAppVer;
  return config;
}

class DummySharedURLLoaderFactory : public network::SharedURLLoaderFactory {
 public:
  DummySharedURLLoaderFactory() {}

  // network::URLLoaderFactory implementation:
  void CreateLoaderAndStart(network::mojom::URLLoaderRequest loader,
                            int32_t routing_id,
                            int32_t request_id,
                            uint32_t options,
                            const network::ResourceRequest& request,
                            network::mojom::URLLoaderClientPtr client,
                            const net::MutableNetworkTrafficAnnotationTag&
                                traffic_annotation) override {
    // Ensure the client pipe doesn't get closed to avoid SimpleURLLoader seeing
    // a connection error.
    clients_.push_back(std::move(client));
  }

  void Clone(network::mojom::URLLoaderFactoryRequest request) override {
    NOTREACHED();
  }

  // network::SharedURLLoaderFactoryInfo implementation
  std::unique_ptr<network::SharedURLLoaderFactoryInfo> Clone() override {
    NOTREACHED();
    return nullptr;
  }

 private:
  friend class base::RefCounted<DummySharedURLLoaderFactory>;
  ~DummySharedURLLoaderFactory() override = default;

  std::vector<network::mojom::URLLoaderClientPtr> clients_;
};

}  // namespace

TestBlacklistStateFetcher::TestBlacklistStateFetcher(
    BlacklistStateFetcher* fetcher) : fetcher_(fetcher) {
  fetcher_->SetSafeBrowsingConfig(CreateSafeBrowsingProtocolConfig());

  url_loader_factory_ = base::MakeRefCounted<DummySharedURLLoaderFactory>();
  fetcher_->url_loader_factory_ = url_loader_factory_.get();
}

TestBlacklistStateFetcher::~TestBlacklistStateFetcher() {
}

void TestBlacklistStateFetcher::SetBlacklistVerdict(
    const std::string& id, ClientCRXListInfoResponse_Verdict state) {
  verdicts_[id] = state;
}

bool TestBlacklistStateFetcher::HandleFetcher(const std::string& id) {
  network::SimpleURLLoader* url_loader = nullptr;
  for (auto& it : fetcher_->requests_) {
    if (it.second.second == id) {
      url_loader = it.second.first.get();
      break;
    }
  }

  if (!url_loader)
    return false;

  ClientCRXListInfoResponse response;
  if (base::ContainsKey(verdicts_, id))
    response.set_verdict(verdicts_[id]);
  else
    response.set_verdict(ClientCRXListInfoResponse::NOT_IN_BLACKLIST);

  std::string response_str;
  response.SerializeToString(&response_str);

  fetcher_->OnURLLoaderCompleteInternal(url_loader, response_str, 200, net::OK);

  return true;
}

}  // namespace extensions
