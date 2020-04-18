// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_SYNCABLE_SYNCABLE_ID_H_
#define COMPONENTS_SYNC_SYNCABLE_SYNCABLE_ID_H_

#include <iosfwd>
#include <limits>
#include <memory>
#include <sstream>
#include <string>

#include "base/containers/hash_tables.h"
#include "base/trace_event/memory_usage_estimator.h"

namespace base {
class Value;
}  // namespace base

namespace sql {
class Statement;
}  // namespace sql

namespace syncer {
namespace syncable {

class Id;
struct EntryKernel;

std::ostream& operator<<(std::ostream& out, const Id& id);

// For historical reasons, 3 concepts got everloaded into the Id:
// 1. A unique, opaque identifier for the object.
// 2. Flag specifing whether server know about this object.
// 3. Flag for root.
//
// We originally wrapped an integer for this information, but now we use a
// string. It will have one of three forms:
// 1. c<client only opaque id> for client items that have not been committed.
// 2. r for the root item.
// 3. s<server provided opaque id> for items that the server knows about.
class Id {
 public:
  inline Id() {}
  inline Id(const Id& that) { Copy(that); }
  inline Id& operator=(const Id& that) {
    Copy(that);
    return *this;
  }
  inline void Copy(const Id& that) { this->s_ = that.s_; }
  inline bool IsRoot() const { return "r" == s_; }
  inline bool ServerKnows() const {
    return !IsNull() && (s_[0] == 's' || s_ == "r");
  }

  inline bool IsNull() const { return s_.empty(); }
  inline void Clear() { s_.clear(); }
  inline int compare(const Id& that) const { return s_.compare(that.s_); }
  inline bool operator==(const Id& that) const { return s_ == that.s_; }
  inline bool operator!=(const Id& that) const { return s_ != that.s_; }
  inline bool operator<(const Id& that) const { return s_ < that.s_; }
  inline bool operator>(const Id& that) const { return s_ > that.s_; }

  const std::string& value() const { return s_; }

  // Return the next highest ID in the lexicographic ordering.  This is
  // useful for computing upper bounds on std::sets that are ordered
  // by operator<.
  Id GetLexicographicSuccessor() const;

  // Dumps the ID as a value and returns it.
  std::unique_ptr<base::Value> ToValue() const;

  // Three functions are used to work with our proto buffers.
  std::string GetServerId() const;
  static Id CreateFromServerId(const std::string& server_id);
  // This should only be used if you get back a reference to a local
  // id from the server. Returns a client only opaque id.
  static Id CreateFromClientString(const std::string& local_id);

  // This method returns an ID that will compare less than any valid ID.
  // The returned ID is not a valid ID itself.  This is useful for
  // computing lower bounds on std::sets that are ordered by operator<.
  static Id GetLeastIdForLexicographicComparison();

  // Gets root ID.
  static Id GetRoot();

 private:
  friend std::unique_ptr<EntryKernel> UnpackEntry(sql::Statement* statement,
                                                  int* total_created_entries);
  friend void BindFields(const EntryKernel& entry, sql::Statement* statement);
  friend std::ostream& operator<<(std::ostream& out, const Id& id);
  friend class SyncableIdTest;

  std::string s_;
};

inline size_t EstimateMemoryUsage(const Id& id) {
  return base::trace_event::EstimateMemoryUsage(id.value());
}

}  // namespace syncable
}  // namespace syncer

#endif  // COMPONENTS_SYNC_SYNCABLE_SYNCABLE_ID_H_
