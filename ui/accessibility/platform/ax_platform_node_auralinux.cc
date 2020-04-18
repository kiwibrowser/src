// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/accessibility/platform/ax_platform_node_auralinux.h"

#include <stdint.h>

#include "base/command_line.h"
#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/accessibility/ax_action_data.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/accessibility/ax_text_utils.h"
#include "ui/accessibility/ax_tree_data.h"
#include "ui/accessibility/platform/atk_util_auralinux.h"
#include "ui/accessibility/platform/ax_platform_atk_hyperlink.h"
#include "ui/accessibility/platform/ax_platform_node_delegate.h"
#include "ui/gfx/geometry/rect_conversions.h"

//
// ax_platform_node_auralinux AtkObject definition and implementation.
//

G_BEGIN_DECLS

#define AX_PLATFORM_NODE_AURALINUX_TYPE (ax_platform_node_auralinux_get_type())
#define AX_PLATFORM_NODE_AURALINUX(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST( \
        (obj), AX_PLATFORM_NODE_AURALINUX_TYPE, AXPlatformNodeAuraLinuxObject))
#define AX_PLATFORM_NODE_AURALINUX_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST( \
        (klass), AX_PLATFORM_NODE_AURALINUX_TYPE, AXPlatformNodeAuraLinuxClass))
#define IS_AX_PLATFORM_NODE_AURALINUX(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE((obj), AX_PLATFORM_NODE_AURALINUX_TYPE))
#define IS_AX_PLATFORM_NODE_AURALINUX_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE((klass), AX_PLATFORM_NODE_AURALINUX_TYPE))
#define AX_PLATFORM_NODE_AURALINUX_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS( \
        (obj), AX_PLATFORM_NODE_AURALINUX_TYPE, AXPlatformNodeAuraLinuxClass))

typedef struct _AXPlatformNodeAuraLinuxObject AXPlatformNodeAuraLinuxObject;
typedef struct _AXPlatformNodeAuraLinuxClass AXPlatformNodeAuraLinuxClass;

// TODO(aleventhal) Remove this and use atk_role_get_name() once the following
// GNOME bug is fixed: https://bugzilla.gnome.org/show_bug.cgi?id=795983
const char* role_names[] = {
    "invalid",  // ATK_ROLE_INVALID.
    "accelerator label",
    "alert",
    "animation",
    "arrow",
    "calendar",
    "canvas",
    "check box",
    "check menu item",
    "color chooser",
    "column header",
    "combo box",
    "dateeditor",
    "desktop icon",
    "desktop frame",
    "dial",
    "dialog",
    "directory pane",
    "drawing area",
    "file chooser",
    "filler",
    "fontchooser",
    "frame",
    "glass pane",
    "html container",
    "icon",
    "image",
    "internal frame",
    "label",
    "layered pane",
    "list",
    "list item",
    "menu",
    "menu bar",
    "menu item",
    "option pane",
    "page tab",
    "page tab list",
    "panel",
    "password text",
    "popup menu",
    "progress bar",
    "push button",
    "radio button",
    "radio menu item",
    "root pane",
    "row header",
    "scroll bar",
    "scroll pane",
    "separator",
    "slider",
    "split pane",
    "spin button",
    "statusbar",
    "table",
    "table cell",
    "table column header",
    "table row header",
    "tear off menu item",
    "terminal",
    "text",
    "toggle button",
    "tool bar",
    "tool tip",
    "tree",
    "tree table",
    "unknown",
    "viewport",
    "window",
    "header",
    "footer",
    "paragraph",
    "ruler",
    "application",
    "autocomplete",
    "edit bar",
    "embedded component",
    "entry",
    "chart",
    "caption",
    "document frame",
    "heading",
    "page",
    "section",
    "redundant object",
    "form",
    "link",
    "input method window",
    "table row",
    "tree item",
    "document spreadsheet",
    "document presentation",
    "document text",
    "document web",
    "document email",
    "comment",
    "list box",
    "grouping",
    "image map",
    "notification",
    "info bar",
    "level bar",
    "title bar",
    "block quote",
    "audio",
    "video",
    "definition",
    "article",
    "landmark",
    "log",
    "marquee",
    "math",
    "rating",
    "timer",
    "description list",
    "description term",
    "description value",
    "static",
    "math fraction",
    "math root",
    "subscript",
    "superscript",
    "footnote",  // ATK_ROLE_FOOTNOTE = 122.
};

#if defined(ATK_CHECK_VERSION) && ATK_CHECK_VERSION(2, 16, 0)
#define ATK_216
#endif

#if defined(ATK_CHECK_VERSION) && ATK_CHECK_VERSION(2, 26, 0)
#define ATK_226
#endif

#if defined(ATK_216)
constexpr AtkRole kStaticRole = ATK_ROLE_STATIC;
#else
constexpr AtkRole kStaticRole = ATK_ROLE_TEXT;
#endif

#if defined(ATK_226)
constexpr AtkRole kAtkFootnoteRole = ATK_ROLE_FOOTNOTE;
#else
constexpr AtkRole kAtkFootnoteRole = ATK_ROLE_LIST_ITEM;
#endif

struct _AXPlatformNodeAuraLinuxObject {
  AtkObject parent;
  ui::AXPlatformNodeAuraLinux* m_object;
};

struct _AXPlatformNodeAuraLinuxClass {
  AtkObjectClass parent_class;
};

GType ax_platform_node_auralinux_get_type();

static gpointer ax_platform_node_auralinux_parent_class = nullptr;

static ui::AXPlatformNodeAuraLinux* ToAXPlatformNodeAuraLinux(
    AXPlatformNodeAuraLinuxObject* atk_object) {
  if (!atk_object)
    return nullptr;

  return atk_object->m_object;
}

static ui::AXPlatformNodeAuraLinux* AtkObjectToAXPlatformNodeAuraLinux(
    AtkObject* atk_object) {
  if (!atk_object)
    return nullptr;

  if (IS_AX_PLATFORM_NODE_AURALINUX(atk_object))
    return ToAXPlatformNodeAuraLinux(AX_PLATFORM_NODE_AURALINUX(atk_object));

  return nullptr;
}

static const gchar* ax_platform_node_auralinux_get_name(AtkObject* atk_object) {
  ui::AXPlatformNodeAuraLinux* obj =
      AtkObjectToAXPlatformNodeAuraLinux(atk_object);
  if (!obj)
    return nullptr;

  ax::mojom::NameFrom name_from = obj->GetData().GetNameFrom();
  if (obj->GetStringAttribute(ax::mojom::StringAttribute::kName).empty() &&
      name_from != ax::mojom::NameFrom::kAttributeExplicitlyEmpty)
    return nullptr;

  return obj->GetStringAttribute(ax::mojom::StringAttribute::kName).c_str();
}

static const gchar* ax_platform_node_auralinux_get_description(
    AtkObject* atk_object) {
  ui::AXPlatformNodeAuraLinux* obj =
      AtkObjectToAXPlatformNodeAuraLinux(atk_object);
  if (!obj)
    return nullptr;

  return obj->GetStringAttribute(ax::mojom::StringAttribute::kDescription)
      .c_str();
}

static gint ax_platform_node_auralinux_get_index_in_parent(
    AtkObject* atk_object) {
  ui::AXPlatformNodeAuraLinux* obj =
    AtkObjectToAXPlatformNodeAuraLinux(atk_object);

  if (!obj)
    return -1;

  return obj->GetIndexInParent();
}

static AtkObject* ax_platform_node_auralinux_get_parent(AtkObject* atk_object) {
  ui::AXPlatformNodeAuraLinux* obj =
      AtkObjectToAXPlatformNodeAuraLinux(atk_object);
  if (!obj)
    return nullptr;

  return obj->GetParent();
}

