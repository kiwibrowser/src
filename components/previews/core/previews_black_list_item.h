// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PREVIEWS_CORE_PREVIEWS_BLACK_LIST_ITEM_H_
#define COMPONENTS_PREVIEWS_CORE_PREVIEWS_BLACK_LIST_ITEM_H_

#include <stdint.h>

#include <map>
#include <memory>
#include <queue>
#include <string>

#include "base/callback.h"
#include "base/macros.h"
#include "base/optional.h"
#include "base/time/time.h"

namespace previews {

// Stores the recent black list history for a single host. Stores
// |stored_history_length| of the most recent previews navigations. To determine
// previews eligiblity fewer than |opt_out_black_list_threshold| out of the past
// |stored_history_length| navigations must be opt outs. |black_list_duration|
// is the amount of time that elapses until the host is no longer on the black
// list.
class PreviewsBlackListItem {
 public:
  PreviewsBlackListItem(size_t stored_history_length,
                        int opt_out_black_list_threshold,
                        base::TimeDelta black_list_duration);

  ~PreviewsBlackListItem();

  // Adds a new navigation at the specified |entry_time|.
  void AddPreviewNavigation(bool opt_out, base::Time entry_time);

  // Whether the host name corresponding to |this| should be disallowed from
  // showing previews.
  bool IsBlackListed(base::Time now) const;

  base::Optional<base::Time> most_recent_opt_out_time() const {
    return most_recent_opt_out_time_;
  }

  size_t OptOutRecordsSizeForTesting() const;

 private:
  // A previews navigation to this host is represented by time and whether the
  // navigation was an opt out.
  class OptOutRecord {
   public:
    OptOutRecord(base::Time entry_time, bool opt_out);
    ~OptOutRecord();
    OptOutRecord(OptOutRecord&&) noexcept;
    OptOutRecord& operator=(OptOutRecord&&) noexcept;

    // Used to determine eviction priority.
    bool operator<(const OptOutRecord& other) const;

    // The time that the opt out state was determined.
    base::Time entry_time() const { return entry_time_; }

    // Whether the user opted out of the preview.
    bool opt_out() const { return opt_out_; }

   private:
    // The time that the opt out state was determined.
    base::Time entry_time_;
    // Whether the user opted out of the preview.
    bool opt_out_;

    DISALLOW_COPY_AND_ASSIGN(OptOutRecord);
  };

  // The number of entries to store to determine preview eligibility.
  const size_t max_stored_history_length_;
  // The number opt outs in recent history that will trigger blacklisting.
  const int opt_out_black_list_threshold_;
  // The amount of time to black list a domain after the most recent opt out.
  const base::TimeDelta max_black_list_duration_;

  // The |max_stored_history_length_| most recent previews navigation. Is
  // maintained as a priority queue that has high priority for items that should
  // be evicted (i.e., they are old).
  std::priority_queue<OptOutRecord> opt_out_records_;

  // Time of the most recent opt out.
  base::Optional<base::Time> most_recent_opt_out_time_;

  // The total number of opt outs currently in |opt_out_records_|.
  int total_opt_out_;

  DISALLOW_COPY_AND_ASSIGN(PreviewsBlackListItem);
};

}  // namespace previews

#endif  // COMPONENTS_PREVIEWS_CORE_PREVIEWS_BLACK_LIST_ITEM_H_
