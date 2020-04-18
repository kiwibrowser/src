// Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GESTURES_VECTOR_H__
#define GESTURES_VECTOR_H__

#include <algorithm>

#include "gestures/include/logging.h"

namespace gestures {

// This class allows range-based for loops to iterate over a subset of
// array elements, by only yielding those elements for which the
// AcceptMethod returns true.
// This class wraps around a pair of iterators, all changes to the
// yielded elements will modify the original array.
template <typename ValueType>
class FilteredRange {
 public:
  typedef bool (*AcceptMethod)(const ValueType&);

  // This class defineds a basic forward iterator that iterates over
  // an array but skips elements for which the AcceptMethod yields false.
  class RangeIterator {
   public:
    // creates a new iterator and advances to the first accepted
    // element in the array.
    RangeIterator(ValueType* i, ValueType* end, AcceptMethod accept)
        : iter_(i), end_(end), accept_(accept) {
      NextAcceptedIter();
    }

    // operator++ is required by the STL for forward iterators.
    // Instead of advancing to the next array element, this iterator
    // will advance to the next accepted array element
    ValueType* operator++ () {
      ++iter_;
      NextAcceptedIter();
      return iter_;
    }

    // operator* is required by the STL for forward iterators.
    ValueType& operator*() {
      return *iter_;
    }

    // operator-> is required by the STL for forward iterators.
    ValueType& operator->() {
      return *iter_;
    }

    // operator!= is required by the STL for forward iterators.
    bool operator!= (const RangeIterator& o) {
      return iter_ != o.iter_;
    }

    // operator== is required by the STL for forward iterators.
    bool operator== (const RangeIterator& o) {
      return iter_ == o.iter_;
    }

   private:
    void NextAcceptedIter() {
      while (!accept_(*iter_) && iter_ != end_)
        ++iter_;
    }

    ValueType* iter_;
    ValueType* end_;
    AcceptMethod accept_;
  };

  // Create a new filtered range from begin/end pointer to an array.
  FilteredRange(ValueType* begin, ValueType* end, AcceptMethod accept)
      : begin_(begin), end_(end), accept_(accept) {}

  // Returns a forward iterator to the first accepted element of the array.
  RangeIterator begin() {
    return RangeIterator(begin_, end_, accept_);
  }

  // Returns an iterator to the element after the last element of the array.
  RangeIterator end() {
    return RangeIterator(end_, end_, accept_);
  }

 private:
  ValueType* begin_;
  ValueType* end_;
  AcceptMethod accept_;
};

// The vector class mimicks a subset of the std::vector functionality
// while using a fixed size of memory to avoid calls to malloc/free.
// The limitations of this class are:
// - All insert operations might invalidate existing iterators
// - Currently, the ValueType type should be a POD type or aggregate of PODs,
//   since ctors/dtors aren't called propertly on ValueType objects.
// - Out of range element access will always return the end() iterator
//   and print an error, instead of throwing an exception.
// This class includes a non-standard extension to return a
// FilteredRange object iterating over the underlying array.
template<typename ValueType, size_t kMaxSize>
class vector {
 public:
  typedef ValueType value_type;
  typedef ValueType* iterator;
  typedef const ValueType* const_iterator;
  typedef std::reverse_iterator<iterator> reverse_iterator;
  typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
  typedef bool (*AcceptMethod)(const ValueType&);

  vector() : size_(0) {}
  vector(const vector<ValueType, kMaxSize>& that) {
    *this = that;
  }
  template<size_t kThatSize>
  vector(const vector<ValueType, kThatSize>& that) {
    *this = that;
  }

  size_t size() const { return size_; }
  bool empty() const { return size() == 0; }

  // methods for const element access

  const_iterator begin() const { return buffer_; }
  const_iterator end() const { return &buffer_[size_]; }
  const_reverse_iterator rbegin() const {
    return const_reverse_iterator(end());
  }
  const_reverse_iterator rend() const {
    return const_reverse_iterator(begin());
  }
  const_iterator find(const ValueType& value) const {
    for (size_t i = 0; i < size_; ++i)
      if (buffer_[i] == value)
        return const_iterator(&buffer_[i]);
    return end();
  }
  const ValueType& at(size_t idx) const {
    if (idx >= size()) {
      Err("vector::at: index out of range");
      idx = size() - 1;
    }
    return buffer_[idx];
  }
  const ValueType& operator[](size_t idx) const { return buffer_[idx]; }

  // methods for non-const element access:

  iterator begin() { return buffer_; }
  iterator end() { return &buffer_[size_]; }
  reverse_iterator rbegin() { return reverse_iterator(end()); }
  reverse_iterator rend() { return reverse_iterator(begin()); }
  iterator find(const ValueType& value) {
    return const_cast<iterator>(
        const_cast<const vector<ValueType, kMaxSize>*>(this)->find(value));
  }
  ValueType& at(size_t idx) {
    return const_cast<ValueType&>(
        const_cast<const vector<ValueType, kMaxSize>*>(this)->at(idx));
  }
  ValueType& operator[](size_t idx) { return buffer_[idx]; }

  // methods for inserting elements
  // note that all these methods might invalidate existing iterators

  void push_back(const ValueType& value) {
    insert(end(), value);
  }

  iterator insert(iterator position, const ValueType& value) {
    return insert(position, &value, (&value) + 1);
  }

  iterator insert(iterator position, const_iterator first,
                  const_iterator last) {
    size_t count = last - first;
    if (size_ + count > kMaxSize) {
      Err("vector::insert: out of space!");
      return end();
    }

    std::copy(rbegin(), reverse_iterator(position),
              reverse_iterator(end() + count));
    size_ = size_ + count;
    std::copy(first, last, position);
    return position;
  }

  // methods for erasing elements
  // note that all these methods might invalidate existing iterators

  iterator erase(iterator it) {
    return erase(it, it + 1);
  }

  iterator erase(iterator first, iterator last) {
    size_t count = last - first;
    std::copy(last, end(), first);
    for (iterator it = end() - count, e = end(); it != e; ++it)
      (*it).~ValueType();
    size_ = size_ - count;
    return first;
  }

  void clear() {
    erase(begin(), end());
  }

  template<size_t kThatSize>
  vector<ValueType, kMaxSize>& operator=(
      const vector<ValueType, kThatSize>& that) {
    clear();
    insert(begin(), that.begin(), that.end());
    return *this;
  }

 private:
  ValueType buffer_[kMaxSize];
  size_t size_;
};

}  // namespace gestures

#endif  // GESTURES_VECTOR_H__
