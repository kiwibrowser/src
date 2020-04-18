// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/renovations/page_renovator.h"

#include <utility>

#include "base/bind.h"
#include "base/callback.h"
#include "base/logging.h"

namespace offline_pages {

PageRenovator::PageRenovator(PageRenovationLoader* renovation_loader,
                             std::unique_ptr<ScriptInjector> script_injector,
                             const GURL& request_url)
    : renovation_loader_(renovation_loader),
      script_injector_(std::move(script_injector)) {
  DCHECK(renovation_loader);

  PrepareScript(request_url);
}

PageRenovator::~PageRenovator() {}

void PageRenovator::RunRenovations(base::Closure callback) {
  // Prepare callback and inject combined script.
  base::RepeatingCallback<void(const base::Value&)> cb = base::Bind(
      [](base::Closure callback, const base::Value&) {
        if (callback)
          callback.Run();
      },
      std::move(callback));

  script_injector_->Inject(script_, cb);
}

void PageRenovator::PrepareScript(const GURL& url) {
  std::vector<std::string> renovation_keys;

  // Pick which renovations to run.
  for (const std::unique_ptr<PageRenovation>& renovation :
       renovation_loader_->renovations()) {
    if (renovation->ShouldRun(url)) {
      renovation_keys.push_back(renovation->GetID());
    }
  }

  // Store combined renovation script. TODO(crbug.com/736933): handle
  // failed GetRenovationScript call.
  renovation_loader_->GetRenovationScript(renovation_keys, &script_);
}

}  // namespace offline_pages
