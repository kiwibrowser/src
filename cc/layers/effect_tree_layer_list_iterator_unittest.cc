// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/layers/effect_tree_layer_list_iterator.h"

#include <vector>

#include "base/memory/ptr_util.h"
#include "cc/layers/layer.h"
#include "cc/test/fake_layer_tree_host.h"
#include "cc/test/layer_test_common.h"
#include "cc/test/test_task_graph_runner.h"
#include "cc/trees/layer_tree_host_common.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/transform.h"

namespace cc {
namespace {

class TestLayerImpl : public LayerImpl {
 public:
  static std::unique_ptr<TestLayerImpl> Create(LayerTreeImpl* tree, int id) {
    return base::WrapUnique(new TestLayerImpl(tree, id));
  }
  ~TestLayerImpl() override = default;

  int count_;

 private:
  explicit TestLayerImpl(LayerTreeImpl* tree, int id)
      : LayerImpl(tree, id), count_(-1) {
    SetBounds(gfx::Size(100, 100));
    SetPosition(gfx::PointF());
    SetDrawsContent(true);
  }
};

#define EXPECT_COUNT(layer, target, contrib, itself)                      \
  if (GetRenderSurface(layer)) {                                          \
    EXPECT_EQ(target, target_surface_count_[layer->effect_tree_index()]); \
    EXPECT_EQ(contrib,                                                    \
              contributing_surface_count_[layer->effect_tree_index()]);   \
  }                                                                       \
  EXPECT_EQ(itself, layer->count_);

class EffectTreeLayerListIteratorTest : public testing::Test {
 public:
  EffectTreeLayerListIteratorTest()
      : host_impl_(&task_runner_provider_, &task_graph_runner_), id_(1) {}

  std::unique_ptr<TestLayerImpl> CreateLayer() {
    return TestLayerImpl::Create(host_impl_.active_tree(), id_++);
  }

  void IterateFrontToBack() {
    ResetCounts();
    int count = 0;
    for (EffectTreeLayerListIterator it(host_impl_.active_tree());
         it.state() != EffectTreeLayerListIterator::State::END; ++it, ++count) {
      switch (it.state()) {
        case EffectTreeLayerListIterator::State::LAYER:
          static_cast<TestLayerImpl*>(it.current_layer())->count_ = count;
          break;
        case EffectTreeLayerListIterator::State::TARGET_SURFACE:
          target_surface_count_[it.target_render_surface()->EffectTreeIndex()] =
              count;
          break;
        case EffectTreeLayerListIterator::State::CONTRIBUTING_SURFACE:
          contributing_surface_count_[it.current_render_surface()
                                          ->EffectTreeIndex()] = count;
          break;
        default:
          NOTREACHED();
      }
    }
  }

  void ResetCounts() {
    for (LayerImpl* layer : *host_impl_.active_tree()) {
      static_cast<TestLayerImpl*>(layer)->count_ = -1;
    }

    target_surface_count_ = std::vector<int>(
        host_impl_.active_tree()->property_trees()->effect_tree.size(), -1);
    contributing_surface_count_ = std::vector<int>(
        host_impl_.active_tree()->property_trees()->effect_tree.size(), -1);
  }

 protected:
  FakeImplTaskRunnerProvider task_runner_provider_;
  TestTaskGraphRunner task_graph_runner_;
  FakeLayerTreeHostImpl host_impl_;

  int id_;

