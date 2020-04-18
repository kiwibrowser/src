// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_LOCATION_BAR_LOCATION_BAR_VIEW_MAC_H_
#define CHROME_BROWSER_UI_COCOA_LOCATION_BAR_LOCATION_BAR_VIEW_MAC_H_

#import <Cocoa/Cocoa.h>
#include <stddef.h>

#include <memory>
#include <string>

#include "base/macros.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/cocoa/omnibox/omnibox_view_mac.h"
#include "chrome/browser/ui/location_bar/location_bar.h"
#include "chrome/browser/ui/omnibox/chrome_omnibox_edit_controller.h"
#include "chrome/browser/ui/page_action/page_action_icon_container.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "components/prefs/pref_member.h"
#include "components/security_state/core/security_state.h"
#include "components/zoom/zoom_event_manager_observer.h"

@class AutocompleteTextField;
class CommandUpdater;
class ContentSettingDecoration;
class KeywordHintDecoration;
class LocationBarDecoration;
class ManagePasswordsDecoration;
class Profile;
class SaveCreditCardDecoration;
class SelectedKeywordDecoration;
class StarDecoration;
class TranslateDecoration;
class PageInfoBubbleDecoration;
class ZoomDecoration;
class ZoomDecorationTest;

namespace {
class LocationBarViewMacTest;
}

// Enumeration of the type of verbose that is displayed on the page info
// decoration.
enum class PageInfoVerboseType {
  kNone = 0,
  kSecurity,
  kEVCert,
  kChrome,
  kExtension
};

// A C++ bridge class that represents the location bar UI element to
// the portable code.  Wires up an OmniboxViewMac instance to
// the location bar text field, which handles most of the work.

