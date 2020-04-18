// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_CLIENT_POLICY_CONTROLLER_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_CLIENT_POLICY_CONTROLLER_H_

#include <stdint.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/time/time.h"
#include "components/offline_pages/core/offline_page_client_policy.h"

namespace offline_pages {

// This is the class which is a singleton for offline page model
// to get client policies based on namespaces.
class ClientPolicyController {
 public:
  ClientPolicyController();
  ~ClientPolicyController();

  // Generates a client policy from the input values.
  static const OfflinePageClientPolicy MakePolicy(
      const std::string& name_space,
      LifetimePolicy::LifetimeType lifetime_type,
      const base::TimeDelta& expiration_period,
      size_t page_limit,
      size_t pages_allowed_per_url);

  // Get the client policy for |name_space|.
  const OfflinePageClientPolicy& GetPolicy(const std::string& name_space) const;

  // Returns a list of all known namespaces.
  std::vector<std::string> GetAllNamespaces() const;

  // Returns whether pages for |name_space| should be removed on cache reset.
  bool IsRemovedOnCacheReset(const std::string& name_space) const;
  const std::vector<std::string>& GetNamespacesRemovedOnCacheReset() const;

  // Returns whether pages for |name_space| are shown in Download UI.
  bool IsSupportedByDownload(const std::string& name_space) const;
  const std::vector<std::string>& GetNamespacesSupportedByDownload() const;

  // Returns whether pages for |name_space| are explicitly offlined due to user
  // action.
  bool IsUserRequestedDownload(const std::string& name_space) const;
  const std::vector<std::string>& GetNamespacesForUserRequestedDownload() const;

  // Returns whether pages for |name_space| are shown in recent tabs UI,
  // currently only available on NTP.
  bool IsShownAsRecentlyVisitedSite(const std::string& name_space) const;
  const std::vector<std::string>& GetNamespacesShownAsRecentlyVisitedSite()
      const;

  // Returns whether pages for |name_space| should never be shown outside the
  // tab they were generated in.
  bool IsRestrictedToOriginalTab(const std::string& name_space) const;
  const std::vector<std::string>& GetNamespacesRestrictedToOriginalTab() const;

  bool IsDisabledWhenPrefetchDisabled(const std::string& name_space) const;
  const std::vector<std::string>& GetNamespacesDisabledWhenPrefetchDisabled()
      const;

  // Returns whether pages for |name_space| originate from suggested URLs and
  // are downloaded on behalf of user.
  bool IsSuggested(const std::string& name_space) const;

  // Returns whether we should allow pages for |name_space| to trigger
  // downloads.
  bool ShouldAllowDownloads(const std::string& name_space) const;

  void AddPolicyForTest(const std::string& name_space,
                        const OfflinePageClientPolicyBuilder& builder);

 private:
  // The map from name_space to a client policy. Will be generated
  // as pre-defined values for now.
  std::map<std::string, OfflinePageClientPolicy> policies_;

  // Memoizing results.
  mutable std::unique_ptr<std::vector<std::string>>
      cache_reset_namespace_cache_;
  mutable std::unique_ptr<std::vector<std::string>> download_namespace_cache_;
  mutable std::unique_ptr<std::vector<std::string>>
      user_requested_download_namespace_cache_;
  mutable std::unique_ptr<std::vector<std::string>> recent_tab_namespace_cache_;
  mutable std::unique_ptr<std::vector<std::string>> show_in_original_tab_cache_;
  mutable std::unique_ptr<std::vector<std::string>>
      disabled_when_prefetch_disabled_cache_;

  DISALLOW_COPY_AND_ASSIGN(ClientPolicyController);
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_CLIENT_POLICY_CONTROLLER_H_
