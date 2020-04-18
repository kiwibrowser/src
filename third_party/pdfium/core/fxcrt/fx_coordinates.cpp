// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fxcrt/fx_coordinates.h"

#include <utility>

#include "core/fxcrt/fx_extension.h"

namespace {

void MatchFloatRange(float f1, float f2, int* i1, int* i2) {
  int length = static_cast<int>(ceil(f2 - f1));
  int i1_1 = static_cast<int>(floor(f1));
  int i1_2 = static_cast<int>(ceil(f1));
  float error1 = f1 - i1_1 + fabsf(f2 - i1_1 - length);
  float error2 = i1_2 - f1 + fabsf(f2 - i1_2 - length);

  *i1 = error1 > error2 ? i1_2 : i1_1;
  *i2 = *i1 + length;
}

#if _FX_PLATFORM_ == _FX_PLATFORM_WINDOWS_
static_assert(sizeof(FX_RECT) == sizeof(RECT), "FX_RECT vs. RECT mismatch");
static_assert(offsetof(FX_RECT, left) == offsetof(RECT, left),
              "FX_RECT vs. RECT mismatch");
static_assert(offsetof(FX_RECT, top) == offsetof(RECT, top),
              "FX_RECT vs. RECT mismatch");
static_assert(offsetof(FX_RECT, right) == offsetof(RECT, right),
              "FX_RECT vs. RECT mismatch");
static_assert(offsetof(FX_RECT, bottom) == offsetof(RECT, bottom),
              "FX_RECT vs. RECT mismatch");
static_assert(sizeof(FX_RECT::left) == sizeof(RECT::left),
              "FX_RECT vs. RECT mismatch");
static_assert(sizeof(FX_RECT::top) == sizeof(RECT::top),
              "FX_RECT vs. RECT mismatch");
static_assert(sizeof(FX_RECT::right) == sizeof(RECT::right),
              "FX_RECT vs. RECT mismatch");
static_assert(sizeof(FX_RECT::bottom) == sizeof(RECT::bottom),
              "FX_RECT vs. RECT mismatch");
#endif

}  // namespace

void FX_RECT::Normalize() {
  if (left > right)
    std::swap(left, right);
  if (top > bottom)
    std::swap(top, bottom);
}

void FX_RECT::Intersect(const FX_RECT& src) {
  FX_RECT src_n = src;
  src_n.Normalize();
  Normalize();
  left = std::max(left, src_n.left);
  top = std::max(top, src_n.top);
  right = std::min(right, src_n.right);
  bottom = std::min(bottom, src_n.bottom);
  if (left > right || top > bottom) {
    left = top = right = bottom = 0;
  }
}

CFX_FloatRect::CFX_FloatRect(const FX_RECT& rect) {
  left = rect.left;
  top = rect.bottom;
  right = rect.right;
  bottom = rect.top;
}

// static
CFX_FloatRect CFX_FloatRect::GetBBox(const CFX_PointF* pPoints, int nPoints) {
  if (nPoints == 0)
    return CFX_FloatRect();

  float min_x = pPoints->x;
  float max_x = pPoints->x;
  float min_y = pPoints->y;
  float max_y = pPoints->y;
  for (int i = 1; i < nPoints; i++) {
    min_x = std::min(min_x, pPoints[i].x);
    max_x = std::max(max_x, pPoints[i].x);
    min_y = std::min(min_y, pPoints[i].y);
    max_y = std::max(max_y, pPoints[i].y);
  }
  return CFX_FloatRect(min_x, min_y, max_x, max_y);
}

void CFX_FloatRect::Normalize() {
  if (left > right)
    std::swap(left, right);
  if (bottom > top)
    std::swap(top, bottom);
}

void CFX_FloatRect::Reset() {
  left = 0.0f;
  right = 0.0f;
  bottom = 0.0f;
  top = 0.0f;
}

void CFX_FloatRect::Intersect(const CFX_FloatRect& other_rect) {
  Normalize();
  CFX_FloatRect other = other_rect;
  other.Normalize();
  left = std::max(left, other.left);
  bottom = std::max(bottom, other.bottom);
  right = std::min(right, other.right);
  top = std::min(top, other.top);
  if (left > right || bottom > top)
    Reset();
}

void CFX_FloatRect::Union(const CFX_FloatRect& other_rect) {
  Normalize();
  CFX_FloatRect other = other_rect;
  other.Normalize();
  left = std::min(left, other.left);
  bottom = std::min(bottom, other.bottom);
  right = std::max(right, other.right);
  top = std::max(top, other.top);
}

FX_RECT CFX_FloatRect::GetOuterRect() const {
  FX_RECT rect;
  rect.left = static_cast<int>(floor(left));
  rect.bottom = static_cast<int>(ceil(top));
  rect.right = static_cast<int>(ceil(right));
  rect.top = static_cast<int>(floor(bottom));
  rect.Normalize();
  return rect;
}

