/*
 * Copyright (C) 2003, 2006 Apple Computer, Inc.  All rights reserved.
 * Copyright (C) 2005 Nokia.  All rights reserved.
 *               2008 Eric Seidel <eric@webkit.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_GEOMETRY_FLOAT_SIZE_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_GEOMETRY_FLOAT_SIZE_H_

#include <iosfwd>

#include "build/build_config.h"
#include "third_party/blink/renderer/platform/geometry/int_point.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"
#include "third_party/blink/renderer/platform/wtf/forward.h"

#if defined(OS_MACOSX)
typedef struct CGSize CGSize;

#ifdef __OBJC__
#import <Foundation/Foundation.h>
#endif
#endif

struct SkSize;

namespace gfx {
class SizeF;
class Vector2dF;
}  // namespace gfx

namespace blink {

class IntSize;
class LayoutSize;

class PLATFORM_EXPORT FloatSize {
  DISALLOW_NEW();

 public:
  FloatSize() : width_(0), height_(0) {}
  FloatSize(float width, float height) : width_(width), height_(height) {}
  explicit FloatSize(const IntSize& size)
      : width_(size.Width()), height_(size.Height()) {}
  FloatSize(const SkSize&);
  explicit FloatSize(const LayoutSize&);

  static FloatSize NarrowPrecision(double width, double height);

  float Width() const { return width_; }
  float Height() const { return height_; }

  void SetWidth(float width) { width_ = width; }
  void SetHeight(float height) { height_ = height; }

  bool IsEmpty() const { return width_ <= 0 || height_ <= 0; }
  bool IsZero() const {
    return fabs(width_) < std::numeric_limits<float>::epsilon() &&
           fabs(height_) < std::numeric_limits<float>::epsilon();
  }
  bool IsExpressibleAsIntSize() const;

  float AspectRatio() const { return width_ / height_; }

  float Area() const { return width_ * height_; }

  void Expand(float width, float height) {
    width_ += width;
    height_ += height;
  }

  void Scale(float s) { Scale(s, s); }

  void Scale(float scale_x, float scale_y) {
    width_ *= scale_x;
    height_ *= scale_y;
  }

  void ScaleAndFloor(float scale) {
    width_ = floorf(width_ * scale);
    height_ = floorf(height_ * scale);
  }

  FloatSize ExpandedTo(const FloatSize& other) const {
    return FloatSize(width_ > other.width_ ? width_ : other.width_,
                     height_ > other.height_ ? height_ : other.height_);
  }

  FloatSize ShrunkTo(const FloatSize& other) const {
    return FloatSize(width_ < other.width_ ? width_ : other.width_,
                     height_ < other.height_ ? height_ : other.height_);
  }

  void ClampNegativeToZero() { *this = ExpandedTo(FloatSize()); }

  float DiagonalLength() const;
  float DiagonalLengthSquared() const {
    return width_ * width_ + height_ * height_;
  }

  FloatSize TransposedSize() const { return FloatSize(height_, width_); }

  FloatSize ScaledBy(float scale) const { return ScaledBy(scale, scale); }

  FloatSize ScaledBy(float scale_x, float scale_y) const {
    return FloatSize(width_ * scale_x, height_ * scale_y);
  }

#if defined(OS_MACOSX)
  explicit FloatSize(
      const CGSize&);  // don't do this implicitly since it's lossy
  operator CGSize() const;
#endif

  operator SkSize() const;
  // Use this only for logical sizes, which can not be negative. Things that are
  // offsets instead, and can be negative, should use a gfx::Vector2dF.
  explicit operator gfx::SizeF() const;
  // FloatSize is used as an offset, which can be negative, but gfx::SizeF can
  // not. The Vector2dF type is used for offsets instead.
  explicit operator gfx::Vector2dF() const;

  String ToString() const;

 private:
  float width_, height_;
};

inline FloatSize& operator+=(FloatSize& a, const FloatSize& b) {
  a.SetWidth(a.Width() + b.Width());
  a.SetHeight(a.Height() + b.Height());
  return a;
}

inline FloatSize& operator-=(FloatSize& a, const FloatSize& b) {
  a.SetWidth(a.Width() - b.Width());
  a.SetHeight(a.Height() - b.Height());
  return a;
}

inline FloatSize operator+(const FloatSize& a, const FloatSize& b) {
  return FloatSize(a.Width() + b.Width(), a.Height() + b.Height());
}

inline FloatSize operator-(const FloatSize& a, const FloatSize& b) {
  return FloatSize(a.Width() - b.Width(), a.Height() - b.Height());
}

inline FloatSize operator-(const FloatSize& size) {
  return FloatSize(-size.Width(), -size.Height());
}

inline FloatSize operator*(const FloatSize& a, const float b) {
  return FloatSize(a.Width() * b, a.Height() * b);
}

inline FloatSize operator*(const float a, const FloatSize& b) {
  return FloatSize(a * b.Width(), a * b.Height());
}

inline bool operator==(const FloatSize& a, const FloatSize& b) {
  return a.Width() == b.Width() && a.Height() == b.Height();
}

inline bool operator!=(const FloatSize& a, const FloatSize& b) {
  return a.Width() != b.Width() || a.Height() != b.Height();
}

inline IntSize RoundedIntSize(const FloatSize& p) {
  return IntSize(clampTo<int>(roundf(p.Width())),
                 clampTo<int>(roundf(p.Height())));
}

inline IntSize FlooredIntSize(const FloatSize& p) {
  return IntSize(clampTo<int>(floorf(p.Width())),
                 clampTo<int>(floorf(p.Height())));
}

inline IntSize ExpandedIntSize(const FloatSize& p) {
  return IntSize(clampTo<int>(ceilf(p.Width())),
                 clampTo<int>(ceilf(p.Height())));
}

inline IntPoint FlooredIntPoint(const FloatSize& p) {
  return IntPoint(clampTo<int>(floorf(p.Width())),
                  clampTo<int>(floorf(p.Height())));
}

PLATFORM_EXPORT std::ostream& operator<<(std::ostream&, const FloatSize&);
PLATFORM_EXPORT WTF::TextStream& operator<<(WTF::TextStream&, const FloatSize&);

}  // namespace blink

// Allows this class to be stored in a HeapVector.
WTF_ALLOW_CLEAR_UNUSED_SLOTS_WITH_MEM_FUNCTIONS(blink::FloatSize);

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_GEOMETRY_FLOAT_SIZE_H_
