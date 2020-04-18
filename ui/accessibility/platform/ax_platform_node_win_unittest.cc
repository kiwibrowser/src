// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/accessibility/platform/ax_platform_node.h"

#include <atlbase.h>
#include <atlcom.h>
#include <oleacc.h>
#include <wrl/client.h>

#include <memory>

#include "base/win/scoped_bstr.h"
#include "base/win/scoped_variant.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/iaccessible2/ia2_api_all.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/accessibility/platform/ax_platform_node_unittest.h"
#include "ui/accessibility/platform/ax_platform_node_win.h"
#include "ui/accessibility/platform/test_ax_node_wrapper.h"
#include "ui/base/win/atl_module.h"

using Microsoft::WRL::ComPtr;
using base::win::ScopedBstr;
using base::win::ScopedVariant;

namespace ui {

namespace {

// Most IAccessible functions require a VARIANT set to CHILDID_SELF as
// the first argument.
ScopedVariant SELF(CHILDID_SELF);

}  // namespace

class AXPlatformNodeWinTest : public ui::AXPlatformNodeTest {
 public:
  AXPlatformNodeWinTest() {}
  ~AXPlatformNodeWinTest() override {}

  void SetUp() override {
    win::CreateATLModuleIfNeeded();
  }

  void TearDown() override {
    // Destroy the tree and make sure we're not leaking any objects.
    tree_.reset(nullptr);
    ASSERT_EQ(0U, AXPlatformNodeWin::GetInstanceCountForTesting());
  }

 protected:
  ComPtr<IAccessible> IAccessibleFromNode(AXNode* node) {
    TestAXNodeWrapper* wrapper =
        TestAXNodeWrapper::GetOrCreate(tree_.get(), node);
    if (!wrapper)
      return ComPtr<IAccessible>();
    AXPlatformNode* ax_platform_node = wrapper->ax_platform_node();
    IAccessible* iaccessible = ax_platform_node->GetNativeViewAccessible();
    return ComPtr<IAccessible>(iaccessible);
  }

  ComPtr<IAccessible> GetRootIAccessible() {
    return IAccessibleFromNode(GetRootNode());
  }

  ComPtr<IAccessible2> ToIAccessible2(ComPtr<IUnknown> unknown) {
    CHECK(unknown);
    ComPtr<IServiceProvider> service_provider;
    unknown.CopyTo(service_provider.GetAddressOf());
    ComPtr<IAccessible2> result;
    CHECK(SUCCEEDED(service_provider->QueryService(IID_IAccessible2,
                                                   result.GetAddressOf())));
    return result;
  }

  ComPtr<IAccessible2> ToIAccessible2(ComPtr<IAccessible> accessible) {
    CHECK(accessible);
    ComPtr<IServiceProvider> service_provider;
    accessible.CopyTo(service_provider.GetAddressOf());
    ComPtr<IAccessible2> result;
    CHECK(SUCCEEDED(service_provider->QueryService(IID_IAccessible2,
                                                   result.GetAddressOf())));
    return result;
  }

  ComPtr<IAccessible2_2> ToIAccessible2_2(ComPtr<IAccessible> accessible) {
    CHECK(accessible);
    ComPtr<IServiceProvider> service_provider;
    accessible.CopyTo(service_provider.GetAddressOf());
    ComPtr<IAccessible2_2> result;
    CHECK(SUCCEEDED(service_provider->QueryService(IID_IAccessible2_2,
                                                   result.GetAddressOf())));
    return result;
  }

  void CheckVariantHasName(ScopedVariant& variant,
                           const wchar_t* expected_name) {
    ASSERT_NE(nullptr, variant.ptr());
    ComPtr<IAccessible> accessible;
    ASSERT_HRESULT_SUCCEEDED(
        V_DISPATCH(variant.ptr())
            ->QueryInterface(IID_PPV_ARGS(accessible.GetAddressOf())));
    ScopedBstr name;
    EXPECT_EQ(S_OK, accessible->get_accName(SELF, name.Receive()));
    EXPECT_STREQ(expected_name, name);
  }

  void CheckIUnknownHasName(ComPtr<IUnknown> unknown,
                            const wchar_t* expected_name) {
    ComPtr<IAccessible2> accessible = ToIAccessible2(unknown);
    ASSERT_NE(nullptr, accessible.Get());

    ScopedBstr name;
    EXPECT_EQ(S_OK, accessible->get_accName(SELF, name.Receive()));
    EXPECT_STREQ(expected_name, name);
  }

