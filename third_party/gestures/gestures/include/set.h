// Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GESTURES_SET_H__
#define GESTURES_SET_H__

#include <algorithm>

#include "gestures/include/gestures.h"
#include "gestures/include/logging.h"
#include "gestures/include/vector.h"

// This is a set class that doesn't call out to malloc/free. Many of the
// names were chosen to mirror std::set.

// A template parameter to this class is kMaxSize, which is the max number
// of elements that such a set can hold. Internally, it contains an array
// of Elt objects.

// Differences from std::set:
// - Many methods are unimplemented
// - insert()/erase() invalidate existing iterators
// - Currently, the Elt type should be a POD type or aggregate of PODs,
//   since ctors/dtors aren't called propertly on Elt objects.

namespace gestures {

template<typename Elt, size_t kMaxSize>
class set {
 public:
  typedef Elt* iterator;
  typedef const Elt* const_iterator;

  set() {}
  set(const set<Elt, kMaxSize>& that) {
    *this = that;
  }
  template<size_t kThatSize>
  set(const set<Elt, kThatSize>& that) {
    *this = that;
  }

  size_t size() const { return vector_.size(); }
  bool empty() const { return vector_.empty(); }

  const_iterator begin() const { return vector_.begin(); }
  const_iterator end() const { return vector_.end(); }
  const_iterator find(const Elt& value) const { return vector_.find(value); }

  // Non-const versions:
  iterator begin() { return vector_.begin(); }
  iterator end() { return vector_.end(); }
  iterator find(const Elt& value) { return vector_.find(value); }

  // Unlike std::set, invalidates iterators.
  std::pair<iterator, bool> insert(const Elt& value) {
    iterator it = find(value);
    if (it != end())
      return std::make_pair(it, false);

    it = vector_.insert(vector_.end(), value);
    return std::make_pair(it, it != vector_.end());
  }

  // Returns number of elements removed (0 or 1).
  // Unlike std::set, invalidates iterators.
  size_t erase(const Elt& value) {
    iterator it = vector_.find(value);
    if (it == vector_.end())
      return 0;
    vector_.erase(it);
    return 1;
  }
  void erase(iterator it) { vector_.erase(it); }
  void clear() { vector_.clear(); }

  template<size_t kThatSize>
  set<Elt, kMaxSize>& operator=(const set<Elt, kThatSize>& that) {
    vector_.clear();
    vector_.insert(vector_.begin(), that.begin(), that.end());
    return *this;
  }

 protected:
  vector<Elt, kMaxSize> vector_;
};

template<typename Elt, size_t kLeftMaxSize, size_t kRightMaxSize>
inline bool operator==(const set<Elt, kLeftMaxSize>& left,
                       const set<Elt, kRightMaxSize>& right) {
  if (left.size() != right.size())
    return false;
  for (typename set<Elt, kLeftMaxSize>::const_iterator left_it = left.begin(),
           left_end = left.end(); left_it != left_end; ++left_it)
    if (right.find(*left_it) == right.end())
      return false;
  return true;
}
template<typename Elt, size_t kLeftMaxSize, size_t kRightMaxSize>
inline bool operator!=(const set<Elt, kLeftMaxSize>& left,
                       const set<Elt, kRightMaxSize>& right) {
  return !(left == right);
}

template<typename Set, typename Elt>
inline bool SetContainsValue(const Set& the_set,
                             const Elt& elt) {
  return the_set.find(elt) != the_set.end();
}

// Removes all elements from |reduced| which are not in |required|
template<typename ReducedSet, typename RequiredSet>
inline void SetRemoveMissing(ReducedSet* reduced, const RequiredSet& required) {
  typename ReducedSet::iterator it = reduced->begin();
  while (it != reduced->end()) {
    if (SetContainsValue(required, *it)) {
      ++it;
      continue;
    }
    if (it == reduced->begin()) {
      reduced->erase(it);
      it = reduced->begin();
    } else {
      typename ReducedSet::iterator it_copy = it;
      --it;
      reduced->erase(it_copy);
      ++it;
    }
  }
}

// Removes any ids from the set that are not finger ids in hs.
template<size_t kSetSize>
void RemoveMissingIdsFromSet(set<short, kSetSize>* the_set,
                             const HardwareState& hs) {
  short old_ids[the_set->size()];
  size_t old_ids_len = 0;
  for (typename set<short, kSetSize>::const_iterator it = the_set->begin();
       it != the_set->end(); ++it)
    if (!hs.GetFingerState(*it))
      old_ids[old_ids_len++] = *it;
  for (size_t i = 0; i < old_ids_len; i++)
    the_set->erase(old_ids[i]);
}

// returns Difference = left - right.
template<typename LeftSet, typename RightSet>
LeftSet SetSubtract(const LeftSet& left, const RightSet& right) {
  if (left.empty() || right.empty())
    return left;
  LeftSet ret;
  for (typename LeftSet::const_iterator it = left.begin(), e = left.end();
       it != e; ++it) {
    if (!SetContainsValue(right, *it))
      ret.insert(*it);
  }
  return ret;
}

}  // namespace gestures

#endif  // GESTURES_SET_H__
