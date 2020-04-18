// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/trees/layer_tree_impl.h"

#include "base/macros.h"
#include "cc/layers/heads_up_display_layer_impl.h"
#include "cc/test/fake_layer_tree_host_impl.h"
#include "cc/test/geometry_test_utils.h"
#include "cc/test/layer_test_common.h"
#include "cc/trees/clip_node.h"
#include "cc/trees/draw_property_utils.h"
#include "cc/trees/layer_tree_host_common.h"
#include "cc/trees/layer_tree_host_impl.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace cc {
namespace {

class LayerTreeImplTestSettings : public LayerTreeSettings {
 public:
  LayerTreeImplTestSettings() {
    layer_transforms_should_scale_layer_contents = true;
  }
};

class LayerTreeImplTest : public testing::Test {
 public:
  LayerTreeImplTest() : impl_test_(LayerTreeImplTestSettings()) {}

  FakeLayerTreeHostImpl& host_impl() const { return *impl_test_.host_impl(); }

  LayerImpl* root_layer() { return impl_test_.root_layer_for_testing(); }

  const RenderSurfaceList& GetRenderSurfaceList() const {
    return host_impl().active_tree()->GetRenderSurfaceList();
  }

  void ExecuteCalculateDrawProperties(LayerImpl* root_layer) {
    // We are probably not testing what is intended if the root_layer bounds are
    // empty.
    DCHECK(!root_layer->bounds().IsEmpty());

    render_surface_list_impl_.clear();
    LayerTreeHostCommon::CalcDrawPropsImplInputsForTesting inputs(
        root_layer, root_layer->bounds(), &render_surface_list_impl_);
    inputs.can_adjust_raster_scales = true;
    LayerTreeHostCommon::CalculateDrawPropertiesForTesting(&inputs);
  }

  int HitTestSimpleTree(int root_id,
                        int left_child_id,
                        int right_child_id,
                        int root_sorting_context,
                        int left_child_sorting_context,
                        int right_child_sorting_context,
                        float root_depth,
                        float left_child_depth,
                        float right_child_depth) {
    host_impl().active_tree()->SetRootLayerForTesting(nullptr);

    std::unique_ptr<LayerImpl> root =
        LayerImpl::Create(host_impl().active_tree(), root_id);
    std::unique_ptr<LayerImpl> left_child =
        LayerImpl::Create(host_impl().active_tree(), left_child_id);
    std::unique_ptr<LayerImpl> right_child =
        LayerImpl::Create(host_impl().active_tree(), right_child_id);

    gfx::Size bounds(100, 100);
    {
      gfx::Transform translate_z;
      translate_z.Translate3d(0, 0, root_depth);
      root->test_properties()->transform = translate_z;
      root->test_properties()->sorting_context_id = root_sorting_context;
      root->SetBounds(bounds);
      root->SetDrawsContent(true);
    }
    {
      gfx::Transform translate_z;
      translate_z.Translate3d(0, 0, left_child_depth);
      left_child->test_properties()->transform = translate_z;
      left_child->test_properties()->sorting_context_id =
          left_child_sorting_context;
      left_child->SetBounds(bounds);
      left_child->SetDrawsContent(true);
      left_child->test_properties()->should_flatten_transform = false;
    }
    {
      gfx::Transform translate_z;
      translate_z.Translate3d(0, 0, right_child_depth);
      right_child->test_properties()->transform = translate_z;
      right_child->test_properties()->sorting_context_id =
          right_child_sorting_context;
      right_child->SetBounds(bounds);
      right_child->SetDrawsContent(true);
    }

    root->test_properties()->AddChild(std::move(left_child));
    root->test_properties()->AddChild(std::move(right_child));

    host_impl().SetViewportSize(root->bounds());
    host_impl().active_tree()->SetRootLayerForTesting(std::move(root));
    host_impl().UpdateNumChildrenAndDrawPropertiesForActiveTree();
    CHECK_EQ(1u, GetRenderSurfaceList().size());

    gfx::PointF test_point = gfx::PointF(1.f, 1.f);
    LayerImpl* result_layer =
        host_impl().active_tree()->FindLayerThatIsHitByPoint(test_point);

    CHECK(result_layer);
    return result_layer->id();
  }

