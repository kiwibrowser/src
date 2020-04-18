// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_BROWSING_DATA_BROWSING_DATA_FILTER_BUILDER_IMPL_H_
#define CONTENT_BROWSER_BROWSING_DATA_BROWSING_DATA_FILTER_BUILDER_IMPL_H_

#include <set>

#include "base/macros.h"
#include "content/public/browser/browsing_data_filter_builder.h"

namespace content {

class CONTENT_EXPORT BrowsingDataFilterBuilderImpl
    : public BrowsingDataFilterBuilder {
 public:
  explicit BrowsingDataFilterBuilderImpl(Mode mode);
  ~BrowsingDataFilterBuilderImpl() override;

  // BrowsingDataFilterBuilder implementation:
  void AddOrigin(const url::Origin& origin) override;
  void AddRegisterableDomain(const std::string& registrable_domain) override;
  bool IsEmptyBlacklist() const override;
  base::RepeatingCallback<bool(const GURL&)>
      BuildGeneralFilter() const override;
  network::mojom::ClearDataFilterPtr BuildNetworkServiceFilter() const override;
  network::mojom::CookieDeletionFilterPtr BuildCookieDeletionFilter()
      const override;
  base::RepeatingCallback<bool(const std::string& site)>
      BuildPluginFilter() const override;
  Mode GetMode() const override;
  std::unique_ptr<BrowsingDataFilterBuilder> Copy() const override;
  bool operator==(const BrowsingDataFilterBuilder& other) const override;

 private:
  Mode mode_;

  std::set<url::Origin> origins_;
  std::set<std::string> domains_;

  DISALLOW_COPY_AND_ASSIGN(BrowsingDataFilterBuilderImpl);
};

}  // content

#endif  // CONTENT_BROWSER_BROWSING_DATA_BROWSING_DATA_FILTER_BUILDER_IMPL_H_
