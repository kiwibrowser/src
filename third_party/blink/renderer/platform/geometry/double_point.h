
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_GEOMETRY_DOUBLE_POINT_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_GEOMETRY_DOUBLE_POINT_H_

#include <iosfwd>
#include "third_party/blink/renderer/platform/geometry/double_size.h"
#include "third_party/blink/renderer/platform/geometry/float_point.h"
#include "third_party/blink/renderer/platform/geometry/int_point.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"
#include "third_party/blink/renderer/platform/wtf/forward.h"

namespace blink {

class LayoutPoint;

class PLATFORM_EXPORT DoublePoint {
  DISALLOW_NEW();

 public:
  DoublePoint() : x_(0), y_(0) {}
  DoublePoint(double x, double y) : x_(x), y_(y) {}
  DoublePoint(const IntPoint& p) : x_(p.X()), y_(p.Y()) {}
  DoublePoint(const FloatPoint& p) : x_(p.X()), y_(p.Y()) {}
  explicit DoublePoint(const LayoutPoint&);

  explicit DoublePoint(const IntSize& size)
      : x_(size.Width()), y_(size.Height()) {}

  explicit DoublePoint(const FloatSize&);

  explicit DoublePoint(const DoubleSize& size)
      : x_(size.Width()), y_(size.Height()) {}

  static DoublePoint Zero() { return DoublePoint(); }

  DoublePoint ExpandedTo(const DoublePoint&) const;
  DoublePoint ShrunkTo(const DoublePoint&) const;

  double X() const { return x_; }
  double Y() const { return y_; }
  void SetX(double x) { x_ = x; }
  void SetY(double y) { y_ = y; }

  void Move(const DoubleSize& s) {
    x_ += s.Width();
    y_ += s.Height();
  }

  void Move(double x, double y) {
    x_ += x;
    y_ += y;
  }

  void MoveBy(const DoublePoint& p) {
    x_ += p.X();
    y_ += p.Y();
  }

  void Scale(float sx, float sy) {
    x_ *= sx;
    y_ *= sy;
  }

  DoublePoint ScaledBy(float scale) const {
    return DoublePoint(x_ * scale, y_ * scale);
  }

  String ToString() const;

 private:
  double x_, y_;
};

inline bool operator==(const DoublePoint& a, const DoublePoint& b) {
  return a.X() == b.X() && a.Y() == b.Y();
}

inline bool operator!=(const DoublePoint& a, const DoublePoint& b) {
  return a.X() != b.X() || a.Y() != b.Y();
}

inline DoublePoint& operator+=(DoublePoint& a, const DoubleSize& b) {
  a.SetX(a.X() + b.Width());
  a.SetY(a.Y() + b.Height());
  return a;
}

inline DoublePoint& operator-=(DoublePoint& a, const DoubleSize& b) {
  a.SetX(a.X() - b.Width());
  a.SetY(a.Y() - b.Height());
  return a;
}

inline DoublePoint operator+(const DoublePoint& a, const DoubleSize& b) {
  return DoublePoint(a.X() + b.Width(), a.Y() + b.Height());
}

inline DoubleSize operator-(const DoublePoint& a, const DoublePoint& b) {
  return DoubleSize(a.X() - b.X(), a.Y() - b.Y());
}

inline DoublePoint operator-(const DoublePoint& a) {
  return DoublePoint(-a.X(), -a.Y());
}

inline DoublePoint operator-(const DoublePoint& a, const DoubleSize& b) {
  return DoublePoint(a.X() - b.Width(), a.Y() - b.Height());
}

inline IntPoint RoundedIntPoint(const DoublePoint& p) {
  return IntPoint(clampTo<int>(round(p.X())), clampTo<int>(round(p.Y())));
}

inline IntPoint CeiledIntPoint(const DoublePoint& p) {
  return IntPoint(clampTo<int>(ceil(p.X())), clampTo<int>(ceil(p.Y())));
}

inline IntPoint FlooredIntPoint(const DoublePoint& p) {
  return IntPoint(clampTo<int>(floor(p.X())), clampTo<int>(floor(p.Y())));
}

inline FloatPoint ToFloatPoint(const DoublePoint& a) {
  return FloatPoint(a.X(), a.Y());
}

inline DoubleSize ToDoubleSize(const DoublePoint& a) {
  return DoubleSize(a.X(), a.Y());
}

PLATFORM_EXPORT std::ostream& operator<<(std::ostream&, const DoublePoint&);

}  // namespace blink

#endif
