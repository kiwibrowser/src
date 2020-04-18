// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/canvas/canvas2d/canvas_rendering_context_2d.h"

#include <memory>
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/frame/local_frame_view.h"
#include "third_party/blink/renderer/core/frame/settings.h"
#include "third_party/blink/renderer/core/html/canvas/html_canvas_element.h"
#include "third_party/blink/renderer/core/html/canvas/image_data.h"
#include "third_party/blink/renderer/core/loader/empty_clients.h"
#include "third_party/blink/renderer/core/testing/page_test_base.h"
#include "third_party/blink/renderer/modules/accessibility/ax_object.h"
#include "third_party/blink/renderer/modules/accessibility/ax_object_cache_impl.h"
#include "third_party/blink/renderer/modules/canvas/canvas2d/canvas_gradient.h"
#include "third_party/blink/renderer/modules/canvas/canvas2d/canvas_pattern.h"
#include "third_party/blink/renderer/modules/canvas/canvas2d/hit_region_options.h"
#include "third_party/blink/renderer/modules/webgl/webgl_rendering_context.h"

using testing::Mock;

namespace blink {

class CanvasRenderingContext2DAPITest : public PageTestBase {
 protected:
  CanvasRenderingContext2DAPITest();
  void SetUp() override;

  HTMLCanvasElement& CanvasElement() const { return *canvas_element_; }
  CanvasRenderingContext2D* Context2d() const;

  void CreateContext(OpacityMode);