static gint ax_platform_node_auralinux_get_n_children(AtkObject* atk_object) {
  ui::AXPlatformNodeAuraLinux* obj =
      AtkObjectToAXPlatformNodeAuraLinux(atk_object);
  if (!obj)
    return 0;

  return obj->GetChildCount();
}

static AtkObject* ax_platform_node_auralinux_ref_child(
    AtkObject* atk_object, gint index) {
  ui::AXPlatformNodeAuraLinux* obj =
      AtkObjectToAXPlatformNodeAuraLinux(atk_object);
  if (!obj)
    return nullptr;

  AtkObject* result = obj->ChildAtIndex(index);
  if (result)
    g_object_ref(result);
  return result;
}

static AtkRelationSet* ax_platform_node_auralinux_ref_relation_set(
    AtkObject* atk_object) {
  ui::AXPlatformNodeAuraLinux* obj =
      AtkObjectToAXPlatformNodeAuraLinux(atk_object);
  AtkRelationSet* atk_relation_set =
      ATK_OBJECT_CLASS(ax_platform_node_auralinux_parent_class)->
      ref_relation_set(atk_object);

  if (!obj)
    return atk_relation_set;

  obj->GetAtkRelations(atk_relation_set);
  return atk_relation_set;
}

static AtkAttributeSet* ax_platform_node_auralinux_get_attributes(
    AtkObject* atk_object) {
  return NULL;
}

static AtkRole ax_platform_node_auralinux_get_role(AtkObject* atk_object) {
  ui::AXPlatformNodeAuraLinux* obj =
      AtkObjectToAXPlatformNodeAuraLinux(atk_object);
  if (!obj)
    return ATK_ROLE_INVALID;
  return obj->GetAtkRole();
}

static AtkStateSet* ax_platform_node_auralinux_ref_state_set(
    AtkObject* atk_object) {
  AtkStateSet* atk_state_set =
      ATK_OBJECT_CLASS(ax_platform_node_auralinux_parent_class)->
      ref_state_set(atk_object);

  ui::AXPlatformNodeAuraLinux* obj =
      AtkObjectToAXPlatformNodeAuraLinux(atk_object);
  if (!obj) {
    atk_state_set_add_state(atk_state_set, ATK_STATE_DEFUNCT);
  } else {
    obj->GetAtkState(atk_state_set);
  }
  return atk_state_set;
}

//
// AtkComponent interface
//

static gfx::Point FindAtkObjectParentCoords(AtkObject* atk_object) {
  if (!atk_object)
    return gfx::Point(0, 0);

  if (atk_object_get_role(atk_object) == ATK_ROLE_WINDOW) {
    int x, y;
    atk_component_get_extents(ATK_COMPONENT(atk_object),
        &x, &y, nullptr, nullptr, ATK_XY_WINDOW);
    gfx::Point window_coords(x, y);
    return window_coords;
  }
  atk_object = atk_object_get_parent(atk_object);

  return FindAtkObjectParentCoords(atk_object);
}

static void ax_platform_node_auralinux_get_extents(AtkComponent* atk_component,
                                                   gint* x, gint* y,
                                                   gint* width, gint* height,
                                                   AtkCoordType coord_type) {
  g_return_if_fail(ATK_IS_COMPONENT(atk_component));

  if (x)
    *x = 0;
  if (y)
    *y = 0;
  if (width)
    *width = 0;
  if (height)
    *height = 0;

  AtkObject* atk_object = ATK_OBJECT(atk_component);
  ui::AXPlatformNodeAuraLinux* obj =
      AtkObjectToAXPlatformNodeAuraLinux(atk_object);
  if (!obj)
    return;

  obj->GetExtents(x, y, width, height, coord_type);
}

static void ax_platform_node_auralinux_get_position(AtkComponent* atk_component,
                                                    gint* x, gint* y,
                                                    AtkCoordType coord_type) {
  g_return_if_fail(ATK_IS_COMPONENT(atk_component));

  if (x)
    *x = 0;
  if (y)
    *y = 0;

  AtkObject* atk_object = ATK_OBJECT(atk_component);
  ui::AXPlatformNodeAuraLinux* obj =
      AtkObjectToAXPlatformNodeAuraLinux(atk_object);
  if (!obj)
    return;

  obj->GetPosition(x, y, coord_type);
}

static void ax_platform_node_auralinux_get_size(AtkComponent* atk_component,
                                                gint* width, gint* height) {
  g_return_if_fail(ATK_IS_COMPONENT(atk_component));

  if (width)
    *width = 0;
  if (height)
    *height = 0;

  AtkObject* atk_object = ATK_OBJECT(atk_component);
  ui::AXPlatformNodeAuraLinux* obj =
      AtkObjectToAXPlatformNodeAuraLinux(atk_object);
  if (!obj)
    return;

  obj->GetSize(width, height);
}

static AtkObject* ax_platform_node_auralinux_ref_accessible_at_point(
    AtkComponent* atk_component,
    gint x,
    gint y,
    AtkCoordType coord_type) {
  g_return_val_if_fail(ATK_IS_COMPONENT(atk_component), nullptr);
  AtkObject* atk_object = ATK_OBJECT(atk_component);
  ui::AXPlatformNodeAuraLinux* obj =
      AtkObjectToAXPlatformNodeAuraLinux(atk_object);
  if (!obj)
    return nullptr;

  AtkObject* result = obj->HitTestSync(x, y, coord_type);
  if (result)
    g_object_ref(result);
  return result;
}

static gboolean ax_platform_node_auralinux_grab_focus(
    AtkComponent* atk_component) {
  g_return_val_if_fail(ATK_IS_COMPONENT(atk_component), FALSE);
  AtkObject* atk_object = ATK_OBJECT(atk_component);
  ui::AXPlatformNodeAuraLinux* obj =
      AtkObjectToAXPlatformNodeAuraLinux(atk_object);
  if (!obj)
    return FALSE;

  return obj->GrabFocus();
}

void ax_component_interface_base_init(AtkComponentIface* iface) {
  iface->get_extents = ax_platform_node_auralinux_get_extents;
  iface->get_position = ax_platform_node_auralinux_get_position;
  iface->get_size = ax_platform_node_auralinux_get_size;
  iface->ref_accessible_at_point =
      ax_platform_node_auralinux_ref_accessible_at_point;
  iface->grab_focus = ax_platform_node_auralinux_grab_focus;
}

static const GInterfaceInfo ComponentInfo = {
  reinterpret_cast<GInterfaceInitFunc>(ax_component_interface_base_init), 0, 0
};

//
// AtkAction interface
//

static gboolean ax_platform_node_auralinux_do_action(AtkAction* atk_action,
                                                     gint index) {
  g_return_val_if_fail(ATK_IS_ACTION(atk_action), FALSE);
  g_return_val_if_fail(!index, FALSE);

  AtkObject* atk_object = ATK_OBJECT(atk_action);
  ui::AXPlatformNodeAuraLinux* obj =
      AtkObjectToAXPlatformNodeAuraLinux(atk_object);
  if (!obj)
    return FALSE;

  return obj->DoDefaultAction();
}

static gint ax_platform_node_auralinux_get_n_actions(AtkAction* atk_action) {
  g_return_val_if_fail(ATK_IS_ACTION(atk_action), 0);

  AtkObject* atk_object = ATK_OBJECT(atk_action);
  ui::AXPlatformNodeAuraLinux* obj =
      AtkObjectToAXPlatformNodeAuraLinux(atk_object);
  if (!obj)
    return 0;

  return 1;
}

