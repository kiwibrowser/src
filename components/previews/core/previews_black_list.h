// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PREVIEWS_CORE_PREVIEWS_BLACK_LIST_H_
#define COMPONENTS_PREVIEWS_CORE_PREVIEWS_BLACK_LIST_H_

#include <stdint.h>

#include <memory>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/containers/queue.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/optional.h"
#include "base/threading/thread_checker.h"
#include "base/time/time.h"
#include "components/previews/core/previews_black_list_delegate.h"
#include "components/previews/core/previews_experiments.h"
#include "components/previews/core/previews_opt_out_store.h"

class GURL;

namespace base {
class Clock;
}

namespace previews {
class PreviewsBlackListItem;

enum class PreviewsEligibilityReason {
  // The preview navigation was allowed.
  ALLOWED = 0,
  // The black list was not initialized.
  BLACKLIST_UNAVAILABLE = 1,
  // The black list has not loaded from disk yet.
  BLACKLIST_DATA_NOT_LOADED = 2,
  // The user has opted out of a preview recently.
  USER_RECENTLY_OPTED_OUT = 3,
  // The user has opted out of previews often, and is no longer shown previews
  // on any host.
  USER_BLACKLISTED = 4,
  // The user has opted out of previews on a specific host often, and was not
  // not shown a previews on that host.
  HOST_BLACKLISTED = 5,
  // The network quality estimate is not available.
  NETWORK_QUALITY_UNAVAILABLE = 6,
  // The network was fast enough to not warrant previews.
  NETWORK_NOT_SLOW = 7,
  // If the page was reloaded, the user should not be shown a stale preview.
  RELOAD_DISALLOWED = 8,
  // The host is explicitly blacklisted by the server, so the user was not shown
  // a preview.
  HOST_BLACKLISTED_BY_SERVER = 9,
  // The host is not whitelisted by the server for a preview decision that uses
  // server optimization hints.
  HOST_NOT_WHITELISTED_BY_SERVER = 10,
  // The preview is allowed but without an expected check of server optimization
  // hints because they are not enabled (features::kOptimizationHints).
  ALLOWED_WITHOUT_OPTIMIZATION_HINTS = 11,
  // The preview type chosen as the committed preview.
  COMMITTED = 12,
  // Previews blocked by a Cache-Control:no-transform directive.
  CACHE_CONTROL_NO_TRANSFORM = 13,
  LAST,
};

// Manages the state of black listed domains for the previews experiment. Loads
// the stored black list from |opt_out_store| and manages an in memory black
// list on the IO thread. Updates to the black list are stored in memory and
// pushed to the store. Asynchronous modifications are stored in a queue and
// executed in order. Reading from the black list is always synchronous, and if
// the black list is not currently loaded (e.g., at startup, after clearing
// browsing history), domains are reported as black listed. The list stores no
// more than previews::params::MaxInMemoryHostsInBlackList hosts in-memory,
// which defaults to 100.
class PreviewsBlackList {
 public:
  // |opt_out_store| is the backing store to retrieve and store black list
  // information, and can be null. When |opt_out_store| is null, the in-memory
  // map will be immediately loaded to empty. If |opt_out_store| is non-null,
  // it will be used to load the in-memory map asynchronously.
  // |blacklist_delegate| is a single object listening for blacklist events, and
  // it is guaranteed to overlive the life time of |this|.
  PreviewsBlackList(std::unique_ptr<PreviewsOptOutStore> opt_out_store,
                    base::Clock* clock,
                    PreviewsBlacklistDelegate* blacklist_delegate);
  virtual ~PreviewsBlackList();

  // Asynchronously adds a new navigation to to the in-memory black list and
  // backing store. |opt_out| is whether the user opted out of the preview or
  // navigated away from the page without opting out. |type| is only passed to
  // the backing store. If the in memory map has reached the max number of hosts
  // allowed, and |url| is a new host, a host will be evicted based on recency
  // of the hosts most recent opt out. It returns the time used for recording
  // the moment when the navigation is added for logging.
  base::Time AddPreviewNavigation(const GURL& url,
                                  bool opt_out,
                                  PreviewsType type);

  // Synchronously determines if |host_name| should be allowed to show previews.
  // Returns the reason the blacklist disallowed the preview, or
  // PreviewsEligibilityReason::ALLOWED if the preview is allowed. Record
  // checked reasons in |passed_reasons|. Virtualized in testing.
  virtual PreviewsEligibilityReason IsLoadedAndAllowed(
      const GURL& url,
      PreviewsType type,
      std::vector<PreviewsEligibilityReason>* passed_reasons) const;

  // Asynchronously deletes all entries in the in-memory black list. Informs
  // the backing store to delete entries between |begin_time| and |end_time|,
  // and reloads entries into memory from the backing store. If the embedder
  // passed in a null store, resets all history in the in-memory black list.
  void ClearBlackList(base::Time begin_time, base::Time end_time);

  // Returns a new PreviewsBlackListItem representing |host_name|. Adds the new
  // item to |black_list_item_map|.
  static PreviewsBlackListItem* GetOrCreateBlackListItemForMap(
      BlackListItemMap* black_list_item_map,
      const std::string& host_name);

  // Returns a new PreviewsBlackListItem for the host indifferent black list
  // that does not consider host name when determining eligibility.
  static std::unique_ptr<PreviewsBlackListItem>
  CreateHostIndifferentBlackListItem();

 private:
  // Synchronous version of AddPreviewNavigation method. |time| is the time
  // stamp of when the navigation was determined to be an opt-out or non-opt
  // out.
  void AddPreviewNavigationSync(const GURL& host_name,
                                bool opt_out,
                                PreviewsType type,
                                base::Time time);

  // Synchronous version of ClearBlackList method.
  void ClearBlackListSync(base::Time begin_time, base::Time end_time);

  // Callback passed to the backing store when loading black list information.
  // Moves the |black_list_item_map| and |host_indifferent_black_list_item| into
  // the in-memory black list and runs any outstanding tasks.
  void LoadBlackListDone(
      std::unique_ptr<BlackListItemMap> black_list_item_map,
      std::unique_ptr<PreviewsBlackListItem> host_indifferent_black_list_item);

  // Called while waiting for the black list to be loaded from the backing
  // store.
  // Enqueues a task to run when when loading black list information has
  // completed. Maintains the order that tasks were called in.
  void QueuePendingTask(base::Closure callback);

  // Map maintaining the in-memory black list.
  std::unique_ptr<BlackListItemMap> black_list_item_map_;

  // Host indifferent opt out history.
  std::unique_ptr<PreviewsBlackListItem> host_indifferent_black_list_item_;

  // Whether the black list is done being loaded from the backing store.
  bool loaded_;

  // The time of the last opt out for this session.
  base::Optional<base::Time> last_opt_out_time_;

  // The backing store of the black list information.
  std::unique_ptr<PreviewsOptOutStore> opt_out_store_;

  // Callbacks to be run after loading information from the backing store has
  // completed.
  base::queue<base::Closure> pending_callbacks_;

  base::Clock* clock_;

  // The delegate listening to this blacklist. |blacklist_delegate_| lifetime is
  // guaranteed to overlive |this|.
  PreviewsBlacklistDelegate* blacklist_delegate_;

  base::ThreadChecker thread_checker_;

  base::WeakPtrFactory<PreviewsBlackList> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(PreviewsBlackList);
};

}  // namespace previews

#endif  // COMPONENTS_PREVIEWS_CORE_PREVIEWS_BLACK_LIST_H_
