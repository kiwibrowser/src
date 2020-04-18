// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_FRAME_GLOBAL_MENU_BAR_X11_H_
#define CHROME_BROWSER_UI_VIEWS_FRAME_GLOBAL_MENU_BAR_X11_H_

#include <map>
#include <string>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_observer.h"
#include "chrome/browser/command_observer.h"
#include "chrome/browser/profiles/avatar_menu.h"
#include "chrome/browser/profiles/avatar_menu_observer.h"
#include "chrome/browser/ui/browser_list_observer.h"
#include "components/history/core/browser/history_types.h"
#include "components/history/core/browser/top_sites_observer.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/sessions/core/tab_restore_service.h"
#include "components/sessions/core/tab_restore_service_observer.h"
#include "ui/base/glib/glib_signal.h"
#include "ui/views/widget/desktop_aura/desktop_window_tree_host_observer_x11.h"

typedef struct _DbusmenuMenuitem DbusmenuMenuitem;
typedef struct _DbusmenuServer   DbusmenuServer;

namespace history {
class TopSites;
}

namespace ui {
class Accelerator;
}

class Browser;
class BrowserView;
class Profile;

class BrowserDesktopWindowTreeHostX11;
struct GlobalMenuBarCommand;

// Controls the Mac style menu bar on Unity.
//
// Unity has an Apple-like menu bar at the top of the screen that changes
// depending on the active window. In the GTK port, we had a hidden GtkMenuBar
// object in each GtkWindow which existed only to be scrapped by the
// libdbusmenu-gtk code. Since we don't have GtkWindows anymore, we need to
// interface directly with the lower level libdbusmenu-glib, which we
// opportunistically dlopen() since not everyone is running Ubuntu.
class GlobalMenuBarX11 : public AvatarMenuObserver,
                         public BrowserListObserver,
                         public CommandObserver,
                         public history::TopSitesObserver,
                         public sessions::TabRestoreServiceObserver,
                         public views::DesktopWindowTreeHostObserverX11 {
 public:
  GlobalMenuBarX11(BrowserView* browser_view,
                   BrowserDesktopWindowTreeHostX11* host);
  ~GlobalMenuBarX11() override;

  // Creates the object path for DbusemenuServer which is attached to |xid|.
  static std::string GetPathForWindow(unsigned long xid);

 private:
  struct HistoryItem;
  typedef std::map<int, DbusmenuMenuitem*> CommandIDMenuItemMap;

  // Builds a separator.
  DbusmenuMenuitem* BuildSeparator();

  // Creates an individual menu item from a title and command, and subscribes
  // to the activation signal.
  DbusmenuMenuitem* BuildMenuItem(const std::string& label, int tag_id);

  // Creates a DbusmenuServer, and attaches all the menu items.
  void InitServer(unsigned long xid);

  // Stops listening to enable state changed events.
  void Disable();

  // Creates a whole menu defined with |commands| and titled with the string
  // |menu_str_id|. Then appends it to |parent|.
  DbusmenuMenuitem* BuildStaticMenu(DbusmenuMenuitem* parent,
                                    int menu_str_id,
                                    GlobalMenuBarCommand* commands);

  // Sets the accelerator for |item|.
  void RegisterAccelerator(DbusmenuMenuitem* item,
                           const ui::Accelerator& accelerator);

  // Creates a HistoryItem from the data in |entry|.
  HistoryItem* HistoryItemForTab(const sessions::TabRestoreService::Tab& entry);

  // Creates a menu item form |item| and inserts it in |menu| at |index|.
  void AddHistoryItemToMenu(HistoryItem* item,
                            DbusmenuMenuitem* menu,
                            int tag,
                            int index);

  // Sends a message off to History for data.
  void GetTopSitesData();

  // Callback to receive data requested from GetTopSitesData().
  void OnTopSitesReceived(const history::MostVisitedURLList& visited_list);

  // Updates the visibility of the bookmark bar on pref changes.
  void OnBookmarkBarVisibilityChanged();

  void RebuildProfilesMenu();

  // Find the first index of the item in |menu| with the tag |tag_id|.
  int GetIndexOfMenuItemWithTag(DbusmenuMenuitem* menu, int tag_id);

  // This will remove all menu items in |menu| with |tag| as their tag. This
  // clears state about HistoryItems* that we keep to prevent that data from
  // going stale. That's why this method recurses into its child menus.
  void ClearMenuSection(DbusmenuMenuitem* menu, int tag_id);

  // Deleter function for HistoryItem implementation detail.
  static void DeleteHistoryItem(void* void_item);

  // Overridden from AvatarMenuObserver:
  void OnAvatarMenuChanged(AvatarMenu* avatar_menu) override;

  // Overridden from BrowserListObserver:
  void OnBrowserSetLastActive(Browser* browser) override;

  // Overridden from CommandObserver:
  void EnabledStateChangedForCommand(int id, bool enabled) override;

  // Overridden from history::TopSitesObserver:
  void TopSitesLoaded(history::TopSites* top_sites) override;
  void TopSitesChanged(history::TopSites* top_sites,
                       ChangeReason change_reason) override;

  // Overridden from TabRestoreServiceObserver:
  void TabRestoreServiceChanged(sessions::TabRestoreService* service) override;
  void TabRestoreServiceDestroyed(
      sessions::TabRestoreService* service) override;

  // Overridden from views::DesktopWindowTreeHostObserverX11:
  void OnWindowMapped(unsigned long xid) override;
  void OnWindowUnmapped(unsigned long xid) override;

  CHROMEG_CALLBACK_1(GlobalMenuBarX11, void, OnItemActivated, DbusmenuMenuitem*,
                     unsigned int);
  CHROMEG_CALLBACK_1(GlobalMenuBarX11, void, OnHistoryItemActivated,
                     DbusmenuMenuitem*, unsigned int);
  CHROMEG_CALLBACK_0(GlobalMenuBarX11, void, OnHistoryMenuAboutToShow,
                     DbusmenuMenuitem*);
  CHROMEG_CALLBACK_1(GlobalMenuBarX11, void, OnProfileItemActivated,
                     DbusmenuMenuitem*, unsigned int);
  CHROMEG_CALLBACK_1(GlobalMenuBarX11, void, OnEditProfileItemActivated,
                     DbusmenuMenuitem*, unsigned int);
  CHROMEG_CALLBACK_1(GlobalMenuBarX11, void, OnCreateProfileItemActivated,
                     DbusmenuMenuitem*, unsigned int);

  Browser* const browser_;
  Profile* profile_;
  BrowserView* browser_view_;
  BrowserDesktopWindowTreeHostX11* host_;

  // Maps command ids to DbusmenuMenuitems so we can modify their
  // enabled/checked state in response to state change notifications.
  CommandIDMenuItemMap id_to_menu_item_;

  DbusmenuServer* server_;
  DbusmenuMenuitem* root_item_;
  DbusmenuMenuitem* history_menu_;
  DbusmenuMenuitem* profiles_menu_;

  // Tracks value of the kShowBookmarkBar preference.
  PrefChangeRegistrar pref_change_registrar_;

  scoped_refptr<history::TopSites> top_sites_;

  sessions::TabRestoreService* tab_restore_service_;  // weak

  std::unique_ptr<AvatarMenu> avatar_menu_;

  ScopedObserver<history::TopSites, history::TopSitesObserver> scoped_observer_;

  // For callbacks may be run after destruction.
  base::WeakPtrFactory<GlobalMenuBarX11> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(GlobalMenuBarX11);
};

#endif  // CHROME_BROWSER_UI_VIEWS_FRAME_GLOBAL_MENU_BAR_X11_H_