static const gchar* ax_platform_node_auralinux_get_action_description(
    AtkAction*,
    gint) {
  // Not implemented. Right now Orca does not provide this and
  // Chromium is not providing a string for the action description.
  return nullptr;
}

static const gchar* ax_platform_node_auralinux_get_action_name(
    AtkAction* atk_action,
    gint index) {
  g_return_val_if_fail(ATK_IS_ACTION(atk_action), nullptr);
  g_return_val_if_fail(!index, nullptr);

  AtkObject* atk_object = ATK_OBJECT(atk_action);
  ui::AXPlatformNodeAuraLinux* obj =
      AtkObjectToAXPlatformNodeAuraLinux(atk_object);
  if (!obj)
    return nullptr;

  return obj->GetDefaultActionName();
}

static const gchar* ax_platform_node_auralinux_get_action_keybinding(
    AtkAction* atk_action,
    gint index) {
  g_return_val_if_fail(ATK_IS_ACTION(atk_action), nullptr);
  g_return_val_if_fail(!index, nullptr);

  AtkObject* atk_object = ATK_OBJECT(atk_action);
  ui::AXPlatformNodeAuraLinux* obj =
      AtkObjectToAXPlatformNodeAuraLinux(atk_object);
  if (!obj)
    return nullptr;

  return obj->GetStringAttribute(ax::mojom::StringAttribute::kAccessKey)
      .c_str();
}

void ax_action_interface_base_init(AtkActionIface* iface) {
  iface->do_action = ax_platform_node_auralinux_do_action;
  iface->get_n_actions = ax_platform_node_auralinux_get_n_actions;
  iface->get_description = ax_platform_node_auralinux_get_action_description;
  iface->get_name = ax_platform_node_auralinux_get_action_name;
  iface->get_keybinding = ax_platform_node_auralinux_get_action_keybinding;
}

static const GInterfaceInfo ActionInfo = {
    reinterpret_cast<GInterfaceInitFunc>(ax_action_interface_base_init),
    nullptr, nullptr};

// AtkDocument interface.

static const gchar* ax_platform_node_auralinux_get_document_attribute_value(
    AtkDocument* atk_doc,
    const gchar* attribute) {
  g_return_val_if_fail(ATK_IS_DOCUMENT(atk_doc), nullptr);

  AtkObject* atk_object = ATK_OBJECT(atk_doc);
  ui::AXPlatformNodeAuraLinux* obj =
      AtkObjectToAXPlatformNodeAuraLinux(atk_object);
  if (!obj)
    return nullptr;

  return obj->GetDocumentAttributeValue(attribute);
}

static AtkAttributeSet* ax_platform_node_auralinux_get_document_attributes(
    AtkDocument* atk_doc) {
  g_return_val_if_fail(ATK_IS_DOCUMENT(atk_doc), 0);

  AtkObject* atk_object = ATK_OBJECT(atk_doc);
  ui::AXPlatformNodeAuraLinux* obj =
      AtkObjectToAXPlatformNodeAuraLinux(atk_object);
  if (!obj)
    return nullptr;

  return obj->GetDocumentAttributes();
}

void ax_document_interface_base_init(AtkDocumentIface* iface) {
  iface->get_document_attribute_value =
      ax_platform_node_auralinux_get_document_attribute_value;
  iface->get_document_attributes =
      ax_platform_node_auralinux_get_document_attributes;
}

static const GInterfaceInfo DocumentInfo = {
    reinterpret_cast<GInterfaceInitFunc>(ax_document_interface_base_init),
    nullptr, nullptr};

//
// AtkImage interface.
//

static void ax_platform_node_auralinux_get_image_position(
    AtkImage* atk_img,
    gint* x,
    gint* y,
    AtkCoordType coord_type) {
  g_return_if_fail(ATK_IMAGE(atk_img));

  AtkObject* atk_object = ATK_OBJECT(atk_img);
  ui::AXPlatformNodeAuraLinux* obj =
      AtkObjectToAXPlatformNodeAuraLinux(atk_object);
  if (!obj)
    return;

  obj->GetPosition(x, y, coord_type);
}

static const gchar* ax_platform_node_auralinux_get_image_description(
    AtkImage* atk_img) {
  g_return_val_if_fail(ATK_IMAGE(atk_img), nullptr);

  AtkObject* atk_object = ATK_OBJECT(atk_img);
  ui::AXPlatformNodeAuraLinux* obj =
      AtkObjectToAXPlatformNodeAuraLinux(atk_object);
  if (!obj)
    return nullptr;

  return obj->GetStringAttribute(ax::mojom::StringAttribute::kDescription)
      .c_str();
}

static void ax_platform_node_auralinux_get_image_size(AtkImage* atk_img,
                                                      gint* width,
                                                      gint* height) {
  g_return_if_fail(ATK_IMAGE(atk_img));

  AtkObject* atk_object = ATK_OBJECT(atk_img);
  ui::AXPlatformNodeAuraLinux* obj =
      AtkObjectToAXPlatformNodeAuraLinux(atk_object);
  if (!obj)
    return;

  obj->GetSize(width, height);
}

void ax_image_interface_base_init(AtkImageIface* iface) {
  iface->get_image_position = ax_platform_node_auralinux_get_image_position;
  iface->get_image_description =
      ax_platform_node_auralinux_get_image_description;
  iface->get_image_size = ax_platform_node_auralinux_get_image_size;
}

static const GInterfaceInfo ImageInfo = {
    reinterpret_cast<GInterfaceInitFunc>(ax_image_interface_base_init), nullptr,
    nullptr};

//
// AtkValue interface
//

static void ax_platform_node_auralinux_get_current_value(AtkValue* atk_value,
                                                         GValue* value) {
  g_return_if_fail(ATK_VALUE(atk_value));

  AtkObject* atk_object = ATK_OBJECT(atk_value);
  ui::AXPlatformNodeAuraLinux* obj =
      AtkObjectToAXPlatformNodeAuraLinux(atk_object);
  if (!obj)
    return;

  obj->GetFloatAttributeInGValue(ax::mojom::FloatAttribute::kValueForRange,
                                 value);
}

static void ax_platform_node_auralinux_get_minimum_value(AtkValue* atk_value,
                                                         GValue* value) {
  g_return_if_fail(ATK_VALUE(atk_value));

  AtkObject* atk_object = ATK_OBJECT(atk_value);
  ui::AXPlatformNodeAuraLinux* obj =
      AtkObjectToAXPlatformNodeAuraLinux(atk_object);
  if (!obj)
    return;

  obj->GetFloatAttributeInGValue(ax::mojom::FloatAttribute::kMinValueForRange,
                                 value);
}

static void ax_platform_node_auralinux_get_maximum_value(AtkValue* atk_value,
                                                         GValue* value) {
  g_return_if_fail(ATK_VALUE(atk_value));

  AtkObject* atk_object = ATK_OBJECT(atk_value);
  ui::AXPlatformNodeAuraLinux* obj =
      AtkObjectToAXPlatformNodeAuraLinux(atk_object);
  if (!obj)
    return;

  obj->GetFloatAttributeInGValue(ax::mojom::FloatAttribute::kMaxValueForRange,
                                 value);
}

static void ax_platform_node_auralinux_get_minimum_increment(
    AtkValue* atk_value,
    GValue* value) {
  g_return_if_fail(ATK_VALUE(atk_value));

  AtkObject* atk_object = ATK_OBJECT(atk_value);
  ui::AXPlatformNodeAuraLinux* obj =
      AtkObjectToAXPlatformNodeAuraLinux(atk_object);
  if (!obj)
    return;

  obj->GetFloatAttributeInGValue(ax::mojom::FloatAttribute::kStepValueForRange,
                                 value);
}

