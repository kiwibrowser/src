// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gfx/skia_color_space_util.h"

#include <algorithm>
#include <cmath>
#include <vector>

#include "base/logging.h"

namespace gfx {

float SkTransferFnEvalUnclamped(const SkColorSpaceTransferFn& fn, float x) {
  if (x < fn.fD)
    return fn.fC * x + fn.fF;
  return std::pow(fn.fA * x + fn.fB, fn.fG) + fn.fE;
}

float SkTransferFnEval(const SkColorSpaceTransferFn& fn, float x) {
  float fn_at_x_unclamped = SkTransferFnEvalUnclamped(fn, x);
  return std::min(std::max(fn_at_x_unclamped, 0.f), 1.f);
}

SkColorSpaceTransferFn SkTransferFnInverse(const SkColorSpaceTransferFn& fn) {
  SkColorSpaceTransferFn fn_inv = {0};
  if (fn.fA > 0 && fn.fG > 0) {
    double a_to_the_g = std::pow(fn.fA, fn.fG);
    fn_inv.fA = 1.f / a_to_the_g;
    fn_inv.fB = -fn.fE / a_to_the_g;
    fn_inv.fG = 1.f / fn.fG;
  }
  fn_inv.fD = fn.fC * fn.fD + fn.fF;
  fn_inv.fE = -fn.fB / fn.fA;
  if (fn.fC != 0) {
    fn_inv.fC = 1.f / fn.fC;
    fn_inv.fF = -fn.fF / fn.fC;
  }
  return fn_inv;
}

bool SkTransferFnsApproximatelyCancel(const SkColorSpaceTransferFn& a,
                                      const SkColorSpaceTransferFn& b) {
  const float kStep = 1.f / 8.f;
  const float kEpsilon = 2.5f / 256.f;
  for (float x = 0; x <= 1.f; x += kStep) {
    float a_of_x = SkTransferFnEval(a, x);
    float b_of_a_of_x = SkTransferFnEval(b, a_of_x);
    if (std::abs(b_of_a_of_x - x) > kEpsilon)
      return false;
  }
  return true;
}

bool SkTransferFnIsApproximatelyIdentity(const SkColorSpaceTransferFn& a) {
  const float kStep = 1.f / 8.f;
  const float kEpsilon = 2.5f / 256.f;
  for (float x = 0; x <= 1.f; x += kStep) {
    float a_of_x = SkTransferFnEval(a, x);
    if (std::abs(a_of_x - x) > kEpsilon)
      return false;
  }
  return true;
}

bool SkMatrixIsApproximatelyIdentity(const SkMatrix44& m) {
  const float kEpsilon = 1.f / 256.f;
  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 4; ++j) {
      float identity_value = i == j ? 1 : 0;
      float value = m.get(i, j);
      if (std::abs(identity_value - value) > kEpsilon)
        return false;
    }
  }
  return true;
}

}  // namespace gfx
