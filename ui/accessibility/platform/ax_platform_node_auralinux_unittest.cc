// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Chromium cannot upgrade to ATK 2.12 API as it still needs to run
// valid builds for Ubuntu Trusty.
#define ATK_DISABLE_DEPRECATION_WARNINGS

#include <atk/atk.h>

#include "testing/gtest/include/gtest/gtest.h"
#include "ui/accessibility/platform/ax_platform_node_unittest.h"
#include "ui/accessibility/platform/test_ax_node_wrapper.h"

namespace ui {

class AXPlatformNodeAuraLinuxTest : public AXPlatformNodeTest {
 public:
  AXPlatformNodeAuraLinuxTest() {}
  ~AXPlatformNodeAuraLinuxTest() override {}

  void SetUp() override {}

 protected:
  AtkObject* AtkObjectFromNode(AXNode* node) {
    TestAXNodeWrapper* wrapper =
        TestAXNodeWrapper::GetOrCreate(tree_.get(), node);
    if (!wrapper)
      return nullptr;
    AXPlatformNode* ax_platform_node = wrapper->ax_platform_node();
    AtkObject* atk_object = ax_platform_node->GetNativeViewAccessible();
    return atk_object;
  }

  AtkObject* GetRootAtkObject() { return AtkObjectFromNode(GetRootNode()); }
};

//
// AtkObject tests
//

TEST_F(AXPlatformNodeAuraLinuxTest, TestAtkObjectDetachedObject) {
  AXNodeData root;
  root.id = 1;
  root.SetName("Name");
  Init(root);

  AtkObject* root_obj(GetRootAtkObject());
  ASSERT_TRUE(ATK_IS_OBJECT(root_obj));
  g_object_ref(root_obj);

  const gchar* name = atk_object_get_name(root_obj);
  EXPECT_STREQ("Name", name);

  AtkStateSet* state_set = atk_object_ref_state_set(root_obj);
  ASSERT_TRUE(ATK_IS_STATE_SET(state_set));
  EXPECT_FALSE(atk_state_set_contains_state(state_set, ATK_STATE_DEFUNCT));
  g_object_unref(state_set);

  tree_.reset(new AXTree());
  EXPECT_EQ(nullptr, atk_object_get_name(root_obj));

  state_set = atk_object_ref_state_set(root_obj);
  ASSERT_TRUE(ATK_IS_STATE_SET(state_set));
  EXPECT_TRUE(atk_state_set_contains_state(state_set, ATK_STATE_DEFUNCT));
  g_object_unref(state_set);

  g_object_unref(root_obj);
}

TEST_F(AXPlatformNodeAuraLinuxTest, TestAtkObjectName) {
  AXNodeData root;
  root.id = 1;
  root.SetName("Name");
  Init(root);

  AtkObject* root_obj(GetRootAtkObject());
  ASSERT_TRUE(ATK_IS_OBJECT(root_obj));
  g_object_ref(root_obj);

  const gchar* name = atk_object_get_name(root_obj);
  EXPECT_STREQ("Name", name);

  g_object_unref(root_obj);
}

TEST_F(AXPlatformNodeAuraLinuxTest, TestAtkObjectDescription) {
  AXNodeData root;
  root.id = 1;
  root.AddStringAttribute(ax::mojom::StringAttribute::kDescription,
                          "Description");
  Init(root);

  AtkObject* root_obj(GetRootAtkObject());
  ASSERT_TRUE(ATK_IS_OBJECT(root_obj));
  g_object_ref(root_obj);

  const gchar* description = atk_object_get_description(root_obj);
  EXPECT_STREQ("Description", description);

  g_object_unref(root_obj);
}

TEST_F(AXPlatformNodeAuraLinuxTest, TestAtkObjectRole) {
  AXNodeData root;
  root.id = 1;
  root.child_ids.push_back(2);

  AXNodeData child;
  child.id = 2;

  Init(root, child);
  AXNode* child_node = GetRootNode()->children()[0];

  child.role = ax::mojom::Role::kAlert;
  child_node->SetData(child);
  AtkObject* child_obj(AtkObjectFromNode(child_node));
  ASSERT_TRUE(ATK_IS_OBJECT(child_obj));
  g_object_ref(child_obj);
  EXPECT_EQ(ATK_ROLE_ALERT, atk_object_get_role(child_obj));
  g_object_unref(child_obj);

  child.role = ax::mojom::Role::kButton;
  child_node->SetData(child);
  child_obj = AtkObjectFromNode(child_node);
  ASSERT_TRUE(ATK_IS_OBJECT(child_obj));
  g_object_ref(child_obj);
  EXPECT_EQ(ATK_ROLE_PUSH_BUTTON, atk_object_get_role(child_obj));
  g_object_unref(child_obj);

  child.role = ax::mojom::Role::kCanvas;
  child_node->SetData(child);
  child_obj = AtkObjectFromNode(child_node);
  ASSERT_TRUE(ATK_IS_OBJECT(child_obj));
  g_object_ref(child_obj);
  EXPECT_EQ(ATK_ROLE_CANVAS, atk_object_get_role(child_obj));
  g_object_unref(child_obj);
}

TEST_F(AXPlatformNodeAuraLinuxTest, TestAtkObjectState) {
  AXNodeData root;
  root.id = 1;
  root.AddState(ax::mojom::State::kDefault);
  root.AddState(ax::mojom::State::kExpanded);

  Init(root);

  AtkObject* root_obj(GetRootAtkObject());
  ASSERT_TRUE(ATK_IS_OBJECT(root_obj));
  g_object_ref(root_obj);

  AtkStateSet* state_set = atk_object_ref_state_set(root_obj);
  ASSERT_TRUE(ATK_IS_STATE_SET(state_set));
  ASSERT_TRUE(atk_state_set_contains_state(state_set, ATK_STATE_DEFAULT));
  ASSERT_TRUE(atk_state_set_contains_state(state_set, ATK_STATE_EXPANDED));
  ASSERT_FALSE(atk_state_set_contains_state(state_set, ATK_STATE_SELECTED));
  g_object_unref(state_set);

  g_object_unref(root_obj);
}

TEST_F(AXPlatformNodeAuraLinuxTest, TestAtkObjectChildAndParent) {
  AXNodeData root;
  root.id = 1;
  root.child_ids.push_back(2);
  root.child_ids.push_back(3);

  AXNodeData button;
  button.role = ax::mojom::Role::kButton;
  button.id = 2;

  AXNodeData checkbox;
  checkbox.role = ax::mojom::Role::kCheckBox;
  checkbox.id = 3;

  Init(root, button, checkbox);
  AXNode* button_node = GetRootNode()->children()[0];
  AXNode* checkbox_node = GetRootNode()->children()[1];
  AtkObject* root_obj = GetRootAtkObject();
  AtkObject* button_obj = AtkObjectFromNode(button_node);
  AtkObject* checkbox_obj = AtkObjectFromNode(checkbox_node);

  ASSERT_TRUE(ATK_IS_OBJECT(root_obj));
  EXPECT_EQ(2, atk_object_get_n_accessible_children(root_obj));
  ASSERT_TRUE(ATK_IS_OBJECT(button_obj));
  EXPECT_EQ(0, atk_object_get_n_accessible_children(button_obj));
  ASSERT_TRUE(ATK_IS_OBJECT(checkbox_obj));
  EXPECT_EQ(0, atk_object_get_n_accessible_children(checkbox_obj));

  {
    AtkObject* result = atk_object_ref_accessible_child(root_obj, 0);
    EXPECT_TRUE(ATK_IS_OBJECT(root_obj));
    EXPECT_EQ(result, button_obj);
    g_object_unref(result);
  }
  {
    AtkObject* result = atk_object_ref_accessible_child(root_obj, 1);
    EXPECT_TRUE(ATK_IS_OBJECT(root_obj));
    EXPECT_EQ(result, checkbox_obj);
    g_object_unref(result);
  }

  // Now check parents.
  {
    AtkObject* result = atk_object_get_parent(button_obj);
    EXPECT_TRUE(ATK_IS_OBJECT(result));
    EXPECT_EQ(result, root_obj);
  }
  {
    AtkObject* result = atk_object_get_parent(checkbox_obj);
    EXPECT_TRUE(ATK_IS_OBJECT(result));
    EXPECT_EQ(result, root_obj);
  }
}

TEST_F(AXPlatformNodeAuraLinuxTest, TestAtkObjectIndexInParent) {
  AXNodeData root;
  root.id = 1;
  root.child_ids.push_back(2);
  root.child_ids.push_back(3);

  AXNodeData left;
  left.id = 2;

  AXNodeData right;
  right.id = 3;

  Init(root, left, right);

  AtkObject* root_obj(GetRootAtkObject());
  ASSERT_TRUE(ATK_IS_OBJECT(root_obj));
  g_object_ref(root_obj);

  AtkObject* left_obj = atk_object_ref_accessible_child(root_obj, 0);
  ASSERT_TRUE(ATK_IS_OBJECT(left_obj));
  AtkObject* right_obj = atk_object_ref_accessible_child(root_obj, 1);
  ASSERT_TRUE(ATK_IS_OBJECT(right_obj));

  EXPECT_EQ(0, atk_object_get_index_in_parent(left_obj));
  EXPECT_EQ(1, atk_object_get_index_in_parent(right_obj));

  g_object_unref(left_obj);
  g_object_unref(right_obj);
  g_object_unref(root_obj);
}

//
// AtkComponent tests
//

TEST_F(AXPlatformNodeAuraLinuxTest, TestAtkComponentRefAtPoint) {
  AXNodeData root;
  root.id = 0;
  root.child_ids.push_back(1);
  root.child_ids.push_back(2);
  root.location = gfx::RectF(0, 0, 30, 30);

  AXNodeData node1;
  node1.id = 1;
  node1.location = gfx::RectF(0, 0, 10, 10);
  node1.SetName("Name1");

  AXNodeData node2;
  node2.id = 2;
  node2.location = gfx::RectF(20, 20, 10, 10);
  node2.SetName("Name2");

  Init(root, node1, node2);

  AtkObject* root_obj(GetRootAtkObject());
  EXPECT_TRUE(ATK_IS_OBJECT(root_obj));
  EXPECT_TRUE(ATK_IS_COMPONENT(root_obj));
  g_object_ref(root_obj);

  AtkObject* child_obj = atk_component_ref_accessible_at_point(
      ATK_COMPONENT(root_obj), 50, 50, ATK_XY_SCREEN);
  EXPECT_EQ(nullptr, child_obj);

  // this is directly on node 1.
  child_obj = atk_component_ref_accessible_at_point(ATK_COMPONENT(root_obj), 5,
                                                    5, ATK_XY_SCREEN);
  ASSERT_NE(nullptr, child_obj);
  EXPECT_TRUE(ATK_IS_OBJECT(child_obj));

  const gchar* name = atk_object_get_name(child_obj);
  EXPECT_STREQ("Name1", name);

  g_object_unref(child_obj);
  g_object_unref(root_obj);
}

TEST_F(AXPlatformNodeAuraLinuxTest, TestAtkComponentsGetExtentsPositionSize) {
  AXNodeData root;
  root.id = 1;
  root.role = ax::mojom::Role::kWindow;
  root.location = gfx::RectF(10, 40, 800, 600);
  root.child_ids.push_back(2);

  AXNodeData child;
  child.id = 2;
  child.location = gfx::RectF(100, 150, 200, 200);
  Init(root, child);

  TestAXNodeWrapper::SetGlobalCoordinateOffset(gfx::Vector2d(100, 200));

  AtkObject* root_obj = GetRootAtkObject();
  ASSERT_TRUE(ATK_IS_OBJECT(root_obj));
  ASSERT_TRUE(ATK_IS_COMPONENT(root_obj));
  g_object_ref(root_obj);

  gint x_left, y_top, width, height;
  atk_component_get_extents(ATK_COMPONENT(root_obj), &x_left, &y_top, &width,
                            &height, ATK_XY_SCREEN);
  EXPECT_EQ(110, x_left);
  EXPECT_EQ(240, y_top);
  EXPECT_EQ(800, width);
  EXPECT_EQ(600, height);

  atk_component_get_position(ATK_COMPONENT(root_obj), &x_left, &y_top,
                             ATK_XY_SCREEN);
  EXPECT_EQ(110, x_left);
  EXPECT_EQ(240, y_top);

  atk_component_get_extents(ATK_COMPONENT(root_obj), &x_left, &y_top, &width,
                            &height, ATK_XY_WINDOW);
  EXPECT_EQ(110, x_left);
  EXPECT_EQ(240, y_top);
  EXPECT_EQ(800, width);
  EXPECT_EQ(600, height);

  atk_component_get_position(ATK_COMPONENT(root_obj), &x_left, &y_top,
                             ATK_XY_WINDOW);
  EXPECT_EQ(110, x_left);
  EXPECT_EQ(240, y_top);

  atk_component_get_size(ATK_COMPONENT(root_obj), &width, &height);
  EXPECT_EQ(800, width);
  EXPECT_EQ(600, height);

  AXNode* child_node = GetRootNode()->children()[0];
  AtkObject* child_obj = AtkObjectFromNode(child_node);
  ASSERT_TRUE(ATK_IS_OBJECT(child_obj));
  ASSERT_TRUE(ATK_IS_COMPONENT(child_obj));
  g_object_ref(child_obj);

  atk_component_get_extents(ATK_COMPONENT(child_obj), &x_left, &y_top, &width,
                            &height, ATK_XY_SCREEN);
  EXPECT_EQ(200, x_left);
  EXPECT_EQ(350, y_top);
  EXPECT_EQ(200, width);
  EXPECT_EQ(200, height);

  atk_component_get_extents(ATK_COMPONENT(child_obj), &x_left, &y_top, &width,
                            &height, ATK_XY_WINDOW);
  EXPECT_EQ(90, x_left);
  EXPECT_EQ(110, y_top);
  EXPECT_EQ(200, width);
  EXPECT_EQ(200, height);

  atk_component_get_extents(ATK_COMPONENT(child_obj), nullptr, &y_top, &width,
                            &height, ATK_XY_SCREEN);
  EXPECT_EQ(200, height);
  atk_component_get_extents(ATK_COMPONENT(child_obj), &x_left, nullptr, &width,
                            &height, ATK_XY_SCREEN);
  EXPECT_EQ(200, x_left);
  atk_component_get_extents(ATK_COMPONENT(child_obj), &x_left, &y_top, nullptr,
                            &height, ATK_XY_SCREEN);
  EXPECT_EQ(350, y_top);
  atk_component_get_extents(ATK_COMPONENT(child_obj), &x_left, &y_top, &width,
                            nullptr, ATK_XY_SCREEN);
  EXPECT_EQ(200, width);

  g_object_unref(child_obj);
  g_object_unref(root_obj);
}

//
// AtkValue tests
//

TEST_F(AXPlatformNodeAuraLinuxTest, TestAtkValueGetCurrentValue) {
  AXNodeData root;
  root.id = 1;
  root.role = ax::mojom::Role::kSlider;
  root.AddFloatAttribute(ax::mojom::FloatAttribute::kValueForRange, 5.0);
  Init(root);

  AtkObject* root_obj(GetRootAtkObject());
  ASSERT_TRUE(ATK_IS_OBJECT(root_obj));
  ASSERT_TRUE(ATK_IS_VALUE(root_obj));
  g_object_ref(root_obj);

  GValue current_value = G_VALUE_INIT;
  atk_value_get_current_value(ATK_VALUE(root_obj), &current_value);

  EXPECT_EQ(G_TYPE_FLOAT, G_VALUE_TYPE(&current_value));
  EXPECT_EQ(5.0, g_value_get_float(&current_value));

  g_value_unset(&current_value);
  g_object_unref(root_obj);
}

TEST_F(AXPlatformNodeAuraLinuxTest, TestAtkValueGetMaximumValue) {
  AXNodeData root;
  root.id = 1;
  root.role = ax::mojom::Role::kSlider;
  root.AddFloatAttribute(ax::mojom::FloatAttribute::kMaxValueForRange, 5.0);
  Init(root);

  AtkObject* root_obj(GetRootAtkObject());
  ASSERT_TRUE(ATK_IS_OBJECT(root_obj));
  ASSERT_TRUE(ATK_IS_VALUE(root_obj));
  g_object_ref(root_obj);

  GValue max_value = G_VALUE_INIT;
  atk_value_get_maximum_value(ATK_VALUE(root_obj), &max_value);

  EXPECT_EQ(G_TYPE_FLOAT, G_VALUE_TYPE(&max_value));
  EXPECT_EQ(5.0, g_value_get_float(&max_value));

  g_value_unset(&max_value);
  g_object_unref(root_obj);
}

TEST_F(AXPlatformNodeAuraLinuxTest, TestAtkValueGetMinimumValue) {
  AXNodeData root;
  root.id = 1;
  root.role = ax::mojom::Role::kSlider;
  root.AddFloatAttribute(ax::mojom::FloatAttribute::kMinValueForRange, 5.0);
  Init(root);

  AtkObject* root_obj(GetRootAtkObject());
  ASSERT_TRUE(ATK_IS_OBJECT(root_obj));
  ASSERT_TRUE(ATK_IS_VALUE(root_obj));
  g_object_ref(root_obj);

  GValue min_value = G_VALUE_INIT;
  atk_value_get_minimum_value(ATK_VALUE(root_obj), &min_value);

  EXPECT_EQ(G_TYPE_FLOAT, G_VALUE_TYPE(&min_value));
  EXPECT_EQ(5.0, g_value_get_float(&min_value));

  g_value_unset(&min_value);
  g_object_unref(root_obj);
}

TEST_F(AXPlatformNodeAuraLinuxTest, TestAtkValueGetMinimumIncrement) {
  AXNodeData root;
  root.id = 1;
  root.role = ax::mojom::Role::kSlider;
  root.AddFloatAttribute(ax::mojom::FloatAttribute::kStepValueForRange, 5.0);
  Init(root);

  AtkObject* root_obj(GetRootAtkObject());
  ASSERT_TRUE(ATK_IS_OBJECT(root_obj));
  ASSERT_TRUE(ATK_IS_VALUE(root_obj));
  g_object_ref(root_obj);

  GValue increment = G_VALUE_INIT;
  atk_value_get_minimum_increment(ATK_VALUE(root_obj), &increment);

  EXPECT_EQ(G_TYPE_FLOAT, G_VALUE_TYPE(&increment));
  EXPECT_EQ(5.0, g_value_get_float(&increment));

  g_value_unset(&increment);
  g_object_unref(root_obj);
}

//
// AtkHyperlinkImpl interface
//

TEST_F(AXPlatformNodeAuraLinuxTest, TestAtkHyperlink) {
  AXNodeData root;
  root.id = 1;
  root.role = ax::mojom::Role::kLink;
  root.AddStringAttribute(ax::mojom::StringAttribute::kUrl, "http://foo.com");
  Init(root);

  AtkObject* root_obj(GetRootAtkObject());
  ASSERT_TRUE(ATK_IS_OBJECT(root_obj));
  ASSERT_TRUE(ATK_IS_HYPERLINK_IMPL(root_obj));
  g_object_ref(root_obj);

  AtkHyperlink* hyperlink(
      atk_hyperlink_impl_get_hyperlink(ATK_HYPERLINK_IMPL(root_obj)));
  ASSERT_TRUE(ATK_IS_HYPERLINK(hyperlink));

  EXPECT_EQ(1, atk_hyperlink_get_n_anchors(hyperlink));
  gchar* uri = atk_hyperlink_get_uri(hyperlink, 0);
  EXPECT_STREQ("http://foo.com", uri);
  g_free(uri);

  g_object_unref(hyperlink);
  g_object_unref(root_obj);
}

}  // namespace ui
