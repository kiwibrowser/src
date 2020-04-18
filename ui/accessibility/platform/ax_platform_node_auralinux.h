// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_ACCESSIBILITY_AX_PLATFORM_NODE_AURALINUX_H_
#define UI_ACCESSIBILITY_AX_PLATFORM_NODE_AURALINUX_H_

#include <atk/atk.h>

#include "base/macros.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/accessibility/ax_export.h"
#include "ui/accessibility/platform/ax_platform_node_base.h"

// Some ATK interfaces require returning a (const gchar*), use
// this macro to make it safe to return a pointer to a temporary
// string.
#define ATK_AURALINUX_RETURN_STRING(str_expr) \
  {                                           \
    static std::string result;                \
    result = (str_expr);                      \
    return result.c_str();                    \
  }

namespace ui {

// Implements accessibility on Aura Linux using ATK.
class AXPlatformNodeAuraLinux : public AXPlatformNodeBase {
 public:
  AXPlatformNodeAuraLinux();
  ~AXPlatformNodeAuraLinux() override;

  // Set or get the root-level Application object that's the parent of all
  // top-level windows.
  AX_EXPORT static void SetApplication(AXPlatformNode* application);
  static AXPlatformNode* application() { return application_; }

  static void EnsureGTypeInit();

  // Do asynchronous static initialization.
  AX_EXPORT static void StaticInitialize();

  AX_EXPORT void DataChanged();
  void Destroy() override;
  AX_EXPORT void AddAccessibilityTreeProperties(base::DictionaryValue* dict);

  AtkRole GetAtkRole();
  void GetAtkState(AtkStateSet* state_set);
  void GetAtkRelations(AtkRelationSet* atk_relation_set);
  void GetExtents(gint* x, gint* y, gint* width, gint* height,
                  AtkCoordType coord_type);
  void GetPosition(gint* x, gint* y, AtkCoordType coord_type);
  void GetSize(gint* width, gint* height);
  gfx::NativeViewAccessible HitTestSync(gint x,
                                        gint y,
                                        AtkCoordType coord_type);
  bool GrabFocus();
  bool DoDefaultAction();
  const gchar* GetDefaultActionName();

  void SetExtentsRelativeToAtkCoordinateType(
      gint* x, gint* y, gint* width, gint* height,
      AtkCoordType coord_type);

  // AtkDocument helpers
  const gchar* GetDocumentAttributeValue(const gchar* attribute) const;
  AtkAttributeSet* GetDocumentAttributes() const;

  // AtkHyperlink helpers
  AtkHyperlink* GetAtkHyperlink();

  // Misc helpers
  void GetFloatAttributeInGValue(ax::mojom::FloatAttribute attr, GValue* value);

  // Event helpers
  void OnFocused();

  // AXPlatformNode overrides.
  gfx::NativeViewAccessible GetNativeViewAccessible() override;
  void NotifyAccessibilityEvent(ax::mojom::Event event_type) override;

  // AXPlatformNodeBase overrides.
  void Init(AXPlatformNodeDelegate* delegate) override;
  int GetIndexInParent() override;

 private:
  enum AtkInterfaces {
    ATK_ACTION_INTERFACE,
    ATK_COMPONENT_INTERFACE,
    ATK_DOCUMENT_INTERFACE,
    ATK_EDITABLE_TEXT_INTERFACE,
    ATK_HYPERLINK_INTERFACE,
    ATK_HYPERTEXT_INTERFACE,
    ATK_IMAGE_INTERFACE,
    ATK_SELECTION_INTERFACE,
    ATK_TABLE_INTERFACE,
    ATK_TEXT_INTERFACE,
    ATK_VALUE_INTERFACE,
  };
  static const char* GetUniqueAccessibilityGTypeName(int interface_mask);
  int GetGTypeInterfaceMask();
  GType GetAccessibilityGType();
  AtkObject* CreateAtkObject();
  void DestroyAtkObjects();

  // Keep information of latest AtkInterfaces mask to refresh atk object
  // interfaces accordingly if needed.
  int interface_mask_;

  // We own a reference to these ref-counted objects.
  AtkObject* atk_object_;
  AtkHyperlink* atk_hyperlink_;

  // The root-level Application object that's the parent of all
  // top-level windows.
  static AXPlatformNode* application_;

  // The last AtkObject with keyboard focus. Tracking this is required
  // to emit the ATK_STATE_FOCUSED change to false.
  static AtkObject* current_focused_;

  DISALLOW_COPY_AND_ASSIGN(AXPlatformNodeAuraLinux);
};

}  // namespace ui

#endif  // UI_ACCESSIBILITY_AX_PLATFORM_NODE_AURALINUX_H_
