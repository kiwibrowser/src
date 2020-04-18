// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TESTING_RANGE_SET_H_
#define TESTING_RANGE_SET_H_

#include <stddef.h>

#include <set>
#include <utility>

class RangeSet {
 public:
  using Range = std::pair<size_t, size_t>;

  RangeSet();
  ~RangeSet();

  bool Contains(const Range& range) const;

  void Union(const Range& range);

  void Union(const RangeSet& range_set);

  bool IsEmpty() const { return ranges().empty(); }

  void Clear() { ranges_.clear(); }

  struct range_compare {
    bool operator()(const Range& lval, const Range& rval) const {
      return lval.first < rval.first;
    }
  };

  using RangesContainer = std::set<Range, range_compare>;
  const RangesContainer& ranges() const { return ranges_; }

 private:
  Range FixDirection(const Range& range) const;

  bool IsEmptyRange(const Range& range) const;

  RangesContainer ranges_;
};

#endif  // TESTING_RANGE_SET_H_
