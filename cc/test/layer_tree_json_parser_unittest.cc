// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/test/layer_tree_json_parser.h"

#include <stddef.h>

#include "cc/layers/layer.h"
#include "cc/test/fake_impl_task_runner_provider.h"
#include "cc/test/fake_layer_tree_host.h"
#include "cc/test/fake_layer_tree_host_impl.h"
#include "cc/test/geometry_test_utils.h"
#include "cc/test/test_task_graph_runner.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace cc {

namespace {

bool LayerTreesMatch(LayerImpl* const layer_impl,
                     Layer* const layer) {
#define RETURN_IF_EXPECTATION_FAILS(exp) \
  do { \
    exp; \
    if (testing::UnitTest::GetInstance()->current_test_info()-> \
            result()->Failed()) \
      return false; \
  } while (0)

  RETURN_IF_EXPECTATION_FAILS(
      EXPECT_EQ(layer_impl->test_properties()->children.size(),
                layer->children().size()));
  RETURN_IF_EXPECTATION_FAILS(EXPECT_EQ(layer_impl->bounds(), layer->bounds()));
  RETURN_IF_EXPECTATION_FAILS(
      EXPECT_EQ(layer_impl->position(), layer->position()));
  RETURN_IF_EXPECTATION_FAILS(EXPECT_TRANSFORMATION_MATRIX_EQ(
      layer_impl->test_properties()->transform, layer->transform()));
  RETURN_IF_EXPECTATION_FAILS(EXPECT_EQ(layer_impl->contents_opaque(),
                                        layer->contents_opaque()));
  RETURN_IF_EXPECTATION_FAILS(EXPECT_EQ(layer_impl->scrollable(),
                                        layer->scrollable()));

  RETURN_IF_EXPECTATION_FAILS(
      EXPECT_FLOAT_EQ(layer_impl->Opacity(), layer->opacity()));
  RETURN_IF_EXPECTATION_FAILS(EXPECT_EQ(layer_impl->touch_action_region(),
                                        layer->touch_action_region()));

  for (size_t i = 0; i < layer_impl->test_properties()->children.size(); ++i) {
    RETURN_IF_EXPECTATION_FAILS(
        EXPECT_TRUE(LayerTreesMatch(layer_impl->test_properties()->children[i],
                                    layer->children()[i].get())));
  }

  return true;
#undef RETURN_IF_EXPECTATION_FAILS
}

}  // namespace

class LayerTreeJsonParserSanityCheck : public testing::Test {
};

TEST_F(LayerTreeJsonParserSanityCheck, Basic) {
  FakeImplTaskRunnerProvider task_runner_provider;
  TestTaskGraphRunner task_graph_runner;
  FakeLayerTreeHostImpl host_impl(&task_runner_provider, &task_graph_runner);
  LayerTreeImpl* tree = host_impl.active_tree();

  std::unique_ptr<LayerImpl> root_impl(LayerImpl::Create(tree, 1));
  std::unique_ptr<LayerImpl> parent(LayerImpl::Create(tree, 2));
  std::unique_ptr<LayerImpl> child(LayerImpl::Create(tree, 3));

  root_impl->SetBounds(gfx::Size(100, 100));
  parent->SetBounds(gfx::Size(50, 50));
  child->SetBounds(gfx::Size(40, 40));

  gfx::Transform translate;
  translate.Translate(10, 15);
  child->test_properties()->transform = translate;

  parent->SetPosition(gfx::PointF(25.f, 25.f));

  parent->test_properties()->AddChild(std::move(child));
  root_impl->test_properties()->AddChild(std::move(parent));
  tree->SetRootLayerForTesting(std::move(root_impl));
  tree->BuildPropertyTreesForTesting();

  std::string json = host_impl.LayerTreeAsJson();
  scoped_refptr<Layer> root = ParseTreeFromJson(json, nullptr);
  ASSERT_TRUE(root.get());
  EXPECT_TRUE(LayerTreesMatch(host_impl.active_tree()->root_layer_for_testing(),
                              root.get()));
}

TEST_F(LayerTreeJsonParserSanityCheck, EventHandlerRegions) {
  FakeImplTaskRunnerProvider task_runner_provider;
  TestTaskGraphRunner task_graph_runner;
  FakeLayerTreeHostImpl host_impl(&task_runner_provider, &task_graph_runner);
  LayerTreeImpl* tree = host_impl.active_tree();

  std::unique_ptr<LayerImpl> root_impl(LayerImpl::Create(tree, 1));
  std::unique_ptr<LayerImpl> touch_layer(LayerImpl::Create(tree, 2));

  root_impl->SetBounds(gfx::Size(100, 100));
  touch_layer->SetBounds(gfx::Size(50, 50));

  TouchActionRegion touch_action_region;
  touch_action_region.Union(kTouchActionNone, gfx::Rect(10, 10, 20, 30));
  touch_action_region.Union(kTouchActionNone, gfx::Rect(40, 10, 20, 20));
  touch_layer->SetTouchActionRegion(std::move(touch_action_region));

  root_impl->test_properties()->AddChild(std::move(touch_layer));
  tree->SetRootLayerForTesting(std::move(root_impl));
  tree->BuildPropertyTreesForTesting();

  std::string json = host_impl.LayerTreeAsJson();
  scoped_refptr<Layer> root = ParseTreeFromJson(json, nullptr);
  ASSERT_TRUE(root.get());
  EXPECT_TRUE(LayerTreesMatch(host_impl.active_tree()->root_layer_for_testing(),
                              root.get()));
}

}  // namespace cc
