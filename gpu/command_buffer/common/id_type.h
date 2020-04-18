// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_COMMAND_BUFFER_COMMON_ID_TYPE_H_
#define GPU_COMMAND_BUFFER_COMMON_ID_TYPE_H_

#include <stdint.h>
#include <cstddef>
#include <ostream>
#include <type_traits>

// IdType32<>, IdType64<>, etc. wrap an integer id in a custom, type-safe type.
//
// IdType32<Foo> is an alternative to int, for a class Foo with methods like:
//
//    int GetId() { return id_; };
//    static Foo* FromId(int id) { return g_all_foos_by_id[id]; }
//
// Such methods are a standard means of safely referring to objects across
// thread and process boundaries.  But if a nearby class Bar also represents
// its IDs as a bare int, horrific mixups are possible -- one example, of many,
// is http://crrev.com/365437.  IdType<> offers compile-time protection against
// such mishaps, since IdType32<Foo> is incompatible with IdType32<Bar>, even
// though both just compile down to an int32_t.
//
// Templates in this file:
//   IdType32<T> / IdTypeU32<T>: Signed / unsigned 32-bit IDs
//   IdType64<T> / IdTypeU64<T>: Signed / unsigned 64-bit IDs
//   IdType<>: For when you need a different underlying type or
//             a default/null value other than zero.
//
// IdType32<Foo> behaves just like an int32_t in the following aspects:
// - it can be used as a key in std::map and/or std::unordered_map;
// - it can be used as an argument to DCHECK_EQ or streamed to LOG(ERROR);
// - it has the same memory footprint and runtime overhead as int32_t;
// - it can be copied by memcpy.
// - it can be used in IPC messages.
//
// IdType32<Foo> has the following differences from a bare int32_t:
// - it forces coercions to go through GetUnsafeValue and FromUnsafeValue;
// - it restricts the set of available operations (i.e. no multiplication);
// - it ensures initialization to zero and allows checking against
//   default-initialized values via is_null method.

namespace gpu {

template <typename TypeMarker, typename WrappedType, WrappedType kInvalidValue>
class IdType {
 public:
  IdType() : value_(kInvalidValue) {}
  bool is_null() const { return value_ == kInvalidValue; }

  static IdType FromUnsafeValue(WrappedType value) { return IdType(value); }
  WrappedType GetUnsafeValue() const { return value_; }

  IdType(const IdType& other) = default;
  IdType& operator=(const IdType& other) = default;

  bool operator==(const IdType& other) const { return value_ == other.value_; }
  bool operator!=(const IdType& other) const { return value_ != other.value_; }
  bool operator<(const IdType& other) const { return value_ < other.value_; }

  // Hasher to use in std::unordered_map, std::unordered_set, etc.
  struct Hasher {
    using argument_type = IdType;
    using result_type = std::size_t;
    result_type operator()(const argument_type& id) const {
      return std::hash<WrappedType>()(id.GetUnsafeValue());
    }
  };

 protected:
  explicit IdType(WrappedType val) : value_(val) {}

 private:
  // In theory WrappedType could be any type that supports ==, <, <<, std::hash,
  // etc., but to make things simpler (both for users and for maintainers) we
  // explicitly restrict the design space to integers.  This means the users
  // can safely assume that IdType is relatively small and cheap to copy
  // and the maintainers don't have to worry about WrappedType being a complex
  // type (i.e. std::string or std::pair or a move-only type).
  using IntegralWrappedType =
      typename std::enable_if<std::is_integral<WrappedType>::value,
                              WrappedType>::type;
  IntegralWrappedType value_;
};

// Type aliases for convenience:
template <typename TypeMarker>
using IdType32 = IdType<TypeMarker, int32_t, 0>;
template <typename TypeMarker>
using IdTypeU32 = IdType<TypeMarker, uint32_t, 0>;
template <typename TypeMarker>
using IdType64 = IdType<TypeMarker, int64_t, 0>;
template <typename TypeMarker>
using IdTypeU64 = IdType<TypeMarker, uint64_t, 0>;

template <typename TypeMarker, typename WrappedType, WrappedType kInvalidValue>
std::ostream& operator<<(
    std::ostream& stream,
    const IdType<TypeMarker, WrappedType, kInvalidValue>& id) {
  return stream << id.GetUnsafeValue();
}

}  // namespace gpu

#endif  // CONTENT_COMMON_ID_TYPE_H_
