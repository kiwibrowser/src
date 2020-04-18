// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/tabs/tab_menu_model.h"

#include "base/command_line.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/tabs/tab_strip_model_delegate.h"
#include "chrome/browser/ui/tabs/tab_utils.h"
#include "chrome/common/chrome_features.h"
#include "chrome/grit/generated_resources.h"

TabMenuModel::TabMenuModel(ui::SimpleMenuModel::Delegate* delegate,
                           TabStripModel* tab_strip,
                           int index)
    : ui::SimpleMenuModel(delegate) {
  Build(tab_strip, index);
}

void TabMenuModel::Build(TabStripModel* tab_strip, int index) {
  bool affects_multiple_tabs =
      (tab_strip->IsTabSelected(index) &&
       tab_strip->selection_model().selected_indices().size() > 1);
  AddItemWithStringId(TabStripModel::CommandNewTab, IDS_TAB_CXMENU_NEWTAB);
  AddSeparator(ui::NORMAL_SEPARATOR);
  AddItemWithStringId(TabStripModel::CommandReload, IDS_TAB_CXMENU_RELOAD);
  AddItemWithStringId(TabStripModel::CommandDuplicate,
                      IDS_TAB_CXMENU_DUPLICATE);
  bool will_pin = tab_strip->WillContextMenuPin(index);
  if (affects_multiple_tabs) {
    AddItemWithStringId(
        TabStripModel::CommandTogglePinned,
        will_pin ? IDS_TAB_CXMENU_PIN_TABS : IDS_TAB_CXMENU_UNPIN_TABS);
  } else {
    AddItemWithStringId(
        TabStripModel::CommandTogglePinned,
        will_pin ? IDS_TAB_CXMENU_PIN_TAB : IDS_TAB_CXMENU_UNPIN_TAB);
  }
  if (base::FeatureList::IsEnabled(features::kSoundContentSetting)) {
    if (affects_multiple_tabs) {
      const bool will_mute = !chrome::AreAllSitesMuted(
          *tab_strip, tab_strip->selection_model().selected_indices());
      AddItemWithStringId(TabStripModel::CommandToggleSiteMuted,
                          will_mute ? IDS_TAB_CXMENU_SOUND_MUTE_SITES
                                    : IDS_TAB_CXMENU_SOUND_UNMUTE_SITES);
    } else {
      const bool will_mute = !chrome::IsSiteMuted(*tab_strip, index);
      AddItemWithStringId(TabStripModel::CommandToggleSiteMuted,
                          will_mute ? IDS_TAB_CXMENU_SOUND_MUTE_SITE
                                    : IDS_TAB_CXMENU_SOUND_UNMUTE_SITE);
    }
  } else {
    if (affects_multiple_tabs) {
      const bool will_mute = !chrome::AreAllTabsMuted(
          *tab_strip, tab_strip->selection_model().selected_indices());
      AddItemWithStringId(TabStripModel::CommandToggleTabAudioMuted,
                          will_mute ? IDS_TAB_CXMENU_AUDIO_MUTE_TABS
                                    : IDS_TAB_CXMENU_AUDIO_UNMUTE_TABS);
    } else {
      const bool will_mute =
          !tab_strip->GetWebContentsAt(index)->IsAudioMuted();
      AddItemWithStringId(TabStripModel::CommandToggleTabAudioMuted,
                          will_mute ? IDS_TAB_CXMENU_AUDIO_MUTE_TAB
                                    : IDS_TAB_CXMENU_AUDIO_UNMUTE_TAB);
    }
  }
  AddSeparator(ui::NORMAL_SEPARATOR);
  if (affects_multiple_tabs) {
    AddItemWithStringId(TabStripModel::CommandCloseTab,
                        IDS_TAB_CXMENU_CLOSETABS);
  } else {
    AddItemWithStringId(TabStripModel::CommandCloseTab,
                        IDS_TAB_CXMENU_CLOSETAB);
  }
  AddItemWithStringId(TabStripModel::CommandCloseOtherTabs,
                      IDS_TAB_CXMENU_CLOSEOTHERTABS);
  AddItemWithStringId(TabStripModel::CommandCloseTabsToRight,
                      IDS_TAB_CXMENU_CLOSETABSTORIGHT);
  AddSeparator(ui::NORMAL_SEPARATOR);
  const bool is_window = tab_strip->delegate()->GetRestoreTabType() ==
      TabStripModelDelegate::RESTORE_WINDOW;
  AddItemWithStringId(TabStripModel::CommandRestoreTab,
                      is_window ? IDS_RESTORE_WINDOW : IDS_RESTORE_TAB);
  AddItemWithStringId(TabStripModel::CommandBookmarkAllTabs,
                      IDS_TAB_CXMENU_BOOKMARK_ALL_TABS);
}
