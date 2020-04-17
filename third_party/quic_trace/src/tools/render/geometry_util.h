// Copyright 2018 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef THIRD_PARTY_QUIC_TRACE_TOOLS_GEOMETRY_UTIL_H_
#define THIRD_PARTY_QUIC_TRACE_TOOLS_GEOMETRY_UTIL_H_

#include <algorithm>
#include <cmath>
#include <ostream>

namespace quic_trace {
namespace render {

// A 2D-vector with pairwise operators defined.  Named vec2 after the GLSL type
// of the same name and similar behavior.
struct alignas(8) vec2 {
  float x;
  float y;

  constexpr vec2() : x(0.f), y(0.f) {}
  constexpr vec2(float x, float y) : x(x), y(y) {}

  vec2& operator+=(const vec2& other) {
    x += other.x;
    y += other.y;
    return *this;
  }
};

constexpr vec2 operator+(vec2 lhs, vec2 rhs) {
  return vec2(lhs.x + rhs.x, lhs.y + rhs.y);
}
constexpr vec2 operator-(vec2 lhs, vec2 rhs) {
  return vec2(lhs.x - rhs.x, lhs.y - rhs.y);
}
constexpr vec2 operator*(float lhs, vec2 rhs) {
  return vec2(lhs * rhs.x, lhs * rhs.y);
}
constexpr vec2 operator*(vec2 lhs, vec2 rhs) {
  return vec2(lhs.x * rhs.x, lhs.y * rhs.y);
}
constexpr vec2 operator/(vec2 lhs, vec2 rhs) {
  return vec2(lhs.x / rhs.x, lhs.y / rhs.y);
}
constexpr vec2 operator/(vec2 lhs, float rhs) {
  return vec2(lhs.x / rhs, lhs.y / rhs);
}
constexpr bool operator==(vec2 lhs, vec2 rhs) {
  return lhs.x == rhs.x && lhs.y == rhs.y;
}
constexpr bool operator!=(vec2 lhs, vec2 rhs) {
  return lhs.x != rhs.x || lhs.y != rhs.y;
}

inline vec2 PointwiseMin(vec2 a, vec2 b) {
  return vec2(std::min(a.x, b.x), std::min(a.y, b.y));
}
inline vec2 PointwiseMax(vec2 a, vec2 b) {
  return vec2(std::max(a.x, b.x), std::max(a.y, b.y));
}

inline std::ostream& operator<<(std::ostream& os, vec2 vector) {
  os << "(" << vector.x << ", " << vector.y << ")";
  return os;
}

inline float DotProduct(vec2 a, vec2 b) {
  return a.x * b.x + a.y * b.y;
}
inline float DistanceSquared(vec2 a, vec2 b) {
  const vec2 delta = a - b;
  return DotProduct(delta, delta);
}

// A rectangle formed by vertices |origin| and |origin + size|.
struct Box {
  vec2 origin;
  vec2 size;
};

constexpr bool operator==(const Box& lhs, const Box& rhs) {
  return lhs.origin == rhs.origin && lhs.size == rhs.size;
}
constexpr bool operator!=(const Box& lhs, const Box& rhs) {
  return lhs.origin != rhs.origin || lhs.size != rhs.size;
}

inline Box BoundingBox(vec2 a, vec2 b) {
  return Box{PointwiseMin(a, b),
             vec2(std::abs(a.x - b.x), std::abs(a.y - b.y))};
}

inline Box BoundingBox(const Box& a, const Box& b) {
  return BoundingBox(PointwiseMin(a.origin, b.origin),
                     PointwiseMax(a.origin + a.size, b.origin + b.size));
}

inline Box IntersectBoxes(const Box& a, const Box& b) {
  vec2 corner_low = PointwiseMax(a.origin, b.origin);
  vec2 corner_high = PointwiseMin(a.origin + a.size, b.origin + b.size);
  if (corner_low.x < corner_high.x && corner_low.y < corner_high.y) {
    return BoundingBox(corner_low, corner_high);
  } else {
    return Box{};
  }
}

inline bool IsInside(const Box& inner, const Box& outer) {
  const vec2 corner_inner = inner.origin + inner.size;
  const vec2 corner_outer = outer.origin + outer.size;
  return inner.origin.x >= outer.origin.x && inner.origin.y >= outer.origin.y &&
         corner_inner.x <= corner_outer.x && corner_inner.y <= corner_outer.y;
}

inline bool IsInside(vec2 point, const Box& box) {
  return point.x >= box.origin.x && point.x <= box.origin.x + box.size.x &&
         point.y >= box.origin.y && point.y <= box.origin.y + box.size.y;
}

inline vec2 BoxCenter(const Box& box) {
  return box.origin + box.size / 2;
}

}  // namespace render
}  // namespace quic_trace

#endif  // THIRD_PARTY_QUIC_TRACE_TOOLS_GEOMETRY_UTIL_H_
