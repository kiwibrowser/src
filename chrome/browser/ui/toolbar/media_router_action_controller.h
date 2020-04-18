// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_TOOLBAR_MEDIA_ROUTER_ACTION_CONTROLLER_H_
#define CHROME_BROWSER_UI_TOOLBAR_MEDIA_ROUTER_ACTION_CONTROLLER_H_

#include <vector>

#include "chrome/browser/media/router/issues_observer.h"
#include "chrome/browser/media/router/media_routes_observer.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_change_registrar.h"

class ComponentActionDelegate;

// Controller for MediaRouterAction that determines when to show and hide the
// action icon on the toolbar. There should be one instance of this class per
// profile, and it should only be used on the UI thread.
class MediaRouterActionController : public media_router::IssuesObserver,
                                    public media_router::MediaRoutesObserver {
 public:
  explicit MediaRouterActionController(Profile* profile);
  // Constructor for injecting dependencies in tests.
  MediaRouterActionController(
      Profile* profile,
      media_router::MediaRouter* router,
      ComponentActionDelegate* component_action_delegate);
  ~MediaRouterActionController() override;

  // Whether the media router action is shown by an administrator policy.
  static bool IsActionShownByPolicy(Profile* profile);

  // Gets and sets the preference for whether the media router action should be
  // pinned to the toolbar/overflow menu.
  static bool GetAlwaysShowActionPref(Profile* profile);
  static void SetAlwaysShowActionPref(Profile* profile, bool always_show);

  // media_router::IssuesObserver:
  void OnIssue(const media_router::Issue& issue) override;
  void OnIssuesCleared() override;

  // media_router::MediaRoutesObserver:
  void OnRoutesUpdated(const std::vector<media_router::MediaRoute>& routes,
                       const std::vector<media_router::MediaRoute::Id>&
                           joinable_route_ids) override;

  // Called when a Media Router dialog is shown or hidden, and updates the
  // visibility of the action icon. Overridden in tests.
  virtual void OnDialogShown();
  virtual void OnDialogHidden();

 private:
  friend class MediaRouterActionControllerUnitTest;
  FRIEND_TEST_ALL_PREFIXES(MediaRouterActionControllerUnitTest,
                           EphemeralIconForRoutesAndIssues);
  FRIEND_TEST_ALL_PREFIXES(MediaRouterActionControllerUnitTest,
                           EphemeralIconForDialog);

  // Adds or removes the Media Router action icon to/from
  // |component_action_delegate_| if necessary, depending on whether or not
  // we have issues, local routes or a dialog.
  virtual void MaybeAddOrRemoveAction();

  // Returns |true| if the Media Router action should be present on the toolbar
  // or the overflow menu.
  bool ShouldEnableAction() const;

  // The profile |this| is associated with. There should be one instance of this
  // class per profile.
  Profile* const profile_;

  // The delegate that is responsible for showing and hiding the icon on the
  // toolbar. It outlives |this|.
  ComponentActionDelegate* const component_action_delegate_;

  bool has_issue_ = false;
  bool has_local_display_route_ = false;

  // Whether the media router action is shown by an administrator policy.
  bool shown_by_policy_;

  // The number of dialogs that are currently open.
  size_t dialog_count_ = 0;

  PrefChangeRegistrar pref_change_registrar_;

  DISALLOW_COPY_AND_ASSIGN(MediaRouterActionController);
};

#endif  // CHROME_BROWSER_UI_TOOLBAR_MEDIA_ROUTER_ACTION_CONTROLLER_H_