static void ax_value_interface_base_init(AtkValueIface* iface) {
  iface->get_current_value = ax_platform_node_auralinux_get_current_value;
  iface->get_maximum_value = ax_platform_node_auralinux_get_maximum_value;
  iface->get_minimum_value = ax_platform_node_auralinux_get_minimum_value;
  iface->get_minimum_increment =
      ax_platform_node_auralinux_get_minimum_increment;
}

static const GInterfaceInfo ValueInfo = {
    reinterpret_cast<GInterfaceInitFunc>(ax_value_interface_base_init), nullptr,
    nullptr};

//
// AtkHyperlinkImpl interface.
//

static AtkHyperlink* ax_platform_node_auralinux_get_hyperlink(
    AtkHyperlinkImpl* atk_hyperlink_impl) {
  g_return_val_if_fail(ATK_HYPERLINK_IMPL(atk_hyperlink_impl), 0);

  AtkObject* atk_object = ATK_OBJECT(atk_hyperlink_impl);
  ui::AXPlatformNodeAuraLinux* obj =
      AtkObjectToAXPlatformNodeAuraLinux(atk_object);
  if (!obj)
    return 0;

  AtkHyperlink* atk_hyperlink = obj->GetAtkHyperlink();
  g_object_ref(atk_hyperlink);

  return atk_hyperlink;
}

void ax_hyperlink_impl_interface_base_init(AtkHyperlinkImplIface* iface) {
  iface->get_hyperlink = ax_platform_node_auralinux_get_hyperlink;
}

static const GInterfaceInfo HyperlinkImplInfo = {
    reinterpret_cast<GInterfaceInitFunc>(ax_hyperlink_impl_interface_base_init),
    nullptr, nullptr};

//
// The rest of the AXPlatformNodeAtk code, not specific to one
// of the Atk* interfaces.
//

static void ax_platform_node_auralinux_init(AtkObject* atk_object,
                                            gpointer data) {
  if (ATK_OBJECT_CLASS(ax_platform_node_auralinux_parent_class)->initialize) {
    ATK_OBJECT_CLASS(ax_platform_node_auralinux_parent_class)->initialize(
        atk_object, data);
  }

  AX_PLATFORM_NODE_AURALINUX(atk_object)->m_object =
      reinterpret_cast<ui::AXPlatformNodeAuraLinux*>(data);
}

static void ax_platform_node_auralinux_finalize(GObject* atk_object) {
  G_OBJECT_CLASS(ax_platform_node_auralinux_parent_class)->finalize(atk_object);
}

static void ax_platform_node_auralinux_class_init(AtkObjectClass* klass) {
  GObjectClass* gobject_class = G_OBJECT_CLASS(klass);
  ax_platform_node_auralinux_parent_class = g_type_class_peek_parent(klass);

  gobject_class->finalize = ax_platform_node_auralinux_finalize;
  klass->initialize = ax_platform_node_auralinux_init;
  klass->get_name = ax_platform_node_auralinux_get_name;
  klass->get_description = ax_platform_node_auralinux_get_description;
  klass->get_parent = ax_platform_node_auralinux_get_parent;
  klass->get_n_children = ax_platform_node_auralinux_get_n_children;
  klass->ref_child = ax_platform_node_auralinux_ref_child;
  klass->get_role = ax_platform_node_auralinux_get_role;
  klass->ref_state_set = ax_platform_node_auralinux_ref_state_set;
  klass->get_index_in_parent = ax_platform_node_auralinux_get_index_in_parent;
  klass->ref_relation_set = ax_platform_node_auralinux_ref_relation_set;
  klass->get_attributes = ax_platform_node_auralinux_get_attributes;
}

GType ax_platform_node_auralinux_get_type() {
  ui::AXPlatformNodeAuraLinux::EnsureGTypeInit();

  static volatile gsize type_volatile = 0;
  if (g_once_init_enter(&type_volatile)) {
    static const GTypeInfo tinfo = {
        sizeof(AXPlatformNodeAuraLinuxClass),
        (GBaseInitFunc) nullptr,
        (GBaseFinalizeFunc) nullptr,
        (GClassInitFunc)ax_platform_node_auralinux_class_init,
        (GClassFinalizeFunc) nullptr,
        nullptr,                               /* class data */
        sizeof(AXPlatformNodeAuraLinuxObject), /* instance size */
        0,                                     /* nb preallocs */
        (GInstanceInitFunc) nullptr,
        nullptr /* value table */
    };

    GType type = g_type_register_static(
        ATK_TYPE_OBJECT, "AXPlatformNodeAuraLinux", &tinfo, GTypeFlags(0));
    g_once_init_leave(&type_volatile, type);
  }

  return type_volatile;
}

void ax_platform_node_auralinux_detach(
    AXPlatformNodeAuraLinuxObject* atk_object) {
  if (atk_object->m_object) {
    atk_object_notify_state_change(ATK_OBJECT(atk_object), ATK_STATE_DEFUNCT,
                                   TRUE);
  }
  atk_object->m_object = nullptr;
}

G_END_DECLS