 private:
  LayerTestCommon::LayerImplTest impl_test_;
  RenderSurfaceList render_surface_list_impl_;
};

TEST_F(LayerTreeImplTest, HitTestingForSingleLayer) {
  gfx::Size bounds(100, 100);
  LayerImpl* root = root_layer();
  root->SetBounds(bounds);
  root->SetDrawsContent(true);

  host_impl().SetViewportSize(root->bounds());
  host_impl().UpdateNumChildrenAndDrawPropertiesForActiveTree();

  // Sanity check the scenario we just created.
  ASSERT_EQ(1u, GetRenderSurfaceList().size());
  ASSERT_EQ(1, GetRenderSurface(root_layer())->num_contributors());

  // Hit testing for a point outside the layer should return a null pointer.
  gfx::PointF test_point(101.f, 101.f);
  LayerImpl* result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPoint(test_point);
  EXPECT_FALSE(result_layer);

  test_point = gfx::PointF(-1.f, -1.f);
  result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPoint(test_point);
  EXPECT_FALSE(result_layer);

  // Hit testing for a point inside should return the root layer.
  test_point = gfx::PointF(1.f, 1.f);
  result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPoint(test_point);
  ASSERT_TRUE(result_layer);
  EXPECT_EQ(root->id(), result_layer->id());

  test_point = gfx::PointF(99.f, 99.f);
  result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPoint(test_point);
  ASSERT_TRUE(result_layer);
  EXPECT_EQ(root->id(), result_layer->id());
}

TEST_F(LayerTreeImplTest, UpdateViewportAndHitTest) {
  // Ensures that the viewport rect is correctly updated by the clip tree.
  gfx::Size bounds(100, 100);
  LayerImpl* root = root_layer();
  root->SetBounds(bounds);
  root->SetDrawsContent(true);

  host_impl().SetViewportSize(root->bounds());
  host_impl().UpdateNumChildrenAndDrawPropertiesForActiveTree();
  EXPECT_EQ(
      gfx::RectF(gfx::SizeF(bounds)),
      host_impl().active_tree()->property_trees()->clip_tree.ViewportClip());
  EXPECT_EQ(gfx::Rect(bounds), root->visible_layer_rect());

  gfx::Size new_bounds(50, 50);
  host_impl().SetViewportSize(new_bounds);
  gfx::PointF test_point(51.f, 51.f);
  host_impl().active_tree()->FindLayerThatIsHitByPoint(test_point);
  EXPECT_EQ(
      gfx::RectF(gfx::SizeF(new_bounds)),
      host_impl().active_tree()->property_trees()->clip_tree.ViewportClip());
  EXPECT_EQ(gfx::Rect(new_bounds), root->visible_layer_rect());
}

TEST_F(LayerTreeImplTest, HitTestingForSingleLayerAndHud) {
  LayerImpl* root = root_layer();
  root->SetBounds(gfx::Size(100, 100));
  root->SetDrawsContent(true);

  // Create hud and add it as a child of root.
  std::unique_ptr<HeadsUpDisplayLayerImpl> hud =
      HeadsUpDisplayLayerImpl::Create(host_impl().active_tree(), 11111);
  hud->SetBounds(gfx::Size(200, 200));
  hud->SetDrawsContent(true);

  host_impl().SetViewportSize(hud->bounds());
  host_impl().active_tree()->set_hud_layer(hud.get());
  root->test_properties()->AddChild(std::move(hud));
  host_impl().UpdateNumChildrenAndDrawPropertiesForActiveTree();

  // Sanity check the scenario we just created.
  ASSERT_EQ(1u, GetRenderSurfaceList().size());
  ASSERT_EQ(2, GetRenderSurface(root_layer())->num_contributors());

  // Hit testing for a point inside HUD, but outside root should return null
  gfx::PointF test_point(101.f, 101.f);
  LayerImpl* result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPoint(test_point);
  EXPECT_FALSE(result_layer);

  test_point = gfx::PointF(-1.f, -1.f);
  result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPoint(test_point);
  EXPECT_FALSE(result_layer);

  // Hit testing for a point inside should return the root layer, never the HUD
  // layer.
  test_point = gfx::PointF(1.f, 1.f);
  result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPoint(test_point);
  ASSERT_TRUE(result_layer);
  EXPECT_EQ(root->id(), result_layer->id());

  test_point = gfx::PointF(99.f, 99.f);
  result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPoint(test_point);
  ASSERT_TRUE(result_layer);
  EXPECT_EQ(root->id(), result_layer->id());
}

TEST_F(LayerTreeImplTest, HitTestingForUninvertibleTransform) {
  gfx::Transform uninvertible_transform;
  uninvertible_transform.matrix().set(0, 0, 0.0);
  uninvertible_transform.matrix().set(1, 1, 0.0);
  uninvertible_transform.matrix().set(2, 2, 0.0);
  uninvertible_transform.matrix().set(3, 3, 0.0);
  ASSERT_FALSE(uninvertible_transform.IsInvertible());

  LayerImpl* root = root_layer();
  root->test_properties()->transform = uninvertible_transform;
  root->SetBounds(gfx::Size(100, 100));
  root->SetDrawsContent(true);

  host_impl().SetViewportSize(root->bounds());
  host_impl().UpdateNumChildrenAndDrawPropertiesForActiveTree();
  // Sanity check the scenario we just created.
  ASSERT_EQ(1u, GetRenderSurfaceList().size());
  ASSERT_EQ(1, GetRenderSurface(root_layer())->num_contributors());
  ASSERT_FALSE(root_layer()->ScreenSpaceTransform().IsInvertible());

  // Hit testing any point should not hit the layer. If the invertible matrix is
  // accidentally ignored and treated like an identity, then the hit testing
  // will incorrectly hit the layer when it shouldn't.
  gfx::PointF test_point(1.f, 1.f);
  LayerImpl* result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPoint(test_point);
  EXPECT_FALSE(result_layer);

  test_point = gfx::PointF(10.f, 10.f);
  result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPoint(test_point);
  EXPECT_FALSE(result_layer);

  test_point = gfx::PointF(10.f, 30.f);
  result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPoint(test_point);
  EXPECT_FALSE(result_layer);

  test_point = gfx::PointF(50.f, 50.f);
  result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPoint(test_point);
  EXPECT_FALSE(result_layer);

  test_point = gfx::PointF(67.f, 48.f);
  result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPoint(test_point);
  EXPECT_FALSE(result_layer);

  test_point = gfx::PointF(99.f, 99.f);
  result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPoint(test_point);
  EXPECT_FALSE(result_layer);

  test_point = gfx::PointF(-1.f, -1.f);
  result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPoint(test_point);
  EXPECT_FALSE(result_layer);
}

TEST_F(LayerTreeImplTest, HitTestingForSinglePositionedLayer) {
  // This layer is positioned, and hit testing should correctly know where the
  // layer is located.
  LayerImpl* root = root_layer();
  root->SetPosition(gfx::PointF(50.f, 50.f));
  root->SetBounds(gfx::Size(100, 100));
  root->SetDrawsContent(true);

  host_impl().SetViewportSize(root->bounds());
  host_impl().UpdateNumChildrenAndDrawPropertiesForActiveTree();

  // Sanity check the scenario we just created.
  ASSERT_EQ(1u, GetRenderSurfaceList().size());
  ASSERT_EQ(1, GetRenderSurface(root_layer())->num_contributors());

  // Hit testing for a point outside the layer should return a null pointer.
  gfx::PointF test_point(49.f, 49.f);
  LayerImpl* result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPoint(test_point);
  EXPECT_FALSE(result_layer);

  // Even though the layer exists at (101, 101), it should not be visible there
  // since the root render surface would clamp it.
  test_point = gfx::PointF(101.f, 101.f);
  result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPoint(test_point);
  EXPECT_FALSE(result_layer);

  // Hit testing for a point inside should return the root layer.
  test_point = gfx::PointF(51.f, 51.f);
  result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPoint(test_point);
  ASSERT_TRUE(result_layer);
  EXPECT_EQ(root->id(), result_layer->id());

  test_point = gfx::PointF(99.f, 99.f);
  result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPoint(test_point);
  ASSERT_TRUE(result_layer);
  EXPECT_EQ(root->id(), result_layer->id());
}

TEST_F(LayerTreeImplTest, HitTestingForSingleRotatedLayer) {
  gfx::Transform rotation45_degrees_about_center;
  rotation45_degrees_about_center.Translate(50.0, 50.0);
  rotation45_degrees_about_center.RotateAboutZAxis(45.0);
  rotation45_degrees_about_center.Translate(-50.0, -50.0);

  LayerImpl* root = root_layer();
  root->test_properties()->transform = rotation45_degrees_about_center;
  root->SetBounds(gfx::Size(100, 100));
  root->SetDrawsContent(true);

  host_impl().SetViewportSize(root->bounds());
  host_impl().UpdateNumChildrenAndDrawPropertiesForActiveTree();

  // Sanity check the scenario we just created.
  ASSERT_EQ(1u, GetRenderSurfaceList().size());
  ASSERT_EQ(1, GetRenderSurface(root_layer())->num_contributors());

  // Hit testing for points outside the layer.
  // These corners would have been inside the un-transformed layer, but they
  // should not hit the correctly transformed layer.
  gfx::PointF test_point(99.f, 99.f);
  LayerImpl* result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPoint(test_point);
  EXPECT_FALSE(result_layer);

  test_point = gfx::PointF(1.f, 1.f);
  result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPoint(test_point);
  EXPECT_FALSE(result_layer);

  // Hit testing for a point inside should return the root layer.
  test_point = gfx::PointF(1.f, 50.f);
  result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPoint(test_point);
  ASSERT_TRUE(result_layer);
  EXPECT_EQ(root->id(), result_layer->id());

  // Hit testing the corners that would overlap the unclipped layer, but are
  // outside the clipped region.
  test_point = gfx::PointF(50.f, -1.f);
  result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPoint(test_point);
  ASSERT_FALSE(result_layer);

  test_point = gfx::PointF(-1.f, 50.f);
  result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPoint(test_point);
  ASSERT_FALSE(result_layer);
}

TEST_F(LayerTreeImplTest, HitTestingClipNodeDifferentTransformAndTargetIds) {
  // Tests hit testing on a layer whose clip node has different transform and
  // target id.
  LayerImpl* root = root_layer();
  root->SetBounds(gfx::Size(500, 500));

  gfx::Transform translation;
  translation.Translate(100, 100);
  std::unique_ptr<LayerImpl> render_surface =
      LayerImpl::Create(host_impl().active_tree(), 2);
  render_surface->test_properties()->transform = translation;
  render_surface->SetBounds(gfx::Size(100, 100));
  render_surface->test_properties()->force_render_surface = true;

  gfx::Transform scale_matrix;
  scale_matrix.Scale(2, 2);
  std::unique_ptr<LayerImpl> scale =
      LayerImpl::Create(host_impl().active_tree(), 3);
  scale->test_properties()->transform = scale_matrix;
  scale->SetBounds(gfx::Size(50, 50));

  std::unique_ptr<LayerImpl> clip =
      LayerImpl::Create(host_impl().active_tree(), 4);
  clip->SetBounds(gfx::Size(25, 25));
  clip->SetMasksToBounds(true);

  std::unique_ptr<LayerImpl> test =
      LayerImpl::Create(host_impl().active_tree(), 5);
  test->SetBounds(gfx::Size(100, 100));
  test->SetDrawsContent(true);

  clip->test_properties()->AddChild(std::move(test));
  scale->test_properties()->AddChild(std::move(clip));
  render_surface->test_properties()->AddChild(std::move(scale));
  root->test_properties()->AddChild(std::move(render_surface));

  host_impl().SetViewportSize(root->bounds());
  host_impl().UpdateNumChildrenAndDrawPropertiesForActiveTree();

  gfx::PointF test_point(160.f, 160.f);
  LayerImpl* result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPoint(test_point);
  EXPECT_FALSE(result_layer);

  test_point = gfx::PointF(140.f, 140.f);
  result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPoint(test_point);
  ASSERT_TRUE(result_layer);
  EXPECT_EQ(5, result_layer->id());
}

TEST_F(LayerTreeImplTest, HitTestingSiblings) {
  // This tests hit testing when the test point hits only one of the siblings.
  LayerImpl* root = root_layer();
  root->SetBounds(gfx::Size(100, 100));

  std::unique_ptr<LayerImpl> child1 =
      LayerImpl::Create(host_impl().active_tree(), 2);
  child1->SetBounds(gfx::Size(25, 25));
  child1->SetMasksToBounds(true);
  child1->SetDrawsContent(true);

  std::unique_ptr<LayerImpl> child2 =
      LayerImpl::Create(host_impl().active_tree(), 3);
  child2->SetBounds(gfx::Size(75, 75));
  child2->SetMasksToBounds(true);
  child2->SetDrawsContent(true);

  root->test_properties()->AddChild(std::move(child1));
  root->test_properties()->AddChild(std::move(child2));

  host_impl().SetViewportSize(root->bounds());
  host_impl().UpdateNumChildrenAndDrawPropertiesForActiveTree();

  gfx::PointF test_point(50.f, 50.f);
  LayerImpl* result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPoint(test_point);
  ASSERT_TRUE(result_layer);
  EXPECT_EQ(3, result_layer->id());
}

TEST_F(LayerTreeImplTest, HitTestingForSinglePerspectiveLayer) {
  // perspective_projection_about_center * translation_by_z is designed so that
  // the 100 x 100 layer becomes 50 x 50, and remains centered at (50, 50).
  gfx::Transform perspective_projection_about_center;
  perspective_projection_about_center.Translate(50.0, 50.0);
  perspective_projection_about_center.ApplyPerspectiveDepth(1.0);
  perspective_projection_about_center.Translate(-50.0, -50.0);
  gfx::Transform translation_by_z;
  translation_by_z.Translate3d(0.0, 0.0, -1.0);

  LayerImpl* root = root_layer();
  root->test_properties()->transform =
      (perspective_projection_about_center * translation_by_z);
  root->SetBounds(gfx::Size(100, 100));
  root->SetDrawsContent(true);

  host_impl().SetViewportSize(root->bounds());
  host_impl().UpdateNumChildrenAndDrawPropertiesForActiveTree();

  // Sanity check the scenario we just created.
  ASSERT_EQ(1u, GetRenderSurfaceList().size());
  ASSERT_EQ(1, GetRenderSurface(root_layer())->num_contributors());

  // Hit testing for points outside the layer.
  // These corners would have been inside the un-transformed layer, but they
  // should not hit the correctly transformed layer.
  gfx::PointF test_point(24.f, 24.f);
  LayerImpl* result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPoint(test_point);
  EXPECT_FALSE(result_layer);

  test_point = gfx::PointF(76.f, 76.f);
  result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPoint(test_point);
  EXPECT_FALSE(result_layer);

  // Hit testing for a point inside should return the root layer.
  test_point = gfx::PointF(26.f, 26.f);
  result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPoint(test_point);
  ASSERT_TRUE(result_layer);
  EXPECT_EQ(root->id(), result_layer->id());

  test_point = gfx::PointF(74.f, 74.f);
  result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPoint(test_point);
  ASSERT_TRUE(result_layer);
  EXPECT_EQ(root->id(), result_layer->id());
}

TEST_F(LayerTreeImplTest, HitTestingForSimpleClippedLayer) {
  // Test that hit-testing will only work for the visible portion of a layer,
  // and not the entire layer bounds. Here we just test the simple axis-aligned
  // case.
  LayerImpl* root = root_layer();
  root->SetBounds(gfx::Size(100, 100));
  {
    std::unique_ptr<LayerImpl> clipping_layer =
        LayerImpl::Create(host_impl().active_tree(), 123);
    // this layer is positioned, and hit testing should correctly know where the
    // layer is located.
    clipping_layer->SetPosition(gfx::PointF(25.f, 25.f));
    clipping_layer->SetBounds(gfx::Size(50, 50));
    clipping_layer->SetMasksToBounds(true);

    std::unique_ptr<LayerImpl> child =
        LayerImpl::Create(host_impl().active_tree(), 456);
    child->SetPosition(gfx::PointF(-50.f, -50.f));
    child->SetBounds(gfx::Size(300, 300));
    child->SetDrawsContent(true);
    clipping_layer->test_properties()->AddChild(std::move(child));
    root->test_properties()->AddChild(std::move(clipping_layer));
  }

  host_impl().SetViewportSize(root->bounds());
  host_impl().UpdateNumChildrenAndDrawPropertiesForActiveTree();

  // Sanity check the scenario we just created.
  ASSERT_EQ(1u, GetRenderSurfaceList().size());
  ASSERT_EQ(1, GetRenderSurface(root_layer())->num_contributors());
  LayerImpl* child_layer = host_impl().active_tree()->LayerById(456);
  EXPECT_TRUE(child_layer->contributes_to_drawn_render_surface());

  // Hit testing for a point outside the layer should return a null pointer.
  // Despite the child layer being very large, it should be clipped to the root
  // layer's bounds.
  gfx::PointF test_point(24.f, 24.f);
  LayerImpl* result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPoint(test_point);
  EXPECT_FALSE(result_layer);

  // Even though the layer exists at (101, 101), it should not be visible there
  // since the clipping_layer would clamp it.
  test_point = gfx::PointF(76.f, 76.f);
  result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPoint(test_point);
  EXPECT_FALSE(result_layer);

  // Hit testing for a point inside should return the child layer.
  test_point = gfx::PointF(26.f, 26.f);
  result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPoint(test_point);
  ASSERT_TRUE(result_layer);
  EXPECT_EQ(456, result_layer->id());

  test_point = gfx::PointF(74.f, 74.f);
  result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPoint(test_point);
  ASSERT_TRUE(result_layer);
  EXPECT_EQ(456, result_layer->id());
}

TEST_F(LayerTreeImplTest, HitTestingForMultiClippedRotatedLayer) {
  // This test checks whether hit testing correctly avoids hit testing with
  // multiple ancestors that clip in non axis-aligned ways. To pass this test,
  // the hit testing algorithm needs to recognize that multiple parent layers
  // may clip the layer, and should not actually hit those clipped areas.
  //
  // The child and grand_child layers are both initialized to clip the
  // rotated_leaf. The child layer is rotated about the top-left corner, so that
  // the root + child clips combined create a triangle. The rotated_leaf will
  // only be visible where it overlaps this triangle.
  //
  LayerImpl* root = root_layer();

  root->SetBounds(gfx::Size(100, 100));
  root->SetMasksToBounds(true);
  // Visible rects computed by combinig clips in target space and root space
  // don't match because of rotation transforms. So, we skip
  // verify_visible_rect_calculations.
  {
    std::unique_ptr<LayerImpl> child =
        LayerImpl::Create(host_impl().active_tree(), 456);
    std::unique_ptr<LayerImpl> grand_child =
        LayerImpl::Create(host_impl().active_tree(), 789);
    std::unique_ptr<LayerImpl> rotated_leaf =
        LayerImpl::Create(host_impl().active_tree(), 2468);

    child->SetPosition(gfx::PointF(10.f, 10.f));
    child->SetBounds(gfx::Size(80, 80));
    child->SetMasksToBounds(true);

    gfx::Transform rotation45_degrees_about_corner;
    rotation45_degrees_about_corner.RotateAboutZAxis(45.0);

    // This is positioned with respect to its parent which is already at
    // position (10, 10).
    // The size is to ensure it covers at least sqrt(2) * 100.
    grand_child->SetBounds(gfx::Size(200, 200));
    grand_child->test_properties()->transform = rotation45_degrees_about_corner;
    grand_child->SetMasksToBounds(true);

    // Rotates about the center of the layer
    gfx::Transform rotated_leaf_transform;
    rotated_leaf_transform.Translate(
        -10.0, -10.0);  // cancel out the grand_parent's position
    rotated_leaf_transform.RotateAboutZAxis(
        -45.0);  // cancel out the corner 45-degree rotation of the parent.
    rotated_leaf_transform.Translate(50.0, 50.0);
    rotated_leaf_transform.RotateAboutZAxis(45.0);
    rotated_leaf_transform.Translate(-50.0, -50.0);
    rotated_leaf->SetBounds(gfx::Size(100, 100));
    rotated_leaf->test_properties()->transform = rotated_leaf_transform;
    rotated_leaf->SetDrawsContent(true);

    grand_child->test_properties()->AddChild(std::move(rotated_leaf));
    child->test_properties()->AddChild(std::move(grand_child));
    root->test_properties()->AddChild(std::move(child));

    ExecuteCalculateDrawProperties(root);
  }

  host_impl().SetViewportSize(root->bounds());
  host_impl().UpdateNumChildrenAndDrawPropertiesForActiveTree();
  // (11, 89) is close to the the bottom left corner within the clip, but it is
  // not inside the layer.
  gfx::PointF test_point(11.f, 89.f);
  LayerImpl* result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPoint(test_point);
  EXPECT_FALSE(result_layer);

  // Closer inwards from the bottom left will overlap the layer.
  test_point = gfx::PointF(25.f, 75.f);
  result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPoint(test_point);
  ASSERT_TRUE(result_layer);
  EXPECT_EQ(2468, result_layer->id());

  // (4, 50) is inside the unclipped layer, but that corner of the layer should
  // be clipped away by the grandparent and should not get hit. If hit testing
  // blindly uses visible content rect without considering how parent may clip
  // the layer, then hit testing would accidentally think that the point
  // successfully hits the layer.
  test_point = gfx::PointF(4.f, 50.f);
  result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPoint(test_point);
  EXPECT_FALSE(result_layer);

  // (11, 50) is inside the layer and within the clipped area.
  test_point = gfx::PointF(11.f, 50.f);
  result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPoint(test_point);
  ASSERT_TRUE(result_layer);
  EXPECT_EQ(2468, result_layer->id());

  // Around the middle, just to the right and up, would have hit the layer
  // except that that area should be clipped away by the parent.
  test_point = gfx::PointF(51.f, 49.f);
  result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPoint(test_point);
  EXPECT_FALSE(result_layer);

  // Around the middle, just to the left and down, should successfully hit the
  // layer.
  test_point = gfx::PointF(49.f, 51.f);
  result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPoint(test_point);
  ASSERT_TRUE(result_layer);
  EXPECT_EQ(2468, result_layer->id());
}

TEST_F(LayerTreeImplTest, HitTestingForNonClippingIntermediateLayer) {
  // This test checks that hit testing code does not accidentally clip to layer
  // bounds for a layer that actually does not clip.

  LayerImpl* root = root_layer();
  root->SetBounds(gfx::Size(100, 100));
  {
    std::unique_ptr<LayerImpl> intermediate_layer =
        LayerImpl::Create(host_impl().active_tree(), 123);
    // this layer is positioned, and hit testing should correctly know where the
    // layer is located.
    intermediate_layer->SetPosition(gfx::PointF(10.f, 10.f));
    intermediate_layer->SetBounds(gfx::Size(50, 50));
    // Sanity check the intermediate layer should not clip.
    ASSERT_FALSE(intermediate_layer->masks_to_bounds());
    ASSERT_FALSE(intermediate_layer->test_properties()->mask_layer);

    // The child of the intermediate_layer is translated so that it does not
    // overlap intermediate_layer at all.  If child is incorrectly clipped, we
    // would not be able to hit it successfully.
    std::unique_ptr<LayerImpl> child =
        LayerImpl::Create(host_impl().active_tree(), 456);
    child->SetPosition(gfx::PointF(60.f, 60.f));  // 70, 70 in screen space
    child->SetBounds(gfx::Size(20, 20));
    child->SetDrawsContent(true);
    intermediate_layer->test_properties()->AddChild(std::move(child));
    root->test_properties()->AddChild(std::move(intermediate_layer));
  }

  host_impl().SetViewportSize(root->bounds());
  host_impl().UpdateNumChildrenAndDrawPropertiesForActiveTree();

  // Sanity check the scenario we just created.
  ASSERT_EQ(1u, GetRenderSurfaceList().size());
  ASSERT_EQ(1, GetRenderSurface(root_layer())->num_contributors());
  LayerImpl* child_layer = host_impl().active_tree()->LayerById(456);
  EXPECT_TRUE(child_layer->contributes_to_drawn_render_surface());

  // Hit testing for a point outside the layer should return a null pointer.
  gfx::PointF test_point(69.f, 69.f);
  LayerImpl* result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPoint(test_point);
  EXPECT_FALSE(result_layer);

  test_point = gfx::PointF(91.f, 91.f);
  result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPoint(test_point);
  EXPECT_FALSE(result_layer);

  // Hit testing for a point inside should return the child layer.
  test_point = gfx::PointF(71.f, 71.f);
  result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPoint(test_point);
  ASSERT_TRUE(result_layer);
  EXPECT_EQ(456, result_layer->id());

  test_point = gfx::PointF(89.f, 89.f);
  result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPoint(test_point);
  ASSERT_TRUE(result_layer);
  EXPECT_EQ(456, result_layer->id());
}

TEST_F(LayerTreeImplTest, HitTestingForMultipleLayers) {
  LayerImpl* root = root_layer();
  root->SetBounds(gfx::Size(100, 100));
  root->SetDrawsContent(true);
  {
    // child 1 and child2 are initialized to overlap between x=50 and x=60.
    // grand_child is set to overlap both child1 and child2 between y=50 and
    // y=60.  The expected stacking order is: (front) child2, (second)
    // grand_child, (third) child1, and (back) the root layer behind all other
    // layers.

    std::unique_ptr<LayerImpl> child1 =
        LayerImpl::Create(host_impl().active_tree(), 2);
    std::unique_ptr<LayerImpl> child2 =
        LayerImpl::Create(host_impl().active_tree(), 3);
    std::unique_ptr<LayerImpl> grand_child1 =
        LayerImpl::Create(host_impl().active_tree(), 4);

    child1->SetPosition(gfx::PointF(10.f, 10.f));
    child1->SetBounds(gfx::Size(50, 50));
    child1->SetDrawsContent(true);

    child2->SetPosition(gfx::PointF(50.f, 10.f));
    child2->SetBounds(gfx::Size(50, 50));
    child2->SetDrawsContent(true);

    // Remember that grand_child is positioned with respect to its parent (i.e.
    // child1).  In screen space, the intended position is (10, 50), with size
    // 100 x 50.
    grand_child1->SetPosition(gfx::PointF(0.f, 40.f));
    grand_child1->SetBounds(gfx::Size(100, 50));
    grand_child1->SetDrawsContent(true);

    child1->test_properties()->AddChild(std::move(grand_child1));
    root->test_properties()->AddChild(std::move(child1));
    root->test_properties()->AddChild(std::move(child2));

    ExecuteCalculateDrawProperties(root);
  }

  LayerImpl* child1 = root->test_properties()->children[0];
  LayerImpl* child2 = root->test_properties()->children[1];
  LayerImpl* grand_child1 = child1->test_properties()->children[0];

  host_impl().SetViewportSize(root->bounds());
  host_impl().UpdateNumChildrenAndDrawPropertiesForActiveTree();

  // Sanity check the scenario we just created.
  ASSERT_TRUE(child1);
  ASSERT_TRUE(child2);
  ASSERT_TRUE(grand_child1);
  ASSERT_EQ(1u, GetRenderSurfaceList().size());

  RenderSurfaceImpl* root_render_surface = GetRenderSurface(root);
  ASSERT_EQ(4, root_render_surface->num_contributors());
  EXPECT_TRUE(root_layer()->contributes_to_drawn_render_surface());
  EXPECT_TRUE(child1->contributes_to_drawn_render_surface());
  EXPECT_TRUE(child2->contributes_to_drawn_render_surface());
  EXPECT_TRUE(grand_child1->contributes_to_drawn_render_surface());

  // Nothing overlaps the root at (1, 1), so hit testing there should find
  // the root layer.
  gfx::PointF test_point = gfx::PointF(1.f, 1.f);
  LayerImpl* result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPoint(test_point);
  ASSERT_TRUE(result_layer);
  EXPECT_EQ(1, result_layer->id());

  // At (15, 15), child1 and root are the only layers. child1 is expected to be
  // on top.
  test_point = gfx::PointF(15.f, 15.f);
  result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPoint(test_point);
  ASSERT_TRUE(result_layer);
  EXPECT_EQ(2, result_layer->id());

  // At (51, 20), child1 and child2 overlap. child2 is expected to be on top.
  test_point = gfx::PointF(51.f, 20.f);
  result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPoint(test_point);
  ASSERT_TRUE(result_layer);
  EXPECT_EQ(3, result_layer->id());

  // At (80, 51), child2 and grand_child1 overlap. child2 is expected to be on
  // top.
  test_point = gfx::PointF(80.f, 51.f);
  result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPoint(test_point);
  ASSERT_TRUE(result_layer);
  EXPECT_EQ(3, result_layer->id());

  // At (51, 51), all layers overlap each other. child2 is expected to be on top
  // of all other layers.
  test_point = gfx::PointF(51.f, 51.f);
  result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPoint(test_point);
  ASSERT_TRUE(result_layer);
  EXPECT_EQ(3, result_layer->id());

  // At (20, 51), child1 and grand_child1 overlap. grand_child1 is expected to
  // be on top.
  test_point = gfx::PointF(20.f, 51.f);
  result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPoint(test_point);
  ASSERT_TRUE(result_layer);
  EXPECT_EQ(4, result_layer->id());
}

TEST_F(LayerTreeImplTest, HitTestingSameSortingContextTied) {
  int hit_layer_id = HitTestSimpleTree(/* ids */ 1, 2, 3,
                                       /* sorting_contexts */ 10, 10, 10,
                                       /* depths */ 0, 0, 0);
  // 3 is the last in tree order, and so should be on top.
  EXPECT_EQ(3, hit_layer_id);
}

TEST_F(LayerTreeImplTest, HitTestingSameSortingContextChildWins) {
  int hit_layer_id = HitTestSimpleTree(/* ids */ 1, 2, 3,
                                       /* sorting_contexts */ 10, 10, 10,
                                       /* depths */ 0, 1, 0);
  EXPECT_EQ(2, hit_layer_id);
}

TEST_F(LayerTreeImplTest, HitTestingWithoutSortingContext) {
  int hit_layer_id = HitTestSimpleTree(/* ids */ 1, 2, 3,
                                       /* sorting_contexts */ 0, 0, 0,
                                       /* depths */ 0, 1, 0);
  EXPECT_EQ(3, hit_layer_id);
}

TEST_F(LayerTreeImplTest, HitTestingDistinctSortingContext) {
  int hit_layer_id = HitTestSimpleTree(/* ids */ 1, 2, 3,
                                       /* sorting_contexts */ 10, 11, 12,
                                       /* depths */ 0, 1, 0);
  EXPECT_EQ(3, hit_layer_id);
}

TEST_F(LayerTreeImplTest, HitTestingSameSortingContextParentWins) {
  int hit_layer_id = HitTestSimpleTree(/* ids */ 1, 2, 3,
                                       /* sorting_contexts */ 10, 10, 10,
                                       /* depths */ 0, -1, -1);
  EXPECT_EQ(1, hit_layer_id);
}

TEST_F(LayerTreeImplTest, HitTestingForMultipleLayersAtVaryingDepths) {
  LayerImpl* root = root_layer();
  root->SetBounds(gfx::Size(100, 100));
  root->SetDrawsContent(true);
  root->test_properties()->should_flatten_transform = false;
  root->test_properties()->sorting_context_id = 1;
  {
    // child 1 and child2 are initialized to overlap between x=50 and x=60.
    // grand_child is set to overlap both child1 and child2 between y=50 and
    // y=60.  The expected stacking order is: (front) child2, (second)
    // grand_child, (third) child1, and (back) the root layer behind all other
    // layers.

    std::unique_ptr<LayerImpl> child1 =
        LayerImpl::Create(host_impl().active_tree(), 2);
    std::unique_ptr<LayerImpl> child2 =
        LayerImpl::Create(host_impl().active_tree(), 3);
    std::unique_ptr<LayerImpl> grand_child1 =
        LayerImpl::Create(host_impl().active_tree(), 4);

    child1->SetPosition(gfx::PointF(10.f, 10.f));
    child1->SetBounds(gfx::Size(50, 50));
    child1->SetDrawsContent(true);
    child1->test_properties()->should_flatten_transform = false;
    child1->test_properties()->sorting_context_id = 1;

    child2->SetPosition(gfx::PointF(50.f, 10.f));
    child2->SetBounds(gfx::Size(50, 50));
    gfx::Transform translate_z;
    translate_z.Translate3d(0, 0, 10.f);
    child2->test_properties()->transform = translate_z;
    child2->SetDrawsContent(true);
    child2->test_properties()->should_flatten_transform = false;
    child2->test_properties()->sorting_context_id = 1;

    // Remember that grand_child is positioned with respect to its parent (i.e.
    // child1).  In screen space, the intended position is (10, 50), with size
    // 100 x 50.
    grand_child1->SetPosition(gfx::PointF(0.f, 40.f));
    grand_child1->SetBounds(gfx::Size(100, 50));
    grand_child1->SetDrawsContent(true);
    grand_child1->test_properties()->should_flatten_transform = false;

    child1->test_properties()->AddChild(std::move(grand_child1));
    root->test_properties()->AddChild(std::move(child1));
    root->test_properties()->AddChild(std::move(child2));
  }

  LayerImpl* child1 = root->test_properties()->children[0];
  LayerImpl* child2 = root->test_properties()->children[1];
  LayerImpl* grand_child1 = child1->test_properties()->children[0];

  host_impl().SetViewportSize(root->bounds());
  host_impl().UpdateNumChildrenAndDrawPropertiesForActiveTree();

  // Sanity check the scenario we just created.
  ASSERT_TRUE(child1);
  ASSERT_TRUE(child2);
  ASSERT_TRUE(grand_child1);
  ASSERT_EQ(1u, GetRenderSurfaceList().size());

  // Nothing overlaps the root_layer at (1, 1), so hit testing there should find
  // the root layer.
  gfx::PointF test_point = gfx::PointF(1.f, 1.f);
  LayerImpl* result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPoint(test_point);
  ASSERT_TRUE(result_layer);
  EXPECT_EQ(1, result_layer->id());

  // At (15, 15), child1 and root are the only layers. child1 is expected to be
  // on top.
  test_point = gfx::PointF(15.f, 15.f);
  result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPoint(test_point);
  ASSERT_TRUE(result_layer);
  EXPECT_EQ(2, result_layer->id());

  // At (51, 20), child1 and child2 overlap. child2 is expected to be on top,
  // as it was transformed to the foreground.
  test_point = gfx::PointF(51.f, 20.f);
  result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPoint(test_point);
  ASSERT_TRUE(result_layer);
  EXPECT_EQ(3, result_layer->id());

  // At (80, 51), child2 and grand_child1 overlap. child2 is expected to
  // be on top, as it was transformed to the foreground.
  test_point = gfx::PointF(80.f, 51.f);
  result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPoint(test_point);
  ASSERT_TRUE(result_layer);
  EXPECT_EQ(3, result_layer->id());

  // At (51, 51), child1, child2 and grand_child1 overlap. child2 is expected to
  // be on top, as it was transformed to the foreground.
  test_point = gfx::PointF(51.f, 51.f);
  result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPoint(test_point);
  ASSERT_TRUE(result_layer);
  EXPECT_EQ(3, result_layer->id());

  // At (20, 51), child1 and grand_child1 overlap. grand_child1 is expected to
  // be on top, as it descends from child1.
  test_point = gfx::PointF(20.f, 51.f);
  result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPoint(test_point);
  ASSERT_TRUE(result_layer);
  EXPECT_EQ(4, result_layer->id());
}

TEST_F(LayerTreeImplTest, HitTestingRespectsClipParents) {
  LayerImpl* root = root_layer();
  root->SetBounds(gfx::Size(100, 100));
  root->SetDrawsContent(true);
  {
    std::unique_ptr<LayerImpl> child =
        LayerImpl::Create(host_impl().active_tree(), 2);
    std::unique_ptr<LayerImpl> grand_child =
        LayerImpl::Create(host_impl().active_tree(), 4);

    child->SetPosition(gfx::PointF(10.f, 10.f));
    child->SetBounds(gfx::Size(1, 1));
    child->SetDrawsContent(true);
    child->SetMasksToBounds(true);

    grand_child->SetPosition(gfx::PointF(0.f, 40.f));
    grand_child->SetBounds(gfx::Size(100, 50));
    grand_child->SetDrawsContent(true);
    grand_child->test_properties()->force_render_surface = true;

    // This should let |grand_child| "escape" |child|'s clip.
    grand_child->test_properties()->clip_parent = root;
    std::unique_ptr<std::set<LayerImpl*>> clip_children(
        new std::set<LayerImpl*>);
    clip_children->insert(grand_child.get());
    root->test_properties()->clip_children = std::move(clip_children);

    child->test_properties()->AddChild(std::move(grand_child));
    root->test_properties()->AddChild(std::move(child));
  }

  host_impl().SetViewportSize(root->bounds());
  host_impl().UpdateNumChildrenAndDrawPropertiesForActiveTree();

  gfx::PointF test_point(12.f, 52.f);
  LayerImpl* result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPoint(test_point);
  ASSERT_TRUE(result_layer);
  EXPECT_EQ(4, result_layer->id());
}

TEST_F(LayerTreeImplTest, HitTestingRespectsScrollParents) {
  LayerImpl* root = root_layer();
  root->SetBounds(gfx::Size(100, 100));
  root->SetDrawsContent(true);
  {
    std::unique_ptr<LayerImpl> child =
        LayerImpl::Create(host_impl().active_tree(), 2);
    std::unique_ptr<LayerImpl> scroll_child =
        LayerImpl::Create(host_impl().active_tree(), 3);
    std::unique_ptr<LayerImpl> grand_child =
        LayerImpl::Create(host_impl().active_tree(), 4);

    child->SetPosition(gfx::PointF(10.f, 10.f));
    child->SetBounds(gfx::Size(1, 1));
    child->SetDrawsContent(true);
    child->SetMasksToBounds(true);

    scroll_child->SetBounds(gfx::Size(200, 200));
    scroll_child->SetDrawsContent(true);

    // This should cause scroll child and its descendants to be affected by
    // |child|'s clip.
    scroll_child->test_properties()->scroll_parent = child.get();

    grand_child->SetBounds(gfx::Size(200, 200));
    grand_child->SetDrawsContent(true);
    grand_child->test_properties()->force_render_surface = true;

    scroll_child->test_properties()->AddChild(std::move(grand_child));
    root->test_properties()->AddChild(std::move(scroll_child));
    root->test_properties()->AddChild(std::move(child));
  }

  host_impl().SetViewportSize(root->bounds());
  host_impl().UpdateNumChildrenAndDrawPropertiesForActiveTree();

  gfx::PointF test_point(12.f, 52.f);
  LayerImpl* result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPoint(test_point);
  // The |test_point| should have been clipped away by |child|, the scroll
  // parent, so the only thing that should be hit is |root|.
  ASSERT_TRUE(result_layer);
  ASSERT_EQ(1, result_layer->id());
}
TEST_F(LayerTreeImplTest, HitTestingForMultipleLayerLists) {
  //
  // The geometry is set up similarly to the previous case, but
  // all layers are forced to be render surfaces now.
  //
  LayerImpl* root = root_layer();
  root->SetBounds(gfx::Size(100, 100));
  root->SetDrawsContent(true);
  {
    // child 1 and child2 are initialized to overlap between x=50 and x=60.
    // grand_child is set to overlap both child1 and child2 between y=50 and
    // y=60.  The expected stacking order is: (front) child2, (second)
    // grand_child, (third) child1, and (back) the root layer behind all other
    // layers.

    std::unique_ptr<LayerImpl> child1 =
        LayerImpl::Create(host_impl().active_tree(), 2);
    std::unique_ptr<LayerImpl> child2 =
        LayerImpl::Create(host_impl().active_tree(), 3);
    std::unique_ptr<LayerImpl> grand_child1 =
        LayerImpl::Create(host_impl().active_tree(), 4);

    child1->SetPosition(gfx::PointF(10.f, 10.f));
    child1->SetBounds(gfx::Size(50, 50));
    child1->SetDrawsContent(true);
    child1->test_properties()->force_render_surface = true;

    child2->SetPosition(gfx::PointF(50.f, 10.f));
    child2->SetBounds(gfx::Size(50, 50));
    child2->SetDrawsContent(true);
    child2->test_properties()->force_render_surface = true;

    // Remember that grand_child is positioned with respect to its parent (i.e.
    // child1).  In screen space, the intended position is (10, 50), with size
    // 100 x 50.
    grand_child1->SetPosition(gfx::PointF(0.f, 40.f));
    grand_child1->SetBounds(gfx::Size(100, 50));
    grand_child1->SetDrawsContent(true);
    grand_child1->test_properties()->force_render_surface = true;

    child1->test_properties()->AddChild(std::move(grand_child1));
    root->test_properties()->AddChild(std::move(child1));
    root->test_properties()->AddChild(std::move(child2));

    ExecuteCalculateDrawProperties(root);
  }

  LayerImpl* child1 = root->test_properties()->children[0];
  LayerImpl* child2 = root->test_properties()->children[1];
  LayerImpl* grand_child1 = child1->test_properties()->children[0];

  host_impl().SetViewportSize(root->bounds());
  host_impl().UpdateNumChildrenAndDrawPropertiesForActiveTree();

  // Sanity check the scenario we just created.
  ASSERT_TRUE(child1);
  ASSERT_TRUE(child2);
  ASSERT_TRUE(grand_child1);
  ASSERT_TRUE(GetRenderSurface(child1));
  ASSERT_TRUE(GetRenderSurface(child2));
  ASSERT_TRUE(GetRenderSurface(grand_child1));
  ASSERT_EQ(4u, GetRenderSurfaceList().size());
  // The root surface has the root layer, and child1's and child2's render
  // surfaces.
  ASSERT_EQ(3, GetRenderSurface(root)->num_contributors());
  // The child1 surface has the child1 layer and grand_child1's render surface.
  ASSERT_EQ(2, GetRenderSurface(child1)->num_contributors());
  ASSERT_EQ(1, GetRenderSurface(child2)->num_contributors());
  ASSERT_EQ(1, GetRenderSurface(grand_child1)->num_contributors());
  EXPECT_TRUE(root_layer()->contributes_to_drawn_render_surface());
  EXPECT_TRUE(child1->contributes_to_drawn_render_surface());
  EXPECT_TRUE(grand_child1->contributes_to_drawn_render_surface());
  EXPECT_TRUE(child2->contributes_to_drawn_render_surface());

  // Nothing overlaps the root at (1, 1), so hit testing there should find
  // the root layer.
  gfx::PointF test_point(1.f, 1.f);
  LayerImpl* result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPoint(test_point);
  ASSERT_TRUE(result_layer);
  EXPECT_EQ(1, result_layer->id());

  // At (15, 15), child1 and root are the only layers. child1 is expected to be
  // on top.
  test_point = gfx::PointF(15.f, 15.f);
  result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPoint(test_point);
  ASSERT_TRUE(result_layer);
  EXPECT_EQ(2, result_layer->id());

  // At (51, 20), child1 and child2 overlap. child2 is expected to be on top.
  test_point = gfx::PointF(51.f, 20.f);
  result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPoint(test_point);
  ASSERT_TRUE(result_layer);
  EXPECT_EQ(3, result_layer->id());

  // At (80, 51), child2 and grand_child1 overlap. child2 is expected to be on
  // top.
  test_point = gfx::PointF(80.f, 51.f);
  result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPoint(test_point);
  ASSERT_TRUE(result_layer);
  EXPECT_EQ(3, result_layer->id());

  // At (51, 51), all layers overlap each other. child2 is expected to be on top
  // of all other layers.
  test_point = gfx::PointF(51.f, 51.f);
  result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPoint(test_point);
  ASSERT_TRUE(result_layer);
  EXPECT_EQ(3, result_layer->id());

  // At (20, 51), child1 and grand_child1 overlap. grand_child1 is expected to
  // be on top.
  test_point = gfx::PointF(20.f, 51.f);
  result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPoint(test_point);
  ASSERT_TRUE(result_layer);
  EXPECT_EQ(4, result_layer->id());
}

TEST_F(LayerTreeImplTest, HitCheckingTouchHandlerRegionsForSingleLayer) {
  TouchActionRegion touch_action_region;
  touch_action_region.Union(kTouchActionNone, gfx::Rect(10, 10, 50, 50));

  LayerImpl* root = root_layer();
  root->SetBounds(gfx::Size(100, 100));
  root->SetDrawsContent(true);

  host_impl().SetViewportSize(root->bounds());
  host_impl().UpdateNumChildrenAndDrawPropertiesForActiveTree();

  // Sanity check the scenario we just created.
  ASSERT_EQ(1u, GetRenderSurfaceList().size());
  ASSERT_EQ(1, GetRenderSurface(root)->num_contributors());

  // Hit checking for any point should return a null pointer for a layer without
  // any touch event handler regions.
  gfx::PointF test_point(11.f, 11.f);
  LayerImpl* result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPointInTouchHandlerRegion(
          test_point);
  EXPECT_FALSE(result_layer);

  root->SetTouchActionRegion(touch_action_region);
  // Hit checking for a point outside the layer should return a null pointer.
  test_point = gfx::PointF(101.f, 101.f);
  result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPointInTouchHandlerRegion(
          test_point);
  EXPECT_FALSE(result_layer);

  test_point = gfx::PointF(-1.f, -1.f);
  result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPointInTouchHandlerRegion(
          test_point);
  EXPECT_FALSE(result_layer);

  // Hit checking for a point inside the layer, but outside the touch handler
  // region should return a null pointer.
  test_point = gfx::PointF(1.f, 1.f);
  result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPointInTouchHandlerRegion(
          test_point);
  EXPECT_FALSE(result_layer);

  test_point = gfx::PointF(99.f, 99.f);
  result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPointInTouchHandlerRegion(
          test_point);
  EXPECT_FALSE(result_layer);

  // Hit checking for a point inside the touch event handler region should
  // return the root layer.
  test_point = gfx::PointF(11.f, 11.f);
  result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPointInTouchHandlerRegion(
          test_point);
  ASSERT_TRUE(result_layer);
  EXPECT_EQ(root->id(), result_layer->id());

  test_point = gfx::PointF(59.f, 59.f);
  result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPointInTouchHandlerRegion(
          test_point);
  ASSERT_TRUE(result_layer);
  EXPECT_EQ(root->id(), result_layer->id());
}

TEST_F(LayerTreeImplTest,
       HitCheckingTouchHandlerRegionsForUninvertibleTransform) {
  gfx::Transform uninvertible_transform;
  uninvertible_transform.matrix().set(0, 0, 0.0);
  uninvertible_transform.matrix().set(1, 1, 0.0);
  uninvertible_transform.matrix().set(2, 2, 0.0);
  uninvertible_transform.matrix().set(3, 3, 0.0);
  ASSERT_FALSE(uninvertible_transform.IsInvertible());

  TouchActionRegion touch_action_region;
  touch_action_region.Union(kTouchActionNone, gfx::Rect(10, 10, 50, 50));

  LayerImpl* root = root_layer();
  root->test_properties()->transform = uninvertible_transform;
  root->SetBounds(gfx::Size(100, 100));
  root->SetDrawsContent(true);
  root->SetTouchActionRegion(touch_action_region);

  host_impl().SetViewportSize(root->bounds());
  host_impl().UpdateNumChildrenAndDrawPropertiesForActiveTree();

  // Sanity check the scenario we just created.
  ASSERT_EQ(1u, GetRenderSurfaceList().size());
  ASSERT_EQ(1, GetRenderSurface(root)->num_contributors());
  ASSERT_FALSE(root->ScreenSpaceTransform().IsInvertible());

  // Hit checking any point should not hit the touch handler region on the
  // layer. If the invertible matrix is accidentally ignored and treated like an
  // identity, then the hit testing will incorrectly hit the layer when it
  // shouldn't.
  gfx::PointF test_point(1.f, 1.f);
  LayerImpl* result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPointInTouchHandlerRegion(
          test_point);
  EXPECT_FALSE(result_layer);

  test_point = gfx::PointF(10.f, 10.f);
  result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPointInTouchHandlerRegion(
          test_point);
  EXPECT_FALSE(result_layer);

  test_point = gfx::PointF(10.f, 30.f);
  result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPointInTouchHandlerRegion(
          test_point);
  EXPECT_FALSE(result_layer);

  test_point = gfx::PointF(50.f, 50.f);
  result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPointInTouchHandlerRegion(
          test_point);
  EXPECT_FALSE(result_layer);

  test_point = gfx::PointF(67.f, 48.f);
  result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPointInTouchHandlerRegion(
          test_point);
  EXPECT_FALSE(result_layer);

  test_point = gfx::PointF(99.f, 99.f);
  result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPointInTouchHandlerRegion(
          test_point);
  EXPECT_FALSE(result_layer);

  test_point = gfx::PointF(-1.f, -1.f);
  result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPointInTouchHandlerRegion(
          test_point);
  EXPECT_FALSE(result_layer);
}

TEST_F(LayerTreeImplTest,
       HitCheckingTouchHandlerRegionsForSinglePositionedLayer) {
  TouchActionRegion touch_action_region;
  touch_action_region.Union(kTouchActionNone, gfx::Rect(10, 10, 50, 50));

  // This layer is positioned, and hit testing should correctly know where the
  // layer is located.
  LayerImpl* root = root_layer();
  root->SetPosition(gfx::PointF(50.f, 50.f));
  root->SetBounds(gfx::Size(100, 100));
  root->SetDrawsContent(true);
  root->SetTouchActionRegion(touch_action_region);

  host_impl().SetViewportSize(root->bounds());
  host_impl().UpdateNumChildrenAndDrawPropertiesForActiveTree();

  // Sanity check the scenario we just created.
  ASSERT_EQ(1u, GetRenderSurfaceList().size());
  ASSERT_EQ(1, GetRenderSurface(root)->num_contributors());

  // Hit checking for a point outside the layer should return a null pointer.
  gfx::PointF test_point(49.f, 49.f);
  LayerImpl* result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPointInTouchHandlerRegion(
          test_point);
  EXPECT_FALSE(result_layer);

  // Even though the layer has a touch handler region containing (101, 101), it
  // should not be visible there since the root render surface would clamp it.
  test_point = gfx::PointF(101.f, 101.f);
  result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPointInTouchHandlerRegion(
          test_point);
  EXPECT_FALSE(result_layer);

  // Hit checking for a point inside the layer, but outside the touch handler
  // region should return a null pointer.
  test_point = gfx::PointF(51.f, 51.f);
  result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPointInTouchHandlerRegion(
          test_point);
  EXPECT_FALSE(result_layer);

  // Hit checking for a point inside the touch event handler region should
  // return the root layer.
  test_point = gfx::PointF(61.f, 61.f);
  result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPointInTouchHandlerRegion(
          test_point);
  ASSERT_TRUE(result_layer);
  EXPECT_EQ(root->id(), result_layer->id());

  test_point = gfx::PointF(99.f, 99.f);
  result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPointInTouchHandlerRegion(
          test_point);
  ASSERT_TRUE(result_layer);
  EXPECT_EQ(root->id(), result_layer->id());
}

TEST_F(LayerTreeImplTest,
       HitCheckingTouchHandlerRegionsForSingleLayerWithDeviceScale) {
  // The layer's device_scale_factor and page_scale_factor should scale the
  // content rect and we should be able to hit the touch handler region by
  // scaling the points accordingly.

  // Set the bounds of the root layer big enough to fit the child when scaled.
  LayerImpl* root = root_layer();
  root->SetBounds(gfx::Size(100, 100));
  {
    TouchActionRegion touch_action_region;
    touch_action_region.Union(kTouchActionNone, gfx::Rect(10, 10, 30, 30));
    std::unique_ptr<LayerImpl> test_layer =
        LayerImpl::Create(host_impl().active_tree(), 12345);
    test_layer->SetPosition(gfx::PointF(25.f, 25.f));
    test_layer->SetBounds(gfx::Size(50, 50));
    test_layer->SetDrawsContent(true);
    test_layer->SetTouchActionRegion(touch_action_region);
    root->test_properties()->AddChild(std::move(test_layer));
  }

  float device_scale_factor = 3.f;
  float page_scale_factor = 5.f;
  float max_page_scale_factor = 10.f;
  gfx::Size scaled_bounds_for_root = gfx::ScaleToCeiledSize(
      root->bounds(), device_scale_factor * page_scale_factor);
  host_impl().SetViewportSize(scaled_bounds_for_root);

  host_impl().active_tree()->SetDeviceScaleFactor(device_scale_factor);
  LayerTreeImpl::ViewportLayerIds viewport_ids;
  viewport_ids.page_scale = 1;
  viewport_ids.inner_viewport_scroll = 1;
  host_impl().active_tree()->SetViewportLayersFromIds(viewport_ids);
  host_impl().active_tree()->BuildLayerListAndPropertyTreesForTesting();
  host_impl().active_tree()->PushPageScaleFromMainThread(
      page_scale_factor, page_scale_factor, max_page_scale_factor);
  host_impl().active_tree()->SetPageScaleOnActiveTree(page_scale_factor);
  host_impl().UpdateNumChildrenAndDrawPropertiesForActiveTree();

  // Sanity check the scenario we just created.
  // The visible content rect for test_layer is actually 100x100, even though
  // its layout size is 50x50, positioned at 25x25.
  LayerImpl* test_layer = root->test_properties()->children[0];
  ASSERT_EQ(1u, GetRenderSurfaceList().size());
  ASSERT_EQ(1, GetRenderSurface(root)->num_contributors());

  // Check whether the child layer fits into the root after scaled.
  EXPECT_EQ(gfx::Rect(test_layer->bounds()), test_layer->visible_layer_rect());

  // Hit checking for a point outside the layer should return a null pointer
  // (the root layer does not have a touch event handler, so it will not be
  // tested either).
  gfx::PointF test_point(76.f, 76.f);
  test_point =
      gfx::ScalePoint(test_point, device_scale_factor * page_scale_factor);
  LayerImpl* result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPointInTouchHandlerRegion(
          test_point);
  EXPECT_FALSE(result_layer);

  // Hit checking for a point inside the layer, but outside the touch handler
  // region should return a null pointer.
  test_point = gfx::PointF(26.f, 26.f);
  test_point =
      gfx::ScalePoint(test_point, device_scale_factor * page_scale_factor);
  result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPointInTouchHandlerRegion(
          test_point);
  EXPECT_FALSE(result_layer);

  test_point = gfx::PointF(34.f, 34.f);
  test_point =
      gfx::ScalePoint(test_point, device_scale_factor * page_scale_factor);
  result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPointInTouchHandlerRegion(
          test_point);
  EXPECT_FALSE(result_layer);

  test_point = gfx::PointF(65.f, 65.f);
  test_point =
      gfx::ScalePoint(test_point, device_scale_factor * page_scale_factor);
  result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPointInTouchHandlerRegion(
          test_point);
  EXPECT_FALSE(result_layer);

  test_point = gfx::PointF(74.f, 74.f);
  test_point =
      gfx::ScalePoint(test_point, device_scale_factor * page_scale_factor);
  result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPointInTouchHandlerRegion(
          test_point);
  EXPECT_FALSE(result_layer);

  // Hit checking for a point inside the touch event handler region should
  // return the root layer.
  test_point = gfx::PointF(35.f, 35.f);
  test_point =
      gfx::ScalePoint(test_point, device_scale_factor * page_scale_factor);
  result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPointInTouchHandlerRegion(
          test_point);
  ASSERT_TRUE(result_layer);
  EXPECT_EQ(12345, result_layer->id());

  test_point = gfx::PointF(64.f, 64.f);
  test_point =
      gfx::ScalePoint(test_point, device_scale_factor * page_scale_factor);
  result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPointInTouchHandlerRegion(
          test_point);
  ASSERT_TRUE(result_layer);
  EXPECT_EQ(12345, result_layer->id());

  // Check update of page scale factor on the active tree when page scale layer
  // is also the root layer.
  page_scale_factor *= 1.5f;
  host_impl().active_tree()->SetPageScaleOnActiveTree(page_scale_factor);
  EXPECT_EQ(root, host_impl().active_tree()->PageScaleLayer());

  test_point = gfx::PointF(35.f, 35.f);
  test_point =
      gfx::ScalePoint(test_point, device_scale_factor * page_scale_factor);
  result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPointInTouchHandlerRegion(
          test_point);
  ASSERT_TRUE(result_layer);
  EXPECT_EQ(12345, result_layer->id());

  test_point = gfx::PointF(64.f, 64.f);
  test_point =
      gfx::ScalePoint(test_point, device_scale_factor * page_scale_factor);
  result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPointInTouchHandlerRegion(
          test_point);
  ASSERT_TRUE(result_layer);
  EXPECT_EQ(12345, result_layer->id());
}

TEST_F(LayerTreeImplTest, HitCheckingTouchHandlerRegionsForSimpleClippedLayer) {
  // Test that hit-checking will only work for the visible portion of a layer,
  // and not the entire layer bounds. Here we just test the simple axis-aligned
  // case.
  LayerImpl* root = root_layer();
  root->SetBounds(gfx::Size(100, 100));
  {
    std::unique_ptr<LayerImpl> clipping_layer =
        LayerImpl::Create(host_impl().active_tree(), 123);
    // this layer is positioned, and hit testing should correctly know where the
    // layer is located.
    clipping_layer->SetPosition(gfx::PointF(25.f, 25.f));
    clipping_layer->SetBounds(gfx::Size(50, 50));
    clipping_layer->SetMasksToBounds(true);

    TouchActionRegion touch_action_region;
    touch_action_region.Union(kTouchActionNone, gfx::Rect(10, 10, 50, 50));

    std::unique_ptr<LayerImpl> child =
        LayerImpl::Create(host_impl().active_tree(), 456);
    child->SetPosition(gfx::PointF(-50.f, -50.f));
    child->SetBounds(gfx::Size(300, 300));
    child->SetDrawsContent(true);
    child->SetTouchActionRegion(touch_action_region);
    clipping_layer->test_properties()->AddChild(std::move(child));
    root->test_properties()->AddChild(std::move(clipping_layer));
  }

  host_impl().SetViewportSize(root->bounds());
  host_impl().UpdateNumChildrenAndDrawPropertiesForActiveTree();

  // Sanity check the scenario we just created.
  ASSERT_EQ(1u, GetRenderSurfaceList().size());
  ASSERT_EQ(1, GetRenderSurface(root)->num_contributors());
  LayerImpl* child_layer = host_impl().active_tree()->LayerById(456);
  EXPECT_TRUE(child_layer->contributes_to_drawn_render_surface());

  // Hit checking for a point outside the layer should return a null pointer.
  // Despite the child layer being very large, it should be clipped to the root
  // layer's bounds.
  gfx::PointF test_point(24.f, 24.f);
  LayerImpl* result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPointInTouchHandlerRegion(
          test_point);
  EXPECT_FALSE(result_layer);

  // Hit checking for a point inside the layer, but outside the touch handler
  // region should return a null pointer.
  test_point = gfx::PointF(35.f, 35.f);
  result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPointInTouchHandlerRegion(
          test_point);
  EXPECT_FALSE(result_layer);

  test_point = gfx::PointF(74.f, 74.f);
  result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPointInTouchHandlerRegion(
          test_point);
  EXPECT_FALSE(result_layer);

  // Hit checking for a point inside the touch event handler region should
  // return the root layer.
  test_point = gfx::PointF(25.f, 25.f);
  result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPointInTouchHandlerRegion(
          test_point);
  ASSERT_TRUE(result_layer);
  EXPECT_EQ(456, result_layer->id());

  test_point = gfx::PointF(34.f, 34.f);
  result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPointInTouchHandlerRegion(
          test_point);
  ASSERT_TRUE(result_layer);
  EXPECT_EQ(456, result_layer->id());
}

TEST_F(LayerTreeImplTest,
       HitCheckingTouchHandlerRegionsForClippedLayerWithDeviceScale) {
  // The layer's device_scale_factor and page_scale_factor should scale the
  // content rect and we should be able to hit the touch handler region by
  // scaling the points accordingly.

  // Set the bounds of the root layer big enough to fit the child when scaled.
  LayerImpl* root = root_layer();
  root->SetBounds(gfx::Size(100, 100));
  std::unique_ptr<LayerImpl> surface =
      LayerImpl::Create(host_impl().active_tree(), 2);
  surface->SetBounds(gfx::Size(100, 100));
  surface->test_properties()->force_render_surface = true;
  {
    std::unique_ptr<LayerImpl> clipping_layer =
        LayerImpl::Create(host_impl().active_tree(), 123);
    // This layer is positioned, and hit testing should correctly know where the
    // layer is located.
    clipping_layer->SetPosition(gfx::PointF(25.f, 20.f));
    clipping_layer->SetBounds(gfx::Size(50, 50));
    clipping_layer->SetMasksToBounds(true);

    TouchActionRegion touch_action_region;
    touch_action_region.Union(kTouchActionNone, gfx::Rect(0, 0, 300, 300));

    std::unique_ptr<LayerImpl> child =
        LayerImpl::Create(host_impl().active_tree(), 456);
    child->SetPosition(gfx::PointF(-50.f, -50.f));
    child->SetBounds(gfx::Size(300, 300));
    child->SetDrawsContent(true);
    child->SetTouchActionRegion(touch_action_region);
    clipping_layer->test_properties()->AddChild(std::move(child));
    surface->test_properties()->AddChild(std::move(clipping_layer));
    root->test_properties()->AddChild(std::move(surface));
  }

  float device_scale_factor = 3.f;
  float page_scale_factor = 1.f;
  float max_page_scale_factor = 1.f;
  gfx::Size scaled_bounds_for_root = gfx::ScaleToCeiledSize(
      root->bounds(), device_scale_factor * page_scale_factor);
  host_impl().SetViewportSize(scaled_bounds_for_root);

  host_impl().active_tree()->SetDeviceScaleFactor(device_scale_factor);
  LayerTreeImpl::ViewportLayerIds viewport_ids;
  viewport_ids.page_scale = 1;
  viewport_ids.inner_viewport_scroll = 1;
  host_impl().active_tree()->SetViewportLayersFromIds(viewport_ids);
  host_impl().active_tree()->BuildLayerListAndPropertyTreesForTesting();
  host_impl().active_tree()->PushPageScaleFromMainThread(
      page_scale_factor, page_scale_factor, max_page_scale_factor);
  host_impl().active_tree()->SetPageScaleOnActiveTree(page_scale_factor);
  host_impl().UpdateNumChildrenAndDrawPropertiesForActiveTree();

  // Sanity check the scenario we just created.
  ASSERT_EQ(2u, GetRenderSurfaceList().size());

  // Hit checking for a point outside the layer should return a null pointer.
  // Despite the child layer being very large, it should be clipped to the root
  // layer's bounds.
  gfx::PointF test_point(24.f, 24.f);
  test_point =
      gfx::ScalePoint(test_point, device_scale_factor * page_scale_factor);
  LayerImpl* result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPointInTouchHandlerRegion(
          test_point);
  EXPECT_FALSE(result_layer);

  // Hit checking for a point inside the touch event handler region should
  // return the child layer.
  test_point = gfx::PointF(25.f, 25.f);
  test_point =
      gfx::ScalePoint(test_point, device_scale_factor * page_scale_factor);
  result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPointInTouchHandlerRegion(
          test_point);
  ASSERT_TRUE(result_layer);
  EXPECT_EQ(456, result_layer->id());
}

TEST_F(LayerTreeImplTest, HitCheckingTouchHandlerOverlappingRegions) {
  LayerImpl* root = root_layer();
  root->SetBounds(gfx::Size(100, 100));
  {
    std::unique_ptr<LayerImpl> touch_layer =
        LayerImpl::Create(host_impl().active_tree(), 123);
    // this layer is positioned, and hit testing should correctly know where the
    // layer is located.
    touch_layer->SetBounds(gfx::Size(50, 50));
    touch_layer->SetDrawsContent(true);
    TouchActionRegion touch_action_region;
    touch_action_region.Union(kTouchActionNone, gfx::Rect(0, 0, 50, 50));
    touch_layer->SetTouchActionRegion(touch_action_region);
    root->test_properties()->AddChild(std::move(touch_layer));
  }

  {
    std::unique_ptr<LayerImpl> notouch_layer =
        LayerImpl::Create(host_impl().active_tree(), 1234);
    // this layer is positioned, and hit testing should correctly know where the
    // layer is located.
    notouch_layer->SetPosition(gfx::PointF(0, 25));
    notouch_layer->SetBounds(gfx::Size(50, 50));
    notouch_layer->SetDrawsContent(true);
    root->test_properties()->AddChild(std::move(notouch_layer));
  }

  host_impl().SetViewportSize(root->bounds());
  host_impl().UpdateNumChildrenAndDrawPropertiesForActiveTree();

  // Sanity check the scenario we just created.
  ASSERT_EQ(1u, GetRenderSurfaceList().size());
  ASSERT_EQ(2, GetRenderSurface(root)->num_contributors());
  LayerImpl* touch_layer = host_impl().active_tree()->LayerById(123);
  LayerImpl* notouch_layer = host_impl().active_tree()->LayerById(1234);
  EXPECT_TRUE(touch_layer->contributes_to_drawn_render_surface());
  EXPECT_TRUE(notouch_layer->contributes_to_drawn_render_surface());

  gfx::PointF test_point(35.f, 35.f);
  LayerImpl* result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPointInTouchHandlerRegion(
          test_point);

  // We should have passed through the no-touch layer and found the layer
  // behind it.
  EXPECT_TRUE(result_layer);

  host_impl().active_tree()->LayerById(1234)->SetContentsOpaque(true);
  result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPointInTouchHandlerRegion(
          test_point);

  // Even with an opaque layer in the middle, we should still find the layer
  // with
  // the touch handler behind it (since we can't assume that opaque layers are
  // opaque to hit testing).
  EXPECT_TRUE(result_layer);

  test_point = gfx::PointF(35.f, 15.f);
  result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPointInTouchHandlerRegion(
          test_point);
  ASSERT_TRUE(result_layer);
  EXPECT_EQ(123, result_layer->id());

  test_point = gfx::PointF(35.f, 65.f);
  result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPointInTouchHandlerRegion(
          test_point);
  EXPECT_FALSE(result_layer);
}

TEST_F(LayerTreeImplTest, HitTestingTouchHandlerRegionsForLayerThatIsNotDrawn) {
  LayerImpl* root = root_layer();
  root->SetBounds(gfx::Size(100, 100));
  root->SetDrawsContent(true);
  {
    TouchActionRegion touch_action_region;
    touch_action_region.Union(kTouchActionNone, gfx::Rect(10, 10, 30, 30));
    std::unique_ptr<LayerImpl> test_layer =
        LayerImpl::Create(host_impl().active_tree(), 12345);
    test_layer->SetBounds(gfx::Size(50, 50));
    test_layer->SetDrawsContent(false);
    test_layer->SetTouchActionRegion(touch_action_region);
    root->test_properties()->AddChild(std::move(test_layer));
  }
  host_impl().SetViewportSize(root->bounds());
  host_impl().UpdateNumChildrenAndDrawPropertiesForActiveTree();

  LayerImpl* test_layer = root->test_properties()->children[0];
  // As test_layer doesn't draw content, it shouldn't contribute content to the
  // root surface.
  ASSERT_EQ(1u, GetRenderSurfaceList().size());
  EXPECT_FALSE(test_layer->contributes_to_drawn_render_surface());

  // Hit testing for a point outside the test layer should return null pointer.
  // We also implicitly check that the updated screen space transform of a layer
  // that is not in drawn render surface layer list (test_layer) is used during
  // hit testing (becuase the point is inside test_layer with respect to the old
  // screen space transform).
  gfx::PointF test_point(24.f, 24.f);
  test_layer->SetPosition(gfx::PointF(25.f, 25.f));
  gfx::Transform expected_screen_space_transform;
  expected_screen_space_transform.Translate(25.f, 25.f);

  host_impl().active_tree()->property_trees()->needs_rebuild = true;
  host_impl().active_tree()->BuildLayerListAndPropertyTreesForTesting();
  LayerImpl* result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPointInTouchHandlerRegion(
          test_point);
  EXPECT_FALSE(result_layer);
  EXPECT_FALSE(test_layer->contributes_to_drawn_render_surface());
  EXPECT_TRANSFORMATION_MATRIX_EQ(
      expected_screen_space_transform,
      draw_property_utils::ScreenSpaceTransform(
          test_layer,
          host_impl().active_tree()->property_trees()->transform_tree));

  // We change the position of the test layer such that the test point is now
  // inside the test_layer.
  test_layer = root->test_properties()->children[0];
  test_layer->SetPosition(gfx::PointF(10.f, 10.f));
  test_layer->NoteLayerPropertyChanged();
  expected_screen_space_transform.MakeIdentity();
  expected_screen_space_transform.Translate(10.f, 10.f);

  host_impl().active_tree()->property_trees()->needs_rebuild = true;
  host_impl().active_tree()->BuildLayerListAndPropertyTreesForTesting();
  result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPointInTouchHandlerRegion(
          test_point);
  ASSERT_TRUE(result_layer);
  ASSERT_EQ(test_layer, result_layer);
  EXPECT_FALSE(result_layer->contributes_to_drawn_render_surface());
  EXPECT_TRANSFORMATION_MATRIX_EQ(
      expected_screen_space_transform,
      draw_property_utils::ScreenSpaceTransform(
          test_layer,
          host_impl().active_tree()->property_trees()->transform_tree));
}

TEST_F(LayerTreeImplTest, SelectionBoundsForSingleLayer) {
  LayerImpl* root = root_layer();
  root->SetBounds(gfx::Size(100, 100));
  root->SetDrawsContent(true);

  host_impl().SetViewportSize(root->bounds());
  host_impl().UpdateNumChildrenAndDrawPropertiesForActiveTree();

  // Sanity check the scenario we just created.
  ASSERT_EQ(1u, GetRenderSurfaceList().size());
  ASSERT_EQ(1, GetRenderSurface(root)->num_contributors());

  LayerSelection input;

  input.start.type = gfx::SelectionBound::LEFT;
  input.start.edge_top = gfx::Point(10, 10);
  input.start.edge_bottom = gfx::Point(10, 20);
  input.start.layer_id = root->id();

  input.end.type = gfx::SelectionBound::RIGHT;
  input.end.edge_top = gfx::Point(50, 10);
  input.end.edge_bottom = gfx::Point(50, 30);
  input.end.layer_id = root->id();

  viz::Selection<gfx::SelectionBound> output;

  // Empty input bounds should produce empty output bounds.
  host_impl().active_tree()->GetViewportSelection(&output);
  EXPECT_EQ(gfx::SelectionBound(), output.start);
  EXPECT_EQ(gfx::SelectionBound(), output.end);

  // Selection bounds should produce distinct left and right bounds.
  host_impl().active_tree()->RegisterSelection(input);
  host_impl().active_tree()->GetViewportSelection(&output);
  EXPECT_EQ(input.start.type, output.start.type());
  EXPECT_EQ(gfx::PointF(input.start.edge_bottom), output.start.edge_bottom());
  EXPECT_EQ(gfx::PointF(input.start.edge_top), output.start.edge_top());
  EXPECT_TRUE(output.start.visible());
  EXPECT_EQ(input.end.type, output.end.type());
  EXPECT_EQ(gfx::PointF(input.end.edge_bottom), output.end.edge_bottom());
  EXPECT_EQ(gfx::PointF(input.end.edge_top), output.end.edge_top());
  EXPECT_TRUE(output.end.visible());

  // Insertion bounds should produce identical left and right bounds.
  LayerSelection insertion_input;
  insertion_input.start.type = gfx::SelectionBound::CENTER;
  insertion_input.start.edge_top = gfx::Point(15, 10);
  insertion_input.start.edge_bottom = gfx::Point(15, 30);
  insertion_input.start.layer_id = root->id();
  insertion_input.end = insertion_input.start;
  host_impl().active_tree()->RegisterSelection(insertion_input);
  host_impl().active_tree()->GetViewportSelection(&output);
  EXPECT_EQ(insertion_input.start.type, output.start.type());
  EXPECT_EQ(gfx::PointF(insertion_input.start.edge_bottom),
            output.start.edge_bottom());
  EXPECT_EQ(gfx::PointF(insertion_input.start.edge_top),
            output.start.edge_top());
  EXPECT_TRUE(output.start.visible());
  EXPECT_EQ(output.start, output.end);
}

TEST_F(LayerTreeImplTest, SelectionBoundsForPartialOccludedLayers) {
  LayerImpl* root = root_layer();
  root->SetDrawsContent(true);
  root->SetBounds(gfx::Size(100, 100));

  int clip_layer_id = 1234;
  int clipped_layer_id = 123;

  gfx::Vector2dF clipping_offset(10, 10);
  {
    std::unique_ptr<LayerImpl> clipping_layer =
        LayerImpl::Create(host_impl().active_tree(), clip_layer_id);
    // The clipping layer should occlude the right selection bound.
    clipping_layer->SetPosition(gfx::PointF() + clipping_offset);
    clipping_layer->SetBounds(gfx::Size(50, 50));
    clipping_layer->SetMasksToBounds(true);

    std::unique_ptr<LayerImpl> clipped_layer =
        LayerImpl::Create(host_impl().active_tree(), clipped_layer_id);
    clipped_layer->SetBounds(gfx::Size(100, 100));
    clipped_layer->SetDrawsContent(true);
    clipping_layer->test_properties()->AddChild(std::move(clipped_layer));
    root->test_properties()->AddChild(std::move(clipping_layer));
  }

  host_impl().SetViewportSize(root->bounds());
  host_impl().UpdateNumChildrenAndDrawPropertiesForActiveTree();

  // Sanity check the scenario we just created.
  ASSERT_EQ(1u, GetRenderSurfaceList().size());

  LayerSelection input;
  input.start.type = gfx::SelectionBound::LEFT;
  input.start.edge_top = gfx::Point(25, 10);
  input.start.edge_bottom = gfx::Point(25, 30);
  input.start.layer_id = clipped_layer_id;

  input.end.type = gfx::SelectionBound::RIGHT;
  input.end.edge_top = gfx::Point(75, 10);
  input.end.edge_bottom = gfx::Point(75, 30);
  input.end.layer_id = clipped_layer_id;
  host_impl().active_tree()->RegisterSelection(input);

  // The left bound should be occluded by the clip layer.
  viz::Selection<gfx::SelectionBound> output;
  host_impl().active_tree()->GetViewportSelection(&output);
  EXPECT_EQ(input.start.type, output.start.type());
  auto expected_output_start_top = gfx::PointF(input.start.edge_top);
  auto expected_output_edge_botom = gfx::PointF(input.start.edge_bottom);
  expected_output_start_top.Offset(clipping_offset.x(), clipping_offset.y());
  expected_output_edge_botom.Offset(clipping_offset.x(), clipping_offset.y());
  EXPECT_EQ(expected_output_start_top, output.start.edge_top());
  EXPECT_EQ(expected_output_edge_botom, output.start.edge_bottom());
  EXPECT_TRUE(output.start.visible());
  EXPECT_EQ(input.end.type, output.end.type());
  auto expected_output_end_top = gfx::PointF(input.end.edge_top);
  auto expected_output_end_bottom = gfx::PointF(input.end.edge_bottom);
  expected_output_end_bottom.Offset(clipping_offset.x(), clipping_offset.y());
  expected_output_end_top.Offset(clipping_offset.x(), clipping_offset.y());
  EXPECT_EQ(expected_output_end_top, output.end.edge_top());
  EXPECT_EQ(expected_output_end_bottom, output.end.edge_bottom());
  EXPECT_FALSE(output.end.visible());

  // Handles outside the viewport bounds should be marked invisible.
  input.start.edge_top = gfx::Point(-25, 0);
  input.start.edge_bottom = gfx::Point(-25, 20);
  host_impl().active_tree()->RegisterSelection(input);
  host_impl().active_tree()->GetViewportSelection(&output);
  EXPECT_FALSE(output.start.visible());

  input.start.edge_top = gfx::Point(0, -25);
  input.start.edge_bottom = gfx::Point(0, -5);
  host_impl().active_tree()->RegisterSelection(input);
  host_impl().active_tree()->GetViewportSelection(&output);
  EXPECT_FALSE(output.start.visible());

  // If the handle bottom is partially visible, the handle is marked visible.
  input.start.edge_top = gfx::Point(0, -20);
  input.start.edge_bottom = gfx::Point(0, 1);
  host_impl().active_tree()->RegisterSelection(input);
  host_impl().active_tree()->GetViewportSelection(&output);
  EXPECT_TRUE(output.start.visible());
}

TEST_F(LayerTreeImplTest, SelectionBoundsForScaledLayers) {
  LayerImpl* root = root_layer();
  root->SetDrawsContent(true);
  root->SetBounds(gfx::Size(100, 100));

  int root_layer_id = root->id();
  int sub_layer_id = 2;

  gfx::Vector2dF sub_layer_offset(10, 0);
  {
    std::unique_ptr<LayerImpl> sub_layer =
        LayerImpl::Create(host_impl().active_tree(), sub_layer_id);
    sub_layer->SetPosition(gfx::PointF() + sub_layer_offset);
    sub_layer->SetBounds(gfx::Size(50, 50));
    sub_layer->SetDrawsContent(true);
    root->test_properties()->AddChild(std::move(sub_layer));
  }

  host_impl().active_tree()->BuildPropertyTreesForTesting();

  float device_scale_factor = 3.f;
  float page_scale_factor = 5.f;
  gfx::Size scaled_bounds_for_root = gfx::ScaleToCeiledSize(
      root->bounds(), device_scale_factor * page_scale_factor);
  host_impl().SetViewportSize(scaled_bounds_for_root);

  LayerTreeImpl::ViewportLayerIds viewport_ids;
  viewport_ids.page_scale = root->id();
  host_impl().active_tree()->SetViewportLayersFromIds(viewport_ids);
  host_impl().active_tree()->SetDeviceScaleFactor(device_scale_factor);
  host_impl().active_tree()->SetPageScaleOnActiveTree(page_scale_factor);

  host_impl().active_tree()->PushPageScaleFromMainThread(
      page_scale_factor, page_scale_factor, page_scale_factor);
  host_impl().active_tree()->SetPageScaleOnActiveTree(page_scale_factor);
  host_impl().UpdateNumChildrenAndDrawPropertiesForActiveTree();

  // Sanity check the scenario we just created.
  ASSERT_EQ(1u, GetRenderSurfaceList().size());

  LayerSelection input;
  input.start.type = gfx::SelectionBound::LEFT;
  input.start.edge_top = gfx::Point(10, 10);
  input.start.edge_bottom = gfx::Point(10, 30);
  input.start.layer_id = root_layer_id;

  input.end.type = gfx::SelectionBound::RIGHT;
  input.end.edge_top = gfx::Point(0, 0);
  input.end.edge_bottom = gfx::Point(0, 20);
  input.end.layer_id = sub_layer_id;
  host_impl().active_tree()->RegisterSelection(input);

  // The viewport bounds should be properly scaled by the page scale, but should
  // remain in DIP coordinates.
  viz::Selection<gfx::SelectionBound> output;
  host_impl().active_tree()->GetViewportSelection(&output);
  EXPECT_EQ(input.start.type, output.start.type());
  auto expected_output_start_top = gfx::PointF(input.start.edge_top);
  auto expected_output_edge_bottom = gfx::PointF(input.start.edge_bottom);
  expected_output_start_top.Scale(page_scale_factor);
  expected_output_edge_bottom.Scale(page_scale_factor);
  EXPECT_EQ(expected_output_start_top, output.start.edge_top());
  EXPECT_EQ(expected_output_edge_bottom, output.start.edge_bottom());
  EXPECT_TRUE(output.start.visible());
  EXPECT_EQ(input.end.type, output.end.type());

  auto expected_output_end_top = gfx::PointF(input.end.edge_top);
  auto expected_output_end_bottom = gfx::PointF(input.end.edge_bottom);
  expected_output_end_top.Offset(sub_layer_offset.x(), sub_layer_offset.y());
  expected_output_end_bottom.Offset(sub_layer_offset.x(), sub_layer_offset.y());
  expected_output_end_top.Scale(page_scale_factor);
  expected_output_end_bottom.Scale(page_scale_factor);
  EXPECT_EQ(expected_output_end_top, output.end.edge_top());
  EXPECT_EQ(expected_output_end_bottom, output.end.edge_bottom());
  EXPECT_TRUE(output.end.visible());
}

TEST_F(LayerTreeImplTest, SelectionBoundsForDSFEnabled) {
  LayerImpl* root = root_layer();
  root->SetDrawsContent(true);
  root->SetBounds(gfx::Size(100, 100));

  int root_layer_id = root->id();
  int sub_layer_id = 2;

  gfx::Vector2dF sub_layer_offset(10, 0);
  {
    std::unique_ptr<LayerImpl> sub_layer =
        LayerImpl::Create(host_impl().active_tree(), sub_layer_id);
    sub_layer->SetPosition(gfx::PointF() + sub_layer_offset);
    sub_layer->SetBounds(gfx::Size(50, 50));
    sub_layer->SetDrawsContent(true);
    root->test_properties()->AddChild(std::move(sub_layer));
  }

  host_impl().active_tree()->BuildPropertyTreesForTesting();

  float device_scale_factor = 3.f;
  float painted_device_scale_factor = 5.f;

  LayerTreeImpl::ViewportLayerIds viewport_ids;
  viewport_ids.page_scale = root->id();
  host_impl().active_tree()->SetViewportLayersFromIds(viewport_ids);
  host_impl().active_tree()->SetDeviceScaleFactor(device_scale_factor);
  host_impl().active_tree()->set_painted_device_scale_factor(
      painted_device_scale_factor);

  LayerSelection input;
  input.start.type = gfx::SelectionBound::LEFT;
  input.start.edge_top = gfx::Point(10, 10);
  input.start.edge_bottom = gfx::Point(10, 30);
  input.start.layer_id = root_layer_id;

  input.end.type = gfx::SelectionBound::RIGHT;
  input.end.edge_top = gfx::Point(0, 0);
  input.end.edge_bottom = gfx::Point(0, 20);
  input.end.layer_id = sub_layer_id;
  host_impl().active_tree()->RegisterSelection(input);

  // The viewport bounds should be properly scaled by the page scale, but should
  // remain in DIP coordinates.
  viz::Selection<gfx::SelectionBound> output;
  host_impl().active_tree()->GetViewportSelection(&output);
  EXPECT_EQ(input.start.type, output.start.type());
  auto expected_output_start_top = gfx::PointF(input.start.edge_top);
  auto expected_output_edge_bottom = gfx::PointF(input.start.edge_bottom);
  expected_output_start_top.Scale(
      1.f / (device_scale_factor * painted_device_scale_factor));
  expected_output_edge_bottom.Scale(
      1.f / (device_scale_factor * painted_device_scale_factor));
  EXPECT_EQ(expected_output_start_top, output.start.edge_top());
  EXPECT_EQ(expected_output_edge_bottom, output.start.edge_bottom());
  EXPECT_TRUE(output.start.visible());
  EXPECT_EQ(input.end.type, output.end.type());

  auto expected_output_end_top = gfx::PointF(input.end.edge_top);
  auto expected_output_end_bottom = gfx::PointF(input.end.edge_bottom);
  expected_output_end_top.Offset(sub_layer_offset.x(), sub_layer_offset.y());
  expected_output_end_bottom.Offset(sub_layer_offset.x(), sub_layer_offset.y());
  expected_output_end_top.Scale(
      1.f / (device_scale_factor * painted_device_scale_factor));
  expected_output_end_bottom.Scale(
      1.f / (device_scale_factor * painted_device_scale_factor));
  EXPECT_EQ(expected_output_end_top, output.end.edge_top());
  EXPECT_EQ(expected_output_end_bottom, output.end.edge_bottom());
  EXPECT_TRUE(output.end.visible());
}

TEST_F(LayerTreeImplTest, SelectionBoundsWithLargeTransforms) {
  LayerImpl* root = root_layer();
  root->SetBounds(gfx::Size(100, 100));

  int child_id = 2;
  int grand_child_id = 3;

  gfx::Transform large_transform;
  large_transform.Scale(SkDoubleToMScalar(1e37), SkDoubleToMScalar(1e37));
  large_transform.RotateAboutYAxis(30);

  {
    std::unique_ptr<LayerImpl> child =
        LayerImpl::Create(host_impl().active_tree(), child_id);
    child->test_properties()->transform = large_transform;
    child->SetBounds(gfx::Size(100, 100));

    std::unique_ptr<LayerImpl> grand_child =
        LayerImpl::Create(host_impl().active_tree(), grand_child_id);
    grand_child->test_properties()->transform = large_transform;
    grand_child->SetBounds(gfx::Size(100, 100));
    grand_child->SetDrawsContent(true);

    child->test_properties()->AddChild(std::move(grand_child));
    root->test_properties()->AddChild(std::move(child));
  }

  host_impl().SetViewportSize(root->bounds());
  host_impl().UpdateNumChildrenAndDrawPropertiesForActiveTree();

  LayerSelection input;

  input.start.type = gfx::SelectionBound::LEFT;
  input.start.edge_top = gfx::Point(10, 10);
  input.start.edge_bottom = gfx::Point(10, 20);
  input.start.layer_id = grand_child_id;

  input.end.type = gfx::SelectionBound::RIGHT;
  input.end.edge_top = gfx::Point(50, 10);
  input.end.edge_bottom = gfx::Point(50, 30);
  input.end.layer_id = grand_child_id;

  host_impl().active_tree()->RegisterSelection(input);

  viz::Selection<gfx::SelectionBound> output;
  host_impl().active_tree()->GetViewportSelection(&output);

  // edge_bottom and edge_top aren't allowed to have NaNs, so the selection
  // should be empty.
  EXPECT_EQ(gfx::SelectionBound(), output.start);
  EXPECT_EQ(gfx::SelectionBound(), output.end);
}

TEST_F(LayerTreeImplTest, NumLayersTestOne) {
  // Root is created by the test harness.
  EXPECT_EQ(1u, host_impl().active_tree()->NumLayers());
  EXPECT_TRUE(root_layer());
  // Create another layer, should increment.
  auto layer = LayerImpl::Create(host_impl().active_tree(), 2);
  EXPECT_EQ(2u, host_impl().active_tree()->NumLayers());
}

TEST_F(LayerTreeImplTest, NumLayersSmallTree) {
  EXPECT_EQ(1u, host_impl().active_tree()->NumLayers());
  LayerImpl* root = root_layer();
  root->test_properties()->AddChild(
      LayerImpl::Create(host_impl().active_tree(), 2));
  root->test_properties()->AddChild(
      LayerImpl::Create(host_impl().active_tree(), 3));
  root->test_properties()->children[1]->test_properties()->AddChild(
      LayerImpl::Create(host_impl().active_tree(), 4));
  EXPECT_EQ(4u, host_impl().active_tree()->NumLayers());
}

TEST_F(LayerTreeImplTest, DeviceScaleFactorNeedsDrawPropertiesUpdate) {
  host_impl().active_tree()->BuildPropertyTreesForTesting();
  host_impl().active_tree()->SetDeviceScaleFactor(1.f);
  host_impl().active_tree()->UpdateDrawProperties();
  EXPECT_FALSE(host_impl().active_tree()->needs_update_draw_properties());
  host_impl().active_tree()->SetDeviceScaleFactor(2.f);
  EXPECT_TRUE(host_impl().active_tree()->needs_update_draw_properties());
}

TEST_F(LayerTreeImplTest, RasterColorSpaceDoesNotNeedDrawPropertiesUpdate) {
  host_impl().active_tree()->BuildPropertyTreesForTesting();
  host_impl().active_tree()->SetRasterColorSpace(
      1, gfx::ColorSpace::CreateXYZD50());
  host_impl().active_tree()->UpdateDrawProperties();
  EXPECT_FALSE(host_impl().active_tree()->needs_update_draw_properties());
  host_impl().active_tree()->SetRasterColorSpace(1,
                                                 gfx::ColorSpace::CreateSRGB());
  EXPECT_FALSE(host_impl().active_tree()->needs_update_draw_properties());
}

TEST_F(LayerTreeImplTest, HitTestingCorrectLayerWheelListener) {
  host_impl().active_tree()->set_event_listener_properties(
      EventListenerClass::kMouseWheel, EventListenerProperties::kBlocking);

  LayerImpl* root = root_layer();
  std::unique_ptr<LayerImpl> left_child =
      LayerImpl::Create(host_impl().active_tree(), 2);
  std::unique_ptr<LayerImpl> right_child =
      LayerImpl::Create(host_impl().active_tree(), 3);

  {
    gfx::Transform translate_z;
    translate_z.Translate3d(0, 0, 10);
    root->test_properties()->transform = translate_z;
    root->SetBounds(gfx::Size(100, 100));
    root->SetDrawsContent(true);
  }
  {
    gfx::Transform translate_z;
    translate_z.Translate3d(0, 0, 10);
    left_child->test_properties()->transform = translate_z;
    left_child->SetBounds(gfx::Size(100, 100));
    left_child->SetDrawsContent(true);
  }
  {
    gfx::Transform translate_z;
    translate_z.Translate3d(0, 0, 10);
    right_child->test_properties()->transform = translate_z;
    right_child->SetBounds(gfx::Size(100, 100));
  }

  root->test_properties()->AddChild(std::move(left_child));
  root->test_properties()->AddChild(std::move(right_child));

  host_impl().SetViewportSize(root->bounds());
  host_impl().UpdateNumChildrenAndDrawPropertiesForActiveTree();
  CHECK_EQ(1u, GetRenderSurfaceList().size());

  gfx::PointF test_point = gfx::PointF(1.f, 1.f);
  LayerImpl* result_layer =
      host_impl().active_tree()->FindLayerThatIsHitByPoint(test_point);

  CHECK(result_layer);
  EXPECT_EQ(2, result_layer->id());
}

namespace {

class PersistentSwapPromise
    : public SwapPromise,
      public base::SupportsWeakPtr<PersistentSwapPromise> {
 public:
  PersistentSwapPromise() = default;
  ~PersistentSwapPromise() override = default;

  void DidActivate() override {}
  MOCK_METHOD2(WillSwap,
               void(viz::CompositorFrameMetadata* metadata,
                    FrameTokenAllocator* frame_token_allocator));
  MOCK_METHOD0(DidSwap, void());

  DidNotSwapAction DidNotSwap(DidNotSwapReason reason) override {
    return DidNotSwapAction::KEEP_ACTIVE;
  }

  void OnCommit() override {}
  int64_t TraceId() const override { return 0; }
};

class NotPersistentSwapPromise
    : public SwapPromise,
      public base::SupportsWeakPtr<NotPersistentSwapPromise> {
 public:
  NotPersistentSwapPromise() = default;
  ~NotPersistentSwapPromise() override = default;

  void DidActivate() override {}
  void WillSwap(viz::CompositorFrameMetadata* metadata,
                FrameTokenAllocator* frame_token_allocator) override {}
  void DidSwap() override {}

  DidNotSwapAction DidNotSwap(DidNotSwapReason reason) override {
    return DidNotSwapAction::BREAK_PROMISE;
  }

  void OnCommit() override {}
  int64_t TraceId() const override { return 0; }
};

}  // namespace

TEST_F(LayerTreeImplTest, PersistentSwapPromisesAreKeptAlive) {
  const size_t promises_count = 2;

  std::vector<base::WeakPtr<PersistentSwapPromise>> persistent_promises;
  std::vector<std::unique_ptr<PersistentSwapPromise>>
      persistent_promises_to_pass;
  for (size_t i = 0; i < promises_count; ++i) {
    persistent_promises_to_pass.push_back(
        std::make_unique<PersistentSwapPromise>());
  }

  for (auto& promise : persistent_promises_to_pass) {
    persistent_promises.push_back(promise->AsWeakPtr());
    host_impl().active_tree()->QueueSwapPromise(std::move(promise));
  }

  std::vector<std::unique_ptr<SwapPromise>> promises;
  host_impl().active_tree()->PassSwapPromises(std::move(promises));
  host_impl().active_tree()->BreakSwapPromises(
      SwapPromise::DidNotSwapReason::SWAP_FAILS);

  ASSERT_EQ(promises_count, persistent_promises.size());
  for (size_t i = 0; i < persistent_promises.size(); ++i) {
    SCOPED_TRACE(testing::Message() << "While checking case #" << i);
    ASSERT_TRUE(persistent_promises[i]);
    EXPECT_CALL(*persistent_promises[i], WillSwap(testing::_, testing::_));
  }
  host_impl().active_tree()->FinishSwapPromises(nullptr, nullptr);
}

TEST_F(LayerTreeImplTest, NotPersistentSwapPromisesAreDroppedWhenSwapFails) {
  const size_t promises_count = 2;

  std::vector<base::WeakPtr<NotPersistentSwapPromise>> not_persistent_promises;
  std::vector<std::unique_ptr<NotPersistentSwapPromise>>
      not_persistent_promises_to_pass;
  for (size_t i = 0; i < promises_count; ++i) {
    not_persistent_promises_to_pass.push_back(
        std::make_unique<NotPersistentSwapPromise>());
  }

  for (auto& promise : not_persistent_promises_to_pass) {
    not_persistent_promises.push_back(promise->AsWeakPtr());
    host_impl().active_tree()->QueueSwapPromise(std::move(promise));
  }
  std::vector<std::unique_ptr<SwapPromise>> promises;
  host_impl().active_tree()->PassSwapPromises(std::move(promises));

  ASSERT_EQ(promises_count, not_persistent_promises.size());
  for (size_t i = 0; i < not_persistent_promises.size(); ++i) {
    EXPECT_FALSE(not_persistent_promises[i]) << "While checking case #" << i;
  }

  // Finally, check that not persistent promise doesn't survive
  // |LayerTreeImpl::BreakSwapPromises|.
  {
    std::unique_ptr<NotPersistentSwapPromise> promise(
        new NotPersistentSwapPromise());
    auto weak_promise = promise->AsWeakPtr();
    host_impl().active_tree()->QueueSwapPromise(std::move(promise));
    host_impl().active_tree()->BreakSwapPromises(
        SwapPromise::DidNotSwapReason::SWAP_FAILS);
    EXPECT_FALSE(weak_promise);
  }
}

}  // namespace
}  // namespace cc