 private:
  Persistent<HTMLCanvasElement> canvas_element_;
};

CanvasRenderingContext2DAPITest::CanvasRenderingContext2DAPITest() = default;

CanvasRenderingContext2D* CanvasRenderingContext2DAPITest::Context2d() const {
  // If the following check fails, perhaps you forgot to call createContext
  // in your test?
  EXPECT_NE(nullptr, CanvasElement().RenderingContext());
  EXPECT_TRUE(CanvasElement().RenderingContext()->Is2d());
  return static_cast<CanvasRenderingContext2D*>(
      CanvasElement().RenderingContext());
}

void CanvasRenderingContext2DAPITest::CreateContext(OpacityMode opacity_mode) {
  String canvas_type("2d");
  CanvasContextCreationAttributesCore attributes;
  attributes.alpha = opacity_mode == kNonOpaque;
  canvas_element_->GetCanvasRenderingContext(canvas_type, attributes);
  Context2d();  // Calling this for the checks
}

void CanvasRenderingContext2DAPITest::SetUp() {
  PageTestBase::SetUp();
  GetDocument().documentElement()->SetInnerHTMLFromString(
      "<body><canvas id='c'></canvas></body>");
  GetDocument().View()->UpdateAllLifecyclePhases();
  canvas_element_ = ToHTMLCanvasElement(GetDocument().getElementById("c"));
}

TEST_F(CanvasRenderingContext2DAPITest, SetShadowColor_Clamping) {
  CreateContext(kNonOpaque);

  Context2d()->setShadowColor("rgba(0,0,0,0)");
  EXPECT_EQ(String("rgba(0, 0, 0, 0)"), Context2d()->shadowColor());
  Context2d()->setShadowColor("rgb(0,0,0)");
  EXPECT_EQ(String("#000000"), Context2d()->shadowColor());
  Context2d()->setShadowColor("rgb(0,999,0)");
  EXPECT_EQ(String("#00ff00"), Context2d()->shadowColor());
  Context2d()->setShadowColor(
      "rgb(0,"
      "999999999999999999999999999999999999999999999999999999999999999999999999"
      "999999999999999999999999999999999999999999999999999999999999999999999999"
      "999999999999999999999999999999999999999999999999999999999999999999999999"
      "999999999999999999999999999999999999999999999999999999999999999999999999"
      "999999999999999999999999999999999999999999999999999999999999999999999999"
      "999999999999999999999999999999999999999999999999999999999999999999999999"
      "999999999999999999999999999999999999999999999999999999999999999999999999"
      ",0)");
  EXPECT_EQ(String("#00ff00"), Context2d()->shadowColor());
  Context2d()->setShadowColor("rgb(0,0,256)");
  EXPECT_EQ(String("#0000ff"), Context2d()->shadowColor());
  Context2d()->setShadowColor(
      "rgb(999999999999999999999999,0,-9999999999999999999999999999)");
  EXPECT_EQ(String("#ff0000"), Context2d()->shadowColor());
  Context2d()->setShadowColor(
      "rgba("
      "999999999999999999999999999999999999999999999999999999999999999999999999"
      "9999999999,9,0,1)");
  EXPECT_EQ(String("#ff0900"), Context2d()->shadowColor());
  Context2d()->setShadowColor(
      "rgba("
      "999999999999999999999999999999999999999999999999999999999999999999999999"
      "9999999999,9,0,-99999999999999999999999999999999999999)");
  EXPECT_EQ(String("rgba(255, 9, 0, 0)"), Context2d()->shadowColor());
  Context2d()->setShadowColor(
      "rgba(7,"
      "999999999999999999999999999999999999999999999999999999999999999999999999"
      "9999999999,0,"
      "999999999999999999999999999999999999999999999999999999999999999999999999"
      "999999999999999999999999999999999999999999999999999999999999999999999999"
      "999999999999999999999999999999999999999999999999999999999999999999999999"
      "999999999999999999999999999999999999999999999999999999999999999999999999"
      "999999999999999999999999999999999999999999999999999999999999999999999999"
      "999999999999999999999999999999999999999999999999999999999999999999999999"
      "999999999999999999999999999999999999999999999999999999999999999999999999"
      "99999999999999999)");
  EXPECT_EQ(String("#07ff00"), Context2d()->shadowColor());
  Context2d()->setShadowColor(
      "rgba(-7,"
      "999999999999999999999999999999999999999999999999999999999999999999999999"
      "9999999999,0,"
      "999999999999999999999999999999999999999999999999999999999999999999999999"
      "999999999999999999999999999999999999999999999999999999999999999999999999"
      "999999999999999999999999999999999999999999999999999999999999999999999999"
      "999999999999999999999999999999999999999999999999999999999999999999999999"
      "999999999999999999999999999999999999999999999999999999999999999999999999"
      "999999999999999999999999999999999999999999999999999999999999999999999999"
      "999999999999999999999999999999999999999999999999999999999999999999999999"
      "99999999999999999)");
  EXPECT_EQ(String("#00ff00"), Context2d()->shadowColor());
  Context2d()->setShadowColor("rgba(0%,100%,0%,0.4)");
  EXPECT_EQ(String("rgba(0, 255, 0, 0.4)"), Context2d()->shadowColor());
}

String TrySettingStrokeStyle(CanvasRenderingContext2D* ctx,
                             const String& value) {
  StringOrCanvasGradientOrCanvasPattern arg1, arg2, arg3;
  arg1.SetString("#666");
  ctx->setStrokeStyle(arg1);
  arg2.SetString(value);
  ctx->setStrokeStyle(arg2);
  ctx->strokeStyle(arg3);
  EXPECT_TRUE(arg3.IsString());
  return arg3.GetAsString();
}

String TrySettingFillStyle(CanvasRenderingContext2D* ctx, const String& value) {
  StringOrCanvasGradientOrCanvasPattern arg1, arg2, arg3;
  arg1.SetString("#666");
  ctx->setFillStyle(arg1);
  arg2.SetString(value);
  ctx->setFillStyle(arg2);
  ctx->fillStyle(arg3);
  EXPECT_TRUE(arg3.IsString());
  return arg3.GetAsString();
}

String TrySettingShadowColor(CanvasRenderingContext2D* ctx,
                             const String& value) {
  ctx->setShadowColor("#666");
  ctx->setShadowColor(value);
  return ctx->shadowColor();
}

void TrySettingColor(CanvasRenderingContext2D* ctx,
                     const String& value,
                     const String& expected) {
  EXPECT_EQ(expected, TrySettingStrokeStyle(ctx, value));
  EXPECT_EQ(expected, TrySettingFillStyle(ctx, value));
  EXPECT_EQ(expected, TrySettingShadowColor(ctx, value));
}

TEST_F(CanvasRenderingContext2DAPITest, ColorSerialization) {
  CreateContext(kNonOpaque);
  // Check round trips
  TrySettingColor(Context2d(), "transparent", "rgba(0, 0, 0, 0)");
  TrySettingColor(Context2d(), "red", "#ff0000");
  TrySettingColor(Context2d(), "white", "#ffffff");
  TrySettingColor(Context2d(), "", "#666666");
  TrySettingColor(Context2d(), "RGBA(0, 0, 0, 0)", "rgba(0, 0, 0, 0)");
  TrySettingColor(Context2d(), "rgba(0,255,0,1.0)", "#00ff00");
  TrySettingColor(Context2d(), "rgba(1,2,3,0.4)", "rgba(1, 2, 3, 0.4)");
  TrySettingColor(Context2d(), "RgB(1,2,3)", "#010203");
  TrySettingColor(Context2d(), "rGbA(1,2,3,0)", "rgba(1, 2, 3, 0)");
}

TEST_F(CanvasRenderingContext2DAPITest, DefaultAttributeValues) {
  CreateContext(kNonOpaque);

  {
    StringOrCanvasGradientOrCanvasPattern value;
    Context2d()->strokeStyle(value);
    EXPECT_TRUE(value.IsString());
    EXPECT_EQ(String("#000000"), value.GetAsString());
  }

  {
    StringOrCanvasGradientOrCanvasPattern value;
    Context2d()->fillStyle(value);
    EXPECT_TRUE(value.IsString());
    EXPECT_EQ(String("#000000"), value.GetAsString());
  }

  EXPECT_EQ(String("rgba(0, 0, 0, 0)"), Context2d()->shadowColor());
}

TEST_F(CanvasRenderingContext2DAPITest, LineDashStateSave) {
  CreateContext(kNonOpaque);

  Vector<double> simple_dash;
  simple_dash.push_back(4);
  simple_dash.push_back(2);

  Context2d()->setLineDash(simple_dash);
  EXPECT_EQ(simple_dash, Context2d()->getLineDash());
  Context2d()->save();
  // Realize the save.
  Context2d()->scale(2, 2);
  EXPECT_EQ(simple_dash, Context2d()->getLineDash());
  Context2d()->restore();
  EXPECT_EQ(simple_dash, Context2d()->getLineDash());
}

TEST_F(CanvasRenderingContext2DAPITest, CreateImageData) {
  CreateContext(kNonOpaque);

  NonThrowableExceptionState exception_state;

  // create a 100x50 imagedata and fill it with white pixels
  ImageData* image_data =
      Context2d()->createImageData(100, 50, exception_state);
  EXPECT_FALSE(exception_state.HadException());
  EXPECT_EQ(100, image_data->width());
  EXPECT_EQ(50, image_data->height());

  for (unsigned i = 0; i < image_data->data()->length(); ++i)
    image_data->data()->Data()[i] = 255;

  EXPECT_EQ(255, image_data->data()->Data()[32]);

  // createImageData(imageData) should create a new ImageData of the same size
  // as 'imageData' but filled with transparent black

  ImageData* same_size_image_data =
      Context2d()->createImageData(image_data, exception_state);
  EXPECT_FALSE(exception_state.HadException());
  EXPECT_EQ(100, same_size_image_data->width());
  EXPECT_EQ(50, same_size_image_data->height());
  EXPECT_EQ(0, same_size_image_data->data()->Data()[32]);

  // createImageData(width, height) takes the absolute magnitude of the size
  // arguments

  ImageData* imgdata1 = Context2d()->createImageData(10, 20, exception_state);
  EXPECT_FALSE(exception_state.HadException());
  ImageData* imgdata2 = Context2d()->createImageData(-10, 20, exception_state);
  EXPECT_FALSE(exception_state.HadException());
  ImageData* imgdata3 = Context2d()->createImageData(10, -20, exception_state);
  EXPECT_FALSE(exception_state.HadException());
  ImageData* imgdata4 = Context2d()->createImageData(-10, -20, exception_state);
  EXPECT_FALSE(exception_state.HadException());

  EXPECT_EQ((unsigned)800, imgdata1->data()->length());
  EXPECT_EQ((unsigned)800, imgdata2->data()->length());
  EXPECT_EQ((unsigned)800, imgdata3->data()->length());
  EXPECT_EQ((unsigned)800, imgdata4->data()->length());
}

TEST_F(CanvasRenderingContext2DAPITest, CreateImageDataTooBig) {
  CreateContext(kNonOpaque);
  DummyExceptionStateForTesting exception_state;
  ImageData* too_big_image_data =
      Context2d()->createImageData(1000000, 1000000, exception_state);
  EXPECT_EQ(nullptr, too_big_image_data);
  EXPECT_TRUE(exception_state.HadException());
  EXPECT_EQ(kV8RangeError, exception_state.Code());
}

TEST_F(CanvasRenderingContext2DAPITest, GetImageDataTooBig) {
  CreateContext(kNonOpaque);
  DummyExceptionStateForTesting exception_state;
  ImageData* image_data =
      Context2d()->getImageData(0, 0, 1000000, 1000000, exception_state);
  EXPECT_EQ(nullptr, image_data);
  EXPECT_TRUE(exception_state.HadException());
  EXPECT_EQ(kV8RangeError, exception_state.Code());
}

TEST_F(CanvasRenderingContext2DAPITest,
       GetImageDataIntegerOverflowNegativeParams) {
  CreateContext(kNonOpaque);
  DummyExceptionStateForTesting exception_state;
  ImageData* image_data = Context2d()->getImageData(
      1, -2147483647, 1, -2147483647, exception_state);
  EXPECT_EQ(nullptr, image_data);
  EXPECT_TRUE(exception_state.HadException());
  EXPECT_EQ(kV8RangeError, exception_state.Code());

  exception_state.ClearException();
  image_data = Context2d()->getImageData(-2147483647, 1, -2147483647, 1,
                                         exception_state);
  EXPECT_EQ(nullptr, image_data);
  EXPECT_TRUE(exception_state.HadException());
  EXPECT_EQ(kV8RangeError, exception_state.Code());
}

void ResetCanvasForAccessibilityRectTest(Document& document) {
  document.documentElement()->SetInnerHTMLFromString(R"HTML(
    <canvas id='canvas' style='position:absolute; top:0px; left:0px;
    padding:10px; margin:5px;'>
    <button id='button'></button></canvas>
  )HTML");
  document.GetSettings()->SetAccessibilityEnabled(true);
  HTMLCanvasElement* canvas =
      ToHTMLCanvasElement(document.getElementById("canvas"));

  String canvas_type("2d");
  CanvasContextCreationAttributesCore attributes;
  attributes.alpha = true;
  canvas->GetCanvasRenderingContext(canvas_type, attributes);

  EXPECT_NE(nullptr, canvas->RenderingContext());
  EXPECT_TRUE(canvas->RenderingContext()->Is2d());
}

TEST_F(CanvasRenderingContext2DAPITest, AccessibilityRectTestForAddHitRegion) {
  ResetCanvasForAccessibilityRectTest(GetDocument());

  Element* button_element = GetDocument().getElementById("button");
  HTMLCanvasElement* canvas =
      ToHTMLCanvasElement(GetDocument().getElementById("canvas"));
  CanvasRenderingContext2D* context =
      static_cast<CanvasRenderingContext2D*>(canvas->RenderingContext());

  NonThrowableExceptionState exception_state;
  HitRegionOptions options;
  options.setControl(button_element);

  context->beginPath();
  context->rect(10, 10, 40, 40);
  context->addHitRegion(options, exception_state);

  AXObjectCacheImpl* ax_object_cache =
      ToAXObjectCacheImpl(GetDocument().ExistingAXObjectCache());
  AXObject* ax_object = ax_object_cache->GetOrCreate(button_element);

  LayoutRect ax_bounds = ax_object->GetBoundsInFrameCoordinates();
  EXPECT_EQ(25, ax_bounds.X().ToInt());
  EXPECT_EQ(25, ax_bounds.Y().ToInt());
  EXPECT_EQ(40, ax_bounds.Width().ToInt());
  EXPECT_EQ(40, ax_bounds.Height().ToInt());
}

TEST_F(CanvasRenderingContext2DAPITest,
       AccessibilityRectTestForDrawFocusIfNeeded) {
  ResetCanvasForAccessibilityRectTest(GetDocument());

  Element* button_element = GetDocument().getElementById("button");
  HTMLCanvasElement* canvas =
      ToHTMLCanvasElement(GetDocument().getElementById("canvas"));
  CanvasRenderingContext2D* context =
      static_cast<CanvasRenderingContext2D*>(canvas->RenderingContext());

  GetDocument().UpdateStyleAndLayoutTreeForNode(canvas);

  context->beginPath();
  context->rect(10, 10, 40, 40);
  context->drawFocusIfNeeded(button_element);

  AXObjectCacheImpl* ax_object_cache =
      ToAXObjectCacheImpl(GetDocument().ExistingAXObjectCache());
  AXObject* ax_object = ax_object_cache->GetOrCreate(button_element);

  LayoutRect ax_bounds = ax_object->GetBoundsInFrameCoordinates();
  EXPECT_EQ(25, ax_bounds.X().ToInt());
  EXPECT_EQ(25, ax_bounds.Y().ToInt());
  EXPECT_EQ(40, ax_bounds.Width().ToInt());
  EXPECT_EQ(40, ax_bounds.Height().ToInt());
}

}  // namespace blink
