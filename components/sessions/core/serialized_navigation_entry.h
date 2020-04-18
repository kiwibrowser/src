// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SESSIONS_CORE_SERIALIZED_NAVIGATION_ENTRY_H_
#define COMPONENTS_SESSIONS_CORE_SERIALIZED_NAVIGATION_ENTRY_H_

#include <stdint.h>

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "base/optional.h"
#include "base/strings/string16.h"
#include "base/time/time.h"
#include "components/sessions/core/sessions_export.h"
#include "ui/base/page_transition_types.h"
#include "url/gurl.h"

namespace base {
class Pickle;
class PickleIterator;
}

namespace sync_pb {
class TabNavigation;
}

namespace sessions {

class SerializedNavigationEntryTestHelper;

// SerializedNavigationEntry is a "freeze-dried" version of NavigationEntry.  It
// contains the data needed to restore a NavigationEntry during session restore
// and tab restore, and it can also be pickled and unpickled.  It is also
// convertible to a sync protocol buffer for session syncing.
//
// Default copy constructor and assignment operator welcome.
class SESSIONS_EXPORT SerializedNavigationEntry {
 public:
  enum BlockedState {
    STATE_INVALID = 0,
    STATE_ALLOWED = 1,
    STATE_BLOCKED = 2,
  };

  // These must match the proto.  They are in priority order such that if a
  // higher value is seen, it should overwrite a lower value.
  enum PasswordState {
    PASSWORD_STATE_UNKNOWN = 0,
    NO_PASSWORD_FIELD = 1,
    HAS_PASSWORD_FIELD = 2,
  };

  // Creates an invalid (index < 0) SerializedNavigationEntry.
  SerializedNavigationEntry();
  SerializedNavigationEntry(const SerializedNavigationEntry& other);
  SerializedNavigationEntry(SerializedNavigationEntry&& other) noexcept;
  ~SerializedNavigationEntry();

  SerializedNavigationEntry& operator=(const SerializedNavigationEntry& other);
  SerializedNavigationEntry& operator=(SerializedNavigationEntry&& other);

  // Construct a SerializedNavigationEntry for a particular index from a sync
  // protocol buffer.  Note that the sync protocol buffer doesn't contain all
  // SerializedNavigationEntry fields.  Also, the timestamp of the returned
  // SerializedNavigationEntry is nulled out, as we assume that the protocol
  // buffer is from a foreign session.
  static SerializedNavigationEntry FromSyncData(
      int index,
      const sync_pb::TabNavigation& sync_data);

  // Note that not all SerializedNavigationEntry fields are preserved.
  // |max_size| is the max number of bytes to write.
  void WriteToPickle(int max_size, base::Pickle* pickle) const;
  bool ReadFromPickle(base::PickleIterator* iterator);

  // Convert this navigation into its sync protocol buffer equivalent.  Note
  // that the protocol buffer doesn't contain all SerializedNavigationEntry
  // fields.
  sync_pb::TabNavigation ToSyncData() const;

  // The index in the NavigationController. This SerializedNavigationEntry is
  // valid only when the index is non-negative.
  int index() const { return index_; }
  void set_index(int index) { index_ = index; }

  // Accessors for some fields taken from NavigationEntry.
  int unique_id() const { return unique_id_; }
  const base::string16& title() const { return title_; }
  const GURL& favicon_url() const { return favicon_url_; }
  int http_status_code() const { return http_status_code_; }
  ui::PageTransition transition_type() const {
    return transition_type_;
  }
  bool has_post_data() const { return has_post_data_; }
  int64_t post_id() const { return post_id_; }
  bool is_overriding_user_agent() const { return is_overriding_user_agent_; }
  base::Time timestamp() const { return timestamp_; }

  BlockedState blocked_state() const { return blocked_state_; }
  void set_blocked_state(BlockedState blocked_state) {
    blocked_state_ = blocked_state;
  }

  PasswordState password_state() const { return password_state_; }
  void set_password_state(PasswordState password_state) {
    password_state_ = password_state;
  }

  const GURL& virtual_url() const { return virtual_url_; }
  void set_virtual_url(const GURL& virtual_url) { virtual_url_ = virtual_url; }

  const std::string& encoded_page_state() const { return encoded_page_state_; }
  void set_encoded_page_state(const std::string& encoded_page_state) {
    encoded_page_state_ = encoded_page_state;
  }

  const GURL& original_request_url() const { return original_request_url_; }
  void set_original_request_url(const GURL& original_request_url) {
    original_request_url_ = original_request_url;
  }

  const GURL& referrer_url() const { return referrer_url_; }
  void set_referrer_url(const GURL& referrer_url) {
    referrer_url_ = referrer_url;
  }

  int referrer_policy() const { return referrer_policy_; }
  void set_referrer_policy(int referrer_policy) {
    referrer_policy_ = referrer_policy;
  }

  std::set<std::string> content_pack_categories() const {
    return content_pack_categories_;
  }
  void set_content_pack_categories(
      const std::set<std::string>& content_pack_categories) {
    content_pack_categories_ = content_pack_categories;
  }
  const std::vector<GURL>& redirect_chain() const { return redirect_chain_; }

  // This class is analogous to content::ReplacedNavigationEntryData.
  // When a history entry is replaced (e.g. history.replaceState()), this
  // contains some information about the entry prior to being replaced. Even if
  // an entry is replaced multiple times, it represents data prior to the
  // *first* replace.
  struct ReplacedNavigationEntryData {
    size_t EstimateMemoryUsage() const;

    GURL first_committed_url;
    base::Time first_timestamp;
    ui::PageTransition first_transition_type;
  };
  const base::Optional<ReplacedNavigationEntryData>& replaced_entry_data()
      const {
    return replaced_entry_data_;
  }

  const std::map<std::string, std::string>& extended_info_map() const {
    return extended_info_map_;
  }

  size_t EstimateMemoryUsage() const;

 private:
  friend class ContentSerializedNavigationBuilder;
  friend class SerializedNavigationEntryTestHelper;
  friend class IOSSerializedNavigationBuilder;
  friend class IOSSerializedNavigationDriver;

  // Index in the NavigationController.
  int index_ = -1;

  // Member variables corresponding to NavigationEntry fields.
  // If you add a new field that can allocate memory, please also add
  // it to the EstimatedMemoryUsage() implementation.
  int unique_id_ = 0;
  GURL referrer_url_;
  int referrer_policy_;
  GURL virtual_url_;
  base::string16 title_;
  std::string encoded_page_state_;
  ui::PageTransition transition_type_ = ui::PAGE_TRANSITION_TYPED;
  bool has_post_data_ = false;
  int64_t post_id_ = -1;
  GURL original_request_url_;
  bool is_overriding_user_agent_ = false;
  base::Time timestamp_;
  GURL favicon_url_;
  int http_status_code_ = 0;
  bool is_restored_ = false;          // Not persisted.
  std::vector<GURL> redirect_chain_;  // Not persisted.
  base::Optional<ReplacedNavigationEntryData>
      replaced_entry_data_;  // Not persisted.

  // Additional information.
  BlockedState blocked_state_ = STATE_INVALID;
  PasswordState password_state_ = PASSWORD_STATE_UNKNOWN;
  std::set<std::string> content_pack_categories_;

  // Provides storage for arbitrary key/value pairs used by features. This
  // data is not synced.
  std::map<std::string, std::string> extended_info_map_;
};

}  // namespace sessions

#endif  // COMPONENTS_SESSIONS_CORE_SERIALIZED_NAVIGATION_ENTRY_H_
