// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_OMNIBOX_LOCATION_BAR_CONTROLLER_IMPL_H_
#define IOS_CHROME_BROWSER_UI_OMNIBOX_LOCATION_BAR_CONTROLLER_IMPL_H_

#import <UIKit/UIKit.h>

#include <memory>

#include "base/strings/utf_string_conversions.h"
#include "components/omnibox/browser/omnibox_view.h"
#include "ios/chrome/browser/ui/omnibox/location_bar_controller.h"
#include "ios/chrome/browser/ui/omnibox/omnibox_view_ios.h"
#include "ui/base/page_transition_types.h"
#include "url/gurl.h"

namespace ios {
class ChromeBrowserState;
}

namespace web {
class WebState;
}

@protocol BrowserCommands;
@protocol LocationBarDelegate;
@protocol LocationBarURLLoader;
@class PageInfoBridge;
class OmniboxViewIOS;
@class OmniboxPopupCoordinator;
@protocol OmniboxPopupPositioner;
@class LocationBarLegacyView;
class ScopedFullscreenDisabler;
class ToolbarModel;

// Concrete implementation of the LocationBarController interface.
class LocationBarControllerImpl : public LocationBarController {
 public:
  LocationBarControllerImpl(LocationBarLegacyView* location_bar_view,
                            ios::ChromeBrowserState* browser_state,
                            id<LocationBarDelegate> delegate,
                            id<BrowserCommands> dispatcher);
  ~LocationBarControllerImpl() override;

  void SetURLLoader(id<LocationBarURLLoader> URLLoader) {
    URLLoader_ = URLLoader;
  };

  // Creates a popup coordinator and wires it to |edit_view_|.
  OmniboxPopupCoordinator* CreatePopupCoordinator(
      id<OmniboxPopupPositioner> positioner);

  // OmniboxEditController implementation
  void OnAutocompleteAccept(const GURL& url,
                            WindowOpenDisposition disposition,
                            ui::PageTransition transition,
                            AutocompleteMatchType::Type type) override;
  void OnChanged() override;
  void OnInputInProgress(bool in_progress) override;
  void OnSetFocus() override;
  ToolbarModel* GetToolbarModel() override;
  const ToolbarModel* GetToolbarModel() const override;

  // WebOmniboxEditController implementation.
  web::WebState* GetWebState() override;
  void OnKillFocus() override;

  // LocationBarController implementation.
  void OnToolbarUpdated() override;
  void HideKeyboardAndEndEditing() override;
  void SetShouldShowHintText(bool show_hint_text) override;
  const OmniboxView* GetLocationEntry() const override;
  OmniboxView* GetLocationEntry() override;
  bool IsShowingPlaceholderWhileCollapsed() override;

 private:
  // Installs a UIButton that serves as the location icon and lock icon.  This
  // button is installed as a left view of |field_|.
  void InstallLocationIcon();

  // Creates and installs the voice search UIButton as a right view of |field_|.
  // Does nothing on tablet.
  void InstallVoiceSearchIcon();


  bool show_hint_text_;
  std::unique_ptr<OmniboxViewIOS> edit_view_;

  // A bridge from a UIControl action to the dispatcher to display a page
  // info popup.
  __strong PageInfoBridge* page_info_bridge_;
  LocationBarLegacyView* location_bar_view_;
  __weak id<LocationBarDelegate> delegate_;
  __weak id<LocationBarURLLoader> URLLoader_;
  // Dispatcher to send commands from the location bar.
  __weak id<BrowserCommands> dispatcher_;
  // The BrowserState passed on construction.
  ios::ChromeBrowserState* browser_state_;
  // The disabler that prevents fullscreen calculations to occur while the
  // location bar is focused.
  std::unique_ptr<ScopedFullscreenDisabler> fullscreen_disabler_;

  bool is_showing_placeholder_while_collapsed_;
};

#endif  // IOS_CHROME_BROWSER_UI_OMNIBOX_LOCATION_BAR_CONTROLLER_IMPL_H_
