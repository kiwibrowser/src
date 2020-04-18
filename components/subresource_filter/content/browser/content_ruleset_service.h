// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SUBRESOURCE_FILTER_CONTENT_BROWSER_CONTENT_RULESET_SERVICE_H_
#define COMPONENTS_SUBRESOURCE_FILTER_CONTENT_BROWSER_CONTENT_RULESET_SERVICE_H_

#include <memory>

#include "base/callback.h"
#include "base/files/file.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "components/subresource_filter/content/browser/verified_ruleset_dealer.h"
#include "components/subresource_filter/core/browser/ruleset_service_delegate.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"

namespace base {
class SequencedTaskRunner;
}  // namespace base

namespace subresource_filter {

class RulesetService;
struct UnindexedRulesetInfo;

// The content-layer specific implementation of RulesetServiceDelegate. Owns the
// underlying RulesetService.
//
// Its main responsibility is receiving new versions of subresource filtering
// rules from the RulesetService, and distributing them to renderer processes,
// where they will be memory-mapped as-needed by the UnverifiedRulesetDealer.
//
// The distribution pipeline looks like this:
//
//                      RulesetService
//                           |
//                           v                  Browser
//                 RulesetServiceDelegate
//                     |              |
//        - - - - - - -|- - - - - - - |- - - - - - - - - -
//                     |       |      |
//                     v              v
//          *RulesetDealer     |  *RulesetDealer
//                 |                |       |
//                 |           |    |       v
//                 v                |      SubresourceFilterAgent
//    SubresourceFilterAgent   |    v
//                                SubresourceFilterAgent
//                             |
//
//         Renderer #1         |          Renderer #n
//
// Note: UnverifiedRulesetDealer is shortened to *RulesetDealer above. There is
// also a VerifiedRulesetDealer which is used similarly on the browser side.
class ContentRulesetService : public RulesetServiceDelegate,
                              content::NotificationObserver {
 public:
  explicit ContentRulesetService(
      scoped_refptr<base::SequencedTaskRunner> blocking_task_runner);
  ~ContentRulesetService() override;

  void SetRulesetPublishedCallbackForTesting(base::Closure callback);

  // RulesetServiceDelegate:
  void PostAfterStartupTask(base::Closure task) override;
  void TryOpenAndSetRulesetFile(
      const base::FilePath& file_path,
      base::OnceCallback<void(base::File)> callback) override;

  void PublishNewRulesetVersion(base::File ruleset_data) override;

  void set_ruleset_service(std::unique_ptr<RulesetService> ruleset_service);

  // Forwards calls to the underlying ruleset_service_.
  void IndexAndStoreAndPublishRulesetIfNeeded(
      const UnindexedRulesetInfo& unindex_ruleset_info);

  VerifiedRulesetDealer::Handle* ruleset_dealer() {
    return ruleset_dealer_.get();
  }

  void SetIsAfterStartupForTesting();

 private:
  // content::NotificationObserver:
  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override;

  content::NotificationRegistrar notification_registrar_;
  base::File ruleset_data_;
  base::Closure ruleset_published_callback_;

  std::unique_ptr<RulesetService> ruleset_service_;
  std::unique_ptr<VerifiedRulesetDealer::Handle> ruleset_dealer_;

  DISALLOW_COPY_AND_ASSIGN(ContentRulesetService);
};

}  // namespace subresource_filter

#endif  // COMPONENTS_SUBRESOURCE_FILTER_CONTENT_BROWSER_CONTENT_RULESET_SERVICE_H_
