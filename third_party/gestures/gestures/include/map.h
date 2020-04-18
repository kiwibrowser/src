// Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GESTURES_MAP_H__
#define GESTURES_MAP_H__

#include <utility>

#include "gestures/include/set.h"

// This is a map class that doesn't call out to malloc/free. Many of the
// names were chosen to mirror std::map.

// A template parameter to this class is kMaxSize, which is the max number
// of elements that such a set can hold. Internally, it contains an array
// of Key and Data objects.

// Differences from std::map:
// - Many methods are unimplemented
// - insert()/erase() invalidate existing iterators
// - Currently, the Key/Data type should be a POD type or aggregate of PODs,
//   since ctors/dtors aren't called propertly on Elt objects.

namespace gestures {

template<typename Key, typename Data, size_t kMaxSize>
class map {
  template<typename KeyE, typename DataE, size_t kLeftMaxSize,
           size_t kRightMaxSize>
  friend bool operator==(const map<KeyE, DataE, kLeftMaxSize>& left,
                         const map<KeyE, DataE, kRightMaxSize>& right);
  template<typename KeyT, typename DataT, size_t kThatSize>
  friend class map;

  typedef std::pair<Key, Data> SetElt;
  typedef typename set<SetElt, kMaxSize>::iterator SetIter;
  typedef typename set<SetElt, kMaxSize>::const_iterator SetConstIter;
 public:
  typedef std::pair<const Key, Data> value_type;
  typedef value_type* iterator;
  typedef const value_type* const_iterator;

  map() {}
  map(const map<Key, Data, kMaxSize>& that) { *this = that; }

  const_iterator begin() const {
    return reinterpret_cast<const_iterator>(set_.begin());
  }
  const_iterator end() const {
    return reinterpret_cast<const_iterator>(set_.end());
  }
  const_iterator find(const Key& key) const {
    SetConstIter it = set_.begin();
    for (SetConstIter e = set_.end(); it != e; ++it)
      if ((*it).first == key)
        break;
    return reinterpret_cast<const_iterator>(it);
  }
  size_t size() const { return set_.size(); }
  bool empty() const { return set_.empty(); }
  // Non-const versions:
  iterator begin() {
    return const_cast<iterator>(
        const_cast<const map<Key, Data, kMaxSize>*>(this)->begin());
  }
  iterator end() {
    return const_cast<iterator>(
        const_cast<const map<Key, Data, kMaxSize>*>(this)->end());
  }
  iterator find(const Key& value) {
    return const_cast<iterator>(
        const_cast<const map<Key, Data, kMaxSize>*>(this)->find(value));
  }

  // Unlike std::map, invalidates iterators.
  std::pair<iterator, bool> insert(const value_type& value) {
    iterator it = find(value.first);
    if (it != end()) {
      (*it).second = value.second;
      return std::make_pair(it, false);
    }
    std::pair<SetIter, bool> rc = set_.insert(value);
    std::pair<iterator, bool> ret(
        reinterpret_cast<iterator>(rc.first), rc.second);
    return ret;
  }

  // Returns number of elements removed (0 or 1).
  // Unlike std::set, invalidates iterators.
  size_t erase(const Key& key) {
    iterator it = find(key);
    if (it == end())
      return 0;
    erase(it);
    return 1;
  }
  void erase(iterator it) {
    set_.erase(reinterpret_cast<SetIter>(it));
  }
  void clear() { set_.clear(); }

  template<size_t kThatSize>
  map<Key, Data, kMaxSize>& operator=(const map<Key, Data, kThatSize>& that) {
    set_ = that.set_;
    return *this;
  }

  Data& operator[](const Key& key) {
    iterator it = find(key);
    if (it == end()) {
      if (set_.size() == kMaxSize) {
        Err("map::operator[]: out of space!");
        return (*--it).second;
      }
      SetElt vt = std::make_pair(key, Data());
      std::pair<SetIter, bool> pr;
      pr = set_.insert(vt);
      return (*(pr.first)).second;
    }
    return (*it).second;
  }

  const Data& operator[](const Key& key) const {
    return (*const_cast<map<Key, Data, kMaxSize>*>(this))[key];
  }

 private:
  set<SetElt, kMaxSize> set_;
};

template<typename Key, typename Data, size_t kLeftMaxSize, size_t kRightMaxSize>
inline bool operator==(const map<Key, Data, kLeftMaxSize>& left,
                       const map<Key, Data, kRightMaxSize>& right) {
  return left.set_ == right.set_;
}
template<typename Key, typename Data, size_t kLeftMaxSize, size_t kRightMaxSize>
inline bool operator!=(const map<Key, Data, kLeftMaxSize>& left,
                       const map<Key, Data, kRightMaxSize>& right) {
  return !(left == right);
}

template<typename Map, typename Key>
bool MapContainsKey(const Map& the_map, const Key& the_key) {
  return the_map.find(the_key) != the_map.end();
}

// Removes any ids from the map that are not finger ids in hs.
template<typename Data, size_t kSetSize>
void RemoveMissingIdsFromMap(map<short, Data, kSetSize>* the_map,
                             const HardwareState& hs) {
  map<short, Data, kSetSize> removed;
  RemoveMissingIdsFromMap(the_map, hs, &removed);
}

// Removes any ids from the map that are not finger ids in hs.
// This implementation supports returning removed elements for
// further processing.
template<typename Data, size_t kSetSize>
void RemoveMissingIdsFromMap(map<short, Data, kSetSize>* the_map,
                             const HardwareState& hs,
                             map<short, Data, kSetSize>* removed) {
  removed->clear();
  for (typename map<short, Data, kSetSize>::const_iterator it =
      the_map->begin(); it != the_map->end(); ++it)
    if (!hs.GetFingerState((*it).first))
      (*removed)[it->first] = it->second;
  for (typename map<short, Data, kSetSize>::const_iterator it =
      removed->begin(); it != removed->end(); ++it)
    the_map->erase(it->first);
}

// Erases the element in the map given by the iterator, returns an iterator
// for the element after the erased one. Useful for erasing elements in a loop.
template<typename Map, typename Iterator>
Iterator MapEraseIterator(Map* the_map, Iterator it) {
  if (it == the_map->begin()) {
    the_map->erase(it);
    return the_map->begin();
  } else {
    Iterator ret = it;
    --ret;
    the_map->erase(it);
    return ++ret;
  }
}

}  // namespace gestures

#endif  // GESTURES_MAP_H__
