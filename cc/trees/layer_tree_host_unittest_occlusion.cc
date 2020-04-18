// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/trees/layer_tree_host.h"

#include "cc/layers/layer.h"
#include "cc/layers/picture_layer.h"
#include "cc/test/fake_content_layer_client.h"
#include "cc/test/layer_test_common.h"
#include "cc/test/layer_tree_test.h"
#include "cc/trees/layer_tree_impl.h"

namespace cc {
namespace {

#define EXPECT_OCCLUSION_EQ(expected, actual)              \
  EXPECT_TRUE(expected.IsEqual(actual))                    \
      << " Expected: " << expected.ToString() << std::endl \
      << " Actual: " << actual.ToString();

class LayerTreeHostOcclusionTest : public LayerTreeTest {
 protected:
  void InitializeSettings(LayerTreeSettings* settings) override {
    settings->minimum_occlusion_tracking_size = gfx::Size();
  }
};

// Verify occlusion is set on the layer draw properties.
class LayerTreeHostOcclusionTestDrawPropertiesOnLayer
    : public LayerTreeHostOcclusionTest {
 public:
  void SetupTree() override {
    scoped_refptr<Layer> root = Layer::Create();
    root->SetBounds(gfx::Size(100, 100));
    root->SetIsDrawable(true);

    child_ = Layer::Create();
    child_->SetBounds(gfx::Size(50, 60));
    child_->SetPosition(gfx::PointF(10.f, 5.5f));
    child_->SetContentsOpaque(true);
    child_->SetIsDrawable(true);
    root->AddChild(child_);

    layer_tree_host()->SetRootLayer(root);
    LayerTreeTest::SetupTree();
  }

  void BeginTest() override { PostSetNeedsCommitToMainThread(); }

  void DrawLayersOnThread(LayerTreeHostImpl* impl) override {
    LayerImpl* root = impl->active_tree()->root_layer_for_testing();
    LayerImpl* child = impl->active_tree()->LayerById(child_->id());

    // Verify the draw properties are valid.
    EXPECT_TRUE(root->contributes_to_drawn_render_surface());
    EXPECT_TRUE(child->contributes_to_drawn_render_surface());

    EXPECT_OCCLUSION_EQ(
        Occlusion(child->DrawTransform(), SimpleEnclosedRegion(),
                  SimpleEnclosedRegion()),
        child->draw_properties().occlusion_in_content_space);
    EXPECT_OCCLUSION_EQ(
        Occlusion(root->DrawTransform(), SimpleEnclosedRegion(),
                  SimpleEnclosedRegion(gfx::Rect(10, 6, 50, 59))),
        root->draw_properties().occlusion_in_content_space);
    EndTest();
  }

  void AfterTest() override {}

 private:
  scoped_refptr<Layer> child_;
};

SINGLE_AND_MULTI_THREAD_TEST_F(LayerTreeHostOcclusionTestDrawPropertiesOnLayer);

// Verify occlusion is set on the render surfaces.
class LayerTreeHostOcclusionTestDrawPropertiesOnSurface
    : public LayerTreeHostOcclusionTest {
 public:
  void SetupTree() override {
    scoped_refptr<Layer> root = Layer::Create();
    root->SetBounds(gfx::Size(100, 100));
    root->SetIsDrawable(true);

    child_ = Layer::Create();
    child_->SetBounds(gfx::Size(1, 1));
    child_->SetPosition(gfx::PointF(10.f, 5.5f));
    child_->SetIsDrawable(true);
    child_->SetForceRenderSurfaceForTesting(true);
    root->AddChild(child_);

    scoped_refptr<Layer> child2 = Layer::Create();
    child2->SetBounds(gfx::Size(10, 12));
    child2->SetPosition(gfx::PointF(13.f, 8.5f));
    child2->SetContentsOpaque(true);
    child2->SetIsDrawable(true);
    root->AddChild(child2);

    layer_tree_host()->SetRootLayer(root);
    LayerTreeTest::SetupTree();
  }

  void BeginTest() override { PostSetNeedsCommitToMainThread(); }

  void DrawLayersOnThread(LayerTreeHostImpl* impl) override {
    LayerImpl* root = impl->active_tree()->root_layer_for_testing();
    LayerImpl* child = impl->active_tree()->LayerById(child_->id());
    RenderSurfaceImpl* surface = GetRenderSurface(child);

    // Verify the draw properties are valid.
    EXPECT_TRUE(root->contributes_to_drawn_render_surface());
    EXPECT_TRUE(child->contributes_to_drawn_render_surface());
    EXPECT_TRUE(GetRenderSurface(child));
    EXPECT_EQ(GetRenderSurface(child), child->render_target());

    EXPECT_OCCLUSION_EQ(
        Occlusion(surface->draw_transform(), SimpleEnclosedRegion(),
                  SimpleEnclosedRegion(gfx::Rect(13, 9, 10, 11))),
        surface->occlusion_in_content_space());
    EndTest();
  }

  void AfterTest() override {}

 private:
  scoped_refptr<Layer> child_;
};

SINGLE_AND_MULTI_THREAD_TEST_F(
    LayerTreeHostOcclusionTestDrawPropertiesOnSurface);

// Verify occlusion is set on mask layers.
class LayerTreeHostOcclusionTestDrawPropertiesOnMask
    : public LayerTreeHostOcclusionTest {
 public:
  void SetupTree() override {
    scoped_refptr<Layer> root = Layer::Create();
    root->SetBounds(gfx::Size(100, 100));
    root->SetIsDrawable(true);

    child_ = Layer::Create();
    child_->SetBounds(gfx::Size(30, 40));
    child_->SetPosition(gfx::PointF(10.f, 5.5f));
    child_->SetIsDrawable(true);
    root->AddChild(child_);

    scoped_refptr<Layer> make_surface_bigger = Layer::Create();
    make_surface_bigger->SetBounds(gfx::Size(100, 100));
    make_surface_bigger->SetPosition(gfx::PointF(-10.f, -15.f));
    make_surface_bigger->SetIsDrawable(true);
    child_->AddChild(make_surface_bigger);

    scoped_refptr<Layer> mask = PictureLayer::Create(&client_);
    mask->SetBounds(gfx::Size(30, 40));
    mask->SetIsDrawable(true);
    child_->SetMaskLayer(mask.get());

    scoped_refptr<Layer> child2 = Layer::Create();
    child2->SetBounds(gfx::Size(10, 12));
    child2->SetPosition(gfx::PointF(13.f, 8.5f));
    child2->SetContentsOpaque(true);
    child2->SetIsDrawable(true);
    root->AddChild(child2);

    layer_tree_host()->SetRootLayer(root);
    LayerTreeTest::SetupTree();
    client_.set_bounds(root->bounds());
  }

  void BeginTest() override { PostSetNeedsCommitToMainThread(); }

  void DrawLayersOnThread(LayerTreeHostImpl* impl) override {
    LayerImpl* root = impl->active_tree()->root_layer_for_testing();
    LayerImpl* child = impl->active_tree()->LayerById(child_->id());
    RenderSurfaceImpl* surface = GetRenderSurface(child);
    LayerImpl* mask = surface->MaskLayer();

    // Verify the draw properties are valid.
    EXPECT_TRUE(root->contributes_to_drawn_render_surface());
    EXPECT_TRUE(child->contributes_to_drawn_render_surface());
    EXPECT_TRUE(GetRenderSurface(child));
    EXPECT_EQ(GetRenderSurface(child), child->render_target());

    gfx::Transform transform = surface->draw_transform();
    transform.PreconcatTransform(child->DrawTransform());

    EXPECT_OCCLUSION_EQ(
        Occlusion(transform, SimpleEnclosedRegion(),
                  SimpleEnclosedRegion(gfx::Rect(13, 9, 10, 11))),
        mask->draw_properties().occlusion_in_content_space);
    EndTest();
  }

  void AfterTest() override {}

 private:
  FakeContentLayerClient client_;
  scoped_refptr<Layer> child_;
};

SINGLE_AND_MULTI_THREAD_TEST_F(LayerTreeHostOcclusionTestDrawPropertiesOnMask);

// Verify occlusion is correctly set on scaled mask layers.
class LayerTreeHostOcclusionTestDrawPropertiesOnScaledMask
    : public LayerTreeHostOcclusionTest {
 public:
  void SetupTree() override {
    scoped_refptr<Layer> root = Layer::Create();
    root->SetBounds(gfx::Size(100, 100));
    root->SetIsDrawable(true);

    gfx::Transform scale;
    scale.Scale(2, 2);

    child_ = Layer::Create();
    child_->SetBounds(gfx::Size(30, 40));
    child_->SetTransform(scale);
    root->AddChild(child_);

    scoped_refptr<Layer> grand_child = Layer::Create();
    grand_child->SetBounds(gfx::Size(100, 100));
    grand_child->SetPosition(gfx::PointF(-10.f, -15.f));
    grand_child->SetIsDrawable(true);
    child_->AddChild(grand_child);

    scoped_refptr<Layer> mask = PictureLayer::Create(&client_);
    mask->SetBounds(gfx::Size(30, 40));
    mask->SetIsDrawable(true);
    child_->SetMaskLayer(mask.get());

    scoped_refptr<Layer> child2 = Layer::Create();
    child2->SetBounds(gfx::Size(10, 11));
    child2->SetPosition(gfx::PointF(13.f, 15.f));
    child2->SetContentsOpaque(true);
    child2->SetIsDrawable(true);
    root->AddChild(child2);

    layer_tree_host()->SetRootLayer(root);
    LayerTreeTest::SetupTree();
    client_.set_bounds(root->bounds());
  }

  void BeginTest() override { PostSetNeedsCommitToMainThread(); }

  void DrawLayersOnThread(LayerTreeHostImpl* impl) override {
    LayerImpl* child = impl->active_tree()->LayerById(child_->id());
    LayerImpl* mask = GetRenderSurface(child)->MaskLayer();

    gfx::Transform scale;
    scale.Scale(2, 2);

    EXPECT_OCCLUSION_EQ(
        Occlusion(scale, SimpleEnclosedRegion(),
                  SimpleEnclosedRegion(gfx::Rect(13, 15, 10, 11))),
        mask->draw_properties().occlusion_in_content_space);
    EndTest();
  }

  void AfterTest() override {}

 private:
  FakeContentLayerClient client_;
  scoped_refptr<Layer> child_;
};

SINGLE_AND_MULTI_THREAD_TEST_F(
    LayerTreeHostOcclusionTestDrawPropertiesOnScaledMask);

}  // namespace
}  // namespace cc
