// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/examples/tree_view_example.h"

#include "base/strings/utf_string_conversions.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/menu/menu_model_adapter.h"
#include "ui/views/controls/menu/menu_runner.h"
#include "ui/views/controls/tree/tree_view.h"
#include "ui/views/controls/tree/tree_view_drawing_provider.h"
#include "ui/views/layout/grid_layout.h"

using base::ASCIIToUTF16;

namespace {

class ExampleTreeViewDrawingProvider : public views::TreeViewDrawingProvider {
 public:
  ExampleTreeViewDrawingProvider() {}
  ~ExampleTreeViewDrawingProvider() override {}

  base::string16 GetAuxiliaryTextForNode(views::TreeView* tree_view,
                                         ui::TreeModelNode* node) override {
    if (tree_view->GetSelectedNode() == node)
      return base::UTF8ToUTF16("Selected");
    return views::TreeViewDrawingProvider::GetAuxiliaryTextForNode(tree_view,
                                                                   node);
  }

  bool ShouldDrawIconForNode(views::TreeView* tree_view,
                             ui::TreeModelNode* node) override {
    return tree_view->GetSelectedNode() != node;
  }
};

}  // namespace

namespace views {
namespace examples {

TreeViewExample::TreeViewExample()
    : ExampleBase("Tree View"),
      model_(std::make_unique<NodeType>(ASCIIToUTF16("root"), 1)) {}

TreeViewExample::~TreeViewExample() {
  // Delete the view before the model.
  tree_view_.reset();
}

void TreeViewExample::CreateExampleView(View* container) {
  // Add some sample data.
  NodeType* colors_node = model_.GetRoot()->Add(
      std::make_unique<NodeType>(ASCIIToUTF16("colors"), 1), 0);
  colors_node->Add(std::make_unique<NodeType>(ASCIIToUTF16("red"), 1), 0);
  colors_node->Add(std::make_unique<NodeType>(ASCIIToUTF16("green"), 1), 1);
  colors_node->Add(std::make_unique<NodeType>(ASCIIToUTF16("blue"), 1), 2);

  NodeType* sheep_node = model_.GetRoot()->Add(
      std::make_unique<NodeType>(ASCIIToUTF16("sheep"), 1), 0);
  sheep_node->Add(std::make_unique<NodeType>(ASCIIToUTF16("Sheep 1"), 1), 0);
  sheep_node->Add(std::make_unique<NodeType>(ASCIIToUTF16("Sheep 2"), 1), 1);

  tree_view_ = std::make_unique<TreeView>();
  tree_view_->set_context_menu_controller(this);
  tree_view_->SetRootShown(false);
  tree_view_->SetModel(&model_);
  tree_view_->SetController(this);
  tree_view_->SetDrawingProvider(
      std::make_unique<ExampleTreeViewDrawingProvider>());
  add_ = new LabelButton(this, ASCIIToUTF16("Add"));
  add_->SetFocusForPlatform();
  add_->set_request_focus_on_press(true);
  remove_ = new LabelButton(this, ASCIIToUTF16("Remove"));
  remove_->SetFocusForPlatform();
  remove_->set_request_focus_on_press(true);
  change_title_ = new LabelButton(this, ASCIIToUTF16("Change Title"));
  change_title_->SetFocusForPlatform();
  change_title_->set_request_focus_on_press(true);

  GridLayout* layout = container->SetLayoutManager(
      std::make_unique<views::GridLayout>(container));

  const int tree_view_column = 0;
  ColumnSet* column_set = layout->AddColumnSet(tree_view_column);
  column_set->AddColumn(GridLayout::FILL, GridLayout::FILL,
                        1.0f, GridLayout::USE_PREF, 0, 0);
  layout->StartRow(1 /* expand */, tree_view_column);
  layout->AddView(tree_view_->CreateParentIfNecessary());

  // Add control buttons horizontally.
  const int button_column = 1;
  column_set = layout->AddColumnSet(button_column);
  for (int i = 0; i < 3; i++) {
    column_set->AddColumn(GridLayout::FILL, GridLayout::FILL,
                          1.0f, GridLayout::USE_PREF, 0, 0);
  }

  layout->StartRow(0 /* no expand */, button_column);
  layout->AddView(add_);
  layout->AddView(remove_);
  layout->AddView(change_title_);
}

void TreeViewExample::AddNewNode() {
  NodeType* selected_node =
      static_cast<NodeType*>(tree_view_->GetSelectedNode());
  if (!selected_node)
    selected_node = model_.GetRoot();
  NodeType* new_node = model_.Add(
      selected_node, std::make_unique<NodeType>(selected_node->GetTitle(), 1),
      selected_node->child_count());
  tree_view_->SetSelectedNode(new_node);
}

bool TreeViewExample::IsCommandIdEnabled(int command_id) {
  return command_id != ID_REMOVE ||
      tree_view_->GetSelectedNode() != model_.GetRoot();
}

void TreeViewExample::ButtonPressed(Button* sender, const ui::Event& event) {
  NodeType* selected_node =
      static_cast<NodeType*>(tree_view_->GetSelectedNode());
  if (sender == add_) {
    AddNewNode();
  } else if (sender == remove_) {
    DCHECK(selected_node);
    DCHECK_NE(model_.GetRoot(), selected_node);
    model_.Remove(selected_node->parent(), selected_node);
  } else if (sender == change_title_) {
    DCHECK(selected_node);
    model_.SetTitle(selected_node,
                    selected_node->GetTitle() + ASCIIToUTF16("new"));
  }
}

void TreeViewExample::OnTreeViewSelectionChanged(TreeView* tree_view) {
  ui::TreeModelNode* node = tree_view_->GetSelectedNode();
  if (node) {
    change_title_->SetEnabled(true);
    remove_->SetEnabled(node != model_.GetRoot());
  } else {
    change_title_->SetEnabled(false);
    remove_->SetEnabled(false);
  }
}

bool TreeViewExample::CanEdit(TreeView* tree_view,
                              ui::TreeModelNode* node) {
  return true;
}

void TreeViewExample::ShowContextMenuForView(View* source,
                                             const gfx::Point& point,
                                             ui::MenuSourceType source_type) {
  context_menu_model_.reset(new ui::SimpleMenuModel(this));
  context_menu_model_->AddItem(ID_EDIT, ASCIIToUTF16("Edit"));
  context_menu_model_->AddItem(ID_REMOVE, ASCIIToUTF16("Remove"));
  context_menu_model_->AddItem(ID_ADD, ASCIIToUTF16("Add"));
  context_menu_runner_.reset(new MenuRunner(context_menu_model_.get(), 0));
  context_menu_runner_->RunMenuAt(source->GetWidget(), nullptr,
                                  gfx::Rect(point, gfx::Size()),
                                  MENU_ANCHOR_TOPLEFT, source_type);
}

bool TreeViewExample::IsCommandIdChecked(int command_id) const {
  return false;
}

bool TreeViewExample::IsCommandIdEnabled(int command_id) const {
  return const_cast<TreeViewExample*>(this)->IsCommandIdEnabled(command_id);
}

void TreeViewExample::ExecuteCommand(int command_id, int event_flags) {
  NodeType* selected_node =
      static_cast<NodeType*>(tree_view_->GetSelectedNode());
  switch (command_id) {
    case ID_EDIT:
      tree_view_->StartEditing(selected_node);
      break;
    case ID_REMOVE:
      model_.Remove(selected_node->parent(), selected_node);
      break;
    case ID_ADD:
      AddNewNode();
      break;
    default:
      NOTREACHED();
  }
}

}  // namespace examples
}  // namespace views
