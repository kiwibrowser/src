// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_ARC_ACCESSIBILITY_AX_TREE_SOURCE_ARC_H_
#define CHROME_BROWSER_CHROMEOS_ARC_ACCESSIBILITY_AX_TREE_SOURCE_ARC_H_

#include <map>
#include <memory>
#include <vector>

#include "components/arc/common/accessibility_helper.mojom.h"
#include "ui/accessibility/ax_host_delegate.h"
#include "ui/accessibility/ax_node.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/accessibility/ax_tree_data.h"
#include "ui/accessibility/ax_tree_serializer.h"
#include "ui/accessibility/ax_tree_source.h"
#include "ui/views/view.h"

namespace aura {
class Window;
}

namespace arc {
class AXTreeSourceArcTest;

using AXTreeArcSerializer =
    ui::AXTreeSerializer<mojom::AccessibilityNodeInfoData*,
                         ui::AXNodeData,
                         ui::AXTreeData>;

// This class represents the accessibility tree from the focused ARC window.
class AXTreeSourceArc
    : public ui::AXTreeSource<mojom::AccessibilityNodeInfoData*,
                              ui::AXNodeData,
                              ui::AXTreeData>,
      public ui::AXHostDelegate {
 public:
  class Delegate {
   public:
    virtual void OnAction(const ui::AXActionData& data) const = 0;
  };

  explicit AXTreeSourceArc(Delegate* delegate);
  ~AXTreeSourceArc() override;

  // AXTreeSource overrides.
  bool GetTreeData(ui::AXTreeData* data) const override;

  // Notify automation of an accessibility event.
  void NotifyAccessibilityEvent(mojom::AccessibilityEventData* event_data);

  // Notify automation of a result to an action.
  void NotifyActionResult(const ui::AXActionData& data, bool result);

  // Attaches tree to an aura window and gives it system focus.
  void Focus(aura::Window* window);

  // Gets the window id of this tree.
  int32_t window_id() const { return window_id_; }

 private:
  friend class arc::AXTreeSourceArcTest;
  class FocusStealer;

  // AXTreeSource overrides.
  mojom::AccessibilityNodeInfoData* GetRoot() const override;
  mojom::AccessibilityNodeInfoData* GetFromId(int32_t id) const override;
  int32_t GetId(mojom::AccessibilityNodeInfoData* node) const override;
  void GetChildren(mojom::AccessibilityNodeInfoData* node,
                   std::vector<mojom::AccessibilityNodeInfoData*>* out_children)
      const override;
  mojom::AccessibilityNodeInfoData* GetParent(
      mojom::AccessibilityNodeInfoData* node) const override;
  bool IsValid(mojom::AccessibilityNodeInfoData* node) const override;
  bool IsEqual(mojom::AccessibilityNodeInfoData* node1,
               mojom::AccessibilityNodeInfoData* node2) const override;
  mojom::AccessibilityNodeInfoData* GetNull() const override;
  void SerializeNode(mojom::AccessibilityNodeInfoData* node,
                     ui::AXNodeData* out_data) const override;

  // Returns bounds of a node which can be passed to AXNodeData.location. Bounds
  // are returned in the following coordinates depending on whether it's root or
  // not.
  // - Root node is relative to its container, i.e. focused window.
  // - Non-root node is relative to the root node of this tree.
  //
  // focused_window is nullptr for notification.
  const gfx::Rect GetBounds(mojom::AccessibilityNodeInfoData* node,
                            aura::Window* focused_window) const;

  // Computes the smallest rect that encloses all of the descendants of |node|.
  gfx::Rect ComputeEnclosingBounds(
      mojom::AccessibilityNodeInfoData* node) const;

  // Helper to recursively compute bounds for |node|. Returns true if non-empty
  // bounds were encountered.
  void ComputeEnclosingBoundsInternal(mojom::AccessibilityNodeInfoData* node,
                                      gfx::Rect& computed_bounds) const;

  // AXHostDelegate overrides.
  void PerformAction(const ui::AXActionData& data) override;

  // Resets tree state.
  void Reset();

  void PopulateAXRole(mojom::AccessibilityNodeInfoData* node,
                      ui::AXNodeData* out_data) const;

  // Maps an AccessibilityNodeInfo to its tree data.
  std::map<int32_t, mojom::AccessibilityNodeInfoData*> tree_map_;
  std::map<int32_t, int32_t> parent_map_;
  std::unique_ptr<AXTreeArcSerializer> current_tree_serializer_;
  int32_t root_id_;
  int32_t window_id_;
  int32_t focused_node_id_;
  bool is_notification_;

  // A delegate that handles accessibility actions on behalf of this tree. The
  // delegate is valid during the lifetime of this tree.
  const Delegate* const delegate_;
  std::unique_ptr<FocusStealer> focus_stealer_;
  std::string package_name_;
  std::map<mojom::AccessibilityNodeInfoData*, gfx::Rect>
      cached_computed_bounds_;

  DISALLOW_COPY_AND_ASSIGN(AXTreeSourceArc);
};

}  // namespace arc

#endif  // CHROME_BROWSER_CHROMEOS_ARC_ACCESSIBILITY_AX_TREE_SOURCE_ARC_H_