FX_RECT CFX_FloatRect::GetInnerRect() const {
  FX_RECT rect;
  rect.left = static_cast<int>(ceil(left));
  rect.bottom = static_cast<int>(floor(top));
  rect.right = static_cast<int>(floor(right));
  rect.top = static_cast<int>(ceil(bottom));
  rect.Normalize();
  return rect;
}

FX_RECT CFX_FloatRect::GetClosestRect() const {
  FX_RECT rect;
  MatchFloatRange(left, right, &rect.left, &rect.right);
  MatchFloatRange(bottom, top, &rect.top, &rect.bottom);
  rect.Normalize();
  return rect;
}

CFX_FloatRect CFX_FloatRect::GetCenterSquare() const {
  float fWidth = Width();
  float fHeight = Height();
  float fHalfWidth = (fWidth > fHeight) ? fHeight / 2 : fWidth / 2;

  float fCenterX = (left + right) / 2.0f;
  float fCenterY = (top + bottom) / 2.0f;
  return CFX_FloatRect(fCenterX - fHalfWidth, fCenterY - fHalfWidth,
                       fCenterX + fHalfWidth, fCenterY + fHalfWidth);
}

bool CFX_FloatRect::Contains(const CFX_PointF& point) const {
  CFX_FloatRect n1(*this);
  n1.Normalize();
  return point.x <= n1.right && point.x >= n1.left && point.y <= n1.top &&
         point.y >= n1.bottom;
}

bool CFX_FloatRect::Contains(const CFX_FloatRect& other_rect) const {
  CFX_FloatRect n1(*this);
  CFX_FloatRect n2(other_rect);
  n1.Normalize();
  n2.Normalize();
  return n2.left >= n1.left && n2.right <= n1.right && n2.bottom >= n1.bottom &&
         n2.top <= n1.top;
}

void CFX_FloatRect::UpdateRect(const CFX_PointF& point) {
  left = std::min(left, point.x);
  bottom = std::min(bottom, point.y);
  right = std::max(right, point.x);
  top = std::max(top, point.y);
}

void CFX_FloatRect::Scale(float fScale) {
  left *= fScale;
  bottom *= fScale;
  right *= fScale;
  top *= fScale;
}

void CFX_FloatRect::ScaleFromCenterPoint(float fScale) {
  float fHalfWidth = (right - left) / 2.0f;
  float fHalfHeight = (top - bottom) / 2.0f;

  float center_x = (left + right) / 2;
  float center_y = (top + bottom) / 2;

  left = center_x - fHalfWidth * fScale;
  bottom = center_y - fHalfHeight * fScale;
  right = center_x + fHalfWidth * fScale;
  top = center_y + fHalfHeight * fScale;
}

FX_RECT CFX_FloatRect::ToFxRect() const {
  return FX_RECT(static_cast<int>(left), static_cast<int>(top),
                 static_cast<int>(right), static_cast<int>(bottom));
}

FX_RECT CFX_FloatRect::ToRoundedFxRect() const {
  return FX_RECT(FXSYS_round(left), FXSYS_round(top), FXSYS_round(right),
                 FXSYS_round(bottom));
}

#ifndef NDEBUG
std::ostream& operator<<(std::ostream& os, const CFX_FloatRect& rect) {
  os << "rect[w " << rect.Width() << " x h " << rect.Height() << " (left "
     << rect.left << ", bot " << rect.bottom << ")]";
  return os;
}

std::ostream& operator<<(std::ostream& os, const CFX_RectF& rect) {
  os << "rect[w " << rect.Width() << " x h " << rect.Height() << " (left "
     << rect.left << ", top " << rect.top << ")]";
  return os;
}
#endif  // NDEBUG

CFX_Matrix CFX_Matrix::GetInverse() const {
  CFX_Matrix inverse;
  float i = a * d - b * c;
  if (fabs(i) == 0)
    return inverse;

  float j = -i;
  inverse.a = d / i;
  inverse.b = b / j;
  inverse.c = c / j;
  inverse.d = a / i;
  inverse.e = (c * f - d * e) / i;
  inverse.f = (a * f - b * e) / j;
  return inverse;
}

void CFX_Matrix::Concat(const CFX_Matrix& m, bool bPrepended) {
  ConcatInternal(m, bPrepended);
}

void CFX_Matrix::ConcatInverse(const CFX_Matrix& src, bool bPrepended) {
  Concat(src.GetInverse(), bPrepended);
}

bool CFX_Matrix::Is90Rotated() const {
  return fabs(a * 1000) < fabs(b) && fabs(d * 1000) < fabs(c);
}

bool CFX_Matrix::IsScaled() const {
  return fabs(b * 1000) < fabs(a) && fabs(c * 1000) < fabs(d);
}

void CFX_Matrix::Translate(float x, float y, bool bPrepended) {
  if (bPrepended) {
    e += x * a + y * c;
    f += y * d + x * b;
    return;
  }
  e += x;
  f += y;
}