class LocationBarViewMac : public LocationBar,
                           public LocationBarTesting,
                           public ChromeOmniboxEditController,
                           public PageActionIconContainer,
                           public zoom::ZoomEventManagerObserver {
 public:
  LocationBarViewMac(AutocompleteTextField* field,
                     CommandUpdater* command_updater,
                     Profile* profile,
                     Browser* browser);
  ~LocationBarViewMac() override;

  // Overridden from LocationBar:
  GURL GetDestinationURL() const override;
  WindowOpenDisposition GetWindowOpenDisposition() const override;
  ui::PageTransition GetPageTransition() const override;
  void AcceptInput() override;
  void FocusLocation(bool select_all) override;
  void FocusSearch() override;
  void UpdateContentSettingsIcons() override;
  void UpdateManagePasswordsIconAndBubble() override;
  void UpdateSaveCreditCardIcon() override;
  void UpdateFindBarIconVisibility() override;
  void UpdateBookmarkStarVisibility() override;
  void UpdateZoomViewVisibility() override;
  void UpdateLocationBarVisibility(bool visible, bool animate) override;
  void SaveStateToContents(content::WebContents* contents) override;
  void Revert() override;
  const OmniboxView* GetOmniboxView() const override;
  OmniboxView* GetOmniboxView() override;
  LocationBarTesting* GetLocationBarForTesting() override;

  // Overridden from LocationBarTesting:
  bool GetBookmarkStarVisibility() override;
  bool TestContentSettingImagePressed(size_t index) override;
  bool IsContentSettingBubbleShowing(size_t index) override;

  // Set/Get the editable state of the field.
  void SetEditable(bool editable);
  bool IsEditable();

  // Set the starred state of the bookmark star.
  void SetStarred(bool starred);

  // Set whether or not the translate icon is lit.
  void SetTranslateIconLit(bool on);

  // Happens when the zoom changes for the active tab. |can_show_bubble| is
  // false when the change in zoom for the active tab wasn't an explicit user
  // action (e.g. switching tabs, creating a new tab, creating a new browser).
  // Additionally, |can_show_bubble| will only be true when the bubble wouldn't
  // be obscured by other UI (app menu) or redundant (+/- from app menu).
  void ZoomChangedForActiveTab(bool can_show_bubble);

  // Checks if the bookmark star should be enabled or not.
  bool IsStarEnabled() const;

  // Get the point in window coordinates in the |decoration| at which the
  // associate bubble aims.
  NSPoint GetBubblePointForDecoration(LocationBarDecoration* decoration) const;

  // Get the point in window coordinates in the save credit card icon for the
  //  save credit card bubble to aim at.
  NSPoint GetSaveCreditCardBubblePoint() const;

  // Get the point in window coordinates for the page info bubble anchor.
  NSPoint GetPageInfoBubblePoint() const;

  // When any image decorations change, call this to ensure everything is
  // redrawn and laid out if necessary.
  void OnDecorationsChanged();

  // Layout the various decorations which live in the field.
  void Layout();

  // Re-draws |decoration| if it's already being displayed.
  void RedrawDecoration(LocationBarDecoration* decoration);

  // Updates the controller, and, if |contents| is non-null, restores saved
  // state that the tab holds.
  void Update(const content::WebContents* contents);

  // Clears any location bar state stored for |contents|.
  void ResetTabState(content::WebContents* contents);

  // Set the location bar's icon to the correct image for the current URL.
  void UpdateLocationIcon();

  // Set the location bar's controls to visibly match the current theme.
  void UpdateColorsToMatchTheme();

  // Notify the location bar that it was added to the browser window. Provides
  // an update point for interface objects that need to set their appearance
  // based on the window's theme.
  void OnAddedToWindow();

  // Notify the location bar that the browser window theme has changed. Provides
  // an update point for interface objects that need to set their appearance
  // based on the window's theme.
  void OnThemeChanged();

  // ChromeOmniboxEditController:
  void UpdateWithoutTabRestore() override;
  void OnChanged() override;
  ToolbarModel* GetToolbarModel() override;
  const ToolbarModel* GetToolbarModel() const override;
  content::WebContents* GetWebContents() override;

  // PageActionIconContainer:
  void UpdatePageActionIcon(PageActionIconType) override;

  PageInfoVerboseType GetPageInfoVerboseType() const;

  // Returns true if the page info decoration should display security verbose.
  // The verbose should only be shown for valid and invalid HTTPS states.
  bool HasSecurityVerboseText() const;

  NSImage* GetKeywordImage(const base::string16& keyword);

  // Returns the color for the vector icon in the location bar.
  SkColor GetLocationBarIconColor() const;

  AutocompleteTextField* GetAutocompleteTextField() { return field_; }

  // Returns true if the location bar is dark.
  bool IsLocationBarDark() const;

  PageInfoBubbleDecoration* page_info_decoration() const {
    return page_info_decoration_.get();
  }

  ManagePasswordsDecoration* manage_passwords_decoration() {
    return manage_passwords_decoration_.get();
  }

  SaveCreditCardDecoration* save_credit_card_decoration() {
    return save_credit_card_decoration_.get();
  }

  StarDecoration* star_decoration() const { return star_decoration_.get(); }

  TranslateDecoration* translate_decoration() const {
    return translate_decoration_.get();
  }

  ZoomDecoration* zoom_decoration() const { return zoom_decoration_.get(); }

  Browser* browser() const { return browser_; }

  // ZoomManagerObserver:
  // Updates the view for the zoom icon when default zoom levels change.
  void OnDefaultZoomLevelChanged() override;

  // Returns the decoration accessibility views for all of this
  // LocationBarViewMac's decorations. The returned NSViews may not have been
  // positioned yet.
  std::vector<NSView*> GetDecorationAccessibilityViews();

 private:
  friend class LocationBarViewMacTest;
  friend ZoomDecorationTest;

  // Posts |notification| to the default notification center.
  void PostNotification(NSString* notification);

  void OnEditBookmarksEnabledChanged();

  void UpdatePageInfoText();

  // Updates visibility of the content settings icons based on the current
  // tab contents state.
  bool RefreshContentSettingsDecorations();

  // Updates the translate decoration in the omnibox with the current translate
  // state.
  void UpdateTranslateDecoration();

  // Updates the zoom decoration in the omnibox with the current zoom level.
  // Returns whether any updates were made.
  bool UpdateZoomDecoration(bool default_zoom_changed);

  // Animates |page_info_decoration_| in or out if applicable. Otherwise,
  // show it without animation.
  void AnimatePageInfoIfPossible(bool tab_changed);

  // Returns true if the |page_info_decoration_| can animate for the |level|.
  bool CanAnimateSecurityLevel(security_state::SecurityLevel level) const;

  // Returns pointers to all of the LocationBarDecorations owned by this
  // LocationBarViewMac. This helper function is used for positioning and
  // re-positioning accessibility views.
  std::vector<LocationBarDecoration*> GetDecorations();

  // Updates |decoration|'s accessibility view's position to match the computed
  // position the decoration will be drawn at, and update its enabled state
  // based on whether |decoration| is accepting presses currently.
  void UpdateAccessibilityView(LocationBarDecoration* decoration);

  std::unique_ptr<OmniboxViewMac> omnibox_view_;

  AutocompleteTextField* field_;  // owned by tab controller

  // A decoration that shows the keyword-search bubble on the left.
  std::unique_ptr<SelectedKeywordDecoration> selected_keyword_decoration_;

  // A decoration that shows an icon to the left of the address. If applicable,
  // it'll also show information about the current page.
  std::unique_ptr<PageInfoBubbleDecoration> page_info_decoration_;

  // Save credit card icon on the right side of the omnibox.
  std::unique_ptr<SaveCreditCardDecoration> save_credit_card_decoration_;

  // Bookmark star right of page actions.
  std::unique_ptr<StarDecoration> star_decoration_;

  // Translate icon at the end of the ominibox.
  std::unique_ptr<TranslateDecoration> translate_decoration_;

  // A zoom icon at the end of the omnibox, which shows at non-standard zoom
  // levels.
  std::unique_ptr<ZoomDecoration> zoom_decoration_;

  // The content blocked decorations.
  std::vector<std::unique_ptr<ContentSettingDecoration>>
      content_setting_decorations_;

  // Keyword hint decoration displayed on the right-hand side.
  std::unique_ptr<KeywordHintDecoration> keyword_hint_decoration_;

  // The right-hand-side button to manage passwords associated with a page.
  std::unique_ptr<ManagePasswordsDecoration> manage_passwords_decoration_;

  Browser* browser_;

  // Used to change the visibility of the star decoration.
  BooleanPrefMember edit_bookmarks_enabled_;

  // Indicates whether or not the location bar is currently visible.
  bool location_bar_visible_;

  // True if there's enough room for the omnibox to show the security verbose.
  bool is_width_available_for_security_verbose_;

  // The security level that's displayed on |page_info_decoration_|.
  security_state::SecurityLevel security_level_;

  DISALLOW_COPY_AND_ASSIGN(LocationBarViewMac);
};

#endif  // CHROME_BROWSER_UI_COCOA_LOCATION_BAR_LOCATION_BAR_VIEW_MAC_H_
