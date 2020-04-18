// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "build/build_config.h"
#include "cc/layers/solid_color_layer.h"
#include "cc/layers/texture_layer.h"
#include "cc/test/fake_picture_layer.h"
#include "cc/test/fake_picture_layer_impl.h"
#include "cc/test/layer_tree_pixel_test.h"
#include "cc/test/solid_color_content_layer_client.h"
#include "cc/trees/layer_tree_impl.h"
#include "components/viz/common/frame_sinks/copy_output_request.h"
#include "components/viz/common/frame_sinks/copy_output_result.h"
#include "components/viz/test/paths.h"
#include "components/viz/test/test_layer_tree_frame_sink.h"

#if !defined(OS_ANDROID)

namespace cc {
namespace {

// Can't templatize a class on its own members, so ReadbackType and
// ReadbackTestConfig are declared here, before LayerTreeHostReadbackPixelTest.
enum ReadbackType {
  READBACK_INVALID,
  READBACK_DEFAULT,
  READBACK_BITMAP,
};

struct ReadbackTestConfig {
  ReadbackTestConfig(LayerTreePixelTest::PixelTestType pixel_test_type_,
                     ReadbackType readback_type_)
      : pixel_test_type(pixel_test_type_), readback_type(readback_type_) {}
  LayerTreePixelTest::PixelTestType pixel_test_type;
  ReadbackType readback_type;
};

class LayerTreeHostReadbackPixelTest
    : public LayerTreePixelTest,
      public testing::WithParamInterface<ReadbackTestConfig> {
 protected:
  LayerTreeHostReadbackPixelTest()
      : readback_type_(READBACK_INVALID),
        insert_copy_request_after_frame_count_(0) {}

  void RunReadbackTest(PixelTestType test_type,
                       ReadbackType readback_type,
                       scoped_refptr<Layer> content_root,
                       base::FilePath file_name) {
    readback_type_ = readback_type;
    RunPixelTest(test_type, content_root, file_name);
  }

  void RunReadbackTestWithReadbackTarget(PixelTestType type,
                                         ReadbackType readback_type,
                                         scoped_refptr<Layer> content_root,
                                         Layer* target,
                                         base::FilePath file_name) {
    readback_type_ = readback_type;
    RunPixelTestWithReadbackTarget(type, content_root, target, file_name);
  }

  std::unique_ptr<viz::CopyOutputRequest> CreateCopyOutputRequest() override {
    std::unique_ptr<viz::CopyOutputRequest> request;

    if (readback_type_ == READBACK_BITMAP) {
      request = std::make_unique<viz::CopyOutputRequest>(
          viz::CopyOutputRequest::ResultFormat::RGBA_BITMAP,
          base::BindOnce(
              &LayerTreeHostReadbackPixelTest::ReadbackResultAsBitmap,
              base::Unretained(this)));
    } else {
      DCHECK_EQ(readback_type_, READBACK_DEFAULT);
      if (test_type_ == PIXEL_TEST_SOFTWARE) {
        request = std::make_unique<viz::CopyOutputRequest>(
            viz::CopyOutputRequest::ResultFormat::RGBA_BITMAP,
            base::BindOnce(
                &LayerTreeHostReadbackPixelTest::ReadbackResultAsBitmap,
                base::Unretained(this)));
      } else {
        DCHECK_EQ(test_type_, PIXEL_TEST_GL);
        request = std::make_unique<viz::CopyOutputRequest>(
            viz::CopyOutputRequest::ResultFormat::RGBA_TEXTURE,
            base::BindOnce(
                &LayerTreeHostReadbackPixelTest::ReadbackResultAsTexture,
                base::Unretained(this)));
      }
    }

    if (!copy_subrect_.IsEmpty())
      request->set_area(copy_subrect_);
    return request;
  }

  void BeginTest() override {
    if (insert_copy_request_after_frame_count_ == 0) {
      Layer* const target =
          readback_target_ ? readback_target_ : layer_tree_host()->root_layer();
      target->RequestCopyOfOutput(CreateCopyOutputRequest());
    }
    PostSetNeedsCommitToMainThread();
  }

  void DidCommitAndDrawFrame() override {
    if (insert_copy_request_after_frame_count_ ==
        layer_tree_host()->SourceFrameNumber()) {
      Layer* const target =
          readback_target_ ? readback_target_ : layer_tree_host()->root_layer();
      target->RequestCopyOfOutput(CreateCopyOutputRequest());
    }
  }

  void ReadbackResultAsBitmap(std::unique_ptr<viz::CopyOutputResult> result) {
    EXPECT_TRUE(task_runner_provider()->IsMainThread());
    EXPECT_FALSE(result->IsEmpty());
    result_bitmap_ = std::make_unique<SkBitmap>(result->AsSkBitmap());
    EXPECT_TRUE(result_bitmap_->readyToDraw());
    EndTest();
  }

  void ReadbackResultAsTexture(std::unique_ptr<viz::CopyOutputResult> result) {
    EXPECT_TRUE(task_runner_provider()->IsMainThread());
    ASSERT_EQ(result->format(), viz::CopyOutputResult::Format::RGBA_TEXTURE);

    gpu::Mailbox mailbox = result->GetTextureResult()->mailbox;
    gpu::SyncToken sync_token = result->GetTextureResult()->sync_token;
    EXPECT_EQ(result->GetTextureResult()->color_space, output_color_space_);
    std::unique_ptr<viz::SingleReleaseCallback> release_callback =
        result->TakeTextureOwnership();

    const SkBitmap bitmap =
        CopyMailboxToBitmap(result->size(), mailbox, sync_token);
    release_callback->Run(gpu::SyncToken(), false);

    ReadbackResultAsBitmap(std::make_unique<viz::CopyOutputSkBitmapResult>(
        result->rect(), bitmap));
  }

  ReadbackType readback_type_;
  gfx::Rect copy_subrect_;
  gfx::ColorSpace output_color_space_ = gfx::ColorSpace::CreateSRGB();
  int insert_copy_request_after_frame_count_;
};

TEST_P(LayerTreeHostReadbackPixelTest, ReadbackRootLayer) {
  scoped_refptr<SolidColorLayer> background =
      CreateSolidColorLayer(gfx::Rect(200, 200), SK_ColorWHITE);

  scoped_refptr<SolidColorLayer> green =
      CreateSolidColorLayer(gfx::Rect(200, 200), SK_ColorGREEN);
  background->AddChild(green);

  RunReadbackTest(GetParam().pixel_test_type, GetParam().readback_type,
                  background, base::FilePath(FILE_PATH_LITERAL("green.png")));
}

TEST_P(LayerTreeHostReadbackPixelTest, ReadbackRootLayerWithChild) {
  scoped_refptr<SolidColorLayer> background =
      CreateSolidColorLayer(gfx::Rect(200, 200), SK_ColorWHITE);

  scoped_refptr<SolidColorLayer> green =
      CreateSolidColorLayer(gfx::Rect(200, 200), SK_ColorGREEN);
  background->AddChild(green);

  scoped_refptr<SolidColorLayer> blue =
      CreateSolidColorLayer(gfx::Rect(150, 150, 50, 50), SK_ColorBLUE);
  green->AddChild(blue);

  RunReadbackTest(
      GetParam().pixel_test_type, GetParam().readback_type, background,
      base::FilePath(FILE_PATH_LITERAL("green_with_blue_corner.png")));
}

TEST_P(LayerTreeHostReadbackPixelTest, ReadbackNonRootLayer) {
  scoped_refptr<SolidColorLayer> background =
      CreateSolidColorLayer(gfx::Rect(200, 200), SK_ColorWHITE);

  scoped_refptr<SolidColorLayer> green =
      CreateSolidColorLayer(gfx::Rect(200, 200), SK_ColorGREEN);
  background->AddChild(green);

  RunReadbackTestWithReadbackTarget(
      GetParam().pixel_test_type, GetParam().readback_type, background,
      green.get(), base::FilePath(FILE_PATH_LITERAL("green.png")));
}

TEST_P(LayerTreeHostReadbackPixelTest, ReadbackSmallNonRootLayer) {
  scoped_refptr<SolidColorLayer> background =
      CreateSolidColorLayer(gfx::Rect(200, 200), SK_ColorWHITE);

  scoped_refptr<SolidColorLayer> green =
      CreateSolidColorLayer(gfx::Rect(100, 100, 100, 100), SK_ColorGREEN);
  background->AddChild(green);

  RunReadbackTestWithReadbackTarget(
      GetParam().pixel_test_type, GetParam().readback_type, background,
      green.get(), base::FilePath(FILE_PATH_LITERAL("green_small.png")));
}

TEST_P(LayerTreeHostReadbackPixelTest, ReadbackSmallNonRootLayerWithChild) {
  scoped_refptr<SolidColorLayer> background =
      CreateSolidColorLayer(gfx::Rect(200, 200), SK_ColorWHITE);

  scoped_refptr<SolidColorLayer> green =
      CreateSolidColorLayer(gfx::Rect(100, 100, 100, 100), SK_ColorGREEN);
  background->AddChild(green);

  scoped_refptr<SolidColorLayer> blue =
      CreateSolidColorLayer(gfx::Rect(50, 50, 50, 50), SK_ColorBLUE);
  green->AddChild(blue);

  RunReadbackTestWithReadbackTarget(
      GetParam().pixel_test_type, GetParam().readback_type, background,
      green.get(),
      base::FilePath(FILE_PATH_LITERAL("green_small_with_blue_corner.png")));
}

TEST_P(LayerTreeHostReadbackPixelTest, ReadbackSubtreeSurroundsTargetLayer) {
  scoped_refptr<SolidColorLayer> background =
      CreateSolidColorLayer(gfx::Rect(0, 0, 200, 200), SK_ColorWHITE);

  scoped_refptr<SolidColorLayer> target =
      CreateSolidColorLayer(gfx::Rect(100, 100, 100, 100), SK_ColorRED);
  background->AddChild(target);

  scoped_refptr<SolidColorLayer> green =
      CreateSolidColorLayer(gfx::Rect(-100, -100, 300, 300), SK_ColorGREEN);
  target->AddChild(green);

  scoped_refptr<SolidColorLayer> blue =
      CreateSolidColorLayer(gfx::Rect(50, 50, 50, 50), SK_ColorBLUE);
  target->AddChild(blue);

  copy_subrect_ = gfx::Rect(0, 0, 100, 100);
  RunReadbackTestWithReadbackTarget(
      GetParam().pixel_test_type, GetParam().readback_type, background,
      target.get(),
      base::FilePath(FILE_PATH_LITERAL("green_small_with_blue_corner.png")));
}

TEST_P(LayerTreeHostReadbackPixelTest,
       ReadbackSubtreeExtendsBeyondTargetLayer) {
  scoped_refptr<SolidColorLayer> background =
      CreateSolidColorLayer(gfx::Rect(0, 0, 200, 200), SK_ColorWHITE);

  scoped_refptr<SolidColorLayer> target =
      CreateSolidColorLayer(gfx::Rect(50, 50, 150, 150), SK_ColorRED);
  background->AddChild(target);

  scoped_refptr<SolidColorLayer> green =
      CreateSolidColorLayer(gfx::Rect(50, 50, 200, 200), SK_ColorGREEN);
  target->AddChild(green);

  scoped_refptr<SolidColorLayer> blue =
      CreateSolidColorLayer(gfx::Rect(100, 100, 50, 50), SK_ColorBLUE);
  target->AddChild(blue);

  copy_subrect_ = gfx::Rect(50, 50, 100, 100);
  RunReadbackTestWithReadbackTarget(
      GetParam().pixel_test_type, GetParam().readback_type, background,
      target.get(),
      base::FilePath(FILE_PATH_LITERAL("green_small_with_blue_corner.png")));
}

TEST_P(LayerTreeHostReadbackPixelTest, ReadbackHiddenSubtree) {
  scoped_refptr<SolidColorLayer> background =
      CreateSolidColorLayer(gfx::Rect(200, 200), SK_ColorBLACK);

  scoped_refptr<SolidColorLayer> hidden_target =
      CreateSolidColorLayer(gfx::Rect(200, 200), SK_ColorGREEN);
  hidden_target->SetHideLayerAndSubtree(true);
  background->AddChild(hidden_target);

  scoped_refptr<SolidColorLayer> blue =
      CreateSolidColorLayer(gfx::Rect(150, 150, 50, 50), SK_ColorBLUE);
  hidden_target->AddChild(blue);

  RunReadbackTestWithReadbackTarget(
      GetParam().pixel_test_type, GetParam().readback_type, background,
      hidden_target.get(),
      base::FilePath(FILE_PATH_LITERAL("green_with_blue_corner.png")));
}

TEST_P(LayerTreeHostReadbackPixelTest,
       HiddenSubtreeNotVisibleWhenDrawnForReadback) {
  scoped_refptr<SolidColorLayer> background =
      CreateSolidColorLayer(gfx::Rect(200, 200), SK_ColorBLACK);

  scoped_refptr<SolidColorLayer> hidden_target =
      CreateSolidColorLayer(gfx::Rect(200, 200), SK_ColorGREEN);
  hidden_target->SetHideLayerAndSubtree(true);
  background->AddChild(hidden_target);

  scoped_refptr<SolidColorLayer> blue =
      CreateSolidColorLayer(gfx::Rect(150, 150, 50, 50), SK_ColorBLUE);
  hidden_target->AddChild(blue);

  hidden_target->RequestCopyOfOutput(
      viz::CopyOutputRequest::CreateStubForTesting());
  RunReadbackTest(GetParam().pixel_test_type, GetParam().readback_type,
                  background, base::FilePath(FILE_PATH_LITERAL("black.png")));
}

TEST_P(LayerTreeHostReadbackPixelTest, ReadbackSubrect) {
  scoped_refptr<SolidColorLayer> background =
      CreateSolidColorLayer(gfx::Rect(200, 200), SK_ColorWHITE);

  scoped_refptr<SolidColorLayer> green =
      CreateSolidColorLayer(gfx::Rect(200, 200), SK_ColorGREEN);
  background->AddChild(green);

  scoped_refptr<SolidColorLayer> blue =
      CreateSolidColorLayer(gfx::Rect(100, 100, 50, 50), SK_ColorBLUE);
  green->AddChild(blue);

  // Grab the middle of the root layer.
  copy_subrect_ = gfx::Rect(50, 50, 100, 100);

  RunReadbackTest(
      GetParam().pixel_test_type, GetParam().readback_type, background,
      base::FilePath(FILE_PATH_LITERAL("green_small_with_blue_corner.png")));
}

TEST_P(LayerTreeHostReadbackPixelTest, ReadbackNonRootLayerSubrect) {
  scoped_refptr<SolidColorLayer> background =
      CreateSolidColorLayer(gfx::Rect(200, 200), SK_ColorWHITE);

  scoped_refptr<SolidColorLayer> green =
      CreateSolidColorLayer(gfx::Rect(25, 25, 150, 150), SK_ColorGREEN);
  background->AddChild(green);

  scoped_refptr<SolidColorLayer> blue =
      CreateSolidColorLayer(gfx::Rect(75, 75, 50, 50), SK_ColorBLUE);
  green->AddChild(blue);

  // Grab the middle of the green layer.
  copy_subrect_ = gfx::Rect(25, 25, 100, 100);

  RunReadbackTestWithReadbackTarget(
      GetParam().pixel_test_type, GetParam().readback_type, background,
      green.get(),
      base::FilePath(FILE_PATH_LITERAL("green_small_with_blue_corner.png")));
}

TEST_P(LayerTreeHostReadbackPixelTest, ReadbackWhenNoDamage) {
  scoped_refptr<SolidColorLayer> background =
      CreateSolidColorLayer(gfx::Rect(0, 0, 200, 200), SK_ColorWHITE);

  scoped_refptr<SolidColorLayer> parent =
      CreateSolidColorLayer(gfx::Rect(0, 0, 150, 150), SK_ColorRED);
  background->AddChild(parent);

  scoped_refptr<SolidColorLayer> target =
      CreateSolidColorLayer(gfx::Rect(0, 0, 100, 100), SK_ColorGREEN);
  parent->AddChild(target);

  scoped_refptr<SolidColorLayer> blue =
      CreateSolidColorLayer(gfx::Rect(50, 50, 50, 50), SK_ColorBLUE);
  target->AddChild(blue);

  insert_copy_request_after_frame_count_ = 1;
  RunReadbackTestWithReadbackTarget(
      GetParam().pixel_test_type, GetParam().readback_type, background,
      target.get(),
      base::FilePath(FILE_PATH_LITERAL("green_small_with_blue_corner.png")));
}

TEST_P(LayerTreeHostReadbackPixelTest, ReadbackOutsideViewportWhenNoDamage) {
  scoped_refptr<SolidColorLayer> background =
      CreateSolidColorLayer(gfx::Rect(0, 0, 200, 200), SK_ColorWHITE);

  scoped_refptr<SolidColorLayer> parent =
      CreateSolidColorLayer(gfx::Rect(0, 0, 200, 200), SK_ColorRED);
  EXPECT_FALSE(parent->masks_to_bounds());
  background->AddChild(parent);

  scoped_refptr<SolidColorLayer> target =
      CreateSolidColorLayer(gfx::Rect(250, 250, 100, 100), SK_ColorGREEN);
  parent->AddChild(target);

  scoped_refptr<SolidColorLayer> blue =
      CreateSolidColorLayer(gfx::Rect(50, 50, 50, 50), SK_ColorBLUE);
  target->AddChild(blue);

  insert_copy_request_after_frame_count_ = 1;
  RunReadbackTestWithReadbackTarget(
      GetParam().pixel_test_type, GetParam().readback_type, background,
      target.get(),
      base::FilePath(FILE_PATH_LITERAL("green_small_with_blue_corner.png")));
}

TEST_P(LayerTreeHostReadbackPixelTest, ReadbackNonRootLayerOutsideViewport) {
  scoped_refptr<SolidColorLayer> background =
      CreateSolidColorLayer(gfx::Rect(200, 200), SK_ColorWHITE);

  scoped_refptr<SolidColorLayer> green =
      CreateSolidColorLayer(gfx::Rect(200, 200), SK_ColorGREEN);
  // Only the top left quarter of the layer is inside the viewport, so the
  // blue layer is entirely outside.
  green->SetPosition(gfx::PointF(100.f, 100.f));
  background->AddChild(green);

  scoped_refptr<SolidColorLayer> blue =
      CreateSolidColorLayer(gfx::Rect(150, 150, 50, 50), SK_ColorBLUE);
  green->AddChild(blue);

  RunReadbackTestWithReadbackTarget(
      GetParam().pixel_test_type, GetParam().readback_type, background,
      green.get(),
      base::FilePath(FILE_PATH_LITERAL("green_with_blue_corner.png")));
}

TEST_P(LayerTreeHostReadbackPixelTest, ReadbackNonRootOrFirstLayer) {
  // This test has 3 render passes with the copy request on the render pass in
  // the middle. This test caught an issue where copy requests on non-root
  // non-first render passes were being treated differently from the first
  // render pass.
  scoped_refptr<SolidColorLayer> background =
      CreateSolidColorLayer(gfx::Rect(200, 200), SK_ColorGREEN);

  scoped_refptr<SolidColorLayer> blue =
      CreateSolidColorLayer(gfx::Rect(150, 150, 50, 50), SK_ColorBLUE);
  blue->RequestCopyOfOutput(viz::CopyOutputRequest::CreateStubForTesting());
  background->AddChild(blue);

  RunReadbackTestWithReadbackTarget(
      GetParam().pixel_test_type, GetParam().readback_type, background,
      background.get(),
      base::FilePath(FILE_PATH_LITERAL("green_with_blue_corner.png")));
}

TEST_P(LayerTreeHostReadbackPixelTest, MultipleReadbacksOnLayer) {
  // This test has 2 copy requests on the background layer. One is added in the
  // test body, another is added in RunReadbackTestWithReadbackTarget. For every
  // copy request after the first, state must be restored via a call to
  // UseRenderPass (see http://crbug.com/99393). This test ensures that the
  // renderer correctly handles cases where UseRenderPass is called multiple
  // times for a single layer.
  scoped_refptr<SolidColorLayer> background =
      CreateSolidColorLayer(gfx::Rect(200, 200), SK_ColorGREEN);

  background->RequestCopyOfOutput(
      viz::CopyOutputRequest::CreateStubForTesting());

  RunReadbackTestWithReadbackTarget(
      GetParam().pixel_test_type, GetParam().readback_type, background,
      background.get(), base::FilePath(FILE_PATH_LITERAL("green.png")));
}

INSTANTIATE_TEST_CASE_P(
    LayerTreeHostReadbackPixelTests,
    LayerTreeHostReadbackPixelTest,
    ::testing::Values(
        ReadbackTestConfig(LayerTreeHostReadbackPixelTest::PIXEL_TEST_SOFTWARE,
                           READBACK_DEFAULT),
        ReadbackTestConfig(LayerTreeHostReadbackPixelTest::PIXEL_TEST_GL,
                           READBACK_DEFAULT),
        ReadbackTestConfig(LayerTreeHostReadbackPixelTest::PIXEL_TEST_GL,
                           READBACK_BITMAP)));

class LayerTreeHostReadbackDeviceScalePixelTest
    : public LayerTreeHostReadbackPixelTest {
 protected:
  LayerTreeHostReadbackDeviceScalePixelTest()
      : device_scale_factor_(1.f),
        white_client_(SK_ColorWHITE, gfx::Size(200, 200)),
        green_client_(SK_ColorGREEN, gfx::Size(200, 200)),
        blue_client_(SK_ColorBLUE, gfx::Size(200, 200)) {}

  void InitializeSettings(LayerTreeSettings* settings) override {
    // Cause the device scale factor to be inherited by contents scales.
    settings->layer_transforms_should_scale_layer_contents = true;
  }

  void SetupTree() override {
    SetInitialDeviceScaleFactor(device_scale_factor_);
    LayerTreePixelTest::SetupTree();
  }

  void DrawLayersOnThread(LayerTreeHostImpl* host_impl) override {
    EXPECT_EQ(device_scale_factor_,
              host_impl->active_tree()->device_scale_factor());
  }

  float device_scale_factor_;
  SolidColorContentLayerClient white_client_;
  SolidColorContentLayerClient green_client_;
  SolidColorContentLayerClient blue_client_;
};

TEST_P(LayerTreeHostReadbackDeviceScalePixelTest, ReadbackSubrect) {
  scoped_refptr<FakePictureLayer> background =
      FakePictureLayer::Create(&white_client_);
  background->SetBounds(gfx::Size(100, 100));
  background->SetIsDrawable(true);

  scoped_refptr<FakePictureLayer> green =
      FakePictureLayer::Create(&green_client_);
  green->SetBounds(gfx::Size(100, 100));
  green->SetIsDrawable(true);
  background->AddChild(green);

  scoped_refptr<FakePictureLayer> blue =
      FakePictureLayer::Create(&blue_client_);
  blue->SetPosition(gfx::PointF(50.f, 50.f));
  blue->SetBounds(gfx::Size(25, 25));
  blue->SetIsDrawable(true);
  green->AddChild(blue);

  // Grab the middle of the root layer.
  copy_subrect_ = gfx::Rect(25, 25, 50, 50);
  device_scale_factor_ = 2.f;
  RunReadbackTest(
      GetParam().pixel_test_type, GetParam().readback_type, background,
      base::FilePath(FILE_PATH_LITERAL("green_small_with_blue_corner.png")));
}

TEST_P(LayerTreeHostReadbackDeviceScalePixelTest, ReadbackNonRootLayerSubrect) {
  scoped_refptr<FakePictureLayer> background =
      FakePictureLayer::Create(&white_client_);
  background->SetBounds(gfx::Size(100, 100));
  background->SetIsDrawable(true);

  scoped_refptr<FakePictureLayer> green =
      FakePictureLayer::Create(&green_client_);
  green->SetPosition(gfx::PointF(10.f, 20.f));
  green->SetBounds(gfx::Size(90, 80));
  green->SetIsDrawable(true);
  background->AddChild(green);

  scoped_refptr<FakePictureLayer> blue =
      FakePictureLayer::Create(&blue_client_);
  blue->SetPosition(gfx::PointF(50.f, 50.f));
  blue->SetBounds(gfx::Size(25, 25));
  blue->SetIsDrawable(true);
  green->AddChild(blue);

  // Grab the green layer's content with blue in the bottom right.
  copy_subrect_ = gfx::Rect(25, 25, 50, 50);
  device_scale_factor_ = 2.f;
  RunReadbackTestWithReadbackTarget(
      GetParam().pixel_test_type, GetParam().readback_type, background,
      green.get(),
      base::FilePath(FILE_PATH_LITERAL("green_small_with_blue_corner.png")));
}

INSTANTIATE_TEST_CASE_P(
    LayerTreeHostReadbackDeviceScalePixelTests,
    LayerTreeHostReadbackDeviceScalePixelTest,
    ::testing::Values(
        ReadbackTestConfig(LayerTreeHostReadbackPixelTest::PIXEL_TEST_SOFTWARE,
                           READBACK_DEFAULT),
        ReadbackTestConfig(LayerTreeHostReadbackPixelTest::PIXEL_TEST_GL,
                           READBACK_DEFAULT),
        ReadbackTestConfig(LayerTreeHostReadbackPixelTest::PIXEL_TEST_GL,
                           READBACK_BITMAP)));

class LayerTreeHostReadbackColorSpacePixelTest
    : public LayerTreeHostReadbackPixelTest {
 protected:
  LayerTreeHostReadbackColorSpacePixelTest()
      : green_client_(SK_ColorGREEN, gfx::Size(200, 200)) {
    output_color_space_ = gfx::ColorSpace::CreateDisplayP3D65();
  }

  std::unique_ptr<viz::TestLayerTreeFrameSink> CreateLayerTreeFrameSink(
      const viz::RendererSettings& renderer_settings,
      double refresh_rate,
      scoped_refptr<viz::ContextProvider> compositor_context_provider,
      scoped_refptr<viz::RasterContextProvider> worker_context_provider)
      override {
    std::unique_ptr<viz::TestLayerTreeFrameSink> frame_sink =
        LayerTreePixelTest::CreateLayerTreeFrameSink(
            renderer_settings, refresh_rate, compositor_context_provider,
            worker_context_provider);
    frame_sink->SetDisplayColorSpace(output_color_space_, output_color_space_);
    return frame_sink;
  }

  SolidColorContentLayerClient green_client_;
};

TEST_P(LayerTreeHostReadbackColorSpacePixelTest, Readback) {
  scoped_refptr<FakePictureLayer> background =
      FakePictureLayer::Create(&green_client_);
  background->SetBounds(gfx::Size(200, 200));
  background->SetIsDrawable(true);

  if (GetParam().pixel_test_type == PIXEL_TEST_SOFTWARE) {
    // Software compositing doesn't support color conversion, so the result will
    // come out in sRGB, regardless of the display's color properties.
    RunReadbackTest(GetParam().pixel_test_type, GetParam().readback_type,
                    background, base::FilePath(FILE_PATH_LITERAL("green.png")));
  } else {
    // GL compositing will convert the sRGB green into P3.
    RunReadbackTest(GetParam().pixel_test_type, GetParam().readback_type,
                    background,
                    base::FilePath(FILE_PATH_LITERAL("srgb_green_in_p3.png")));
  }
}

INSTANTIATE_TEST_CASE_P(LayerTreeHostReadbackColorSpacePixelTests,
                        LayerTreeHostReadbackColorSpacePixelTest,
                        ::testing::Values(ReadbackTestConfig(
                            LayerTreeHostReadbackPixelTest::PIXEL_TEST_GL,
                            READBACK_BITMAP)));

}  // namespace
}  // namespace cc

#endif  // OS_ANDROID