void CFX_Matrix::Scale(float sx, float sy, bool bPrepended) {
  a *= sx;
  d *= sy;
  if (bPrepended) {
    b *= sx;
    c *= sy;
    return;
  }

  b *= sy;
  c *= sx;
  e *= sx;
  f *= sy;
}

void CFX_Matrix::Rotate(float fRadian, bool bPrepended) {
  float cosValue = cos(fRadian);
  float sinValue = sin(fRadian);
  ConcatInternal(CFX_Matrix(cosValue, sinValue, -sinValue, cosValue, 0, 0),
                 bPrepended);
}

void CFX_Matrix::RotateAt(float fRadian, float x, float y, bool bPrepended) {
  Translate(-x, -y, bPrepended);
  Rotate(fRadian, bPrepended);
  Translate(x, y, bPrepended);
}

void CFX_Matrix::Shear(float fAlphaRadian, float fBetaRadian, bool bPrepended) {
  ConcatInternal(CFX_Matrix(1, tan(fAlphaRadian), tan(fBetaRadian), 1, 0, 0),
                 bPrepended);
}

void CFX_Matrix::MatchRect(const CFX_FloatRect& dest,
                           const CFX_FloatRect& src) {
  float fDiff = src.left - src.right;
  a = fabs(fDiff) < 0.001f ? 1 : (dest.left - dest.right) / fDiff;

  fDiff = src.bottom - src.top;
  d = fabs(fDiff) < 0.001f ? 1 : (dest.bottom - dest.top) / fDiff;
  e = dest.left - src.left * a;
  f = dest.bottom - src.bottom * d;
  b = 0;
  c = 0;
}

float CFX_Matrix::GetXUnit() const {
  if (b == 0)
    return (a > 0 ? a : -a);
  if (a == 0)
    return (b > 0 ? b : -b);
  return sqrt(a * a + b * b);
}

float CFX_Matrix::GetYUnit() const {
  if (c == 0)
    return (d > 0 ? d : -d);
  if (d == 0)
    return (c > 0 ? c : -c);
  return sqrt(c * c + d * d);
}

CFX_FloatRect CFX_Matrix::GetUnitRect() const {
  return TransformRect(CFX_FloatRect(0.f, 0.f, 1.f, 1.f));
}

float CFX_Matrix::TransformXDistance(float dx) const {
  float fx = a * dx;
  float fy = b * dx;
  return sqrt(fx * fx + fy * fy);
}

float CFX_Matrix::TransformDistance(float distance) const {
  return distance * (GetXUnit() + GetYUnit()) / 2;
}

CFX_PointF CFX_Matrix::Transform(const CFX_PointF& point) const {
  return CFX_PointF(a * point.x + c * point.y + e,
                    b * point.x + d * point.y + f);
}
std::tuple<float, float, float, float> CFX_Matrix::TransformRect(
    const float& left,
    const float& right,
    const float& top,
    const float& bottom) const {
  CFX_PointF points[] = {
      {left, top}, {left, bottom}, {right, top}, {right, bottom}};
  for (int i = 0; i < 4; i++)
    points[i] = Transform(points[i]);

  float new_right = points[0].x;
  float new_left = points[0].x;
  float new_top = points[0].y;
  float new_bottom = points[0].y;
  for (int i = 1; i < 4; i++) {
    new_right = std::max(new_right, points[i].x);
    new_left = std::min(new_left, points[i].x);
    new_top = std::max(new_top, points[i].y);
    new_bottom = std::min(new_bottom, points[i].y);
  }
  return std::make_tuple(new_left, new_right, new_top, new_bottom);
}

CFX_RectF CFX_Matrix::TransformRect(const CFX_RectF& rect) const {
  float left;
  float right;
  float bottom;
  float top;
  std::tie(left, right, bottom, top) =
      TransformRect(rect.left, rect.right(), rect.bottom(), rect.top);
  return CFX_RectF(left, top, right - left, bottom - top);
}

CFX_FloatRect CFX_Matrix::TransformRect(const CFX_FloatRect& rect) const {
  float left;
  float right;
  float top;
  float bottom;
  std::tie(left, right, top, bottom) =
      TransformRect(rect.left, rect.right, rect.top, rect.bottom);
  return CFX_FloatRect(left, bottom, right, top);
}

void CFX_Matrix::ConcatInternal(const CFX_Matrix& other, bool prepend) {
  CFX_Matrix left;
  CFX_Matrix right;
  if (prepend) {
    left = other;
    right = *this;
  } else {
    left = *this;
    right = other;
  }

  a = left.a * right.a + left.b * right.c;
  b = left.a * right.b + left.b * right.d;
  c = left.c * right.a + left.d * right.c;
  d = left.c * right.b + left.d * right.d;
  e = left.e * right.a + left.f * right.c + right.e;
  f = left.e * right.b + left.f * right.d + right.f;
}
