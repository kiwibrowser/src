// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <map>
#include <memory>

#include "ui/views/accessibility/native_view_accessibility_base.h"

#include "base/lazy_instance.h"
#include "base/threading/thread_task_runner_handle.h"
#include "ui/accessibility/platform/ax_platform_node.h"
#include "ui/events/event_utils.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/views/controls/native/native_view_host.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

namespace views {

namespace {

base::LazyInstance<std::map<int32_t, ui::AXPlatformNode*>>::Leaky
    g_unique_id_to_ax_platform_node = LAZY_INSTANCE_INITIALIZER;

// Information required to fire a delayed accessibility event.
struct QueuedEvent {
  ax::mojom::Event type;
  int32_t node_id;
};

base::LazyInstance<std::vector<QueuedEvent>>::Leaky g_event_queue =
    LAZY_INSTANCE_INITIALIZER;

bool g_is_queueing_events = false;

bool IsAccessibilityFocusableWhenEnabled(View* view) {
  return view->focus_behavior() != View::FocusBehavior::NEVER &&
         view->IsDrawn();
}

// Used to determine if a View should be ignored by accessibility clients by
// being a non-keyboard-focusable child of a keyboard-focusable ancestor. E.g.,
// LabelButtons contain Labels, but a11y should just show that there's a button.
bool IsViewUnfocusableDescendantOfFocusableAncestor(View* view) {
  if (IsAccessibilityFocusableWhenEnabled(view))
    return false;

  while (view->parent()) {
    view = view->parent();
    if (IsAccessibilityFocusableWhenEnabled(view))
      return true;
  }
  return false;
}

ui::AXPlatformNode* FromNativeWindow(gfx::NativeWindow native_window) {
  Widget* widget = Widget::GetWidgetForNativeWindow(native_window);
  if (!widget)
    return nullptr;

  View* view = widget->GetRootView();
  if (!view)
    return nullptr;

  gfx::NativeViewAccessible native_view_accessible =
      view->GetNativeViewAccessible();
  if (!native_view_accessible)
    return nullptr;

  return ui::AXPlatformNode::FromNativeViewAccessible(native_view_accessible);
}

ui::AXPlatformNode* PlatformNodeFromNodeID(int32_t id) {
  // Note: For Views, node IDs and unique IDs are the same - but that isn't
  // necessarily true for all AXPlatformNodes.
  auto it = g_unique_id_to_ax_platform_node.Get().find(id);

  if (it == g_unique_id_to_ax_platform_node.Get().end())
    return nullptr;

  return it->second;
}

void FireEvent(QueuedEvent event) {
  ui::AXPlatformNode* node = PlatformNodeFromNodeID(event.node_id);
  if (node)
    node->NotifyAccessibilityEvent(event.type);
}

void FlushQueue() {
  DCHECK(g_is_queueing_events);
  for (QueuedEvent event : g_event_queue.Get())
    FireEvent(event);
  g_is_queueing_events = false;
  g_event_queue.Get().clear();
}

}  // namespace

// static
int32_t NativeViewAccessibilityBase::fake_focus_view_id_ = 0;

NativeViewAccessibilityBase::NativeViewAccessibilityBase(View* view)
    : ViewAccessibility(view) {
  ax_node_ = ui::AXPlatformNode::Create(this);
  DCHECK(ax_node_);

  static bool first_time = true;
  if (first_time) {
    ui::AXPlatformNode::RegisterNativeWindowHandler(
        base::BindRepeating(&FromNativeWindow));
    first_time = false;
  }

  g_unique_id_to_ax_platform_node.Get()[GetUniqueId().Get()] = ax_node_;
}

NativeViewAccessibilityBase::~NativeViewAccessibilityBase() {
  g_unique_id_to_ax_platform_node.Get().erase(GetUniqueId().Get());
  ax_node_->Destroy();
}

gfx::NativeViewAccessible NativeViewAccessibilityBase::GetNativeObject() {
  return ax_node_->GetNativeViewAccessible();
}

void NativeViewAccessibilityBase::NotifyAccessibilityEvent(
    ax::mojom::Event event_type) {
  if (g_is_queueing_events) {
    g_event_queue.Get().push_back({event_type, GetUniqueId().Get()});
    return;
  }

  ax_node_->NotifyAccessibilityEvent(event_type);

  // A focus context event is intended to send a focus event and a delay
  // before the next focus event. It makes sense to delay the entire next
  // synchronous batch of next events so that ordering remains the same.
  if (event_type == ax::mojom::Event::kFocusContext) {
    // Begin queueing subsequent events and flush queue asynchronously.
    g_is_queueing_events = true;
    base::OnceCallback<void()> cb = base::BindOnce(&FlushQueue);
    base::ThreadTaskRunnerHandle::Get()->PostTask(FROM_HERE, std::move(cb));
  }
}

// ui::AXPlatformNodeDelegate

const ui::AXNodeData& NativeViewAccessibilityBase::GetData() const {
  // Clear it, then populate it.
  data_ = ui::AXNodeData();
  GetAccessibleNodeData(&data_);

  // View::IsDrawn is true if a View is visible and all of its ancestors are
  // visible too, since invisibility inherits.
  //
  // TODO(dmazzoni): Maybe consider moving this to ViewAccessibility?
  // This will require ensuring that Chrome OS invalidates the whole
  // subtree when a View changes its visibility state.
  if (!view()->IsDrawn())
    data_.AddState(ax::mojom::State::kInvisible);

  // Make sure this element is excluded from the a11y tree if there's a
  // focusable parent. All keyboard focusable elements should be leaf nodes.
  // Exceptions to this rule will themselves be accessibility focusable.
  //
  // TODO(dmazzoni): this code was added to support MacViews acccessibility,
  // because we needed a way to mark a View as a leaf node in the
  // accessibility tree. We need to replace this with a cross-platform
  // solution that works for ChromeVox, too, and move it to ViewAccessibility.
  if (IsViewUnfocusableDescendantOfFocusableAncestor(view()))
    data_.role = ax::mojom::Role::kIgnored;

  return data_;
}

const ui::AXTreeData& NativeViewAccessibilityBase::GetTreeData() const {
  CR_DEFINE_STATIC_LOCAL(ui::AXTreeData, empty_data, ());
  return empty_data;
}

int NativeViewAccessibilityBase::GetChildCount() {
  if (IsLeaf())
    return 0;
  int child_count = view()->child_count();

  std::vector<Widget*> child_widgets;
  PopulateChildWidgetVector(&child_widgets);
  child_count += child_widgets.size();

  return child_count;
}

gfx::NativeViewAccessible NativeViewAccessibilityBase::ChildAtIndex(int index) {
  if (IsLeaf())
    return nullptr;

  // If this is a root view, our widget might have child widgets. Include
  std::vector<Widget*> child_widgets;
  PopulateChildWidgetVector(&child_widgets);
  int child_widget_count = static_cast<int>(child_widgets.size());

  if (index < view()->child_count()) {
    return view()->child_at(index)->GetNativeViewAccessible();
  } else if (index < view()->child_count() + child_widget_count) {
    Widget* child_widget = child_widgets[index - view()->child_count()];
    return child_widget->GetRootView()->GetNativeViewAccessible();
  }

  return nullptr;
}

gfx::NativeWindow NativeViewAccessibilityBase::GetTopLevelWidget() {
  if (view()->GetWidget())
    return view()->GetWidget()->GetTopLevelWidget()->GetNativeWindow();
  return nullptr;
}

gfx::NativeViewAccessible NativeViewAccessibilityBase::GetParent() {
  if (view()->parent())
    return view()->parent()->GetNativeViewAccessible();

  if (Widget* widget = view()->GetWidget()) {
    Widget* top_widget = widget->GetTopLevelWidget();
    if (top_widget && widget != top_widget && top_widget->GetRootView())
      return top_widget->GetRootView()->GetNativeViewAccessible();
  }

  return nullptr;
}

gfx::Rect NativeViewAccessibilityBase::GetClippedScreenBoundsRect() const {
  // We could optionally add clipping here if ever needed.
  return view()->GetBoundsInScreen();
}

gfx::Rect NativeViewAccessibilityBase::GetUnclippedScreenBoundsRect() const {
  return view()->GetBoundsInScreen();
}

gfx::NativeViewAccessible NativeViewAccessibilityBase::HitTestSync(int x,
                                                                   int y) {
  if (!view() || !view()->GetWidget())
    return nullptr;

  if (IsLeaf())
    return GetNativeObject();

  // Search child widgets first, since they're on top in the z-order.
  std::vector<Widget*> child_widgets;
  PopulateChildWidgetVector(&child_widgets);
  for (Widget* child_widget : child_widgets) {
    View* child_root_view = child_widget->GetRootView();
    gfx::Point point(x, y);
    View::ConvertPointFromScreen(child_root_view, &point);
    if (child_root_view->HitTestPoint(point))
      return child_root_view->GetNativeViewAccessible();
  }

  gfx::Point point(x, y);
  View::ConvertPointFromScreen(view(), &point);
  if (!view()->HitTestPoint(point))
    return nullptr;

  // Check if the point is within any of the immediate children of this
  // view. We don't have to search further because AXPlatformNode will
  // do a recursive hit test if we return anything other than |this| or NULL.
  for (int i = view()->child_count() - 1; i >= 0; --i) {
    View* child_view = view()->child_at(i);
    if (!child_view->visible())
      continue;

    gfx::Point point_in_child_coords(point);
    view()->ConvertPointToTarget(view(), child_view, &point_in_child_coords);
    if (child_view->HitTestPoint(point_in_child_coords))
      return child_view->GetNativeViewAccessible();
  }

  // If it's not inside any of our children, it's inside this view.
  return GetNativeObject();
}

void NativeViewAccessibilityBase::OnAutofillShown() {
  // When the autofill is shown, treat it and the currently selected item as
  // focused, even though the actual focus is in the browser's currently
  // focused textfield.
  DCHECK(!fake_focus_view_id_) << "Cannot have more that one fake focus.";
  fake_focus_view_id_ = GetUniqueId().Get();
  ui::AXPlatformNode::OnAutofillShown();
}

void NativeViewAccessibilityBase::OnAutofillHidden() {
  DCHECK(fake_focus_view_id_) << "No autofill fake focus set.";
  DCHECK_EQ(fake_focus_view_id_, GetUniqueId().Get())
      << "Cannot clear autofill fake focus on an object that did not have it.";
  fake_focus_view_id_ = 0;
  ui::AXPlatformNode::OnAutofillHidden();
}

gfx::NativeViewAccessible NativeViewAccessibilityBase::GetFocus() {
  FocusManager* focus_manager = view()->GetFocusManager();
  View* focused_view =
      focus_manager ? focus_manager->GetFocusedView() : nullptr;
  if (fake_focus_view_id_) {
    ui::AXPlatformNode* ax_node = PlatformNodeFromNodeID(fake_focus_view_id_);
    if (ax_node)
      return ax_node->GetNativeViewAccessible();
  }
  return focused_view ? focused_view->GetNativeViewAccessible() : nullptr;
}

ui::AXPlatformNode* NativeViewAccessibilityBase::GetFromNodeID(int32_t id) {
  return PlatformNodeFromNodeID(id);
}

int NativeViewAccessibilityBase::GetIndexInParent() const {
  return -1;
}

gfx::AcceleratedWidget
NativeViewAccessibilityBase::GetTargetForNativeAccessibilityEvent() {
  return gfx::kNullAcceleratedWidget;
}

int NativeViewAccessibilityBase::GetTableRowCount() const {
  return 0;
}

int NativeViewAccessibilityBase::GetTableColCount() const {
  return 0;
}

std::vector<int32_t> NativeViewAccessibilityBase::GetColHeaderNodeIds(
    int32_t col_index) const {
  return std::vector<int32_t>();
}

std::vector<int32_t> NativeViewAccessibilityBase::GetRowHeaderNodeIds(
    int32_t row_index) const {
  return std::vector<int32_t>();
}

int32_t NativeViewAccessibilityBase::GetCellId(int32_t row_index,
                                               int32_t col_index) const {
  return 0;
}

int32_t NativeViewAccessibilityBase::CellIdToIndex(int32_t cell_id) const {
  return -1;
}

int32_t NativeViewAccessibilityBase::CellIndexToId(int32_t cell_index) const {
  return 0;
}

bool NativeViewAccessibilityBase::AccessibilityPerformAction(
    const ui::AXActionData& data) {
  return view()->HandleAccessibleAction(data);
}

bool NativeViewAccessibilityBase::ShouldIgnoreHoveredStateForTesting() {
  return false;
}

bool NativeViewAccessibilityBase::IsOffscreen() const {
  // TODO: need to implement.
  return false;
}

std::set<int32_t> NativeViewAccessibilityBase::GetReverseRelations(
    ax::mojom::IntAttribute attr,
    int32_t dst_id) {
  return std::set<int32_t>();
}

std::set<int32_t> NativeViewAccessibilityBase::GetReverseRelations(
    ax::mojom::IntListAttribute attr,
    int32_t dst_id) {
  return std::set<int32_t>();
}

const ui::AXUniqueId& NativeViewAccessibilityBase::GetUniqueId() const {
  return ViewAccessibility::GetUniqueId();
}

void NativeViewAccessibilityBase::PopulateChildWidgetVector(
    std::vector<Widget*>* result_child_widgets) {
  // Only attach child widgets to the root view.
  Widget* widget = view()->GetWidget();
  // Note that during window close, a Widget may exist in a state where it has
  // no NativeView, but hasn't yet torn down its view hierarchy.
  if (!widget || !widget->GetNativeView() || widget->GetRootView() != view())
    return;

  std::set<Widget*> child_widgets;
  Widget::GetAllOwnedWidgets(widget->GetNativeView(), &child_widgets);
  for (auto iter = child_widgets.begin(); iter != child_widgets.end(); ++iter) {
    Widget* child_widget = *iter;
    DCHECK_NE(widget, child_widget);

    if (!child_widget->IsVisible())
      continue;

    if (widget->GetNativeWindowProperty(kWidgetNativeViewHostKey))
      continue;

    result_child_widgets->push_back(child_widget);
  }
}

}  // namespace views
