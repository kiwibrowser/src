// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_COCOA_TEXT_SERVICES_CONTEXT_MENU_H_
#define UI_BASE_COCOA_TEXT_SERVICES_CONTEXT_MENU_H_

#include "base/i18n/rtl.h"
#include "base/macros.h"
#include "base/strings/string16.h"
#include "ui/base/models/simple_menu_model.h"
#include "ui/base/ui_base_export.h"

namespace ui {

// This class is used to append and handle the Speech and BiDi submenu for the
// context menu.
class UI_BASE_EXPORT TextServicesContextMenu
    : public SimpleMenuModel::Delegate {
 public:
  class UI_BASE_EXPORT Delegate {
   public:
    // Returns the selected text.
    virtual base::string16 GetSelectedText() const = 0;

    // Returns true if |direction| should be enabled in the BiDi submenu.
    virtual bool IsTextDirectionEnabled(
        base::i18n::TextDirection direction) const = 0;

    // Returns true if |direction| should be checked in the BiDi submenu.
    virtual bool IsTextDirectionChecked(
        base::i18n::TextDirection direction) const = 0;

    // Updates the text direction to |direction|.
    virtual void UpdateTextDirection(base::i18n::TextDirection direction) = 0;
  };

  explicit TextServicesContextMenu(Delegate* delegate);

  // Methods for speaking.
  static void SpeakText(const base::string16& text);
  static void StopSpeaking();
  static bool IsSpeaking();

  // Appends text service items to |model|. A submenu is added for speech,
  // which |this| serves as the delegate for.
  void AppendToContextMenu(SimpleMenuModel* model);

  // Appends items to the context menu applicable to editable content. A
  // submenu is added for bidirection, which |this| serves as a delegate for.
  void AppendEditableItems(SimpleMenuModel* model);

  // Returns true if |command_id| is handled by this class.
  bool SupportsCommand(int command_id) const;

  // SimpleMenuModel::Delegate:
  bool IsCommandIdChecked(int command_id) const override;
  bool IsCommandIdEnabled(int command_id) const override;
  void ExecuteCommand(int command_id, int event_flags) override;

 private:
  // Model for the "Speech" submenu.
  ui::SimpleMenuModel speech_submenu_model_;

  // Model for the BiDi input submenu.
  ui::SimpleMenuModel bidi_submenu_model_;

  Delegate* delegate_;  // Weak.

  DISALLOW_COPY_AND_ASSIGN(TextServicesContextMenu);
};

}  // namespace ui

#endif  // UI_BASE_COCOA_TEXT_SERVICES_CONTEXT_MENU_H_