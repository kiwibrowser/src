// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/csspaint/paint_rendering_context_2d.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace blink {
namespace {

static const int kWidth = 50;
static const int kHeight = 75;
static const float kZoom = 1.0;

class PaintRenderingContext2DTest : public testing::Test {
 protected:
  void SetUp() override;

  Persistent<PaintRenderingContext2D> ctx_;
};

void PaintRenderingContext2DTest::SetUp() {
  PaintRenderingContext2DSettings context_settings;
  context_settings.setAlpha(false);
  ctx_ = PaintRenderingContext2D::Create(
      IntSize(kWidth, kHeight), CanvasColorParams(), context_settings, kZoom);
}

void TrySettingStrokeStyle(PaintRenderingContext2D* ctx,
                           const String& expected,
                           const String& value) {
  StringOrCanvasGradientOrCanvasPattern result, arg, dummy;
  dummy.SetString("red");
  arg.SetString(value);
  ctx->setStrokeStyle(dummy);
  ctx->setStrokeStyle(arg);
  ctx->strokeStyle(result);
  EXPECT_EQ(expected, result.GetAsString());
}

TEST_F(PaintRenderingContext2DTest, testParseColorOrCurrentColor) {
  TrySettingStrokeStyle(ctx_.Get(), "#0000ff", "blue");
  TrySettingStrokeStyle(ctx_.Get(), "#000000", "currentColor");
}

TEST_F(PaintRenderingContext2DTest, testWidthAndHeight) {
  EXPECT_EQ(kWidth, ctx_->Width());
  EXPECT_EQ(kHeight, ctx_->Height());
}

TEST_F(PaintRenderingContext2DTest, testBasicState) {
  const double kShadowBlurBefore = 2;
  const double kShadowBlurAfter = 3;

  const String line_join_before = "bevel";
  const String line_join_after = "round";

  ctx_->setShadowBlur(kShadowBlurBefore);
  ctx_->setLineJoin(line_join_before);
  EXPECT_EQ(kShadowBlurBefore, ctx_->shadowBlur());
  EXPECT_EQ(line_join_before, ctx_->lineJoin());

  ctx_->save();

  ctx_->setShadowBlur(kShadowBlurAfter);
  ctx_->setLineJoin(line_join_after);
  EXPECT_EQ(kShadowBlurAfter, ctx_->shadowBlur());
  EXPECT_EQ(line_join_after, ctx_->lineJoin());

  ctx_->restore();

  EXPECT_EQ(kShadowBlurBefore, ctx_->shadowBlur());
  EXPECT_EQ(line_join_before, ctx_->lineJoin());
}

}  // namespace
}  // namespace blink
