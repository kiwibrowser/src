// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/accessibility/accessibility_tree_formatter_utils_win.h"

#include <oleacc.h>

#include <map>
#include <string>

#include "base/macros.h"
#include "base/memory/singleton.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "third_party/iaccessible2/ia2_api_all.h"

namespace content {
namespace {
struct PlatformConstantToNameEntry {
  int32_t value;
  const char* name;
};

base::string16 GetNameForPlatformConstant(
    const PlatformConstantToNameEntry table[],
    size_t table_size,
    int32_t value) {
  for (size_t i = 0; i < table_size; ++i) {
    auto& entry = table[i];
    if (entry.value == value)
      return base::ASCIIToUTF16(entry.name);
  }
  return L"";
}
}

#define QUOTE(X) \
  { X, #X }

CONTENT_EXPORT base::string16 IAccessibleRoleToString(int32_t ia_role) {
  // MSAA / IAccessible roles. Each one of these is also a valid
  // IAccessible2 role.
  static const PlatformConstantToNameEntry ia_table[] = {
      QUOTE(ROLE_SYSTEM_ALERT),          QUOTE(ROLE_SYSTEM_ANIMATION),
      QUOTE(ROLE_SYSTEM_APPLICATION),    QUOTE(ROLE_SYSTEM_BORDER),
      QUOTE(ROLE_SYSTEM_BUTTONDROPDOWN), QUOTE(ROLE_SYSTEM_BUTTONDROPDOWNGRID),
      QUOTE(ROLE_SYSTEM_BUTTONMENU),     QUOTE(ROLE_SYSTEM_CARET),
      QUOTE(ROLE_SYSTEM_CELL),           QUOTE(ROLE_SYSTEM_CHARACTER),
      QUOTE(ROLE_SYSTEM_CHART),          QUOTE(ROLE_SYSTEM_CHECKBUTTON),
      QUOTE(ROLE_SYSTEM_CLIENT),         QUOTE(ROLE_SYSTEM_CLOCK),
      QUOTE(ROLE_SYSTEM_COLUMN),         QUOTE(ROLE_SYSTEM_COLUMNHEADER),
      QUOTE(ROLE_SYSTEM_COMBOBOX),       QUOTE(ROLE_SYSTEM_CURSOR),
      QUOTE(ROLE_SYSTEM_DIAGRAM),        QUOTE(ROLE_SYSTEM_DIAL),
      QUOTE(ROLE_SYSTEM_DIALOG),         QUOTE(ROLE_SYSTEM_DOCUMENT),
      QUOTE(ROLE_SYSTEM_DROPLIST),       QUOTE(ROLE_SYSTEM_EQUATION),
      QUOTE(ROLE_SYSTEM_GRAPHIC),        QUOTE(ROLE_SYSTEM_GRIP),
      QUOTE(ROLE_SYSTEM_GROUPING),       QUOTE(ROLE_SYSTEM_HELPBALLOON),
      QUOTE(ROLE_SYSTEM_HOTKEYFIELD),    QUOTE(ROLE_SYSTEM_INDICATOR),
      QUOTE(ROLE_SYSTEM_IPADDRESS),      QUOTE(ROLE_SYSTEM_LINK),
      QUOTE(ROLE_SYSTEM_LIST),           QUOTE(ROLE_SYSTEM_LISTITEM),
      QUOTE(ROLE_SYSTEM_MENUBAR),        QUOTE(ROLE_SYSTEM_MENUITEM),
      QUOTE(ROLE_SYSTEM_MENUPOPUP),      QUOTE(ROLE_SYSTEM_OUTLINE),
      QUOTE(ROLE_SYSTEM_OUTLINEBUTTON),  QUOTE(ROLE_SYSTEM_OUTLINEITEM),
      QUOTE(ROLE_SYSTEM_PAGETAB),        QUOTE(ROLE_SYSTEM_PAGETABLIST),
      QUOTE(ROLE_SYSTEM_PANE),           QUOTE(ROLE_SYSTEM_PROGRESSBAR),
      QUOTE(ROLE_SYSTEM_PROPERTYPAGE),   QUOTE(ROLE_SYSTEM_PUSHBUTTON),
      QUOTE(ROLE_SYSTEM_RADIOBUTTON),    QUOTE(ROLE_SYSTEM_ROW),
      QUOTE(ROLE_SYSTEM_ROWHEADER),      QUOTE(ROLE_SYSTEM_SCROLLBAR),
      QUOTE(ROLE_SYSTEM_SEPARATOR),      QUOTE(ROLE_SYSTEM_SLIDER),
      QUOTE(ROLE_SYSTEM_SOUND),          QUOTE(ROLE_SYSTEM_SPINBUTTON),
      QUOTE(ROLE_SYSTEM_SPLITBUTTON),    QUOTE(ROLE_SYSTEM_STATICTEXT),
      QUOTE(ROLE_SYSTEM_STATUSBAR),      QUOTE(ROLE_SYSTEM_TABLE),
      QUOTE(ROLE_SYSTEM_TEXT),           QUOTE(ROLE_SYSTEM_TITLEBAR),
      QUOTE(ROLE_SYSTEM_TOOLBAR),        QUOTE(ROLE_SYSTEM_TOOLTIP),
      QUOTE(ROLE_SYSTEM_WHITESPACE),     QUOTE(ROLE_SYSTEM_WINDOW),
  };

  return GetNameForPlatformConstant(ia_table, arraysize(ia_table), ia_role);
}

CONTENT_EXPORT base::string16 IAccessible2RoleToString(int32_t ia2_role) {
  base::string16 result = IAccessibleRoleToString(ia2_role);
  if (!result.empty())
    return result;

  static const PlatformConstantToNameEntry ia2_table[] = {
      QUOTE(IA2_ROLE_CANVAS),
      QUOTE(IA2_ROLE_CAPTION),
      QUOTE(IA2_ROLE_CHECK_MENU_ITEM),
      QUOTE(IA2_ROLE_COLOR_CHOOSER),
      QUOTE(IA2_ROLE_DATE_EDITOR),
      QUOTE(IA2_ROLE_DESKTOP_ICON),
      QUOTE(IA2_ROLE_DESKTOP_PANE),
      QUOTE(IA2_ROLE_DIRECTORY_PANE),
      QUOTE(IA2_ROLE_EDITBAR),
      QUOTE(IA2_ROLE_EMBEDDED_OBJECT),
      QUOTE(IA2_ROLE_ENDNOTE),
      QUOTE(IA2_ROLE_FILE_CHOOSER),
      QUOTE(IA2_ROLE_FONT_CHOOSER),
      QUOTE(IA2_ROLE_FOOTER),
      QUOTE(IA2_ROLE_FOOTNOTE),
      QUOTE(IA2_ROLE_FORM),
      QUOTE(IA2_ROLE_FRAME),
      QUOTE(IA2_ROLE_GLASS_PANE),
      QUOTE(IA2_ROLE_HEADER),
      QUOTE(IA2_ROLE_HEADING),
      QUOTE(IA2_ROLE_ICON),
      QUOTE(IA2_ROLE_IMAGE_MAP),
      QUOTE(IA2_ROLE_INPUT_METHOD_WINDOW),
      QUOTE(IA2_ROLE_INTERNAL_FRAME),
      QUOTE(IA2_ROLE_LABEL),
      QUOTE(IA2_ROLE_LAYERED_PANE),
      QUOTE(IA2_ROLE_NOTE),
      QUOTE(IA2_ROLE_OPTION_PANE),
      QUOTE(IA2_ROLE_PAGE),
      QUOTE(IA2_ROLE_PARAGRAPH),
      QUOTE(IA2_ROLE_RADIO_MENU_ITEM),
      QUOTE(IA2_ROLE_REDUNDANT_OBJECT),
      QUOTE(IA2_ROLE_ROOT_PANE),
      QUOTE(IA2_ROLE_RULER),
      QUOTE(IA2_ROLE_SCROLL_PANE),
      QUOTE(IA2_ROLE_SECTION),
      QUOTE(IA2_ROLE_SHAPE),
      QUOTE(IA2_ROLE_SPLIT_PANE),
      QUOTE(IA2_ROLE_TEAR_OFF_MENU),
      QUOTE(IA2_ROLE_TERMINAL),
      QUOTE(IA2_ROLE_TEXT_FRAME),
      QUOTE(IA2_ROLE_TOGGLE_BUTTON),
      QUOTE(IA2_ROLE_UNKNOWN),
      QUOTE(IA2_ROLE_VIEW_PORT),
      QUOTE(IA2_ROLE_COMPLEMENTARY_CONTENT),
      QUOTE(IA2_ROLE_LANDMARK),
      QUOTE(IA2_ROLE_LEVEL_BAR)};

  return GetNameForPlatformConstant(ia2_table, arraysize(ia2_table), ia2_role);
}

CONTENT_EXPORT base::string16 AccessibilityEventToString(int32_t event) {
  static const PlatformConstantToNameEntry event_table[] = {
      QUOTE(EVENT_OBJECT_CREATE),
      QUOTE(EVENT_OBJECT_DESTROY),
      QUOTE(EVENT_OBJECT_SHOW),
      QUOTE(EVENT_OBJECT_HIDE),
      QUOTE(EVENT_OBJECT_REORDER),
      QUOTE(EVENT_OBJECT_FOCUS),
      QUOTE(EVENT_OBJECT_SELECTION),
      QUOTE(EVENT_OBJECT_SELECTIONADD),
      QUOTE(EVENT_OBJECT_SELECTIONREMOVE),
      QUOTE(EVENT_OBJECT_SELECTIONWITHIN),
      QUOTE(EVENT_OBJECT_STATECHANGE),
      QUOTE(EVENT_OBJECT_LOCATIONCHANGE),
      QUOTE(EVENT_OBJECT_NAMECHANGE),
      QUOTE(EVENT_OBJECT_DESCRIPTIONCHANGE),
      QUOTE(EVENT_OBJECT_VALUECHANGE),
      QUOTE(EVENT_OBJECT_PARENTCHANGE),
      QUOTE(EVENT_OBJECT_HELPCHANGE),
      QUOTE(EVENT_OBJECT_DEFACTIONCHANGE),
      QUOTE(EVENT_OBJECT_ACCELERATORCHANGE),
      QUOTE(EVENT_OBJECT_INVOKED),
      QUOTE(EVENT_OBJECT_TEXTSELECTIONCHANGED),
      QUOTE(EVENT_OBJECT_CONTENTSCROLLED),
      QUOTE(EVENT_OBJECT_LIVEREGIONCHANGED),
      QUOTE(EVENT_OBJECT_HOSTEDOBJECTSINVALIDATED),
      QUOTE(EVENT_OBJECT_DRAGSTART),
      QUOTE(EVENT_OBJECT_DRAGCANCEL),
      QUOTE(EVENT_OBJECT_DRAGCOMPLETE),
      QUOTE(EVENT_OBJECT_DRAGENTER),
      QUOTE(EVENT_OBJECT_DRAGLEAVE),
      QUOTE(EVENT_OBJECT_DRAGDROPPED),
      QUOTE(EVENT_SYSTEM_ALERT),
      QUOTE(EVENT_SYSTEM_SCROLLINGSTART),
      QUOTE(EVENT_SYSTEM_SCROLLINGEND),
      QUOTE(IA2_EVENT_ACTION_CHANGED),
      QUOTE(IA2_EVENT_ACTIVE_DESCENDANT_CHANGED),
      QUOTE(IA2_EVENT_DOCUMENT_ATTRIBUTE_CHANGED),
      QUOTE(IA2_EVENT_DOCUMENT_CONTENT_CHANGED),
      QUOTE(IA2_EVENT_DOCUMENT_LOAD_COMPLETE),
      QUOTE(IA2_EVENT_DOCUMENT_LOAD_STOPPED),
      QUOTE(IA2_EVENT_DOCUMENT_RELOAD),
      QUOTE(IA2_EVENT_HYPERLINK_END_INDEX_CHANGED),
      QUOTE(IA2_EVENT_HYPERLINK_NUMBER_OF_ANCHORS_CHANGED),
      QUOTE(IA2_EVENT_HYPERLINK_SELECTED_LINK_CHANGED),
      QUOTE(IA2_EVENT_HYPERTEXT_LINK_ACTIVATED),
      QUOTE(IA2_EVENT_HYPERTEXT_LINK_SELECTED),
      QUOTE(IA2_EVENT_HYPERLINK_START_INDEX_CHANGED),
      QUOTE(IA2_EVENT_HYPERTEXT_CHANGED),
      QUOTE(IA2_EVENT_HYPERTEXT_NLINKS_CHANGED),
      QUOTE(IA2_EVENT_OBJECT_ATTRIBUTE_CHANGED),
      QUOTE(IA2_EVENT_PAGE_CHANGED),
      QUOTE(IA2_EVENT_SECTION_CHANGED),
      QUOTE(IA2_EVENT_TABLE_CAPTION_CHANGED),
      QUOTE(IA2_EVENT_TABLE_COLUMN_DESCRIPTION_CHANGED),
      QUOTE(IA2_EVENT_TABLE_COLUMN_HEADER_CHANGED),
      QUOTE(IA2_EVENT_TABLE_MODEL_CHANGED),
      QUOTE(IA2_EVENT_TABLE_ROW_DESCRIPTION_CHANGED),
      QUOTE(IA2_EVENT_TABLE_ROW_HEADER_CHANGED),
      QUOTE(IA2_EVENT_TABLE_SUMMARY_CHANGED),
      QUOTE(IA2_EVENT_TEXT_ATTRIBUTE_CHANGED),
      QUOTE(IA2_EVENT_TEXT_CARET_MOVED),
      QUOTE(IA2_EVENT_TEXT_CHANGED),
      QUOTE(IA2_EVENT_TEXT_COLUMN_CHANGED),
      QUOTE(IA2_EVENT_TEXT_INSERTED),
      QUOTE(IA2_EVENT_TEXT_REMOVED),
      QUOTE(IA2_EVENT_TEXT_UPDATED),
      QUOTE(IA2_EVENT_TEXT_SELECTION_CHANGED),
      QUOTE(IA2_EVENT_VISIBLE_DATA_CHANGED),
  };

  return GetNameForPlatformConstant(event_table, arraysize(event_table), event);
}

void IAccessibleStateToStringVector(int32_t ia_state,
                                    std::vector<base::string16>* result) {
#define QUOTE_STATE(X) \
  { STATE_SYSTEM_##X, #X }
  // MSAA / IAccessible states. Unlike roles, these are not also IA2 states.
  //
  // Note: for historical reasons these are in numerical order rather than
  // alphabetical order. Changing the order would change the order in which
  // the states are printed, which would affect a bunch of tests.
  static const PlatformConstantToNameEntry ia_table[] = {
      QUOTE_STATE(UNAVAILABLE),     QUOTE_STATE(SELECTED),
      QUOTE_STATE(FOCUSED),         QUOTE_STATE(PRESSED),
      QUOTE_STATE(CHECKED),         QUOTE_STATE(MIXED),
      QUOTE_STATE(READONLY),        QUOTE_STATE(HOTTRACKED),
      QUOTE_STATE(DEFAULT),         QUOTE_STATE(EXPANDED),
      QUOTE_STATE(COLLAPSED),       QUOTE_STATE(BUSY),
      QUOTE_STATE(FLOATING),        QUOTE_STATE(MARQUEED),
      QUOTE_STATE(ANIMATED),        QUOTE_STATE(INVISIBLE),
      QUOTE_STATE(OFFSCREEN),       QUOTE_STATE(SIZEABLE),
      QUOTE_STATE(MOVEABLE),        QUOTE_STATE(SELFVOICING),
      QUOTE_STATE(FOCUSABLE),       QUOTE_STATE(SELECTABLE),
      QUOTE_STATE(LINKED),          QUOTE_STATE(TRAVERSED),
      QUOTE_STATE(MULTISELECTABLE), QUOTE_STATE(EXTSELECTABLE),
      QUOTE_STATE(ALERT_LOW),       QUOTE_STATE(ALERT_MEDIUM),
      QUOTE_STATE(ALERT_HIGH),      QUOTE_STATE(PROTECTED),
      QUOTE_STATE(HASPOPUP),
  };
  for (auto& entry : ia_table) {
    if (entry.value & ia_state)
      result->push_back(base::ASCIIToUTF16(entry.name));
  }
}

base::string16 IAccessibleStateToString(int32_t ia_state) {
  std::vector<base::string16> strings;
  IAccessibleStateToStringVector(ia_state, &strings);
  return base::JoinString(strings, base::ASCIIToUTF16(","));
}

void IAccessible2StateToStringVector(int32_t ia2_state,
                                     std::vector<base::string16>* result) {
  // Note: for historical reasons these are in numerical order rather than
  // alphabetical order. Changing the order would change the order in which
  // the states are printed, which would affect a bunch of tests.
  static const PlatformConstantToNameEntry ia2_table[] = {
      QUOTE(IA2_STATE_ACTIVE),
      QUOTE(IA2_STATE_ARMED),
      QUOTE(IA2_STATE_CHECKABLE),
      QUOTE(IA2_STATE_DEFUNCT),
      QUOTE(IA2_STATE_EDITABLE),
      QUOTE(IA2_STATE_HORIZONTAL),
      QUOTE(IA2_STATE_ICONIFIED),
      QUOTE(IA2_STATE_INVALID_ENTRY),
      QUOTE(IA2_STATE_MANAGES_DESCENDANTS),
      QUOTE(IA2_STATE_MODAL),
      QUOTE(IA2_STATE_MULTI_LINE),
      QUOTE(IA2_STATE_REQUIRED),
      QUOTE(IA2_STATE_SELECTABLE_TEXT),
      QUOTE(IA2_STATE_SINGLE_LINE),
      QUOTE(IA2_STATE_STALE),
      QUOTE(IA2_STATE_SUPPORTS_AUTOCOMPLETION),
      QUOTE(IA2_STATE_TRANSIENT),
      QUOTE(IA2_STATE_VERTICAL),
      // Untested states include those that would be repeated on nearly
      // every node or would vary based on window size.
      // QUOTE(IA2_STATE_OPAQUE) // Untested.
  };

  for (auto& entry : ia2_table) {
    if (entry.value & ia2_state)
      result->push_back(base::ASCIIToUTF16(entry.name));
  }
}

base::string16 IAccessible2StateToString(int32_t ia2_state) {
  std::vector<base::string16> strings;
  IAccessible2StateToStringVector(ia2_state, &strings);
  return base::JoinString(strings, base::ASCIIToUTF16(","));
}

}  // namespace content