  ComPtr<IAccessibleTableCell> GetCellInTable() {
    ComPtr<IAccessible> root_obj(GetRootIAccessible());

    ComPtr<IAccessibleTable2> table;
    root_obj.CopyTo(table.GetAddressOf());
    if (!table)
      return ComPtr<IAccessibleTableCell>();

    ComPtr<IUnknown> cell;
    table->get_cellAt(1, 1, cell.GetAddressOf());
    if (!cell)
      return ComPtr<IAccessibleTableCell>();

    ComPtr<IAccessibleTableCell> table_cell;
    cell.CopyTo(table_cell.GetAddressOf());
    return table_cell;
  }
};

TEST_F(AXPlatformNodeWinTest, TestIAccessibleDetachedObject) {
  AXNodeData root;
  root.id = 1;
  root.SetName("Name");
  Init(root);

  ComPtr<IAccessible> root_obj(GetRootIAccessible());
  ScopedBstr name;
  EXPECT_EQ(S_OK, root_obj->get_accName(SELF, name.Receive()));
  EXPECT_STREQ(L"Name", name);

  tree_.reset(new AXTree());
  ScopedBstr name2;
  EXPECT_EQ(E_FAIL, root_obj->get_accName(SELF, name2.Receive()));
}

TEST_F(AXPlatformNodeWinTest, TestIAccessibleHitTest) {
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

  ComPtr<IAccessible> root_obj(GetRootIAccessible());

  ScopedVariant obj;

  // This is way outside of the root node
  EXPECT_EQ(S_FALSE, root_obj->accHitTest(50, 50, obj.Receive()));
  EXPECT_EQ(VT_EMPTY, obj.type());

  // this is directly on node 1.
  EXPECT_EQ(S_OK, root_obj->accHitTest(5, 5, obj.Receive()));
  ASSERT_NE(nullptr, obj.ptr());

  CheckVariantHasName(obj, L"Name1");
}

TEST_F(AXPlatformNodeWinTest, TestIAccessibleName) {
  AXNodeData root;
  root.id = 1;
  root.SetName("Name");
  Init(root);

  ComPtr<IAccessible> root_obj(GetRootIAccessible());
  ScopedBstr name;
  EXPECT_EQ(S_OK, root_obj->get_accName(SELF, name.Receive()));
  EXPECT_STREQ(L"Name", name);

  EXPECT_EQ(E_INVALIDARG, root_obj->get_accName(SELF, nullptr));
  ScopedVariant bad_id(999);
  ScopedBstr name2;
  EXPECT_EQ(E_INVALIDARG, root_obj->get_accName(bad_id, name2.Receive()));
}

TEST_F(AXPlatformNodeWinTest, TestIAccessibleDescription) {
  AXNodeData root;
  root.id = 1;
  root.AddStringAttribute(ax::mojom::StringAttribute::kDescription,
                          "Description");
  Init(root);

  ComPtr<IAccessible> root_obj(GetRootIAccessible());
  ScopedBstr description;
  EXPECT_EQ(S_OK, root_obj->get_accDescription(SELF, description.Receive()));
  EXPECT_STREQ(L"Description", description);

  EXPECT_EQ(E_INVALIDARG, root_obj->get_accDescription(SELF, nullptr));
  ScopedVariant bad_id(999);
  ScopedBstr d2;
  EXPECT_EQ(E_INVALIDARG, root_obj->get_accDescription(bad_id, d2.Receive()));
}

TEST_F(AXPlatformNodeWinTest, TestIAccessibleValue) {
  AXNodeData root;
  root.id = 1;
  root.AddStringAttribute(ax::mojom::StringAttribute::kValue, "Value");
  Init(root);

  ComPtr<IAccessible> root_obj(GetRootIAccessible());
  ScopedBstr value;
  EXPECT_EQ(S_OK, root_obj->get_accValue(SELF, value.Receive()));
  EXPECT_STREQ(L"Value", value);

  EXPECT_EQ(E_INVALIDARG, root_obj->get_accValue(SELF, nullptr));
  ScopedVariant bad_id(999);
  ScopedBstr v2;
  EXPECT_EQ(E_INVALIDARG, root_obj->get_accValue(bad_id, v2.Receive()));
}

TEST_F(AXPlatformNodeWinTest, TestIAccessibleShortcut) {
  AXNodeData root;
  root.id = 1;
  root.AddStringAttribute(ax::mojom::StringAttribute::kKeyShortcuts,
                          "Shortcut");
  Init(root);

  ComPtr<IAccessible> root_obj(GetRootIAccessible());
  ScopedBstr shortcut;
  EXPECT_EQ(S_OK, root_obj->get_accKeyboardShortcut(SELF, shortcut.Receive()));
  EXPECT_STREQ(L"Shortcut", shortcut);

  EXPECT_EQ(E_INVALIDARG, root_obj->get_accKeyboardShortcut(SELF, nullptr));
  ScopedVariant bad_id(999);
  ScopedBstr k2;
  EXPECT_EQ(E_INVALIDARG,
            root_obj->get_accKeyboardShortcut(bad_id, k2.Receive()));
}

TEST_F(AXPlatformNodeWinTest,
       TestIAccessibleSelectionListBoxOptionNothingSelected) {
  AXNodeData list;
  list.id = 0;
  list.role = ax::mojom::Role::kListBox;

  AXNodeData list_item_1;
  list_item_1.id = 1;
  list_item_1.role = ax::mojom::Role::kListBoxOption;
  list_item_1.SetName("Name1");

  AXNodeData list_item_2;
  list_item_2.id = 2;
  list_item_2.role = ax::mojom::Role::kListBoxOption;
  list_item_2.SetName("Name2");

  list.child_ids.push_back(list_item_1.id);
  list.child_ids.push_back(list_item_2.id);

  Init(list, list_item_1, list_item_2);

  ComPtr<IAccessible> root_obj(GetRootIAccessible());
  ASSERT_NE(nullptr, root_obj.Get());

  ScopedVariant selection;
  EXPECT_EQ(S_OK, root_obj->get_accSelection(selection.Receive()));
  EXPECT_EQ(VT_EMPTY, selection.type());
}

TEST_F(AXPlatformNodeWinTest,
       TestIAccessibleSelectionListBoxOptionOneSelected) {
  AXNodeData list;
  list.id = 0;
  list.role = ax::mojom::Role::kListBox;

  AXNodeData list_item_1;
  list_item_1.id = 1;
  list_item_1.role = ax::mojom::Role::kListBoxOption;
  list_item_1.AddBoolAttribute(ax::mojom::BoolAttribute::kSelected, true);
  list_item_1.SetName("Name1");

  AXNodeData list_item_2;
  list_item_2.id = 2;
  list_item_2.role = ax::mojom::Role::kListBoxOption;
  list_item_2.SetName("Name2");

  list.child_ids.push_back(list_item_1.id);
  list.child_ids.push_back(list_item_2.id);

  Init(list, list_item_1, list_item_2);

  ComPtr<IAccessible> root_obj(GetRootIAccessible());
  ASSERT_NE(nullptr, root_obj.Get());

  ScopedVariant selection;
  EXPECT_EQ(S_OK, root_obj->get_accSelection(selection.Receive()));
  EXPECT_EQ(VT_DISPATCH, selection.type());

  CheckVariantHasName(selection, L"Name1");
}

TEST_F(AXPlatformNodeWinTest,
       TestIAccessibleSelectionListBoxOptionMultipleSelected) {
  AXNodeData list;
  list.id = 0;
  list.role = ax::mojom::Role::kListBox;

  AXNodeData list_item_1;
  list_item_1.id = 1;
  list_item_1.role = ax::mojom::Role::kListBoxOption;
  list_item_1.AddBoolAttribute(ax::mojom::BoolAttribute::kSelected, true);
  list_item_1.SetName("Name1");

  AXNodeData list_item_2;
  list_item_2.id = 2;
  list_item_2.role = ax::mojom::Role::kListBoxOption;
  list_item_2.AddBoolAttribute(ax::mojom::BoolAttribute::kSelected, true);
  list_item_2.SetName("Name2");

  AXNodeData list_item_3;
  list_item_3.id = 3;
  list_item_3.role = ax::mojom::Role::kListBoxOption;
  list_item_3.SetName("Name3");

  list.child_ids.push_back(list_item_1.id);
  list.child_ids.push_back(list_item_2.id);
  list.child_ids.push_back(list_item_3.id);

  Init(list, list_item_1, list_item_2, list_item_3);

  ComPtr<IAccessible> root_obj(GetRootIAccessible());
  ASSERT_NE(nullptr, root_obj.Get());

  ScopedVariant selection;
  EXPECT_EQ(S_OK, root_obj->get_accSelection(selection.Receive()));
  EXPECT_EQ(VT_UNKNOWN, selection.type());
  ASSERT_NE(nullptr, selection.ptr());

  // Loop through the selections and  make sure we have the right ones.
  ComPtr<IEnumVARIANT> accessibles;
  ASSERT_HRESULT_SUCCEEDED(
      V_UNKNOWN(selection.ptr())
          ->QueryInterface(IID_PPV_ARGS(accessibles.GetAddressOf())));
  ULONG retrieved_count;

  // Check out the first selected item.
  {
    ScopedVariant item;
    HRESULT hr = accessibles->Next(1, item.Receive(), &retrieved_count);
    EXPECT_EQ(S_OK, hr);

    ComPtr<IAccessible> accessible;
    ASSERT_HRESULT_SUCCEEDED(
        V_DISPATCH(item.ptr())
            ->QueryInterface(IID_PPV_ARGS(accessible.GetAddressOf())));
    ScopedBstr name;
    EXPECT_EQ(S_OK, accessible->get_accName(SELF, name.Receive()));
    EXPECT_STREQ(L"Name1", name);
  }

  // And the second selected element.
  {
    ScopedVariant item;
    HRESULT hr = accessibles->Next(1, item.Receive(), &retrieved_count);
    EXPECT_EQ(S_OK, hr);

    ComPtr<IAccessible> accessible;
    ASSERT_HRESULT_SUCCEEDED(
        V_DISPATCH(item.ptr())
            ->QueryInterface(IID_PPV_ARGS(accessible.GetAddressOf())));
    ScopedBstr name;
    EXPECT_EQ(S_OK, accessible->get_accName(SELF, name.Receive()));
    EXPECT_STREQ(L"Name2", name);
  }

  // There shouldn't be any more selected.
  {
    ScopedVariant item;
    HRESULT hr = accessibles->Next(1, item.Receive(), &retrieved_count);
    EXPECT_EQ(S_FALSE, hr);
  }
}

TEST_F(AXPlatformNodeWinTest, TestIAccessibleSelectionTableNothingSelected) {
  Init(Build3X3Table());

  ComPtr<IAccessible> root_obj(GetRootIAccessible());
  ASSERT_NE(nullptr, root_obj.Get());

  ScopedVariant selection;
  EXPECT_EQ(S_OK, root_obj->get_accSelection(selection.Receive()));
  EXPECT_EQ(VT_EMPTY, selection.type());
}

TEST_F(AXPlatformNodeWinTest, TestIAccessibleSelectionTableRowOneSelected) {
  AXTreeUpdate update = Build3X3Table();

  // 5 == table_row_1
  update.nodes[5].AddBoolAttribute(ax::mojom::BoolAttribute::kSelected, true);

  Init(update);

  ComPtr<IAccessible> root_obj(GetRootIAccessible());
  ASSERT_NE(nullptr, root_obj.Get());

  ScopedVariant selection;
  EXPECT_EQ(S_OK, root_obj->get_accSelection(selection.Receive()));
  EXPECT_EQ(VT_DISPATCH, selection.type());
  ASSERT_NE(nullptr, selection.ptr());

  ComPtr<IAccessible> row;
  ASSERT_HRESULT_SUCCEEDED(
      V_DISPATCH(selection.ptr())
          ->QueryInterface(IID_PPV_ARGS(row.GetAddressOf())));

  ScopedVariant role;
  EXPECT_HRESULT_SUCCEEDED(row->get_accRole(SELF, role.Receive()));
  EXPECT_EQ(ROLE_SYSTEM_ROW, V_I4(role.ptr()));
}

TEST_F(AXPlatformNodeWinTest,
       TestIAccessibleSelectionTableRowMultipleSelected) {
  AXTreeUpdate update = Build3X3Table();

  // 5 == table_row_1
  // 9 == table_row_2
  update.nodes[5].AddBoolAttribute(ax::mojom::BoolAttribute::kSelected, true);
  update.nodes[9].AddBoolAttribute(ax::mojom::BoolAttribute::kSelected, true);

  Init(update);

  ComPtr<IAccessible> root_obj(GetRootIAccessible());
  ASSERT_NE(nullptr, root_obj.Get());

  ScopedVariant selection;
  EXPECT_EQ(S_OK, root_obj->get_accSelection(selection.Receive()));
  EXPECT_EQ(VT_UNKNOWN, selection.type());
  ASSERT_NE(nullptr, selection.ptr());

  // Loop through the selections and  make sure we have the right ones.
  ComPtr<IEnumVARIANT> accessibles;
  ASSERT_HRESULT_SUCCEEDED(
      V_UNKNOWN(selection.ptr())
          ->QueryInterface(IID_PPV_ARGS(accessibles.GetAddressOf())));
  ULONG retrieved_count;

  // Check out the first selected row.
  {
    ScopedVariant item;
    HRESULT hr = accessibles->Next(1, item.Receive(), &retrieved_count);
    EXPECT_EQ(S_OK, hr);

    ComPtr<IAccessible> accessible;
    ASSERT_HRESULT_SUCCEEDED(
        V_DISPATCH(item.ptr())
            ->QueryInterface(IID_PPV_ARGS(accessible.GetAddressOf())));
    ScopedVariant role;
    EXPECT_HRESULT_SUCCEEDED(accessible->get_accRole(SELF, role.Receive()));
    EXPECT_EQ(ROLE_SYSTEM_ROW, V_I4(role.ptr()));
  }

  // And the second selected element.
  {
    ScopedVariant item;
    HRESULT hr = accessibles->Next(1, item.Receive(), &retrieved_count);
    EXPECT_EQ(S_OK, hr);

    ComPtr<IAccessible> accessible;
    ASSERT_HRESULT_SUCCEEDED(
        V_DISPATCH(item.ptr())
            ->QueryInterface(IID_PPV_ARGS(accessible.GetAddressOf())));
    ScopedVariant role;
    EXPECT_HRESULT_SUCCEEDED(accessible->get_accRole(SELF, role.Receive()));
    EXPECT_EQ(ROLE_SYSTEM_ROW, V_I4(role.ptr()));
  }

  // There shouldn't be any more selected.
  {
    ScopedVariant item;
    HRESULT hr = accessibles->Next(1, item.Receive(), &retrieved_count);
    EXPECT_EQ(S_FALSE, hr);
  }
}

TEST_F(AXPlatformNodeWinTest, TestIAccessibleSelectionTableCellOneSelected) {
  AXTreeUpdate update = Build3X3Table();

  // 7 == table_cell_1
  update.nodes[7].AddBoolAttribute(ax::mojom::BoolAttribute::kSelected, true);

  Init(update);

  ComPtr<IAccessible> root_obj(GetRootIAccessible());
  ASSERT_NE(nullptr, root_obj.Get());

  ComPtr<IDispatch> row2;
  ASSERT_HRESULT_SUCCEEDED(
      root_obj->get_accChild(ScopedVariant(2), row2.GetAddressOf()));
  ComPtr<IAccessible> row2_accessible;
  ASSERT_HRESULT_SUCCEEDED(row2.As(&row2_accessible));

  ScopedVariant selection;
  EXPECT_EQ(S_OK, row2_accessible->get_accSelection(selection.Receive()));
  EXPECT_EQ(VT_DISPATCH, selection.type());
  ASSERT_NE(nullptr, selection.ptr());

  ComPtr<IAccessible> cell;
  ASSERT_HRESULT_SUCCEEDED(
      V_DISPATCH(selection.ptr())
          ->QueryInterface(IID_PPV_ARGS(cell.GetAddressOf())));

  ScopedVariant role;
  EXPECT_HRESULT_SUCCEEDED(cell->get_accRole(SELF, role.Receive()));
  EXPECT_EQ(ROLE_SYSTEM_CELL, V_I4(role.ptr()));

  ScopedBstr name;
  EXPECT_EQ(S_OK, cell->get_accName(SELF, name.Receive()));
  EXPECT_STREQ(L"1", name);
}

TEST_F(AXPlatformNodeWinTest,
       TestIAccessibleSelectionTableCellMultipleSelected) {
  AXTreeUpdate update = Build3X3Table();

  // 11 == table_cell_3
  // 12 == table_cell_4
  update.nodes[11].AddBoolAttribute(ax::mojom::BoolAttribute::kSelected, true);
  update.nodes[12].AddBoolAttribute(ax::mojom::BoolAttribute::kSelected, true);

  Init(update);

  ComPtr<IAccessible> root_obj(GetRootIAccessible());
  ASSERT_NE(nullptr, root_obj.Get());

  ComPtr<IDispatch> row3;
  ASSERT_HRESULT_SUCCEEDED(
      root_obj->get_accChild(ScopedVariant(3), row3.GetAddressOf()));
  ComPtr<IAccessible> row3_accessible;
  ASSERT_HRESULT_SUCCEEDED(row3.As(&row3_accessible));

  ScopedVariant selection;
  EXPECT_EQ(S_OK, row3_accessible->get_accSelection(selection.Receive()));
  EXPECT_EQ(VT_UNKNOWN, selection.type());
  ASSERT_NE(nullptr, selection.ptr());

  // Loop through the selections and  make sure we have the right ones.
  ComPtr<IEnumVARIANT> accessibles;
  ASSERT_HRESULT_SUCCEEDED(
      V_UNKNOWN(selection.ptr())
          ->QueryInterface(IID_PPV_ARGS(accessibles.GetAddressOf())));
  ULONG retrieved_count;

  // Check out the first selected cell.
  {
    ScopedVariant item;
    HRESULT hr = accessibles->Next(1, item.Receive(), &retrieved_count);
    EXPECT_EQ(S_OK, hr);

    ComPtr<IAccessible> accessible;
    ASSERT_HRESULT_SUCCEEDED(
        V_DISPATCH(item.ptr())
            ->QueryInterface(IID_PPV_ARGS(accessible.GetAddressOf())));
    ScopedBstr name;
    EXPECT_EQ(S_OK, accessible->get_accName(SELF, name.Receive()));
    EXPECT_STREQ(L"3", name);
  }

  // And the second selected cell.
  {
    ScopedVariant item;
    HRESULT hr = accessibles->Next(1, item.Receive(), &retrieved_count);
    EXPECT_EQ(S_OK, hr);

    ComPtr<IAccessible> accessible;
    ASSERT_HRESULT_SUCCEEDED(
        V_DISPATCH(item.ptr())
            ->QueryInterface(IID_PPV_ARGS(accessible.GetAddressOf())));
    ScopedBstr name;
    EXPECT_EQ(S_OK, accessible->get_accName(SELF, name.Receive()));
    EXPECT_STREQ(L"4", name);
  }

  // There shouldn't be any more selected.
  {
    ScopedVariant item;
    HRESULT hr = accessibles->Next(1, item.Receive(), &retrieved_count);
    EXPECT_EQ(S_FALSE, hr);
  }
}

TEST_F(AXPlatformNodeWinTest, TestIAccessibleRole) {
  AXNodeData root;
  root.id = 1;
  root.child_ids.push_back(2);

  AXNodeData child;
  child.id = 2;

  Init(root, child);
  AXNode* child_node = GetRootNode()->children()[0];
  ComPtr<IAccessible> child_iaccessible(IAccessibleFromNode(child_node));

  ScopedVariant role;

  child.role = ax::mojom::Role::kAlert;
  child_node->SetData(child);
  EXPECT_EQ(S_OK, child_iaccessible->get_accRole(SELF, role.Receive()));
  EXPECT_EQ(ROLE_SYSTEM_ALERT, V_I4(role.ptr()));

  child.role = ax::mojom::Role::kButton;
  child_node->SetData(child);
  EXPECT_EQ(S_OK, child_iaccessible->get_accRole(SELF, role.Receive()));
  EXPECT_EQ(ROLE_SYSTEM_PUSHBUTTON, V_I4(role.ptr()));

  child.role = ax::mojom::Role::kPopUpButton;
  child_node->SetData(child);
  EXPECT_EQ(S_OK, child_iaccessible->get_accRole(SELF, role.Receive()));
  EXPECT_EQ(ROLE_SYSTEM_BUTTONMENU, V_I4(role.ptr()));

  EXPECT_EQ(E_INVALIDARG, child_iaccessible->get_accRole(SELF, nullptr));
  ScopedVariant bad_id(999);
  EXPECT_EQ(E_INVALIDARG,
            child_iaccessible->get_accRole(bad_id, role.Receive()));
}

TEST_F(AXPlatformNodeWinTest, TestIAccessibleLocation) {
  AXNodeData root;
  root.id = 1;
  root.location = gfx::RectF(10, 40, 800, 600);
  Init(root);

  TestAXNodeWrapper::SetGlobalCoordinateOffset(gfx::Vector2d(100, 200));

  LONG x_left, y_top, width, height;
  EXPECT_EQ(S_OK, GetRootIAccessible()->accLocation(&x_left, &y_top, &width,
                                                    &height, SELF));
  EXPECT_EQ(110, x_left);
  EXPECT_EQ(240, y_top);
  EXPECT_EQ(800, width);
  EXPECT_EQ(600, height);

  EXPECT_EQ(E_INVALIDARG, GetRootIAccessible()->accLocation(
                              nullptr, &y_top, &width, &height, SELF));
  EXPECT_EQ(E_INVALIDARG, GetRootIAccessible()->accLocation(
                              &x_left, nullptr, &width, &height, SELF));
  EXPECT_EQ(E_INVALIDARG, GetRootIAccessible()->accLocation(
                              &x_left, &y_top, nullptr, &height, SELF));
  EXPECT_EQ(E_INVALIDARG, GetRootIAccessible()->accLocation(
                              &x_left, &y_top, &width, nullptr, SELF));
  ScopedVariant bad_id(999);
  EXPECT_EQ(E_INVALIDARG, GetRootIAccessible()->accLocation(
                              &x_left, &y_top, &width, &height, bad_id));
}

TEST_F(AXPlatformNodeWinTest, TestIAccessibleChildAndParent) {
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
  ComPtr<IAccessible> root_iaccessible(GetRootIAccessible());
  ComPtr<IAccessible> button_iaccessible(IAccessibleFromNode(button_node));
  ComPtr<IAccessible> checkbox_iaccessible(IAccessibleFromNode(checkbox_node));

  LONG child_count;
  EXPECT_EQ(S_OK, root_iaccessible->get_accChildCount(&child_count));
  EXPECT_EQ(2L, child_count);
  EXPECT_EQ(S_OK, button_iaccessible->get_accChildCount(&child_count));
  EXPECT_EQ(0L, child_count);
  EXPECT_EQ(S_OK, checkbox_iaccessible->get_accChildCount(&child_count));
  EXPECT_EQ(0L, child_count);

  {
    ComPtr<IDispatch> result;
    EXPECT_EQ(S_OK,
              root_iaccessible->get_accChild(SELF, result.GetAddressOf()));
    EXPECT_EQ(result.Get(), root_iaccessible.Get());
  }

  {
    ComPtr<IDispatch> result;
    ScopedVariant child1(1);
    EXPECT_EQ(S_OK,
              root_iaccessible->get_accChild(child1, result.GetAddressOf()));
    EXPECT_EQ(result.Get(), button_iaccessible.Get());
  }

  {
    ComPtr<IDispatch> result;
    ScopedVariant child2(2);
    EXPECT_EQ(S_OK,
              root_iaccessible->get_accChild(child2, result.GetAddressOf()));
    EXPECT_EQ(result.Get(), checkbox_iaccessible.Get());
  }

  {
    // Asking for child id 3 should fail.
    ComPtr<IDispatch> result;
    ScopedVariant child3(3);
    EXPECT_EQ(E_INVALIDARG,
              root_iaccessible->get_accChild(child3, result.GetAddressOf()));
  }

  // We should be able to ask for the button by its unique id too.
  LONG button_unique_id;
  ComPtr<IAccessible2> button_iaccessible2 = ToIAccessible2(button_iaccessible);
  button_iaccessible2->get_uniqueID(&button_unique_id);
  ASSERT_LT(button_unique_id, 0);
  {
    ComPtr<IDispatch> result;
    ScopedVariant button_id_variant(button_unique_id);
    EXPECT_EQ(S_OK, root_iaccessible->get_accChild(button_id_variant,
                                                   result.GetAddressOf()));
    EXPECT_EQ(result.Get(), button_iaccessible.Get());
  }

  // We shouldn't be able to ask for the root node by its unique ID
  // from one of its children, though.
  LONG root_unique_id;
  ComPtr<IAccessible2> root_iaccessible2 = ToIAccessible2(root_iaccessible);
  root_iaccessible2->get_uniqueID(&root_unique_id);
  ASSERT_LT(root_unique_id, 0);
  {
    ComPtr<IDispatch> result;
    ScopedVariant root_id_variant(root_unique_id);
    EXPECT_EQ(E_INVALIDARG, button_iaccessible->get_accChild(
                                root_id_variant, result.GetAddressOf()));
  }

  // Now check parents.
  {
    ComPtr<IDispatch> result;
    EXPECT_EQ(S_OK, button_iaccessible->get_accParent(result.GetAddressOf()));
    EXPECT_EQ(result.Get(), root_iaccessible.Get());
  }

  {
    ComPtr<IDispatch> result;
    EXPECT_EQ(S_OK, checkbox_iaccessible->get_accParent(result.GetAddressOf()));
    EXPECT_EQ(result.Get(), root_iaccessible.Get());
  }

  {
    ComPtr<IDispatch> result;
    EXPECT_EQ(S_FALSE, root_iaccessible->get_accParent(result.GetAddressOf()));
  }
}

TEST_F(AXPlatformNodeWinTest, TestIAccessible2IndexInParent) {
  AXNodeData root;
  root.id = 1;
  root.child_ids.push_back(2);
  root.child_ids.push_back(3);

  AXNodeData left;
  left.id = 2;

  AXNodeData right;
  right.id = 3;

  Init(root, left, right);
  ComPtr<IAccessible> root_iaccessible(GetRootIAccessible());
  ComPtr<IAccessible2> root_iaccessible2 = ToIAccessible2(root_iaccessible);
  ComPtr<IAccessible> left_iaccessible(
      IAccessibleFromNode(GetRootNode()->children()[0]));
  ComPtr<IAccessible2> left_iaccessible2 = ToIAccessible2(left_iaccessible);
  ComPtr<IAccessible> right_iaccessible(
      IAccessibleFromNode(GetRootNode()->children()[1]));
  ComPtr<IAccessible2> right_iaccessible2 = ToIAccessible2(right_iaccessible);

  LONG index;
  EXPECT_EQ(E_FAIL, root_iaccessible2->get_indexInParent(&index));

  EXPECT_EQ(S_OK, left_iaccessible2->get_indexInParent(&index));
  EXPECT_EQ(0, index);

  EXPECT_EQ(S_OK, right_iaccessible2->get_indexInParent(&index));
  EXPECT_EQ(1, index);
}

TEST_F(AXPlatformNodeWinTest, TestAccNavigate) {
  AXNodeData root;
  root.id = 1;
  root.role = ax::mojom::Role::kRootWebArea;

  AXNodeData child1;
  child1.id = 2;
  child1.role = ax::mojom::Role::kStaticText;
  root.child_ids.push_back(2);

  AXNodeData child2;
  child2.id = 3;
  child2.role = ax::mojom::Role::kStaticText;
  root.child_ids.push_back(3);

  Init(root, child1, child2);
  ComPtr<IAccessible> ia_root(GetRootIAccessible());
  ComPtr<IDispatch> disp_root;
  ASSERT_HRESULT_SUCCEEDED(ia_root.CopyTo(disp_root.GetAddressOf()));
  ScopedVariant var_root(disp_root.Get());
  ComPtr<IAccessible> ia_child1(
      IAccessibleFromNode(GetRootNode()->children()[0]));
  ComPtr<IDispatch> disp_child1;
  ASSERT_HRESULT_SUCCEEDED(ia_child1.CopyTo(disp_child1.GetAddressOf()));
  ScopedVariant var_child1(disp_child1.Get());
  ComPtr<IAccessible> ia_child2(
      IAccessibleFromNode(GetRootNode()->children()[1]));
  ComPtr<IDispatch> disp_child2;
  ASSERT_HRESULT_SUCCEEDED(ia_child2.CopyTo(disp_child2.GetAddressOf()));
  ScopedVariant var_child2(disp_child2.Get());
  ScopedVariant end;

  // Invalid arguments.
  EXPECT_EQ(
      E_INVALIDARG,
      ia_root->accNavigate(NAVDIR_NEXT, ScopedVariant::kEmptyVariant, nullptr));
  EXPECT_EQ(E_INVALIDARG,
            ia_child1->accNavigate(NAVDIR_NEXT, ScopedVariant::kEmptyVariant,
                                   end.AsInput()));
  EXPECT_EQ(VT_EMPTY, end.type());

  // Navigating to first/last child should only be from self.
  EXPECT_EQ(E_INVALIDARG,
            ia_root->accNavigate(NAVDIR_FIRSTCHILD, var_root, end.AsInput()));
  EXPECT_EQ(VT_EMPTY, end.type());
  EXPECT_EQ(E_INVALIDARG,
            ia_root->accNavigate(NAVDIR_LASTCHILD, var_root, end.AsInput()));
  EXPECT_EQ(VT_EMPTY, end.type());

  // Spatial directions are not supported.
  EXPECT_EQ(E_NOTIMPL, ia_child1->accNavigate(NAVDIR_UP, SELF, end.AsInput()));
  EXPECT_EQ(E_NOTIMPL, ia_root->accNavigate(NAVDIR_DOWN, SELF, end.AsInput()));
  EXPECT_EQ(E_NOTIMPL,
            ia_child1->accNavigate(NAVDIR_RIGHT, SELF, end.AsInput()));
  EXPECT_EQ(E_NOTIMPL,
            ia_child2->accNavigate(NAVDIR_LEFT, SELF, end.AsInput()));
  EXPECT_EQ(VT_EMPTY, end.type());

  // Logical directions should be supported.
  EXPECT_EQ(S_OK, ia_root->accNavigate(NAVDIR_FIRSTCHILD, SELF, end.AsInput()));
  EXPECT_EQ(0, var_child1.Compare(end));
  EXPECT_EQ(S_OK, ia_root->accNavigate(NAVDIR_LASTCHILD, SELF, end.AsInput()));
  EXPECT_EQ(0, var_child2.Compare(end));

  EXPECT_EQ(S_OK, ia_child1->accNavigate(NAVDIR_NEXT, SELF, end.AsInput()));
  EXPECT_EQ(0, var_child2.Compare(end));
  EXPECT_EQ(S_OK, ia_child2->accNavigate(NAVDIR_PREVIOUS, SELF, end.AsInput()));
  EXPECT_EQ(0, var_child1.Compare(end));

  // Child indices can also be passed by variant.
  // Indices are one-based.
  EXPECT_EQ(S_OK,
            ia_root->accNavigate(NAVDIR_NEXT, ScopedVariant(1), end.AsInput()));
  EXPECT_EQ(0, var_child2.Compare(end));
  EXPECT_EQ(S_OK, ia_root->accNavigate(NAVDIR_PREVIOUS, ScopedVariant(2),
                                       end.AsInput()));
  EXPECT_EQ(0, var_child1.Compare(end));

  // Test out-of-bounds.
  EXPECT_EQ(S_FALSE,
            ia_child1->accNavigate(NAVDIR_PREVIOUS, SELF, end.AsInput()));
  EXPECT_EQ(VT_EMPTY, end.type());
  EXPECT_EQ(S_FALSE, ia_child2->accNavigate(NAVDIR_NEXT, SELF, end.AsInput()));
  EXPECT_EQ(VT_EMPTY, end.type());

  EXPECT_EQ(S_FALSE, ia_root->accNavigate(NAVDIR_PREVIOUS, ScopedVariant(1),
                                          end.AsInput()));
  EXPECT_EQ(VT_EMPTY, end.type());
  EXPECT_EQ(S_FALSE,
            ia_root->accNavigate(NAVDIR_NEXT, ScopedVariant(2), end.AsInput()));
  EXPECT_EQ(VT_EMPTY, end.type());
}

TEST_F(AXPlatformNodeWinTest, TestIAccessible2TextFieldSetSelection) {
  Init(BuildTextField());

  ComPtr<IAccessible2> ia2_text_field = ToIAccessible2(GetRootIAccessible());
  ComPtr<IAccessibleText> text_field;
  ia2_text_field.CopyTo(text_field.GetAddressOf());
  ASSERT_NE(nullptr, text_field.Get());

  EXPECT_HRESULT_SUCCEEDED(text_field->setSelection(0, 0, 1));
  EXPECT_HRESULT_SUCCEEDED(text_field->setSelection(0, 1, 0));
  EXPECT_HRESULT_SUCCEEDED(text_field->setSelection(0, 2, 2));
  EXPECT_HRESULT_SUCCEEDED(text_field->setSelection(0, IA2_TEXT_OFFSET_CARET,
                                                    IA2_TEXT_OFFSET_LENGTH));

  EXPECT_HRESULT_FAILED(text_field->setSelection(1, 0, 0));
  EXPECT_HRESULT_FAILED(text_field->setSelection(0, 0, 50));
}

// This test is disabled until UpdateStep2ComputeHypertext is migrated over
// to AXPlatformNodeWin because |hypertext_| is only initialized
// on the BrowserAccessibility side.
TEST_F(AXPlatformNodeWinTest,
       DISABLED_TestIAccessible2ContentEditableSetSelection) {
  Init(BuildContentEditable());

  ComPtr<IAccessible2> ia2_text_field = ToIAccessible2(GetRootIAccessible());
  ComPtr<IAccessibleText> content_editable;
  ia2_text_field.CopyTo(content_editable.GetAddressOf());
  ASSERT_NE(nullptr, content_editable.Get());

  EXPECT_HRESULT_SUCCEEDED(content_editable->setSelection(0, 0, 1));
  EXPECT_HRESULT_SUCCEEDED(content_editable->setSelection(0, 1, 0));
  EXPECT_HRESULT_SUCCEEDED(content_editable->setSelection(0, 2, 2));
  EXPECT_HRESULT_SUCCEEDED(content_editable->setSelection(
      0, IA2_TEXT_OFFSET_CARET, IA2_TEXT_OFFSET_LENGTH));

  EXPECT_HRESULT_FAILED(content_editable->setSelection(1, 0, 0));
  EXPECT_HRESULT_FAILED(content_editable->setSelection(0, 0, 50));
}

TEST_F(AXPlatformNodeWinTest, TestIAccessibleTableGetAccessibilityAt) {
  Init(Build3X3Table());

  ComPtr<IAccessible> root_obj(GetRootIAccessible());

  ComPtr<IAccessibleTable> result;
  root_obj.CopyTo(result.GetAddressOf());
  ASSERT_NE(nullptr, result.Get());

  ComPtr<IUnknown> cell_1;
  EXPECT_EQ(S_OK, result->get_accessibleAt(1, 1, cell_1.GetAddressOf()));
  CheckIUnknownHasName(cell_1, L"1");

  ComPtr<IUnknown> cell_2;
  EXPECT_EQ(S_OK, result->get_accessibleAt(1, 2, cell_2.GetAddressOf()));
  CheckIUnknownHasName(cell_2, L"2");

  ComPtr<IUnknown> cell_3;
  EXPECT_EQ(S_OK, result->get_accessibleAt(2, 1, cell_3.GetAddressOf()));
  CheckIUnknownHasName(cell_3, L"3");

  ComPtr<IUnknown> cell_4;
  EXPECT_EQ(S_OK, result->get_accessibleAt(2, 2, cell_4.GetAddressOf()));
  CheckIUnknownHasName(cell_4, L"4");
}

TEST_F(AXPlatformNodeWinTest,
       TestIAccessibleTableGetAccessibilityAtOutOfBounds) {
  Init(Build3X3Table());

  ComPtr<IAccessible> root_obj(GetRootIAccessible());

  ComPtr<IAccessibleTable> result;
  root_obj.CopyTo(result.GetAddressOf());
  ASSERT_NE(nullptr, result.Get());

  {
    ComPtr<IUnknown> cell;
    EXPECT_EQ(E_INVALIDARG,
              result->get_accessibleAt(-1, -1, cell.GetAddressOf()));
  }

  {
    ComPtr<IUnknown> cell;
    EXPECT_EQ(E_INVALIDARG,
              result->get_accessibleAt(0, 5, cell.GetAddressOf()));
  }

  {
    ComPtr<IUnknown> cell;
    EXPECT_EQ(E_INVALIDARG,
              result->get_accessibleAt(5, 0, cell.GetAddressOf()));
  }

  {
    ComPtr<IUnknown> cell;
    EXPECT_EQ(E_INVALIDARG,
              result->get_accessibleAt(10, 10, cell.GetAddressOf()));
  }
}

TEST_F(AXPlatformNodeWinTest, TestIAccessible2ScrollToPoint) {
  AXNodeData root;
  root.id = 1;
  root.role = ax::mojom::Role::kRootWebArea;
  root.location = gfx::RectF(0, 0, 2000, 2000);

  AXNodeData child1;
  child1.id = 2;
  child1.role = ax::mojom::Role::kStaticText;
  child1.location = gfx::RectF(10, 10, 10, 10);
  root.child_ids.push_back(2);

  Init(root, child1);

  ComPtr<IAccessible> root_iaccessible(GetRootIAccessible());
  ComPtr<IDispatch> result;
  EXPECT_EQ(S_OK, root_iaccessible->get_accChild(ScopedVariant(1),
                                                 result.GetAddressOf()));
  ComPtr<IAccessible2> ax_child1;
  EXPECT_EQ(S_OK, result.CopyTo(ax_child1.GetAddressOf()));
  result.Reset();

  LONG x_left, y_top, width, height;
  EXPECT_EQ(S_OK,
            ax_child1->accLocation(&x_left, &y_top, &width, &height, SELF));
  EXPECT_EQ(10, x_left);
  EXPECT_EQ(10, y_top);
  EXPECT_EQ(10, width);
  EXPECT_EQ(10, height);

  ComPtr<IAccessible2> root_iaccessible2 = ToIAccessible2(root_iaccessible);
  EXPECT_EQ(S_OK, root_iaccessible2->scrollToPoint(
                      IA2_COORDTYPE_SCREEN_RELATIVE, 600, 650));

  EXPECT_EQ(S_OK,
            ax_child1->accLocation(&x_left, &y_top, &width, &height, SELF));
  EXPECT_EQ(610, x_left);
  EXPECT_EQ(660, y_top);
  EXPECT_EQ(10, width);
  EXPECT_EQ(10, height);

  EXPECT_EQ(S_OK, root_iaccessible2->scrollToPoint(
                      IA2_COORDTYPE_PARENT_RELATIVE, 0, 0));

  EXPECT_EQ(S_OK,
            ax_child1->accLocation(&x_left, &y_top, &width, &height, SELF));
  EXPECT_EQ(10, x_left);
  EXPECT_EQ(10, y_top);
  EXPECT_EQ(10, width);
  EXPECT_EQ(10, height);
}

TEST_F(AXPlatformNodeWinTest, TestIAccessible2ScrollTo) {
  AXNodeData root;
  root.id = 1;
  root.role = ax::mojom::Role::kRootWebArea;
  root.location = gfx::RectF(0, 0, 2000, 2000);

  AXNodeData child1;
  child1.id = 2;
  child1.role = ax::mojom::Role::kStaticText;
  child1.location = gfx::RectF(10, 10, 10, 10);
  root.child_ids.push_back(2);

  Init(root, child1);

  ComPtr<IAccessible> root_iaccessible(GetRootIAccessible());
  ComPtr<IDispatch> result;
  EXPECT_EQ(S_OK, root_iaccessible->get_accChild(ScopedVariant(1),
                                                 result.GetAddressOf()));
  ComPtr<IAccessible2> ax_child1;
  EXPECT_EQ(S_OK, result.CopyTo(ax_child1.GetAddressOf()));
  result.Reset();

  LONG x_left, y_top, width, height;
  EXPECT_EQ(S_OK,
            ax_child1->accLocation(&x_left, &y_top, &width, &height, SELF));
  EXPECT_EQ(10, x_left);
  EXPECT_EQ(10, y_top);
  EXPECT_EQ(10, width);
  EXPECT_EQ(10, height);

  ComPtr<IAccessible2> root_iaccessible2 = ToIAccessible2(root_iaccessible);
  EXPECT_EQ(S_OK, ax_child1->scrollTo(IA2_SCROLL_TYPE_ANYWHERE));

  EXPECT_EQ(S_OK,
            ax_child1->accLocation(&x_left, &y_top, &width, &height, SELF));
  EXPECT_EQ(0, x_left);
  EXPECT_EQ(0, y_top);
  EXPECT_EQ(10, width);
  EXPECT_EQ(10, height);
}

TEST_F(AXPlatformNodeWinTest, TestIAccessibleTableGetChildIndex) {
  Init(Build3X3Table());

  ComPtr<IAccessible> root_obj(GetRootIAccessible());

  ComPtr<IAccessibleTable> result;
  root_obj.CopyTo(result.GetAddressOf());
  ASSERT_NE(nullptr, result.Get());

  long id;
  EXPECT_EQ(S_OK, result->get_childIndex(0, 0, &id));
  EXPECT_EQ(id, 0);

  EXPECT_EQ(S_OK, result->get_childIndex(0, 1, &id));
  EXPECT_EQ(id, 1);

  EXPECT_EQ(S_OK, result->get_childIndex(1, 0, &id));
  EXPECT_EQ(id, 3);

  EXPECT_EQ(S_OK, result->get_childIndex(1, 1, &id));
  EXPECT_EQ(id, 4);

  EXPECT_EQ(E_INVALIDARG, result->get_childIndex(-1, -1, &id));
  EXPECT_EQ(E_INVALIDARG, result->get_childIndex(0, 5, &id));
  EXPECT_EQ(E_INVALIDARG, result->get_childIndex(5, 0, &id));
  EXPECT_EQ(E_INVALIDARG, result->get_childIndex(5, 5, &id));
}

TEST_F(AXPlatformNodeWinTest, TestIAccessibleTableGetColumnDescription) {
  Init(Build3X3Table());

  ComPtr<IAccessible> root_obj(GetRootIAccessible());

  ComPtr<IAccessibleTable> result;
  root_obj.CopyTo(result.GetAddressOf());
  ASSERT_NE(nullptr, result.Get());

  {
    ScopedBstr name;
    EXPECT_EQ(S_FALSE, result->get_columnDescription(0, name.Receive()));
  }
  {
    ScopedBstr name;
    EXPECT_EQ(S_OK, result->get_columnDescription(1, name.Receive()));
    EXPECT_STREQ(L"column header 1", name);
  }

  {
    ScopedBstr name;
    EXPECT_EQ(S_OK, result->get_columnDescription(2, name.Receive()));
    EXPECT_STREQ(L"column header 2", name);
  }
}

TEST_F(AXPlatformNodeWinTest, TestIAccessibleTableGetColumnExtentAt) {
  // TODO(dougt) This table doesn't have any spanning cells. This test
  // tests get_columnExtentAt for (1) and an invalid input.
  Init(Build3X3Table());

  ComPtr<IAccessible> root_obj(GetRootIAccessible());

  ComPtr<IAccessibleTable> result;
  root_obj.CopyTo(result.GetAddressOf());
  ASSERT_NE(nullptr, result.Get());

  long columns_spanned;
  EXPECT_EQ(S_OK, result->get_columnExtentAt(1, 1, &columns_spanned));
  EXPECT_EQ(columns_spanned, 1);

  EXPECT_EQ(E_INVALIDARG, result->get_columnExtentAt(-1, -1, &columns_spanned));
}

TEST_F(AXPlatformNodeWinTest, TestIAccessibleTableGetColumnIndex) {
  Init(Build3X3Table());

  ComPtr<IAccessible> root_obj(GetRootIAccessible());

  ComPtr<IAccessibleTable> result;
  root_obj.CopyTo(result.GetAddressOf());
  ASSERT_NE(nullptr, result.Get());

  long index;
  EXPECT_EQ(S_OK, result->get_columnIndex(2, &index));
  EXPECT_EQ(index, 2);
  EXPECT_EQ(S_OK, result->get_columnIndex(3, &index));
  EXPECT_EQ(index, 0);

  EXPECT_EQ(E_INVALIDARG, result->get_columnIndex(-1, &index));
}

TEST_F(AXPlatformNodeWinTest, TestIAccessibleTableGetNColumns) {
  Init(Build3X3Table());

  ComPtr<IAccessible> root_obj(GetRootIAccessible());

  ComPtr<IAccessibleTable> result;
  root_obj.CopyTo(result.GetAddressOf());
  ASSERT_NE(nullptr, result.Get());

  long count;
  EXPECT_EQ(S_OK, result->get_nColumns(&count));
  EXPECT_EQ(count, 3);
}

TEST_F(AXPlatformNodeWinTest, TestIAccessibleTableGetNRows) {
  Init(Build3X3Table());

  ComPtr<IAccessible> root_obj(GetRootIAccessible());

  ComPtr<IAccessibleTable> result;
  root_obj.CopyTo(result.GetAddressOf());
  ASSERT_NE(nullptr, result.Get());

  long count;
  EXPECT_EQ(S_OK, result->get_nRows(&count));
  EXPECT_EQ(count, 3);
}

TEST_F(AXPlatformNodeWinTest, TestIAccessibleTableGetRowDescription) {
  Init(Build3X3Table());

  ComPtr<IAccessible> root_obj(GetRootIAccessible());

  ComPtr<IAccessibleTable> result;
  root_obj.CopyTo(result.GetAddressOf());
  ASSERT_NE(nullptr, result.Get());

  {
    ScopedBstr name;
    EXPECT_EQ(S_FALSE, result->get_rowDescription(0, name.Receive()));
  }
  {
    ScopedBstr name;
    EXPECT_EQ(S_OK, result->get_rowDescription(1, name.Receive()));
    EXPECT_STREQ(L"row header 1", name);
  }

  {
    ScopedBstr name;
    EXPECT_EQ(S_OK, result->get_rowDescription(2, name.Receive()));
    EXPECT_STREQ(L"row header 2", name);
  }
}

TEST_F(AXPlatformNodeWinTest, TestIAccessibleTableGetRowExtentAt) {
  // TODO(dougt) This table doesn't have any spanning cells. This test
  // tests get_rowExtentAt for (1) and an invalid input.
  Init(Build3X3Table());

  ComPtr<IAccessible> root_obj(GetRootIAccessible());

  ComPtr<IAccessibleTable> result;
  root_obj.CopyTo(result.GetAddressOf());
  ASSERT_NE(nullptr, result.Get());

  long rows_spanned;
  EXPECT_EQ(S_OK, result->get_rowExtentAt(0, 1, &rows_spanned));
  EXPECT_EQ(rows_spanned, 0);

  EXPECT_EQ(E_INVALIDARG, result->get_columnExtentAt(-1, -1, &rows_spanned));
}

TEST_F(AXPlatformNodeWinTest, TestIAccessibleTableGetRowIndex) {
  Init(Build3X3Table());

  ComPtr<IAccessible> root_obj(GetRootIAccessible());

  ComPtr<IAccessibleTable> result;
  root_obj.CopyTo(result.GetAddressOf());
  ASSERT_NE(nullptr, result.Get());

  long index;
  EXPECT_EQ(S_OK, result->get_rowIndex(2, &index));
  EXPECT_EQ(index, 0);
  EXPECT_EQ(S_OK, result->get_rowIndex(3, &index));
  EXPECT_EQ(index, 1);

  EXPECT_EQ(E_INVALIDARG, result->get_rowIndex(-1, &index));
}

TEST_F(AXPlatformNodeWinTest, TestIAccessibleTableGetRowColumnExtentsAtIndex) {
  Init(Build3X3Table());

  ComPtr<IAccessible> root_obj(GetRootIAccessible());

  ComPtr<IAccessibleTable> result;
  root_obj.CopyTo(result.GetAddressOf());
  ASSERT_NE(nullptr, result.Get());

  long row, column, row_extents, column_extents;
  boolean is_selected;
  EXPECT_EQ(S_OK,
            result->get_rowColumnExtentsAtIndex(0, &row, &column, &row_extents,
                                                &column_extents, &is_selected));

  EXPECT_EQ(row, 0);
  EXPECT_EQ(column, 0);
  EXPECT_EQ(row_extents, 0);
  EXPECT_EQ(column_extents, 0);

  EXPECT_EQ(E_INVALIDARG,
            result->get_rowColumnExtentsAtIndex(-1, &row, &column, &row_extents,
                                                &column_extents, &is_selected));
}

TEST_F(AXPlatformNodeWinTest, TestIAccessibleTableGetCellAt) {
  Init(Build3X3Table());

  ComPtr<IAccessible> root_obj(GetRootIAccessible());

  ComPtr<IAccessibleTable2> result;
  root_obj.CopyTo(result.GetAddressOf());
  ASSERT_NE(nullptr, result.Get());

  {
    ComPtr<IUnknown> cell;
    EXPECT_EQ(S_OK, result->get_cellAt(1, 1, cell.GetAddressOf()));
    CheckIUnknownHasName(cell, L"1");
  }

  {
    ComPtr<IUnknown> cell;
    EXPECT_EQ(E_INVALIDARG, result->get_cellAt(-1, -1, cell.GetAddressOf()));
  }
}

TEST_F(AXPlatformNodeWinTest, TestIAccessibleTableCellGetColumnExtent) {
  Init(Build3X3Table());

  ComPtr<IAccessibleTableCell> cell = GetCellInTable();
  ASSERT_NE(nullptr, cell.Get());

  long column_spanned;
  EXPECT_EQ(S_OK, cell->get_columnExtent(&column_spanned));
  EXPECT_EQ(column_spanned, 1);
}

TEST_F(AXPlatformNodeWinTest, TestIAccessibleTableCellGetColumnHeaderCells) {
  Init(Build3X3Table());

  ComPtr<IAccessibleTableCell> cell = GetCellInTable();
  ASSERT_NE(nullptr, cell.Get());

  IUnknown** cell_accessibles;

  long number_cells;
  EXPECT_EQ(S_OK,
            cell->get_columnHeaderCells(&cell_accessibles, &number_cells));
  EXPECT_EQ(number_cells, 1);
}

TEST_F(AXPlatformNodeWinTest, TestIAccessibleTableCellGetColumnIndex) {
  Init(Build3X3Table());

  ComPtr<IAccessibleTableCell> cell = GetCellInTable();
  ASSERT_NE(nullptr, cell.Get());

  long index;
  EXPECT_EQ(S_OK, cell->get_columnIndex(&index));
  EXPECT_EQ(index, 1);
}

TEST_F(AXPlatformNodeWinTest, TestIAccessibleTableCellGetRowExtent) {
  Init(Build3X3Table());

  ComPtr<IAccessibleTableCell> cell = GetCellInTable();
  ASSERT_NE(nullptr, cell.Get());

  long rows_spanned;
  EXPECT_EQ(S_OK, cell->get_rowExtent(&rows_spanned));
  EXPECT_EQ(rows_spanned, 1);
}

TEST_F(AXPlatformNodeWinTest, TestIAccessibleTableCellGetRowHeaderCells) {
  Init(Build3X3Table());

  ComPtr<IAccessibleTableCell> cell = GetCellInTable();
  ASSERT_NE(nullptr, cell.Get());

  IUnknown** cell_accessibles;

  long number_cells;
  EXPECT_EQ(S_OK, cell->get_rowHeaderCells(&cell_accessibles, &number_cells));
  EXPECT_EQ(number_cells, 1);
}

TEST_F(AXPlatformNodeWinTest, TestIAccessibleTableCellGetRowIndex) {
  Init(Build3X3Table());

  ComPtr<IAccessibleTableCell> cell = GetCellInTable();
  ASSERT_NE(nullptr, cell.Get());

  long index;
  EXPECT_EQ(S_OK, cell->get_rowIndex(&index));
  EXPECT_EQ(index, 1);
}

TEST_F(AXPlatformNodeWinTest, TestIAccessibleTableCellGetRowColumnExtent) {
  Init(Build3X3Table());

  ComPtr<IAccessibleTableCell> cell = GetCellInTable();
  ASSERT_NE(nullptr, cell.Get());

  long row, column, row_extents, column_extents;
  boolean is_selected;
  EXPECT_EQ(S_OK, cell->get_rowColumnExtents(&row, &column, &row_extents,
                                             &column_extents, &is_selected));
  EXPECT_EQ(row, 1);
  EXPECT_EQ(column, 1);
  EXPECT_EQ(row_extents, 1);
  EXPECT_EQ(column_extents, 1);
}

TEST_F(AXPlatformNodeWinTest, TestIAccessibleTableCellGetTable) {
  Init(Build3X3Table());

  ComPtr<IAccessibleTableCell> cell = GetCellInTable();
  ASSERT_NE(nullptr, cell.Get());

  ComPtr<IUnknown> table;
  EXPECT_EQ(S_OK, cell->get_table(table.GetAddressOf()));

  ComPtr<IAccessibleTable> result;
  table.CopyTo(result.GetAddressOf());
  ASSERT_NE(nullptr, result.Get());

  // Check to make sure that this is the right table by checking one cell.
  ComPtr<IUnknown> cell_1;
  EXPECT_EQ(S_OK, result->get_accessibleAt(1, 1, cell_1.GetAddressOf()));
  CheckIUnknownHasName(cell_1, L"1");
}

TEST_F(AXPlatformNodeWinTest, TestIAccessible2GetNRelations) {
  // This is is a duplicated of
  // BrowserAccessibilityTest::TestIAccessible2Relations but without the
  // specific COM/BrowserAccessibility knowledge.
  AXNodeData root;
  root.id = 1;
  root.role = ax::mojom::Role::kRootWebArea;

  std::vector<int32_t> describedby_ids = {1, 2, 3};
  root.AddIntListAttribute(ax::mojom::IntListAttribute::kDescribedbyIds,
                           describedby_ids);

  AXNodeData child1;
  child1.id = 2;
  child1.role = ax::mojom::Role::kStaticText;

  root.child_ids.push_back(2);

  AXNodeData child2;
  child2.id = 3;
  child2.role = ax::mojom::Role::kStaticText;

  root.child_ids.push_back(3);

  Init(root, child1, child2);
  ComPtr<IAccessible> root_iaccessible(GetRootIAccessible());
  ComPtr<IAccessible2> root_iaccessible2 = ToIAccessible2(root_iaccessible);

  ComPtr<IDispatch> result;
  EXPECT_EQ(S_OK, root_iaccessible2->get_accChild(ScopedVariant(1),
                                                  result.GetAddressOf()));
  ComPtr<IAccessible2> ax_child1;
  EXPECT_EQ(S_OK, result.CopyTo(ax_child1.GetAddressOf()));
  result.Reset();

  EXPECT_EQ(S_OK, root_iaccessible2->get_accChild(ScopedVariant(2),
                                                  result.GetAddressOf()));
  ComPtr<IAccessible2> ax_child2;
  EXPECT_EQ(S_OK, result.CopyTo(ax_child2.GetAddressOf()));
  result.Reset();

  LONG n_relations = 0;
  LONG n_targets = 0;
  ScopedBstr relation_type;
  ComPtr<IAccessibleRelation> describedby_relation;
  ComPtr<IAccessibleRelation> description_for_relation;
  ComPtr<IUnknown> target;

  EXPECT_HRESULT_SUCCEEDED(root_iaccessible2->get_nRelations(&n_relations));
  EXPECT_EQ(1, n_relations);

  EXPECT_HRESULT_SUCCEEDED(
      root_iaccessible2->get_relation(0, describedby_relation.GetAddressOf()));

  EXPECT_HRESULT_SUCCEEDED(
      describedby_relation->get_relationType(relation_type.Receive()));
  EXPECT_EQ(L"describedBy", base::string16(relation_type));

  relation_type.Reset();

  EXPECT_HRESULT_SUCCEEDED(describedby_relation->get_nTargets(&n_targets));
  EXPECT_EQ(2, n_targets);

  EXPECT_HRESULT_SUCCEEDED(
      describedby_relation->get_target(0, target.GetAddressOf()));
  target.Reset();

  EXPECT_HRESULT_SUCCEEDED(
      describedby_relation->get_target(1, target.GetAddressOf()));
  target.Reset();

  describedby_relation.Reset();

  // Test the reverse relations.
  EXPECT_HRESULT_SUCCEEDED(ax_child1->get_nRelations(&n_relations));
  EXPECT_EQ(1, n_relations);

  EXPECT_HRESULT_SUCCEEDED(
      ax_child1->get_relation(0, description_for_relation.GetAddressOf()));
  EXPECT_HRESULT_SUCCEEDED(
      description_for_relation->get_relationType(relation_type.Receive()));
  EXPECT_EQ(L"descriptionFor", base::string16(relation_type));
  relation_type.Reset();

  EXPECT_HRESULT_SUCCEEDED(description_for_relation->get_nTargets(&n_targets));
  EXPECT_EQ(1, n_targets);

  EXPECT_HRESULT_SUCCEEDED(
      description_for_relation->get_target(0, target.GetAddressOf()));
  target.Reset();
  description_for_relation.Reset();

  EXPECT_HRESULT_SUCCEEDED(ax_child2->get_nRelations(&n_relations));
  EXPECT_EQ(1, n_relations);

  EXPECT_HRESULT_SUCCEEDED(
      ax_child2->get_relation(0, description_for_relation.GetAddressOf()));
  EXPECT_HRESULT_SUCCEEDED(
      description_for_relation->get_relationType(relation_type.Receive()));
  EXPECT_EQ(L"descriptionFor", base::string16(relation_type));
  relation_type.Reset();

  EXPECT_HRESULT_SUCCEEDED(description_for_relation->get_nTargets(&n_targets));
  EXPECT_EQ(1, n_targets);

  EXPECT_HRESULT_SUCCEEDED(
      description_for_relation->get_target(0, target.GetAddressOf()));
  target.Reset();

  // TODO(dougt): Try adding one more relation.
}

TEST_F(AXPlatformNodeWinTest, TestRelationTargetsOfType) {
  AXNodeData root;
  root.id = 1;
  root.role = ax::mojom::Role::kRootWebArea;
  root.AddIntAttribute(ax::mojom::IntAttribute::kDetailsId, 2);

  AXNodeData child1;
  child1.id = 2;
  child1.role = ax::mojom::Role::kStaticText;

  root.child_ids.push_back(2);

  AXNodeData child2;
  child2.id = 3;
  child2.role = ax::mojom::Role::kStaticText;
  std::vector<int32_t> labelledby_ids = {1, 4};
  child2.AddIntListAttribute(ax::mojom::IntListAttribute::kLabelledbyIds,
                             labelledby_ids);

  root.child_ids.push_back(3);

  AXNodeData child3;
  child3.id = 4;
  child3.role = ax::mojom::Role::kStaticText;
  child3.AddIntAttribute(ax::mojom::IntAttribute::kDetailsId, 2);

  root.child_ids.push_back(4);

  Init(root, child1, child2, child3);
  ComPtr<IAccessible> root_iaccessible(GetRootIAccessible());
  ComPtr<IAccessible2_2> root_iaccessible2 = ToIAccessible2_2(root_iaccessible);

  ComPtr<IDispatch> result;
  EXPECT_EQ(S_OK, root_iaccessible2->get_accChild(ScopedVariant(1),
                                                  result.GetAddressOf()));
  ComPtr<IAccessible2_2> ax_child1;
  EXPECT_EQ(S_OK, result.CopyTo(ax_child1.GetAddressOf()));
  result.Reset();

  EXPECT_EQ(S_OK, root_iaccessible2->get_accChild(ScopedVariant(2),
                                                  result.GetAddressOf()));
  ComPtr<IAccessible2_2> ax_child2;
  EXPECT_EQ(S_OK, result.CopyTo(ax_child2.GetAddressOf()));
  result.Reset();

  EXPECT_EQ(S_OK, root_iaccessible2->get_accChild(ScopedVariant(3),
                                                  result.GetAddressOf()));
  ComPtr<IAccessible2_2> ax_child3;
  EXPECT_EQ(S_OK, result.CopyTo(ax_child3.GetAddressOf()));
  result.Reset();

  {
    ScopedBstr type(L"details");
    IUnknown** targets;
    LONG n_targets;
    EXPECT_EQ(S_OK, root_iaccessible2->get_relationTargetsOfType(
                        type, 0, &targets, &n_targets));
    ASSERT_EQ(1, n_targets);
    EXPECT_EQ(ax_child1.Get(), targets[0]);
    CoTaskMemFree(targets);
  }

  {
    ScopedBstr type(IA2_RELATION_LABELLED_BY);
    IUnknown** targets;
    LONG n_targets;
    EXPECT_EQ(S_OK, ax_child2->get_relationTargetsOfType(type, 0, &targets,
                                                         &n_targets));
    ASSERT_EQ(2, n_targets);
    EXPECT_EQ(root_iaccessible2.Get(), targets[0]);
    EXPECT_EQ(ax_child3.Get(), targets[1]);
    CoTaskMemFree(targets);
  }

  {
    ScopedBstr type(L"detailsFor");
    IUnknown** targets;
    LONG n_targets;
    EXPECT_EQ(S_OK, ax_child1->get_relationTargetsOfType(type, 0, &targets,
                                                         &n_targets));
    ASSERT_EQ(2, n_targets);
    EXPECT_EQ(root_iaccessible2.Get(), targets[0]);
    EXPECT_EQ(ax_child3.Get(), targets[1]);
    CoTaskMemFree(targets);
  }
}

TEST_F(AXPlatformNodeWinTest, TestIAccessibleTableGetNSelectedChildrenZero) {
  Init(Build3X3Table());

  ComPtr<IAccessibleTableCell> cell = GetCellInTable();
  ASSERT_NE(nullptr, cell.Get());

  ComPtr<IUnknown> table;
  EXPECT_EQ(S_OK, cell->get_table(table.GetAddressOf()));

  ComPtr<IAccessibleTable> result;
  table.CopyTo(result.GetAddressOf());
  ASSERT_NE(nullptr, result.Get());

  LONG selectedChildren;
  EXPECT_EQ(S_OK, result->get_nSelectedChildren(&selectedChildren));
  EXPECT_EQ(0, selectedChildren);
}

TEST_F(AXPlatformNodeWinTest, TestIAccessibleTableGetNSelectedChildrenOne) {
  AXTreeUpdate update = Build3X3Table();

  // 7 == table_cell_1
  update.nodes[7].AddBoolAttribute(ax::mojom::BoolAttribute::kSelected, true);
  Init(update);

  ComPtr<IAccessibleTableCell> cell = GetCellInTable();
  ASSERT_NE(nullptr, cell.Get());

  ComPtr<IUnknown> table;
  EXPECT_EQ(S_OK, cell->get_table(table.GetAddressOf()));

  ComPtr<IAccessibleTable> result;
  table.CopyTo(result.GetAddressOf());
  ASSERT_NE(nullptr, result.Get());

  LONG selectedChildren;
  EXPECT_EQ(S_OK, result->get_nSelectedChildren(&selectedChildren));
  EXPECT_EQ(1, selectedChildren);
}

TEST_F(AXPlatformNodeWinTest, TestIAccessibleTableGetNSelectedChildrenMany) {
  AXTreeUpdate update = Build3X3Table();

  // 7 == table_cell_1
  // 8 == table_cell_2
  // 11 == table_cell_3
  // 12 == table_cell_4
  update.nodes[7].AddBoolAttribute(ax::mojom::BoolAttribute::kSelected, true);
  update.nodes[8].AddBoolAttribute(ax::mojom::BoolAttribute::kSelected, true);
  update.nodes[11].AddBoolAttribute(ax::mojom::BoolAttribute::kSelected, true);
  update.nodes[12].AddBoolAttribute(ax::mojom::BoolAttribute::kSelected, true);

  Init(update);

  ComPtr<IAccessibleTableCell> cell = GetCellInTable();
  ASSERT_NE(nullptr, cell.Get());

  ComPtr<IUnknown> table;
  EXPECT_EQ(S_OK, cell->get_table(table.GetAddressOf()));

  ComPtr<IAccessibleTable> result;
  table.CopyTo(result.GetAddressOf());
  ASSERT_NE(nullptr, result.Get());

  LONG selectedChildren;
  EXPECT_EQ(S_OK, result->get_nSelectedChildren(&selectedChildren));
  EXPECT_EQ(4, selectedChildren);
}

TEST_F(AXPlatformNodeWinTest, TestIAccessibleTableGetNSelectedColumnsZero) {
  Init(Build3X3Table());

  ComPtr<IAccessibleTableCell> cell = GetCellInTable();
  ASSERT_NE(nullptr, cell.Get());

  ComPtr<IUnknown> table;
  EXPECT_EQ(S_OK, cell->get_table(table.GetAddressOf()));

  ComPtr<IAccessibleTable> result;
  table.CopyTo(result.GetAddressOf());
  ASSERT_NE(nullptr, result.Get());

  LONG selectedColumns;
  EXPECT_EQ(S_OK, result->get_nSelectedColumns(&selectedColumns));
  EXPECT_EQ(0, selectedColumns);
}

TEST_F(AXPlatformNodeWinTest, TestIAccessibleTableGetNSelectedColumnsOne) {
  AXTreeUpdate update = Build3X3Table();

  // 3 == table_column_header_2
  // 7 == table_cell_1
  // 11 == table_cell_3
  update.nodes[3].AddBoolAttribute(ax::mojom::BoolAttribute::kSelected, true);
  update.nodes[7].AddBoolAttribute(ax::mojom::BoolAttribute::kSelected, true);
  update.nodes[11].AddBoolAttribute(ax::mojom::BoolAttribute::kSelected, true);

  Init(update);

  ComPtr<IAccessibleTableCell> cell = GetCellInTable();
  ASSERT_NE(nullptr, cell.Get());

  ComPtr<IUnknown> table;
  EXPECT_EQ(S_OK, cell->get_table(table.GetAddressOf()));

  ComPtr<IAccessibleTable> result;
  table.CopyTo(result.GetAddressOf());
  ASSERT_NE(nullptr, result.Get());

  LONG selectedColumns;
  EXPECT_EQ(S_OK, result->get_nSelectedColumns(&selectedColumns));
  EXPECT_EQ(1, selectedColumns);
}

TEST_F(AXPlatformNodeWinTest, TestIAccessibleTableGetNSelectedColumnsMany) {
  AXTreeUpdate update = Build3X3Table();

  // 3 == table_column_header_2
  // 7 == table_cell_1
  // 11 == table_cell_3
  update.nodes[3].AddBoolAttribute(ax::mojom::BoolAttribute::kSelected, true);
  update.nodes[7].AddBoolAttribute(ax::mojom::BoolAttribute::kSelected, true);
  update.nodes[11].AddBoolAttribute(ax::mojom::BoolAttribute::kSelected, true);

  // 4 == table_column_header_3
  // 8 == table_cell_2
  // 12 == table_cell_4
  update.nodes[4].AddBoolAttribute(ax::mojom::BoolAttribute::kSelected, true);
  update.nodes[8].AddBoolAttribute(ax::mojom::BoolAttribute::kSelected, true);
  update.nodes[12].AddBoolAttribute(ax::mojom::BoolAttribute::kSelected, true);

  Init(update);

  ComPtr<IAccessibleTableCell> cell = GetCellInTable();
  ASSERT_NE(nullptr, cell.Get());

  ComPtr<IUnknown> table;
  EXPECT_EQ(S_OK, cell->get_table(table.GetAddressOf()));

  ComPtr<IAccessibleTable> result;
  table.CopyTo(result.GetAddressOf());
  ASSERT_NE(nullptr, result.Get());

  LONG selectedColumns;
  EXPECT_EQ(S_OK, result->get_nSelectedColumns(&selectedColumns));
  EXPECT_EQ(2, selectedColumns);
}

TEST_F(AXPlatformNodeWinTest, TestIAccessibleTableGetNSelectedRowsZero) {
  Init(Build3X3Table());

  ComPtr<IAccessibleTableCell> cell = GetCellInTable();
  ASSERT_NE(nullptr, cell.Get());

  ComPtr<IUnknown> table;
  EXPECT_EQ(S_OK, cell->get_table(table.GetAddressOf()));

  ComPtr<IAccessibleTable> result;
  table.CopyTo(result.GetAddressOf());
  ASSERT_NE(nullptr, result.Get());

  LONG selectedRows;
  EXPECT_EQ(S_OK, result->get_nSelectedRows(&selectedRows));
  EXPECT_EQ(0, selectedRows);
}

TEST_F(AXPlatformNodeWinTest, TestIAccessibleTableGetNSelectedRowsOne) {
  AXTreeUpdate update = Build3X3Table();

  // 6 == table_row_header_1
  // 7 == table_cell_1
  // 8 == table_cell_2
  update.nodes[6].AddBoolAttribute(ax::mojom::BoolAttribute::kSelected, true);
  update.nodes[7].AddBoolAttribute(ax::mojom::BoolAttribute::kSelected, true);
  update.nodes[8].AddBoolAttribute(ax::mojom::BoolAttribute::kSelected, true);

  Init(update);

  ComPtr<IAccessibleTableCell> cell = GetCellInTable();
  ASSERT_NE(nullptr, cell.Get());

  ComPtr<IUnknown> table;
  EXPECT_EQ(S_OK, cell->get_table(table.GetAddressOf()));

  ComPtr<IAccessibleTable> result;
  table.CopyTo(result.GetAddressOf());
  ASSERT_NE(nullptr, result.Get());

  LONG selectedRows;
  EXPECT_EQ(S_OK, result->get_nSelectedRows(&selectedRows));
  EXPECT_EQ(1, selectedRows);
}

TEST_F(AXPlatformNodeWinTest, TestIAccessibleTableGetNSelectedRowsMany) {
  AXTreeUpdate update = Build3X3Table();

  // 6 == table_row_header_3
  // 7 == table_cell_1
  // 8 == table_cell_2
  update.nodes[6].AddBoolAttribute(ax::mojom::BoolAttribute::kSelected, true);
  update.nodes[7].AddBoolAttribute(ax::mojom::BoolAttribute::kSelected, true);
  update.nodes[8].AddBoolAttribute(ax::mojom::BoolAttribute::kSelected, true);

  // 10 == table_row_header_3
  // 11 == table_cell_1
  // 12 == table_cell_2
  update.nodes[10].AddBoolAttribute(ax::mojom::BoolAttribute::kSelected, true);
  update.nodes[11].AddBoolAttribute(ax::mojom::BoolAttribute::kSelected, true);
  update.nodes[12].AddBoolAttribute(ax::mojom::BoolAttribute::kSelected, true);

  Init(update);

  ComPtr<IAccessibleTableCell> cell = GetCellInTable();
  ASSERT_NE(nullptr, cell.Get());

  ComPtr<IUnknown> table;
  EXPECT_EQ(S_OK, cell->get_table(table.GetAddressOf()));

  ComPtr<IAccessibleTable> result;
  table.CopyTo(result.GetAddressOf());
  ASSERT_NE(nullptr, result.Get());

  LONG selectedRows;
  EXPECT_EQ(S_OK, result->get_nSelectedRows(&selectedRows));
  EXPECT_EQ(2, selectedRows);
}

TEST_F(AXPlatformNodeWinTest, TestIAccessibleTableGetSelectedChildren) {
  AXTreeUpdate update = Build3X3Table();

  // 7 == table_cell_1
  // 12 == table_cell_4
  update.nodes[7].AddBoolAttribute(ax::mojom::BoolAttribute::kSelected, true);
  update.nodes[12].AddBoolAttribute(ax::mojom::BoolAttribute::kSelected, true);

  Init(update);

  ComPtr<IAccessibleTableCell> cell = GetCellInTable();
  ASSERT_NE(nullptr, cell.Get());

  ComPtr<IUnknown> table;
  EXPECT_EQ(S_OK, cell->get_table(table.GetAddressOf()));

  ComPtr<IAccessibleTable> result;
  table.CopyTo(result.GetAddressOf());
  ASSERT_NE(nullptr, result.Get());

  LONG max = 10;
  LONG* indices;
  LONG count;
  EXPECT_EQ(S_OK, result->get_selectedChildren(max, &indices, &count));
  EXPECT_EQ(2, count);
  EXPECT_EQ(4, indices[0]);
  EXPECT_EQ(8, indices[1]);
}

TEST_F(AXPlatformNodeWinTest, TestIAccessibleTableGetSelectedChildrenZeroMax) {
  AXTreeUpdate update = Build3X3Table();

  // 7 == table_cell_1
  // 12 == table_cell_4
  update.nodes[7].AddBoolAttribute(ax::mojom::BoolAttribute::kSelected, true);
  update.nodes[12].AddBoolAttribute(ax::mojom::BoolAttribute::kSelected, true);

  Init(update);

  ComPtr<IAccessibleTableCell> cell = GetCellInTable();
  ASSERT_NE(nullptr, cell.Get());

  ComPtr<IUnknown> table;
  EXPECT_EQ(S_OK, cell->get_table(table.GetAddressOf()));

  ComPtr<IAccessibleTable> result;
  table.CopyTo(result.GetAddressOf());
  ASSERT_NE(nullptr, result.Get());

  LONG* indices;
  LONG count;
  EXPECT_EQ(E_INVALIDARG, result->get_selectedChildren(0, &indices, &count));
}

TEST_F(AXPlatformNodeWinTest, TestIAccessibleTableGetSelectedColumnsZero) {
  AXTreeUpdate update = Build3X3Table();

  // 7 == table_cell_1
  // 11 == table_cell_3
  update.nodes[7].AddBoolAttribute(ax::mojom::BoolAttribute::kSelected, true);
  update.nodes[11].AddBoolAttribute(ax::mojom::BoolAttribute::kSelected, true);

  Init(update);

  ComPtr<IAccessibleTableCell> cell = GetCellInTable();
  ASSERT_NE(nullptr, cell.Get());

  ComPtr<IUnknown> table;
  EXPECT_EQ(S_OK, cell->get_table(table.GetAddressOf()));

  ComPtr<IAccessibleTable> result;
  table.CopyTo(result.GetAddressOf());
  ASSERT_NE(nullptr, result.Get());

  long max_columns = 10;
  long* columns;
  long n_columns;
  EXPECT_EQ(S_OK,
            result->get_selectedColumns(max_columns, &columns, &n_columns));
  EXPECT_EQ(0, n_columns);
}

TEST_F(AXPlatformNodeWinTest, TestIAccessibleTableGetSelectedColumnsOne) {
  AXTreeUpdate update = Build3X3Table();

  // 3 == table_column_header_2
  // 7 == table_cell_1
  // 11 == table_cell_3
  update.nodes[3].AddBoolAttribute(ax::mojom::BoolAttribute::kSelected, true);
  update.nodes[7].AddBoolAttribute(ax::mojom::BoolAttribute::kSelected, true);
  update.nodes[11].AddBoolAttribute(ax::mojom::BoolAttribute::kSelected, true);

  Init(update);

  ComPtr<IAccessibleTableCell> cell = GetCellInTable();
  ASSERT_NE(nullptr, cell.Get());

  ComPtr<IUnknown> table;
  EXPECT_EQ(S_OK, cell->get_table(table.GetAddressOf()));

  ComPtr<IAccessibleTable> result;
  table.CopyTo(result.GetAddressOf());
  ASSERT_NE(nullptr, result.Get());

  long max_columns = 10;
  long* columns;
  long n_columns;
  EXPECT_EQ(S_OK,
            result->get_selectedColumns(max_columns, &columns, &n_columns));
  EXPECT_EQ(1, n_columns);
  EXPECT_EQ(1, columns[0]);
}

TEST_F(AXPlatformNodeWinTest, TestIAccessibleTableGetSelectedColumnsMany) {
  AXTreeUpdate update = Build3X3Table();

  // 3 == table_column_header_2
  // 7 == table_cell_1
  // 11 == table_cell_3
  update.nodes[3].AddBoolAttribute(ax::mojom::BoolAttribute::kSelected, true);
  update.nodes[7].AddBoolAttribute(ax::mojom::BoolAttribute::kSelected, true);
  update.nodes[11].AddBoolAttribute(ax::mojom::BoolAttribute::kSelected, true);

  // 4 == table_column_header_3
  // 8 == table_cell_2
  // 12 == table_cell_4
  update.nodes[4].AddBoolAttribute(ax::mojom::BoolAttribute::kSelected, true);
  update.nodes[8].AddBoolAttribute(ax::mojom::BoolAttribute::kSelected, true);
  update.nodes[12].AddBoolAttribute(ax::mojom::BoolAttribute::kSelected, true);

  Init(update);

  ComPtr<IAccessibleTableCell> cell = GetCellInTable();
  ASSERT_NE(nullptr, cell.Get());

  ComPtr<IUnknown> table;
  EXPECT_EQ(S_OK, cell->get_table(table.GetAddressOf()));

  ComPtr<IAccessibleTable> result;
  table.CopyTo(result.GetAddressOf());
  ASSERT_NE(nullptr, result.Get());

  long max_columns = 10;
  long* columns;
  long n_columns;
  EXPECT_EQ(S_OK,
            result->get_selectedColumns(max_columns, &columns, &n_columns));
  EXPECT_EQ(2, n_columns);
  EXPECT_EQ(1, columns[0]);
  EXPECT_EQ(2, columns[1]);
}

TEST_F(AXPlatformNodeWinTest, TestIAccessibleTableGetSelectedRowsZero) {
  Init(Build3X3Table());

  ComPtr<IAccessibleTableCell> cell = GetCellInTable();
  ASSERT_NE(nullptr, cell.Get());

  ComPtr<IUnknown> table;
  EXPECT_EQ(S_OK, cell->get_table(table.GetAddressOf()));

  ComPtr<IAccessibleTable> result;
  table.CopyTo(result.GetAddressOf());
  ASSERT_NE(nullptr, result.Get());

  long max_rows = 10;
  long* rows;
  long n_rows;
  EXPECT_EQ(S_OK, result->get_selectedRows(max_rows, &rows, &n_rows));
  EXPECT_EQ(0, n_rows);
}

TEST_F(AXPlatformNodeWinTest, TestIAccessibleTableGetSelectedRowsOne) {
  AXTreeUpdate update = Build3X3Table();

  // 6 == table_row_header_1
  // 7 == table_cell_1
  // 8 == table_cell_2
  update.nodes[6].AddBoolAttribute(ax::mojom::BoolAttribute::kSelected, true);
  update.nodes[7].AddBoolAttribute(ax::mojom::BoolAttribute::kSelected, true);
  update.nodes[8].AddBoolAttribute(ax::mojom::BoolAttribute::kSelected, true);

  Init(update);

  ComPtr<IAccessibleTableCell> cell = GetCellInTable();
  ASSERT_NE(nullptr, cell.Get());

  ComPtr<IUnknown> table;
  EXPECT_EQ(S_OK, cell->get_table(table.GetAddressOf()));

  ComPtr<IAccessibleTable> result;
  table.CopyTo(result.GetAddressOf());
  ASSERT_NE(nullptr, result.Get());

  long max_rows = 10;
  long* rows;
  long n_rows;
  EXPECT_EQ(S_OK, result->get_selectedRows(max_rows, &rows, &n_rows));
  EXPECT_EQ(1, n_rows);
  EXPECT_EQ(1, rows[0]);
}

TEST_F(AXPlatformNodeWinTest, TestIAccessibleTableGetSelectedRowsMany) {
  AXTreeUpdate update = Build3X3Table();

  // 6 == table_row_header_3
  // 7 == table_cell_1
  // 8 == table_cell_2
  update.nodes[6].AddBoolAttribute(ax::mojom::BoolAttribute::kSelected, true);
  update.nodes[7].AddBoolAttribute(ax::mojom::BoolAttribute::kSelected, true);
  update.nodes[8].AddBoolAttribute(ax::mojom::BoolAttribute::kSelected, true);

  // 10 == table_row_header_3
  // 11 == table_cell_1
  // 12 == table_cell_2
  update.nodes[10].AddBoolAttribute(ax::mojom::BoolAttribute::kSelected, true);
  update.nodes[11].AddBoolAttribute(ax::mojom::BoolAttribute::kSelected, true);
  update.nodes[12].AddBoolAttribute(ax::mojom::BoolAttribute::kSelected, true);

  Init(update);

  ComPtr<IAccessibleTableCell> cell = GetCellInTable();
  ASSERT_NE(nullptr, cell.Get());

  ComPtr<IUnknown> table;
  EXPECT_EQ(S_OK, cell->get_table(table.GetAddressOf()));

  ComPtr<IAccessibleTable> result;
  table.CopyTo(result.GetAddressOf());
  ASSERT_NE(nullptr, result.Get());

  long max_rows = 10;
  long* rows;
  long n_rows;
  EXPECT_EQ(S_OK, result->get_selectedRows(max_rows, &rows, &n_rows));
  EXPECT_EQ(2, n_rows);
  EXPECT_EQ(1, rows[0]);
  EXPECT_EQ(2, rows[1]);
}

TEST_F(AXPlatformNodeWinTest, TestIAccessibleTableIsColumnSelected) {
  AXTreeUpdate update = Build3X3Table();

  // 3 == table_column_header_2
  // 7 == table_cell_1
  // 11 == table_cell_3
  update.nodes[3].AddBoolAttribute(ax::mojom::BoolAttribute::kSelected, true);
  update.nodes[7].AddBoolAttribute(ax::mojom::BoolAttribute::kSelected, true);
  update.nodes[11].AddBoolAttribute(ax::mojom::BoolAttribute::kSelected, true);

  Init(update);

  ComPtr<IAccessibleTableCell> cell = GetCellInTable();
  ASSERT_NE(nullptr, cell.Get());

  ComPtr<IUnknown> table;
  EXPECT_EQ(S_OK, cell->get_table(table.GetAddressOf()));

  ComPtr<IAccessibleTable> result;
  table.CopyTo(result.GetAddressOf());
  ASSERT_NE(nullptr, result.Get());

  boolean selected;
  EXPECT_EQ(S_OK, result->get_isColumnSelected(0, &selected));
  EXPECT_FALSE(selected);

  EXPECT_EQ(S_OK, result->get_isColumnSelected(1, &selected));
  EXPECT_TRUE(selected);

  EXPECT_EQ(S_OK, result->get_isColumnSelected(2, &selected));
  EXPECT_FALSE(selected);

  EXPECT_EQ(S_FALSE, result->get_isColumnSelected(3, &selected));
  EXPECT_EQ(S_FALSE, result->get_isColumnSelected(-3, &selected));
}

TEST_F(AXPlatformNodeWinTest, TestIAccessibleTableIsRowSelected) {
  AXTreeUpdate update = Build3X3Table();

  // 6 == table_row_header_3
  // 7 == table_cell_1
  // 8 == table_cell_2
  update.nodes[6].AddBoolAttribute(ax::mojom::BoolAttribute::kSelected, true);
  update.nodes[7].AddBoolAttribute(ax::mojom::BoolAttribute::kSelected, true);
  update.nodes[8].AddBoolAttribute(ax::mojom::BoolAttribute::kSelected, true);

  Init(update);

  ComPtr<IAccessibleTableCell> cell = GetCellInTable();
  ASSERT_NE(nullptr, cell.Get());

  ComPtr<IUnknown> table;
  EXPECT_EQ(S_OK, cell->get_table(table.GetAddressOf()));

  ComPtr<IAccessibleTable> result;
  table.CopyTo(result.GetAddressOf());
  ASSERT_NE(nullptr, result.Get());

  boolean selected;
  EXPECT_EQ(S_OK, result->get_isRowSelected(0, &selected));
  EXPECT_FALSE(selected);

  EXPECT_EQ(S_OK, result->get_isRowSelected(1, &selected));
  EXPECT_TRUE(selected);

  EXPECT_EQ(S_OK, result->get_isRowSelected(2, &selected));
  EXPECT_FALSE(selected);

  EXPECT_EQ(S_FALSE, result->get_isRowSelected(3, &selected));
  EXPECT_EQ(S_FALSE, result->get_isRowSelected(-3, &selected));
}

TEST_F(AXPlatformNodeWinTest, TestIAccessibleTableIsSelected) {
  AXTreeUpdate update = Build3X3Table();

  // 6 == table_row_header_3
  // 7 == table_cell_1
  // 8 == table_cell_2
  update.nodes[6].AddBoolAttribute(ax::mojom::BoolAttribute::kSelected, true);
  update.nodes[7].AddBoolAttribute(ax::mojom::BoolAttribute::kSelected, true);
  update.nodes[8].AddBoolAttribute(ax::mojom::BoolAttribute::kSelected, true);

  Init(update);

  ComPtr<IAccessibleTableCell> cell = GetCellInTable();
  ASSERT_NE(nullptr, cell.Get());

  ComPtr<IUnknown> table;
  EXPECT_EQ(S_OK, cell->get_table(table.GetAddressOf()));

  ComPtr<IAccessibleTable> result;
  table.CopyTo(result.GetAddressOf());
  ASSERT_NE(nullptr, result.Get());

  boolean selected;

  EXPECT_EQ(S_OK, result->get_isSelected(0, 0, &selected));
  EXPECT_FALSE(selected);

  EXPECT_EQ(S_OK, result->get_isSelected(0, 1, &selected));
  EXPECT_FALSE(selected);

  EXPECT_EQ(S_OK, result->get_isSelected(0, 2, &selected));
  EXPECT_FALSE(selected);

  EXPECT_EQ(S_FALSE, result->get_isSelected(0, 4, &selected));

  EXPECT_EQ(S_OK, result->get_isSelected(1, 0, &selected));
  EXPECT_TRUE(selected);

  EXPECT_EQ(S_OK, result->get_isSelected(1, 1, &selected));
  EXPECT_TRUE(selected);

  EXPECT_EQ(S_OK, result->get_isSelected(1, 2, &selected));
  EXPECT_TRUE(selected);

  EXPECT_EQ(S_FALSE, result->get_isSelected(1, 4, &selected));
}

TEST_F(AXPlatformNodeWinTest, TestIAccessibleTable2GetSelectedChildrenZero) {
  Init(Build3X3Table());

  ComPtr<IAccessibleTableCell> cell = GetCellInTable();
  ASSERT_NE(nullptr, cell.Get());

  ComPtr<IUnknown> table;
  EXPECT_EQ(S_OK, cell->get_table(table.GetAddressOf()));

  ComPtr<IAccessibleTable2> result;
  table.CopyTo(result.GetAddressOf());
  ASSERT_NE(nullptr, result.Get());

  IUnknown** cell_accessibles;
  LONG count;
  EXPECT_EQ(S_OK, result->get_selectedCells(&cell_accessibles, &count));
  EXPECT_EQ(0, count);
}

TEST_F(AXPlatformNodeWinTest, TestIAccessibleTable2GetSelectedChildren) {
  AXTreeUpdate update = Build3X3Table();

  // 7 == table_cell_1
  // 12 == table_cell_4
  update.nodes[7].AddBoolAttribute(ax::mojom::BoolAttribute::kSelected, true);
  update.nodes[12].AddBoolAttribute(ax::mojom::BoolAttribute::kSelected, true);

  Init(update);

  ComPtr<IAccessibleTableCell> cell = GetCellInTable();
  ASSERT_NE(nullptr, cell.Get());

  ComPtr<IUnknown> table;
  EXPECT_EQ(S_OK, cell->get_table(table.GetAddressOf()));

  ComPtr<IAccessibleTable2> result;
  table.CopyTo(result.GetAddressOf());
  ASSERT_NE(nullptr, result.Get());

  IUnknown** cell_accessibles;
  LONG count;
  EXPECT_EQ(S_OK, result->get_selectedCells(&cell_accessibles, &count));
  EXPECT_EQ(2, count);

  ComPtr<IUnknown> table_cell_1(cell_accessibles[0]);
  CheckIUnknownHasName(table_cell_1, L"1");

  ComPtr<IUnknown> table_cell_4(cell_accessibles[1]);
  CheckIUnknownHasName(table_cell_4, L"4");
}

TEST_F(AXPlatformNodeWinTest, TestIAccessible2GetGroupPosition) {
  AXNodeData root;
  root.id = 1;
  root.AddIntAttribute(ax::mojom::IntAttribute::kHierarchicalLevel, 1);
  root.AddIntAttribute(ax::mojom::IntAttribute::kSetSize, 1);
  root.AddIntAttribute(ax::mojom::IntAttribute::kPosInSet, 1);
  Init(root);

  ComPtr<IAccessible> root_obj(GetRootIAccessible());
  ComPtr<IAccessible2> iaccessible2 = ToIAccessible2(root_obj);
  LONG level, similar, position;
  EXPECT_EQ(S_OK, iaccessible2->get_groupPosition(&level, &similar, &position));
  EXPECT_EQ(1, level);
  EXPECT_EQ(1, similar);
  EXPECT_EQ(1, position);

  EXPECT_EQ(E_INVALIDARG,
            iaccessible2->get_groupPosition(nullptr, nullptr, nullptr));
}

TEST_F(AXPlatformNodeWinTest, TestIAccessible2GetLocalizedExtendedRole) {
  AXNodeData root;
  root.id = 1;
  root.AddStringAttribute(ax::mojom::StringAttribute::kRoleDescription,
                          "extended role");
  Init(root);

  ComPtr<IAccessible> root_obj(GetRootIAccessible());
  ComPtr<IAccessible2> iaccessible2 = ToIAccessible2(root_obj);
  ScopedBstr role;
  EXPECT_EQ(S_OK, iaccessible2->get_localizedExtendedRole(role.Receive()));
  EXPECT_STREQ(L"extended role", role);
}

TEST_F(AXPlatformNodeWinTest, TestIAccessibleTextGetNCharacters) {
  AXNodeData root;
  root.id = 0;
  root.role = ax::mojom::Role::kStaticText;
  root.child_ids.push_back(1);

  AXNodeData node;
  node.id = 1;
  node.role = ax::mojom::Role::kStaticText;
  node.SetName("Name");

  Init(root, node);

  AXNode* child_node = GetRootNode()->children()[0];
  ComPtr<IAccessible> child_iaccessible(IAccessibleFromNode(child_node));
  ASSERT_NE(nullptr, child_iaccessible.Get());

  ComPtr<IAccessibleText> text;
  child_iaccessible.CopyTo(text.GetAddressOf());
  ASSERT_NE(nullptr, text.Get());

  LONG count;
  EXPECT_HRESULT_SUCCEEDED(text->get_nCharacters(&count));
  EXPECT_EQ(4, count);
}

TEST_F(AXPlatformNodeWinTest, TestIAccessibleTextTextFieldRemoveSelection) {
  Init(BuildTextFieldWithSelectionRange(1, 2));

  ComPtr<IAccessible2> ia2_text_field = ToIAccessible2(GetRootIAccessible());
  ComPtr<IAccessibleText> text_field;
  ia2_text_field.CopyTo(text_field.GetAddressOf());
  ASSERT_NE(nullptr, text_field.Get());

  LONG start_offset, end_offset;
  EXPECT_HRESULT_SUCCEEDED(
      text_field->get_selection(0, &start_offset, &end_offset));
  EXPECT_EQ(1, start_offset);
  EXPECT_EQ(2, end_offset);

  EXPECT_HRESULT_SUCCEEDED(text_field->removeSelection(0));

  // There is no selection, just a caret.
  EXPECT_EQ(E_INVALIDARG,
            text_field->get_selection(0, &start_offset, &end_offset));
}

TEST_F(AXPlatformNodeWinTest,
       TestIAccessibleTextContentEditableRemoveSelection) {
  Init(BuildTextFieldWithSelectionRange(1, 2));

  ComPtr<IAccessible2> ia2_text_field = ToIAccessible2(GetRootIAccessible());
  ComPtr<IAccessibleText> text_field;
  ia2_text_field.CopyTo(text_field.GetAddressOf());
  ASSERT_NE(nullptr, text_field.Get());

  LONG start_offset, end_offset;
  EXPECT_HRESULT_SUCCEEDED(
      text_field->get_selection(0, &start_offset, &end_offset));
  EXPECT_EQ(1, start_offset);
  EXPECT_EQ(2, end_offset);

  EXPECT_HRESULT_SUCCEEDED(text_field->removeSelection(0));

  // There is no selection, just a caret.
  EXPECT_EQ(E_INVALIDARG,
            text_field->get_selection(0, &start_offset, &end_offset));
}

TEST_F(AXPlatformNodeWinTest, TestIAccessibleTextTextFieldGetSelected) {
  Init(BuildTextFieldWithSelectionRange(1, 2));

  ComPtr<IAccessible2> ia2_text_field = ToIAccessible2(GetRootIAccessible());
  ComPtr<IAccessibleText> text_field;
  ia2_text_field.CopyTo(text_field.GetAddressOf());
  ASSERT_NE(nullptr, text_field.Get());

  LONG start_offset, end_offset;

  // We only care about selection_index of zero, so passing anything but 0 as
  // the first parameter should fail.
  EXPECT_EQ(E_INVALIDARG,
            text_field->get_selection(1, &start_offset, &end_offset));

  EXPECT_HRESULT_SUCCEEDED(
      text_field->get_selection(0, &start_offset, &end_offset));
  EXPECT_EQ(1, start_offset);
  EXPECT_EQ(2, end_offset);
}

TEST_F(AXPlatformNodeWinTest, TestIAccessibleTextTextFieldGetSelectedBackward) {
  Init(BuildTextFieldWithSelectionRange(1, 2));

  ComPtr<IAccessible2> ia2_text_field = ToIAccessible2(GetRootIAccessible());
  ComPtr<IAccessibleText> text_field;
  ia2_text_field.CopyTo(text_field.GetAddressOf());
  ASSERT_NE(nullptr, text_field.Get());

  LONG start_offset, end_offset;
  EXPECT_HRESULT_SUCCEEDED(
      text_field->get_selection(0, &start_offset, &end_offset));
  EXPECT_EQ(1, start_offset);
  EXPECT_EQ(2, end_offset);
}

TEST_F(AXPlatformNodeWinTest, TestIAccessibleContentEditabledGetSelected) {
  Init(BuildContentEditableWithSelectionRange(1, 2));

  ComPtr<IAccessible2> ia2_text_field = ToIAccessible2(GetRootIAccessible());
  ComPtr<IAccessibleText> text_field;
  ia2_text_field.CopyTo(text_field.GetAddressOf());
  ASSERT_NE(nullptr, text_field.Get());

  LONG start_offset, end_offset;

  // We only care about selection_index of zero, so passing anything but 0 as
  // the first parameter should fail.
  EXPECT_EQ(E_INVALIDARG,
            text_field->get_selection(1, &start_offset, &end_offset));

  EXPECT_HRESULT_SUCCEEDED(
      text_field->get_selection(0, &start_offset, &end_offset));
  EXPECT_EQ(1, start_offset);
  EXPECT_EQ(2, end_offset);
}

TEST_F(AXPlatformNodeWinTest,
       TestIAccessibleContentEditabledGetSelectedBackward) {
  Init(BuildContentEditableWithSelectionRange(1, 2));

  ComPtr<IAccessible2> ia2_text_field = ToIAccessible2(GetRootIAccessible());
  ComPtr<IAccessibleText> text_field;
  ia2_text_field.CopyTo(text_field.GetAddressOf());
  ASSERT_NE(nullptr, text_field.Get());

  LONG start_offset, end_offset;
  EXPECT_HRESULT_SUCCEEDED(
      text_field->get_selection(0, &start_offset, &end_offset));
  EXPECT_EQ(1, start_offset);
  EXPECT_EQ(2, end_offset);
}

TEST_F(AXPlatformNodeWinTest, TestIAccessibleTextTextFieldAddSelection) {
  Init(BuildTextField());
  ComPtr<IAccessible2> ia2_text_field = ToIAccessible2(GetRootIAccessible());
  ComPtr<IAccessibleText> text_field;
  ia2_text_field.CopyTo(text_field.GetAddressOf());
  ASSERT_NE(nullptr, text_field.Get());

  LONG start_offset, end_offset;
  // There is no selection, just a caret.
  EXPECT_EQ(E_INVALIDARG,
            text_field->get_selection(0, &start_offset, &end_offset));

  EXPECT_HRESULT_SUCCEEDED(text_field->addSelection(1, 2));

  EXPECT_HRESULT_SUCCEEDED(
      text_field->get_selection(0, &start_offset, &end_offset));
  EXPECT_EQ(1, start_offset);
  EXPECT_EQ(2, end_offset);
}

// This test is disabled until UpdateStep2ComputeHypertext is migrated over
// to AXPlatformNodeWin because |hypertext_| is only initialized
// on the BrowserAccessibility side.
TEST_F(AXPlatformNodeWinTest,
       DISABLED_TestIAccessibleTextContentEditableAddSelection) {
  Init(BuildContentEditable());

  ComPtr<IAccessible2> ia2_text_field = ToIAccessible2(GetRootIAccessible());
  ComPtr<IAccessibleText> text_field;
  ia2_text_field.CopyTo(text_field.GetAddressOf());
  ASSERT_NE(nullptr, text_field.Get());

  LONG start_offset, end_offset;
  // There is no selection, just a caret.
  EXPECT_EQ(E_INVALIDARG,
            text_field->get_selection(0, &start_offset, &end_offset));

  EXPECT_HRESULT_SUCCEEDED(text_field->addSelection(1, 2));

  EXPECT_HRESULT_SUCCEEDED(
      text_field->get_selection(0, &start_offset, &end_offset));
  EXPECT_EQ(1, start_offset);
  EXPECT_EQ(2, end_offset);
}

TEST_F(AXPlatformNodeWinTest, TestIAccessibleTextTextFieldGetNSelectionsZero) {
  Init(BuildTextField());

  ComPtr<IAccessible2> ia2_text_field = ToIAccessible2(GetRootIAccessible());
  ComPtr<IAccessibleText> text_field;
  ia2_text_field.CopyTo(text_field.GetAddressOf());
  ASSERT_NE(nullptr, text_field.Get());

  LONG selections;
  EXPECT_HRESULT_SUCCEEDED(text_field->get_nSelections(&selections));
  EXPECT_EQ(0, selections);
}

TEST_F(AXPlatformNodeWinTest,
       TestIAccessibleTextContentEditableGetNSelectionsZero) {
  Init(BuildContentEditable());

  ComPtr<IAccessible2> ia2_text_field = ToIAccessible2(GetRootIAccessible());
  ComPtr<IAccessibleText> text_field;
  ia2_text_field.CopyTo(text_field.GetAddressOf());
  ASSERT_NE(nullptr, text_field.Get());

  LONG selections;
  EXPECT_HRESULT_SUCCEEDED(text_field->get_nSelections(&selections));
  EXPECT_EQ(0, selections);
}

TEST_F(AXPlatformNodeWinTest,
       TestIAccessibleTextContentEditableGetNSelections) {
  Init(BuildContentEditableWithSelectionRange(1, 2));
  ComPtr<IAccessible2> ia2_text_field = ToIAccessible2(GetRootIAccessible());
  ComPtr<IAccessibleText> text_field;
  ia2_text_field.CopyTo(text_field.GetAddressOf());
  ASSERT_NE(nullptr, text_field.Get());

  LONG selections;
  EXPECT_HRESULT_SUCCEEDED(text_field->get_nSelections(&selections));
  EXPECT_EQ(1, selections);
}

TEST_F(AXPlatformNodeWinTest,
       TestIAccessibleTextTextFieldGetCaretOffsetNoCaret) {
  Init(BuildTextField());

  ComPtr<IAccessible2> ia2_text_field = ToIAccessible2(GetRootIAccessible());
  ComPtr<IAccessibleText> text_field;
  ia2_text_field.CopyTo(text_field.GetAddressOf());
  ASSERT_NE(nullptr, text_field.Get());

  LONG offset;
  EXPECT_EQ(S_FALSE, text_field->get_caretOffset(&offset));
  EXPECT_EQ(0, offset);
}

TEST_F(AXPlatformNodeWinTest,
       TestIAccessibleTextTextFieldGetCaretOffsetHasCaret) {
  Init(BuildTextFieldWithSelectionRange(1, 2));

  ComPtr<IAccessible2> ia2_text_field = ToIAccessible2(GetRootIAccessible());
  ComPtr<IAccessibleText> text_field;
  ia2_text_field.CopyTo(text_field.GetAddressOf());
  ASSERT_NE(nullptr, text_field.Get());

  LONG offset;
  EXPECT_HRESULT_SUCCEEDED(text_field->get_caretOffset(&offset));
  EXPECT_EQ(2, offset);
}

TEST_F(AXPlatformNodeWinTest,
       TestIAccessibleTextContextEditableGetCaretOffsetNoCaret) {
  Init(BuildContentEditable());

  ComPtr<IAccessible2> ia2_text_field = ToIAccessible2(GetRootIAccessible());
  ComPtr<IAccessibleText> text_field;
  ia2_text_field.CopyTo(text_field.GetAddressOf());
  ASSERT_NE(nullptr, text_field.Get());

  LONG offset;
  EXPECT_EQ(S_FALSE, text_field->get_caretOffset(&offset));
  EXPECT_EQ(0, offset);
}

TEST_F(AXPlatformNodeWinTest,
       TestIAccessibleTextContentEditableGetCaretOffsetHasCaret) {
  Init(BuildContentEditableWithSelectionRange(1, 2));

  ComPtr<IAccessible2> ia2_text_field = ToIAccessible2(GetRootIAccessible());
  ComPtr<IAccessibleText> text_field;
  ia2_text_field.CopyTo(text_field.GetAddressOf());
  ASSERT_NE(nullptr, text_field.Get());

  LONG offset;
  EXPECT_HRESULT_SUCCEEDED(text_field->get_caretOffset(&offset));
  EXPECT_EQ(2, offset);
}

}  // namespace ui
