// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_PAGE_INFO_PAGE_INFO_BUBBLE_CONTROLLER_H_
#define CHROME_BROWSER_UI_COCOA_PAGE_INFO_PAGE_INFO_BUBBLE_CONTROLLER_H_

#import <Cocoa/Cocoa.h>

#include <memory>

#include "base/mac/scoped_nsobject.h"
#include "base/macros.h"
#import "chrome/browser/ui/cocoa/base_bubble_controller.h"
#include "chrome/browser/ui/page_info/page_info_ui.h"
#include "content/public/browser/web_contents_observer.h"

class LocationBarDecoration;
class PageInfoUIBridge;
@class InspectLinkView;

namespace content {
class WebContents;
}

namespace net {
class X509Certificate;
}

// This NSWindowController subclass manages the InfoBubbleWindow and view that
// are displayed when the user clicks the omnibox security indicator icon.
@interface PageInfoBubbleController : BaseBubbleController {
 @private
  content::WebContents* webContents_;

  base::scoped_nsobject<NSView> contentView_;

  // The main content view for the Permissions tab.
  NSView* securitySectionView_;

  // Displays the short security summary for the page
  // (private/not private/etc.).
  NSTextField* securitySummaryField_;

  // Displays a longer explanation of the page's security state, and how the
  // user should treat it.
  NSTextField* securityDetailsField_;

  // The link button for opening a Chrome Help Center page explaining connection
  // security.
  NSButton* connectionHelpButton_;

  // URL of the page for which the bubble is shown.
  GURL url_;

  // Displays a paragraph to accompany the reset decisions button, explaining
  // that the user has made a decision to trust an invalid security certificate
  // for the current site.
  // This field only shows when there is an acrive certificate exception.
  NSTextField* resetDecisionsField_;

  // The link button for revoking certificate decisions.
  // This link only shows when there is an active certificate exception.
  NSButton* resetDecisionsButton_;

  // The server certificate from the identity info. This should always be
  // non-null on a cryptographic connection, and null otherwise.
  scoped_refptr<net::X509Certificate> certificate_;

  // Separator line.
  NSView* separatorAfterSecuritySection_;

  // Container for the site settings section.
  NSView* siteSettingsSectionView_;

  // Container for certificate info in the site settings section.
  InspectLinkView* certificateView_;

  // Container for cookies info in the site settings section.
  InspectLinkView* cookiesView_;

  // Container for permission info in the site settings section.
  NSView* permissionsView_;

  // The link button for showing site settings.
  NSButton* siteSettingsButton_;

  // The UI translates user actions to specific events and forwards them to the
  // |presenter_|. The |presenter_| handles these events and updates the UI.
  std::unique_ptr<PageInfo> presenter_;

  // Bridge which implements the PageInfoUI interface and forwards
  // methods on to this class.
  std::unique_ptr<PageInfoUIBridge> bridge_;

  // The omnibox icon the bubble is anchored to. The icon is set as active
  // when the bubble is opened, and inactive when the bubble is closed.
  // Usually we would override OmniboxDecorationBubbleController but the page
  // info icon has a race condition where it might switch between
  // LocationIconDecoration and SecurityStateBubbleDecoration.
  LocationBarDecoration* decoration_;  // Weak.

  // The button for changing password decisions.
  // This button only shows when there is an password reuse event.
  NSButton* changePasswordButton_;

  // The button for whitelisting password reuse decisions.
  // This button only shows when there is an password reuse event.
  NSButton* whitelistPasswordReuseButton_;
}

// Designated initializer. The controller will release itself when the bubble
// is closed. |parentWindow| cannot be nil. |webContents| may be nil for
// testing purposes.
- (id)initWithParentWindow:(NSWindow*)parentWindow
          pageInfoUIBridge:(PageInfoUIBridge*)bridge
               webContents:(content::WebContents*)webContents
                       url:(const GURL&)url;

// Return the default width of the window. It may be wider to fit the content.
// This may be overriden by a subclass for testing purposes.
- (CGFloat)defaultWindowWidth;

@end

// Provides a bridge between the PageInfoUI C++ interface and the Cocoa
// implementation in PageInfoBubbleController.
class PageInfoUIBridge : public content::WebContentsObserver,
                         public PageInfoUI {
 public:
  explicit PageInfoUIBridge(content::WebContents* web_contents);
  ~PageInfoUIBridge() override;

  void set_bubble_controller(PageInfoBubbleController* bubble_controller);

  // WebContentsObserver implementation.
  void RenderFrameDeleted(content::RenderFrameHost* render_frame_host) override;

  // PageInfoUI implementations.
  void SetCookieInfo(const CookieInfoList& cookie_info_list) override;
  void SetPermissionInfo(const PermissionInfoList& permission_info_list,
                         ChosenObjectInfoList chosen_object_info_list) override;
  void SetIdentityInfo(const IdentityInfo& identity_info) override;

 protected:
  // WebContentsObserver implementation.
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;

 private:
  // The WebContents the bubble UI is attached to.
  content::WebContents* web_contents_;

  // The Cocoa controller for the bubble UI.
  PageInfoBubbleController* bubble_controller_;

  DISALLOW_COPY_AND_ASSIGN(PageInfoUIBridge);
};

#endif  // CHROME_BROWSER_UI_COCOA_PAGE_INFO_PAGE_INFO_BUBBLE_CONTROLLER_H_
