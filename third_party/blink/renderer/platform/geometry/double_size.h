// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_GEOMETRY_DOUBLE_SIZE_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_GEOMETRY_DOUBLE_SIZE_H_

#include <iosfwd>
#include "third_party/blink/renderer/platform/geometry/float_size.h"
#include "third_party/blink/renderer/platform/geometry/int_size.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"
#include "third_party/blink/renderer/platform/wtf/forward.h"

namespace blink {

class LayoutSize;

class PLATFORM_EXPORT DoubleSize {
  DISALLOW_NEW();

 public:
  DoubleSize() : width_(0), height_(0) {}
  DoubleSize(double width, double height) : width_(width), height_(height) {}
  DoubleSize(const IntSize& p) : width_(p.Width()), height_(p.Height()) {}
  DoubleSize(const FloatSize& s) : width_(s.Width()), height_(s.Height()) {}
  explicit DoubleSize(const LayoutSize&);

  double Width() const { return width_; }
  double Height() const { return height_; }

  void SetWidth(double width) { width_ = width; }
  void SetHeight(double height) { height_ = height; }

  bool IsEmpty() const { return width_ <= 0 || height_ <= 0; }

  bool IsZero() const;

  void Expand(float width, float height) {
    width_ += width;
    height_ += height;
  }

  void Scale(float width_scale, float height_scale) {
    width_ = width_ * width_scale;
    height_ = height_ * height_scale;
  }

  void Scale(float scale) { this->Scale(scale, scale); }

  String ToString() const;

 private:
  double width_, height_;
};

inline DoubleSize& operator+=(DoubleSize& a, const DoubleSize& b) {
  a.SetWidth(a.Width() + b.Width());
  a.SetHeight(a.Height() + b.Height());
  return a;
}

inline DoubleSize& operator-=(DoubleSize& a, const DoubleSize& b) {
  a.SetWidth(a.Width() - b.Width());
  a.SetHeight(a.Height() - b.Height());
  return a;
}

inline DoubleSize operator+(const DoubleSize& a, const DoubleSize& b) {
  return DoubleSize(a.Width() + b.Width(), a.Height() + b.Height());
}

inline DoubleSize operator-(const DoubleSize& a, const DoubleSize& b) {
  return DoubleSize(a.Width() - b.Width(), a.Height() - b.Height());
}

inline bool operator==(const DoubleSize& a, const DoubleSize& b) {
  return a.Width() == b.Width() && a.Height() == b.Height();
}

inline bool operator!=(const DoubleSize& a, const DoubleSize& b) {
  return a.Width() != b.Width() || a.Height() != b.Height();
}

inline IntSize FlooredIntSize(const DoubleSize& p) {
  return IntSize(clampTo<int>(floor(p.Width())),
                 clampTo<int>(floor(p.Height())));
}

inline IntSize RoundedIntSize(const DoubleSize& p) {
  return IntSize(clampTo<int>(roundf(p.Width())),
                 clampTo<int>(roundf(p.Height())));
}

inline IntSize ExpandedIntSize(const DoubleSize& p) {
  return IntSize(clampTo<int>(ceilf(p.Width())),
                 clampTo<int>(ceilf(p.Height())));
}

inline FloatSize ToFloatSize(const DoubleSize& p) {
  return FloatSize(p.Width(), p.Height());
}

PLATFORM_EXPORT std::ostream& operator<<(std::ostream&, const DoubleSize&);

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_GEOMETRY_DOUBLE_SIZE_H_
