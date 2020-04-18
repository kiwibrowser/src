// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_PUBLIC_CPP_BINDINGS_MAP_H_
#define MOJO_PUBLIC_CPP_BINDINGS_MAP_H_

#include <map>
#include <utility>

#include "base/containers/flat_map.h"

namespace mojo {

// TODO(yzshen): These conversion functions should be removed and callsites
// should be revisited and changed to use the same map type.
template <typename Key, typename Value>
base::flat_map<Key, Value> MapToFlatMap(const std::map<Key, Value>& input) {
  return base::flat_map<Key, Value>(input.begin(), input.end());
}

template <typename Key, typename Value>
base::flat_map<Key, Value> MapToFlatMap(std::map<Key, Value>&& input) {
  return base::flat_map<Key, Value>(std::make_move_iterator(input.begin()),
                                    std::make_move_iterator(input.end()));
}

template <typename Key, typename Value>
std::map<Key, Value> FlatMapToMap(const base::flat_map<Key, Value>& input) {
  return std::map<Key, Value>(input.begin(), input.end());
}

template <typename Key, typename Value>
std::map<Key, Value> FlatMapToMap(base::flat_map<Key, Value>&& input) {
  return std::map<Key, Value>(std::make_move_iterator(input.begin()),
                              std::make_move_iterator(input.end()));
}

}  // namespace mojo

#endif  // MOJO_PUBLIC_CPP_BINDINGS_MAP_H_