namespace ui {

void AXPlatformNodeAuraLinux::EnsureGTypeInit() {
#if !GLIB_CHECK_VERSION(2, 36, 0)
  static bool first_time = true;
  if (UNLIKELY(first_time)) {
    g_type_init();
    first_time = false;
  }
#endif
}

const char* AXPlatformNodeAuraLinux::GetUniqueAccessibilityGTypeName(
    int interface_mask) {
  // 37 characters is enough for "AXPlatformNodeAuraLinux%x" with any integer
  // value.
  static char name[37];
  snprintf(name, sizeof(name), "AXPlatformNodeAuraLinux%x", interface_mask);
  return name;
}

int AXPlatformNodeAuraLinux::GetGTypeInterfaceMask() {
  int interface_mask = 0;

  // Component interface is always supported.
  interface_mask |= 1 << ATK_COMPONENT_INTERFACE;

  // Action interface is basic one. It just relays on executing default action
  // for each object.
  interface_mask |= 1 << ATK_ACTION_INTERFACE;

  // Value Interface
  int role = GetAtkRole();
  if (role == ATK_ROLE_SCROLL_BAR || role == ATK_ROLE_SLIDER ||
      role == ATK_ROLE_PROGRESS_BAR || role == ATK_ROLE_SEPARATOR ||
      role == ATK_ROLE_SPIN_BUTTON) {
    interface_mask |= 1 << ATK_VALUE_INTERFACE;
  }

  // Document Interface
  if (role == ATK_ROLE_DOCUMENT_WEB)
    interface_mask |= 1 << ATK_DOCUMENT_INTERFACE;

  // Image Interface
  if (role == ATK_ROLE_IMAGE || role == ATK_ROLE_IMAGE_MAP)
    interface_mask |= 1 << ATK_IMAGE_INTERFACE;

  // HyperlinkImpl interface
  if (role == ATK_ROLE_LINK)
    interface_mask |= 1 << ATK_HYPERLINK_INTERFACE;

  return interface_mask;
}

GType AXPlatformNodeAuraLinux::GetAccessibilityGType() {
  static const GTypeInfo type_info = {
      sizeof(AXPlatformNodeAuraLinuxClass),
      (GBaseInitFunc) nullptr,
      (GBaseFinalizeFunc) nullptr,
      (GClassInitFunc) nullptr,
      (GClassFinalizeFunc) nullptr,
      nullptr,                               /* class data */
      sizeof(AXPlatformNodeAuraLinuxObject), /* instance size */
      0,                                     /* nb preallocs */
      (GInstanceInitFunc) nullptr,
      nullptr /* value table */
  };

  const char* atk_type_name = GetUniqueAccessibilityGTypeName(interface_mask_);
  GType type = g_type_from_name(atk_type_name);
  if (type)
    return type;

  type = g_type_register_static(AX_PLATFORM_NODE_AURALINUX_TYPE, atk_type_name,
                                &type_info, GTypeFlags(0));
  if (interface_mask_ & (1 << ATK_COMPONENT_INTERFACE))
    g_type_add_interface_static(type, ATK_TYPE_COMPONENT, &ComponentInfo);
  if (interface_mask_ & (1 << ATK_ACTION_INTERFACE))
    g_type_add_interface_static(type, ATK_TYPE_ACTION, &ActionInfo);
  if (interface_mask_ & (1 << ATK_DOCUMENT_INTERFACE))
    g_type_add_interface_static(type, ATK_TYPE_DOCUMENT, &DocumentInfo);
  if (interface_mask_ & (1 << ATK_IMAGE_INTERFACE))
    g_type_add_interface_static(type, ATK_TYPE_IMAGE, &ImageInfo);
  if (interface_mask_ & (1 << ATK_VALUE_INTERFACE))
    g_type_add_interface_static(type, ATK_TYPE_VALUE, &ValueInfo);
  if (interface_mask_ & (1 << ATK_HYPERLINK_INTERFACE))
    g_type_add_interface_static(type, ATK_TYPE_HYPERLINK_IMPL,
                                &HyperlinkImplInfo);

  return type;
}

AtkObject* AXPlatformNodeAuraLinux::CreateAtkObject() {
  EnsureGTypeInit();
  interface_mask_ = GetGTypeInterfaceMask();
  GType type = GetAccessibilityGType();
  AtkObject* atk_object = static_cast<AtkObject*>(g_object_new(type, nullptr));

  atk_object_initialize(atk_object, this);

  return ATK_OBJECT(atk_object);
}

void AXPlatformNodeAuraLinux::DestroyAtkObjects() {
  if (atk_hyperlink_) {
    ax_platform_atk_hyperlink_set_object(
        AX_PLATFORM_ATK_HYPERLINK(atk_hyperlink_), nullptr);
    g_object_unref(atk_hyperlink_);
    atk_hyperlink_ = nullptr;
  }
  if (atk_object_) {
    if (atk_object_ == current_focused_)
      current_focused_ = nullptr;
    ax_platform_node_auralinux_detach(AX_PLATFORM_NODE_AURALINUX(atk_object_));
    g_object_unref(atk_object_);
    atk_object_ = nullptr;
  }
}

// static
AXPlatformNode* AXPlatformNode::Create(AXPlatformNodeDelegate* delegate) {
  AXPlatformNodeAuraLinux* node = new AXPlatformNodeAuraLinux();
  node->Init(delegate);
  return node;
}

// static
AXPlatformNode* AXPlatformNode::FromNativeViewAccessible(
    gfx::NativeViewAccessible accessible) {
  return AtkObjectToAXPlatformNodeAuraLinux(accessible);
}

//
// AXPlatformNodeAuraLinux implementation.
//

// static
AXPlatformNode* AXPlatformNodeAuraLinux::application_ = nullptr;

// static
void AXPlatformNodeAuraLinux::SetApplication(AXPlatformNode* application) {
  application_ = application;
}

// static
void AXPlatformNodeAuraLinux::StaticInitialize() {
  AtkUtilAuraLinux::GetInstance()->InitializeAsync();
}

AtkRole AXPlatformNodeAuraLinux::GetAtkRole() {
  switch (GetData().role) {
    case ax::mojom::Role::kAlert:
      return ATK_ROLE_ALERT;
    case ax::mojom::Role::kAlertDialog:
      return ATK_ROLE_ALERT;
    case ax::mojom::Role::kAnchor:
      return ATK_ROLE_LINK;
    case ax::mojom::Role::kAnnotation:
      return ATK_ROLE_PANEL;
    case ax::mojom::Role::kApplication:
      // Don't use ATK_ROLE_APPLICATION, which is for top level app windows,
      // not ARIA applications.
      return ATK_ROLE_EMBEDDED;
    case ax::mojom::Role::kArticle:
      return ATK_ROLE_ARTICLE;
    case ax::mojom::Role::kAudio:
      return ATK_ROLE_AUDIO;
    case ax::mojom::Role::kBanner:
      return ATK_ROLE_LANDMARK;
    case ax::mojom::Role::kBlockquote:
      return ATK_ROLE_BLOCK_QUOTE;
    case ax::mojom::Role::kCaret:
      return ATK_ROLE_UNKNOWN;
    case ax::mojom::Role::kButton:
      return ATK_ROLE_PUSH_BUTTON;
    case ax::mojom::Role::kCanvas:
      return ATK_ROLE_CANVAS;
    case ax::mojom::Role::kCaption:
      return ATK_ROLE_CAPTION;
    case ax::mojom::Role::kCell:
      return ATK_ROLE_TABLE_CELL;
    case ax::mojom::Role::kCheckBox:
      return ATK_ROLE_CHECK_BOX;
    case ax::mojom::Role::kSwitch:
      return ATK_ROLE_TOGGLE_BUTTON;
    case ax::mojom::Role::kColorWell:
      return ATK_ROLE_COLOR_CHOOSER;
    case ax::mojom::Role::kColumn:
      return ATK_ROLE_UNKNOWN;
    case ax::mojom::Role::kColumnHeader:
      return ATK_ROLE_COLUMN_HEADER;
    case ax::mojom::Role::kComboBoxGrouping:
      return ATK_ROLE_COMBO_BOX;
    case ax::mojom::Role::kComboBoxMenuButton:
      return ATK_ROLE_COMBO_BOX;
    case ax::mojom::Role::kComplementary:
      return ATK_ROLE_LANDMARK;
    case ax::mojom::Role::kContentInfo:
      return ATK_ROLE_LANDMARK;
    case ax::mojom::Role::kDate:
      return ATK_ROLE_DATE_EDITOR;
    case ax::mojom::Role::kDateTime:
      return ATK_ROLE_DATE_EDITOR;
    case ax::mojom::Role::kDefinition:
    case ax::mojom::Role::kDescriptionListDetail:
      return ATK_ROLE_DESCRIPTION_VALUE;
    case ax::mojom::Role::kDescriptionList:
      return ATK_ROLE_DESCRIPTION_LIST;
    case ax::mojom::Role::kDescriptionListTerm:
      return ATK_ROLE_DESCRIPTION_TERM;
    case ax::mojom::Role::kDetails:
      return ATK_ROLE_PANEL;
    case ax::mojom::Role::kDialog:
      return ATK_ROLE_DIALOG;
    case ax::mojom::Role::kDirectory:
      return ATK_ROLE_LIST;
    case ax::mojom::Role::kDisclosureTriangle:
      return ATK_ROLE_TOGGLE_BUTTON;
    case ax::mojom::Role::kDocCover:
      return ATK_ROLE_IMAGE;
    case ax::mojom::Role::kDocBackLink:
    case ax::mojom::Role::kDocBiblioRef:
    case ax::mojom::Role::kDocGlossRef:
    case ax::mojom::Role::kDocNoteRef:
      return ATK_ROLE_LINK;
    case ax::mojom::Role::kDocBiblioEntry:
    case ax::mojom::Role::kDocEndnote:
      return ATK_ROLE_LIST_ITEM;
    case ax::mojom::Role::kDocNotice:
    case ax::mojom::Role::kDocTip:
      return ATK_ROLE_COMMENT;
    case ax::mojom::Role::kDocFootnote:
      return kAtkFootnoteRole;
    case ax::mojom::Role::kDocPageBreak:
      return ATK_ROLE_SEPARATOR;
    case ax::mojom::Role::kDocAcknowledgments:
    case ax::mojom::Role::kDocAfterword:
    case ax::mojom::Role::kDocAppendix:
    case ax::mojom::Role::kDocBibliography:
    case ax::mojom::Role::kDocChapter:
    case ax::mojom::Role::kDocConclusion:
    case ax::mojom::Role::kDocCredits:
    case ax::mojom::Role::kDocEndnotes:
    case ax::mojom::Role::kDocEpilogue:
    case ax::mojom::Role::kDocErrata:
    case ax::mojom::Role::kDocForeword:
    case ax::mojom::Role::kDocGlossary:
    case ax::mojom::Role::kDocIndex:
    case ax::mojom::Role::kDocIntroduction:
    case ax::mojom::Role::kDocPageList:
    case ax::mojom::Role::kDocPart:
    case ax::mojom::Role::kDocPreface:
    case ax::mojom::Role::kDocPrologue:
    case ax::mojom::Role::kDocToc:
      return ATK_ROLE_LANDMARK;
    case ax::mojom::Role::kDocAbstract:
    case ax::mojom::Role::kDocColophon:
    case ax::mojom::Role::kDocCredit:
    case ax::mojom::Role::kDocDedication:
    case ax::mojom::Role::kDocEpigraph:
    case ax::mojom::Role::kDocExample:
    case ax::mojom::Role::kDocPullquote:
    case ax::mojom::Role::kDocQna:
      return ATK_ROLE_SECTION;
    case ax::mojom::Role::kDocSubtitle:
      return ATK_ROLE_HEADING;
    case ax::mojom::Role::kDocument:
      return ATK_ROLE_DOCUMENT_WEB;
    case ax::mojom::Role::kEmbeddedObject:
      return ATK_ROLE_PANEL;
    case ax::mojom::Role::kForm:
      return ATK_ROLE_FORM;  // Spec says ATK_ROLE_LANDMARK, checking.
    case ax::mojom::Role::kFigure:
    case ax::mojom::Role::kFeed:
    case ax::mojom::Role::kGenericContainer:
      return ATK_ROLE_PANEL;
    case ax::mojom::Role::kGraphicsDocument:
      return ATK_ROLE_DOCUMENT_WEB;
    case ax::mojom::Role::kGraphicsObject:
      return ATK_ROLE_PANEL;
    case ax::mojom::Role::kGraphicsSymbol:
      return ATK_ROLE_IMAGE;
    case ax::mojom::Role::kGrid:
      return ATK_ROLE_TABLE;
    case ax::mojom::Role::kGroup:
      return ATK_ROLE_PANEL;
    case ax::mojom::Role::kHeading:
      return ATK_ROLE_HEADING;
    case ax::mojom::Role::kIframe:
    case ax::mojom::Role::kIframePresentational:
      return ATK_ROLE_DOCUMENT_WEB;
    case ax::mojom::Role::kIgnored:
      return ATK_ROLE_REDUNDANT_OBJECT;
    case ax::mojom::Role::kImage:
      return ATK_ROLE_IMAGE;
    case ax::mojom::Role::kImageMap:
      return ATK_ROLE_IMAGE_MAP;
    case ax::mojom::Role::kInputTime:
      return ATK_ROLE_DATE_EDITOR;
    case ax::mojom::Role::kLabelText:
      return ATK_ROLE_LABEL;
    case ax::mojom::Role::kLegend:
      return ATK_ROLE_TEXT;
    // Layout table objects are treated the same as Role::kGenericContainer.
    case ax::mojom::Role::kLayoutTable:
      return ATK_ROLE_PANEL;
    case ax::mojom::Role::kLayoutTableCell:
      return ATK_ROLE_PANEL;
    case ax::mojom::Role::kLayoutTableColumn:
      return ATK_ROLE_PANEL;
    case ax::mojom::Role::kLayoutTableRow:
      return ATK_ROLE_PANEL;
    case ax::mojom::Role::kLineBreak:
      return ATK_ROLE_TEXT;
    case ax::mojom::Role::kLink:
      return ATK_ROLE_LINK;
    case ax::mojom::Role::kList:
      return ATK_ROLE_LIST;
    case ax::mojom::Role::kListBox:
      return ATK_ROLE_LIST_BOX;
    // TODO(Accessibility) Use ATK_ROLE_MENU_ITEM inside a combo box, see how
    // ax_platform_node_win.cc code does this.
    case ax::mojom::Role::kListBoxOption:
      return ATK_ROLE_LIST_ITEM;
    case ax::mojom::Role::kListMarker:
      return kStaticRole;
    case ax::mojom::Role::kListItem:
      return ATK_ROLE_LIST_ITEM;
    case ax::mojom::Role::kLog:
      return ATK_ROLE_LOG;
    case ax::mojom::Role::kMain:
      return ATK_ROLE_LANDMARK;
    case ax::mojom::Role::kMark:
      return ATK_ROLE_TEXT;
    case ax::mojom::Role::kMath:
      return ATK_ROLE_MATH;
    case ax::mojom::Role::kMarquee:
      return ATK_ROLE_MARQUEE;
    case ax::mojom::Role::kMenu:
      return ATK_ROLE_MENU;
    case ax::mojom::Role::kMenuButton:
      return ATK_ROLE_MENU;
    case ax::mojom::Role::kMenuBar:
      return ATK_ROLE_MENU_BAR;
    case ax::mojom::Role::kMenuItem:
      return ATK_ROLE_MENU_ITEM;
    case ax::mojom::Role::kMenuItemCheckBox:
      return ATK_ROLE_CHECK_MENU_ITEM;
    case ax::mojom::Role::kMenuItemRadio:
      return ATK_ROLE_RADIO_MENU_ITEM;
    case ax::mojom::Role::kMenuListPopup:
      return ATK_ROLE_MENU;
    case ax::mojom::Role::kMenuListOption:
      return ATK_ROLE_MENU_ITEM;
    case ax::mojom::Role::kMeter:
      return ATK_ROLE_PROGRESS_BAR;
    case ax::mojom::Role::kNavigation:
      return ATK_ROLE_LANDMARK;
    case ax::mojom::Role::kNote:
      return ATK_ROLE_COMMENT;
    case ax::mojom::Role::kPane:
    case ax::mojom::Role::kScrollView:
      return ATK_ROLE_PANEL;
    case ax::mojom::Role::kParagraph:
      return ATK_ROLE_PARAGRAPH;
    case ax::mojom::Role::kPopUpButton:
      return ATK_ROLE_PUSH_BUTTON;
    case ax::mojom::Role::kPre:
      return ATK_ROLE_TEXT;
    case ax::mojom::Role::kProgressIndicator:
      return ATK_ROLE_PROGRESS_BAR;
    case ax::mojom::Role::kRadioButton:
      return ATK_ROLE_RADIO_BUTTON;
    case ax::mojom::Role::kRadioGroup:
      return ATK_ROLE_PANEL;
    case ax::mojom::Role::kRegion: {
      std::string html_tag =
          GetData().GetStringAttribute(ax::mojom::StringAttribute::kHtmlTag);
      if (html_tag == "section" &&
          GetData()
              .GetString16Attribute(ax::mojom::StringAttribute::kName)
              .empty()) {
        // Do not use ARIA mapping for nameless <section>.
        return ATK_ROLE_SECTION;
      } else {
        // Use ARIA mapping.
        return ATK_ROLE_LANDMARK;
      }
    }
    case ax::mojom::Role::kRootWebArea:
      return ATK_ROLE_DOCUMENT_WEB;
    case ax::mojom::Role::kRow:
      return ATK_ROLE_TABLE_ROW;
    case ax::mojom::Role::kRowHeader:
      return ATK_ROLE_TABLE_ROW_HEADER;  // ATK_ROLE_ROW_HEADER also exists.
    case ax::mojom::Role::kRuby:
      return ATK_ROLE_TEXT;
    case ax::mojom::Role::kScrollBar:
      return ATK_ROLE_SCROLL_BAR;
    case ax::mojom::Role::kSearch:
      return ATK_ROLE_LANDMARK;
    case ax::mojom::Role::kSlider:
    case ax::mojom::Role::kSliderThumb:
      return ATK_ROLE_SLIDER;
    case ax::mojom::Role::kSpinButton:
      return ATK_ROLE_SPIN_BUTTON;
    case ax::mojom::Role::kSplitter:
      return ATK_ROLE_SEPARATOR;
    case ax::mojom::Role::kStaticText:
      return ATK_ROLE_TEXT;
    case ax::mojom::Role::kStatus:
      return ATK_ROLE_STATUSBAR;
    case ax::mojom::Role::kSvgRoot:
      return ATK_ROLE_IMAGE;
    case ax::mojom::Role::kTab:
      return ATK_ROLE_PAGE_TAB;
    case ax::mojom::Role::kTable:
      return ATK_ROLE_TABLE;
    case ax::mojom::Role::kTableHeaderContainer:
      return ATK_ROLE_PANEL;
    case ax::mojom::Role::kTabList:
      return ATK_ROLE_PAGE_TAB_LIST;
    case ax::mojom::Role::kTabPanel:
      return ATK_ROLE_SCROLL_PANE;
    case ax::mojom::Role::kTerm:
      return ATK_ROLE_DESCRIPTION_TERM;
    case ax::mojom::Role::kTitleBar:
      return ATK_ROLE_TITLE_BAR;
    case ax::mojom::Role::kInlineTextBox:
    case ax::mojom::Role::kLocationBar:
    case ax::mojom::Role::kTextField:
    case ax::mojom::Role::kSearchBox:
      if (!GetStringAttribute(ax::mojom::StringAttribute::kAutoComplete)
               .empty() ||
          IsAutofillField()) {
        return ATK_ROLE_AUTOCOMPLETE;
        ;
      }
      return ATK_ROLE_ENTRY;
    case ax::mojom::Role::kTextFieldWithComboBox:
      return ATK_ROLE_COMBO_BOX;
    case ax::mojom::Role::kAbbr:
    case ax::mojom::Role::kTime:
      return ATK_ROLE_TEXT;
    case ax::mojom::Role::kTimer:
      return ATK_ROLE_TIMER;
    case ax::mojom::Role::kToggleButton:
      return ATK_ROLE_TOGGLE_BUTTON;
    case ax::mojom::Role::kToolbar:
      return ATK_ROLE_TOOL_BAR;
    case ax::mojom::Role::kTooltip:
      return ATK_ROLE_TOOL_TIP;
    case ax::mojom::Role::kTree:
      return ATK_ROLE_TREE;
    case ax::mojom::Role::kTreeItem:
      return ATK_ROLE_TREE_ITEM;
    case ax::mojom::Role::kTreeGrid:
      return ATK_ROLE_TREE_TABLE;
    case ax::mojom::Role::kVideo:
      return ATK_ROLE_VIDEO;
    case ax::mojom::Role::kWebArea:
    case ax::mojom::Role::kWebView:
      return ATK_ROLE_DOCUMENT_WEB;
    case ax::mojom::Role::kWindow:
      return ATK_ROLE_WINDOW;
    case ax::mojom::Role::kClient:
    case ax::mojom::Role::kDesktop:
    case ax::mojom::Role::kFigcaption:
      return ATK_ROLE_PANEL;
    case ax::mojom::Role::kFooter:
      return ATK_ROLE_FOOTER;
    case ax::mojom::Role::kNone:
    case ax::mojom::Role::kPresentational:
    case ax::mojom::Role::kUnknown:
      return ATK_ROLE_REDUNDANT_OBJECT;
  }
}

void AXPlatformNodeAuraLinux::GetAtkState(AtkStateSet* atk_state_set) {
  AXNodeData data = GetData();
  if (data.HasState(ax::mojom::State::kDefault))
    atk_state_set_add_state(atk_state_set, ATK_STATE_DEFAULT);
  if (data.HasState(ax::mojom::State::kEditable))
    atk_state_set_add_state(atk_state_set, ATK_STATE_EDITABLE);
  if (data.HasState(ax::mojom::State::kExpanded))
    atk_state_set_add_state(atk_state_set, ATK_STATE_EXPANDED);
  if (data.HasState(ax::mojom::State::kFocusable))
    atk_state_set_add_state(atk_state_set, ATK_STATE_FOCUSABLE);
#if defined(ATK_216)
  if (data.HasIntAttribute(ax::mojom::IntAttribute::kHasPopup))
    atk_state_set_add_state(atk_state_set, ATK_STATE_HAS_POPUP);
#endif
  if (data.GetBoolAttribute(ax::mojom::BoolAttribute::kSelected))
    atk_state_set_add_state(atk_state_set, ATK_STATE_SELECTED);
  if (data.HasBoolAttribute(ax::mojom::BoolAttribute::kSelected))
    atk_state_set_add_state(atk_state_set, ATK_STATE_SELECTABLE);

  // Checked state
  const auto checked_state = GetData().GetCheckedState();
  switch (checked_state) {
    case ax::mojom::CheckedState::kMixed:
      atk_state_set_add_state(atk_state_set, ATK_STATE_INDETERMINATE);
      break;
    case ax::mojom::CheckedState::kTrue:
      atk_state_set_add_state(atk_state_set,
                              data.role == ax::mojom::Role::kToggleButton
                                  ? ATK_STATE_PRESSED
                                  : ATK_STATE_CHECKED);
      break;
    default:
      break;
  }

  switch (GetData().GetRestriction()) {
    case ax::mojom::Restriction::kNone:
      atk_state_set_add_state(atk_state_set, ATK_STATE_ENABLED);
      break;
    case ax::mojom::Restriction::kReadOnly:
#if defined(ATK_216)
      atk_state_set_add_state(atk_state_set, ATK_STATE_READ_ONLY);
#endif
      break;
    default:
      break;
  }

  if (delegate_->GetFocus() == GetNativeViewAccessible())
    atk_state_set_add_state(atk_state_set, ATK_STATE_FOCUSED);
}

void AXPlatformNodeAuraLinux::GetAtkRelations(AtkRelationSet* atk_relation_set)
{
}

AXPlatformNodeAuraLinux::AXPlatformNodeAuraLinux()
    : interface_mask_(0), atk_object_(nullptr), atk_hyperlink_(nullptr) {}

AXPlatformNodeAuraLinux::~AXPlatformNodeAuraLinux() {
  DestroyAtkObjects();
}

void AXPlatformNodeAuraLinux::Destroy() {
  DestroyAtkObjects();
  AXPlatformNodeBase::Destroy();
}

void AXPlatformNodeAuraLinux::Init(AXPlatformNodeDelegate* delegate) {
  // Initialize ATK.
  AXPlatformNodeBase::Init(delegate);
  DataChanged();
}

void AXPlatformNodeAuraLinux::DataChanged() {
  if (atk_object_) {
    // If the object's role changes and that causes its
    // interface mask to change, we need to create a new
    // AtkObject for it.
    int interface_mask = GetGTypeInterfaceMask();
    if (interface_mask != interface_mask_)
      DestroyAtkObjects();
  }

  if (!atk_object_) {
    atk_object_ = CreateAtkObject();
  }
}

void AXPlatformNodeAuraLinux::AddAccessibilityTreeProperties(
    base::DictionaryValue* dict) {
  AtkRole role = GetAtkRole();
  if (role != ATK_ROLE_UNKNOWN) {
    int role_index = static_cast<int>(role);
    dict->SetString("role", role_names[role_index]);
  }
  const gchar* name = atk_object_get_name(atk_object_);
  if (name)
    dict->SetString("name", std::string(name));
  const gchar* description = atk_object_get_description(atk_object_);
  if (description)
    dict->SetString("description", std::string(description));

  AtkStateSet* state_set = atk_object_ref_state_set(atk_object_);
  auto states = std::make_unique<base::ListValue>();
  for (int i = ATK_STATE_INVALID; i < ATK_STATE_LAST_DEFINED; i++) {
    AtkStateType state_type = static_cast<AtkStateType>(i);
    if (atk_state_set_contains_state(state_set, state_type))
      states->AppendString(atk_state_type_get_name(state_type));
  }
  dict->Set("states", std::move(states));
}

gfx::NativeViewAccessible AXPlatformNodeAuraLinux::GetNativeViewAccessible() {
  return atk_object_;
}

AtkObject* AXPlatformNodeAuraLinux::current_focused_ = nullptr;

void AXPlatformNodeAuraLinux::OnFocused() {
  DCHECK(atk_object_);

  if (atk_object_ == current_focused_)
    return;

  if (current_focused_) {
    g_signal_emit_by_name(current_focused_, "focus-event", false);
    atk_object_notify_state_change(ATK_OBJECT(current_focused_),
                                   ATK_STATE_FOCUSED, false);
  }

  current_focused_ = atk_object_;
  g_signal_emit_by_name(atk_object_, "focus-event", true);
  atk_object_notify_state_change(ATK_OBJECT(atk_object_), ATK_STATE_FOCUSED,
                                 true);
}

void AXPlatformNodeAuraLinux::NotifyAccessibilityEvent(
    ax::mojom::Event event_type) {
  switch (event_type) {
    case ax::mojom::Event::kFocus:
    case ax::mojom::Event::kFocusContext:
      OnFocused();
      break;
    default:
      break;
  }
}

int AXPlatformNodeAuraLinux::GetIndexInParent() {
  if (!GetParent())
    return -1;

  return delegate_->GetIndexInParent();
}

void AXPlatformNodeAuraLinux::SetExtentsRelativeToAtkCoordinateType(
    gint* x, gint* y, gint* width, gint* height, AtkCoordType coord_type) {
  gfx::Rect extents = delegate_->GetUnclippedScreenBoundsRect();

  if (x)
    *x = extents.x();
  if (y)
    *y = extents.y();
  if (width)
    *width = extents.width();
  if (height)
    *height = extents.height();

  if (coord_type == ATK_XY_WINDOW) {
    if (AtkObject* atk_object = GetParent()) {
      gfx::Point window_coords = FindAtkObjectParentCoords(atk_object);
      if (x)
        *x -= window_coords.x();
      if (y)
        *y -= window_coords.y();
    }
  }
}

void AXPlatformNodeAuraLinux::GetExtents(gint* x, gint* y,
                                         gint* width, gint* height,
                                         AtkCoordType coord_type) {
  SetExtentsRelativeToAtkCoordinateType(x, y,
                                        width, height,
                                        coord_type);
}

void AXPlatformNodeAuraLinux::GetPosition(gint* x, gint* y,
                                          AtkCoordType coord_type) {
  SetExtentsRelativeToAtkCoordinateType(x, y,
                                        nullptr, nullptr,
                                        coord_type);
}

void AXPlatformNodeAuraLinux::GetSize(gint* width, gint* height) {
  gfx::Rect rect_size = gfx::ToEnclosingRect(GetData().location);
  if (width)
    *width = rect_size.width();
  if (height)
    *height = rect_size.height();
}

gfx::NativeViewAccessible
AXPlatformNodeAuraLinux::HitTestSync(gint x, gint y, AtkCoordType coord_type) {
  if (coord_type == ATK_XY_WINDOW) {
    if (AtkObject* atk_object = GetParent()) {
      gfx::Point window_coords = FindAtkObjectParentCoords(atk_object);
      x += window_coords.x();
      y += window_coords.y();
    }
  }

  return delegate_->HitTestSync(x, y);
}

bool AXPlatformNodeAuraLinux::GrabFocus() {
  AXActionData action_data;
  action_data.action = ax::mojom::Action::kFocus;
  return delegate_->AccessibilityPerformAction(action_data);
}

bool AXPlatformNodeAuraLinux::DoDefaultAction() {
  AXActionData action_data;
  action_data.action = ax::mojom::Action::kDoDefault;
  return delegate_->AccessibilityPerformAction(action_data);
}

const gchar* AXPlatformNodeAuraLinux::GetDefaultActionName() {
  int action;
  if (!GetIntAttribute(ax::mojom::IntAttribute::kDefaultActionVerb, &action))
    return nullptr;

  base::string16 action_verb = ui::ActionVerbToUnlocalizedString(
      static_cast<ax::mojom::DefaultActionVerb>(action));

  ATK_AURALINUX_RETURN_STRING(base::UTF16ToUTF8(action_verb));
}

// AtkDocumentHelpers

const gchar* AXPlatformNodeAuraLinux::GetDocumentAttributeValue(
    const gchar* attribute) const {
  if (!g_ascii_strcasecmp(attribute, "DocType"))
    return delegate_->GetTreeData().doctype.c_str();
  else if (!g_ascii_strcasecmp(attribute, "MimeType"))
    return delegate_->GetTreeData().mimetype.c_str();
  else if (!g_ascii_strcasecmp(attribute, "Title"))
    return delegate_->GetTreeData().title.c_str();
  else if (!g_ascii_strcasecmp(attribute, "URI"))
    return delegate_->GetTreeData().url.c_str();

  return nullptr;
}

AtkAttributeSet* AXPlatformNodeAuraLinux::GetDocumentAttributes() const {
  AtkAttributeSet* attribute_set = nullptr;
  const gchar* doc_attributes[] = {"DocType", "MimeType", "Title", "URI"};
  const gchar* value = nullptr;

  for (unsigned i = 0; i < G_N_ELEMENTS(doc_attributes); i++) {
    value = GetDocumentAttributeValue(doc_attributes[i]);
    if (value) {
      AtkAttribute* attribute =
          static_cast<AtkAttribute*>(g_malloc(sizeof(AtkAttribute)));
      attribute->name = g_strdup(doc_attributes[i]);
      attribute->value = g_strdup(value);
      attribute_set = g_slist_prepend(attribute_set, attribute);
    }
  }

  return attribute_set;
}

//
// AtkHyperlink helpers
//

AtkHyperlink* AXPlatformNodeAuraLinux::GetAtkHyperlink() {
  DCHECK(ATK_HYPERLINK_IMPL(atk_object_));
  g_return_val_if_fail(ATK_HYPERLINK_IMPL(atk_object_), 0);

  if (!atk_hyperlink_) {
    atk_hyperlink_ =
        ATK_HYPERLINK(g_object_new(AX_PLATFORM_ATK_HYPERLINK_TYPE, 0));
    ax_platform_atk_hyperlink_set_object(
        AX_PLATFORM_ATK_HYPERLINK(atk_hyperlink_), this);
  }

  return atk_hyperlink_;
}

//
// Misc helpers
//

void AXPlatformNodeAuraLinux::GetFloatAttributeInGValue(
    ax::mojom::FloatAttribute attr,
    GValue* value) {
  float float_val;
  if (GetFloatAttribute(attr, &float_val)) {
    memset(value, 0, sizeof(*value));
    g_value_init(value, G_TYPE_FLOAT);
    g_value_set_float(value, float_val);
  }
}

}  // namespace ui