  // Tracks when each render surface is visited as a target surface or
  // contributing surface. Indexed by effect node id.
  std::vector<int> target_surface_count_;
  std::vector<int> contributing_surface_count_;
};

TEST_F(EffectTreeLayerListIteratorTest, TreeWithNoDrawnLayers) {
  std::unique_ptr<TestLayerImpl> root_layer = CreateLayer();
  root_layer->SetDrawsContent(false);

  TestLayerImpl* root_ptr = root_layer.get();

  host_impl_.active_tree()->SetRootLayerForTesting(std::move(root_layer));

  RenderSurfaceList render_surface_list;
  LayerTreeHostCommon::CalcDrawPropsImplInputsForTesting inputs(
      root_ptr, root_ptr->bounds(), &render_surface_list);
  LayerTreeHostCommon::CalculateDrawPropertiesForTesting(&inputs);

  IterateFrontToBack();
  EXPECT_COUNT(root_ptr, 0, -1, -1);
}

TEST_F(EffectTreeLayerListIteratorTest, SimpleTree) {
  std::unique_ptr<TestLayerImpl> root_layer = CreateLayer();
  std::unique_ptr<TestLayerImpl> first = CreateLayer();
  std::unique_ptr<TestLayerImpl> second = CreateLayer();
  std::unique_ptr<TestLayerImpl> third = CreateLayer();
  std::unique_ptr<TestLayerImpl> fourth = CreateLayer();

  TestLayerImpl* root_ptr = root_layer.get();
  TestLayerImpl* first_ptr = first.get();
  TestLayerImpl* second_ptr = second.get();
  TestLayerImpl* third_ptr = third.get();
  TestLayerImpl* fourth_ptr = fourth.get();

  root_layer->test_properties()->AddChild(std::move(first));
  root_layer->test_properties()->AddChild(std::move(second));
  root_layer->test_properties()->AddChild(std::move(third));
  root_layer->test_properties()->AddChild(std::move(fourth));

  host_impl_.active_tree()->SetRootLayerForTesting(std::move(root_layer));

  RenderSurfaceList render_surface_list;
  LayerTreeHostCommon::CalcDrawPropsImplInputsForTesting inputs(
      root_ptr, root_ptr->bounds(), &render_surface_list);
  LayerTreeHostCommon::CalculateDrawPropertiesForTesting(&inputs);

  IterateFrontToBack();
  EXPECT_COUNT(root_ptr, 5, -1, 4);
  EXPECT_COUNT(first_ptr, 5, -1, 3);
  EXPECT_COUNT(second_ptr, 5, -1, 2);
  EXPECT_COUNT(third_ptr, 5, -1, 1);
  EXPECT_COUNT(fourth_ptr, 5, -1, 0);
}

TEST_F(EffectTreeLayerListIteratorTest, ComplexTree) {
  std::unique_ptr<TestLayerImpl> root_layer = CreateLayer();
  std::unique_ptr<TestLayerImpl> root1 = CreateLayer();
  std::unique_ptr<TestLayerImpl> root2 = CreateLayer();
  std::unique_ptr<TestLayerImpl> root3 = CreateLayer();
  std::unique_ptr<TestLayerImpl> root21 = CreateLayer();
  std::unique_ptr<TestLayerImpl> root22 = CreateLayer();
  std::unique_ptr<TestLayerImpl> root23 = CreateLayer();
  std::unique_ptr<TestLayerImpl> root221 = CreateLayer();
  std::unique_ptr<TestLayerImpl> root231 = CreateLayer();

  TestLayerImpl* root_ptr = root_layer.get();
  TestLayerImpl* root1_ptr = root1.get();
  TestLayerImpl* root2_ptr = root2.get();
  TestLayerImpl* root3_ptr = root3.get();
  TestLayerImpl* root21_ptr = root21.get();
  TestLayerImpl* root22_ptr = root22.get();
  TestLayerImpl* root23_ptr = root23.get();
  TestLayerImpl* root221_ptr = root221.get();
  TestLayerImpl* root231_ptr = root231.get();

  root22->test_properties()->AddChild(std::move(root221));
  root23->test_properties()->AddChild(std::move(root231));
  root2->test_properties()->AddChild(std::move(root21));
  root2->test_properties()->AddChild(std::move(root22));
  root2->test_properties()->AddChild(std::move(root23));
  root_layer->test_properties()->AddChild(std::move(root1));
  root_layer->test_properties()->AddChild(std::move(root2));
  root_layer->test_properties()->AddChild(std::move(root3));

  host_impl_.active_tree()->SetRootLayerForTesting(std::move(root_layer));

  RenderSurfaceList render_surface_list;
  LayerTreeHostCommon::CalcDrawPropsImplInputsForTesting inputs(
      root_ptr, root_ptr->bounds(), &render_surface_list);
  LayerTreeHostCommon::CalculateDrawPropertiesForTesting(&inputs);

  IterateFrontToBack();
  EXPECT_COUNT(root_ptr, 9, -1, 8);
  EXPECT_COUNT(root1_ptr, 9, -1, 7);
  EXPECT_COUNT(root2_ptr, 9, -1, 6);
  EXPECT_COUNT(root21_ptr, 9, -1, 5);
  EXPECT_COUNT(root22_ptr, 9, -1, 4);
  EXPECT_COUNT(root221_ptr, 9, -1, 3);
  EXPECT_COUNT(root23_ptr, 9, -1, 2);
  EXPECT_COUNT(root231_ptr, 9, -1, 1);
  EXPECT_COUNT(root3_ptr, 9, -1, 0);
}

TEST_F(EffectTreeLayerListIteratorTest, ComplexTreeMultiSurface) {
  std::unique_ptr<TestLayerImpl> root_layer = CreateLayer();
  std::unique_ptr<TestLayerImpl> root1 = CreateLayer();
  std::unique_ptr<TestLayerImpl> root2 = CreateLayer();
  std::unique_ptr<TestLayerImpl> root3 = CreateLayer();
  std::unique_ptr<TestLayerImpl> root21 = CreateLayer();
  std::unique_ptr<TestLayerImpl> root22 = CreateLayer();
  std::unique_ptr<TestLayerImpl> root23 = CreateLayer();
  std::unique_ptr<TestLayerImpl> root221 = CreateLayer();
  std::unique_ptr<TestLayerImpl> root231 = CreateLayer();

  TestLayerImpl* root_ptr = root_layer.get();
  TestLayerImpl* root1_ptr = root1.get();
  TestLayerImpl* root2_ptr = root2.get();
  TestLayerImpl* root3_ptr = root3.get();
  TestLayerImpl* root21_ptr = root21.get();
  TestLayerImpl* root22_ptr = root22.get();
  TestLayerImpl* root23_ptr = root23.get();
  TestLayerImpl* root221_ptr = root221.get();
  TestLayerImpl* root231_ptr = root231.get();

  root22->test_properties()->force_render_surface = true;
  root23->test_properties()->force_render_surface = true;
  root2->test_properties()->force_render_surface = true;
  root22->test_properties()->AddChild(std::move(root221));
  root23->test_properties()->AddChild(std::move(root231));
  root2->SetDrawsContent(false);
  root2->test_properties()->AddChild(std::move(root21));
  root2->test_properties()->AddChild(std::move(root22));
  root2->test_properties()->AddChild(std::move(root23));
  root_layer->test_properties()->AddChild(std::move(root1));
  root_layer->test_properties()->AddChild(std::move(root2));
  root_layer->test_properties()->AddChild(std::move(root3));

  host_impl_.active_tree()->SetRootLayerForTesting(std::move(root_layer));

  RenderSurfaceList render_surface_list;
  LayerTreeHostCommon::CalcDrawPropsImplInputsForTesting inputs(
      root_ptr, root_ptr->bounds(), &render_surface_list);
  LayerTreeHostCommon::CalculateDrawPropertiesForTesting(&inputs);

  IterateFrontToBack();
  EXPECT_COUNT(root_ptr, 14, -1, 13);
  EXPECT_COUNT(root1_ptr, 14, -1, 12);
  EXPECT_COUNT(root2_ptr, 10, 11, -1);
  EXPECT_COUNT(root21_ptr, 10, 11, 9);
  EXPECT_COUNT(root22_ptr, 7, 8, 6);
  EXPECT_COUNT(root221_ptr, 7, 8, 5);
  EXPECT_COUNT(root23_ptr, 3, 4, 2);
  EXPECT_COUNT(root231_ptr, 3, 4, 1);
  EXPECT_COUNT(root3_ptr, 14, -1, 0);
}

}  // namespace
}  // namespace cc
