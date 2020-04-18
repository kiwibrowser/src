// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_OFFLINE_PAGES_PREFETCH_PREFETCH_INSTANCE_ID_PROXY_H_
#define CHROME_BROWSER_OFFLINE_PAGES_PREFETCH_PREFETCH_INSTANCE_ID_PROXY_H_

#include <string>

#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "components/gcm_driver/instance_id/instance_id.h"
#include "components/offline_pages/core/prefetch/prefetch_gcm_app_handler.h"

namespace content {
class BrowserContext;
}

namespace offline_pages {

// A factory that can create prefetching InstanceID tokens from a
// BrowserContext, requesting the InstanceIDProfileService on demand.
class PrefetchInstanceIDProxy : public PrefetchGCMAppHandler::TokenFactory {
 public:
  PrefetchInstanceIDProxy(const std::string& app_id,
                          content::BrowserContext* context);
  ~PrefetchInstanceIDProxy() override;

  // PrefetchGCMAppHandler::TokenFactory implementation.
  void GetGCMToken(instance_id::InstanceID::GetTokenCallback callback) override;

 private:
  void GotGCMToken(instance_id::InstanceID::GetTokenCallback callback,
                   const std::string& token,
                   instance_id::InstanceID::Result result);
  std::string app_id_;
  std::string token_;

  // Unowned, the owner should make sure that this class does not outlive the
  // browser context.
  content::BrowserContext* context_;

  base::WeakPtrFactory<PrefetchInstanceIDProxy> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(PrefetchInstanceIDProxy);
};

}  // namespace offline_pages

#endif  // CHROME_BROWSER_OFFLINE_PAGES_PREFETCH_PREFETCH_INSTANCE_ID_PROXY_H_
