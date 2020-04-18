// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/bookmarks/bookmark_bubble_controller.h"

#include "base/mac/bundle_locations.h"
#import "base/mac/sdk_forward_declarations.h"
#include "base/metrics/user_metrics.h"
#include "base/strings/sys_string_conversions.h"
#include "chrome/browser/ui/bookmarks/bookmark_bubble_observer.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_button.h"
#import "chrome/browser/ui/cocoa/browser_window_controller.h"
#import "chrome/browser/ui/cocoa/bubble_sync_promo_controller.h"
#import "chrome/browser/ui/cocoa/dialog_text_field_editor.h"
#import "chrome/browser/ui/cocoa/info_bubble_view.h"
#import "chrome/browser/ui/cocoa/l10n_util.h"
#import "chrome/browser/ui/cocoa/location_bar/location_bar_view_mac.h"
#import "chrome/browser/ui/cocoa/location_bar/star_decoration.h"
#include "chrome/browser/ui/sync/sync_promo_ui.h"
#include "chrome/common/chrome_features.h"
#include "chrome/grit/chromium_strings.h"
#include "chrome/grit/generated_resources.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/browser/bookmark_utils.h"
#include "components/bookmarks/managed/managed_bookmark_service.h"
#include "components/signin/core/browser/signin_metrics.h"
#include "components/strings/grit/components_strings.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "content/public/browser/notification_service.h"
#include "ui/base/cocoa/cocoa_base_utils.h"
#import "ui/base/cocoa/touch_bar_util.h"
#include "ui/base/l10n/l10n_util_mac.h"

using base::UserMetricsAction;
using bookmarks::BookmarkModel;
using bookmarks::BookmarkNode;

namespace {

// Touch bar identifier.
NSString* const kBookmarkBubbleTouchBarId = @"bookmark-bubble";

// Touch bar item identifiers.
NSString* const kRemoveTouchBarId = @"REMOVE";
NSString* const kEditTouchBarId = @"EDIT";
NSString* const kDoneTouchBarId = @"DONE";

}  // end namespace

@interface BookmarkBubbleController (PrivateAPI)
- (void)updateBookmarkNode;
- (void)fillInFolderList;
@end

@implementation BookmarkBubbleController

@synthesize node = node_;

  // Singleton object to act as a representedObject for the "choose another
  // folder" item in the pop up.
+ (id)chooseAnotherFolderObject {
  static id object = [[NSObject alloc] init];
  return object;
}

- (id)initWithParentWindow:(NSWindow*)parentWindow
            bubbleObserver:(bookmarks::BookmarkBubbleObserver*)bubbleObserver
                   managed:(bookmarks::ManagedBookmarkService*)managed
                     model:(BookmarkModel*)model
                      node:(const BookmarkNode*)node
         alreadyBookmarked:(BOOL)alreadyBookmarked {
  DCHECK(managed);
  DCHECK(node);
  if ((self = [super initWithWindowNibPath:@"BookmarkBubble"
                              parentWindow:parentWindow
                                anchoredAt:NSZeroPoint])) {
    bookmarkBubbleObserver_ = bubbleObserver;
    managedBookmarkService_ = managed;
    model_ = model;
    node_ = node;
    alreadyBookmarked_ = alreadyBookmarked;
  }
  return self;
}

- (void)awakeFromNib {
  [super awakeFromNib];

  [[nameTextField_ cell] setUsesSingleLineMode:YES];

  Browser* browser = chrome::FindBrowserWithWindow(self.parentWindow);
  if (SyncPromoUI::ShouldShowSyncPromo(browser->profile())) {
    signin_metrics::RecordSigninImpressionUserActionForAccessPoint(
        signin_metrics::AccessPoint::ACCESS_POINT_BOOKMARK_BUBBLE);

    syncPromoController_.reset(
        [[BubbleSyncPromoController alloc]
            initWithBrowser:browser
              promoStringId:IDS_BOOKMARK_SYNC_PROMO_MESSAGE
               linkStringId:IDS_BOOKMARK_SYNC_PROMO_LINK
                accessPoint:
                    signin_metrics::AccessPoint::ACCESS_POINT_BOOKMARK_BUBBLE]);
    [syncPromoPlaceholder_ addSubview:[syncPromoController_ view]];

    // Resize the sync promo and its placeholder.
    NSRect syncPromoPlaceholderFrame = [syncPromoPlaceholder_ frame];
    CGFloat syncPromoHeight = [syncPromoController_
        preferredHeightForWidth:syncPromoPlaceholderFrame.size.width];
    syncPromoPlaceholderFrame.size.height = syncPromoHeight;

    [syncPromoPlaceholder_ setFrame:syncPromoPlaceholderFrame];
    [[syncPromoController_ view] setFrame:syncPromoPlaceholderFrame];

    // Adjust the height of the bubble so that the sync promo fits in it,
    // except for its bottom border. The xib file hides the left and right
    // borders of the sync promo.
    NSRect bubbleFrame = [[self window] frame];
    bubbleFrame.size.height +=
        syncPromoHeight - [syncPromoController_ borderWidth];
    [[self window] setFrame:bubbleFrame display:YES];
  }
}

- (void)browserWillBeDestroyed {
  bookmarkBubbleObserver_ = nullptr;
}

- (void)notifyBubbleClosed {
  if (!bookmarkBubbleObserver_)
    return;

  bookmarkBubbleObserver_->OnBookmarkBubbleHidden();
  bookmarkBubbleObserver_ = nullptr;
}

// Close the bookmark bubble without changing anything.  Unlike a
// typical dialog's OK/Cancel, where Cancel is "do nothing", all
// buttons on the bubble have the capacity to change the bookmark
// model.  This is an IBOutlet-looking entry point to remove the
// dialog without touching the model.
- (void)dismissWithoutEditing:(id)sender {
  [self close];
}

- (void)windowWillClose:(NSNotification*)notification {
  // We caught a close so we don't need to watch for the parent closing.
  bookmarkObserver_.reset();
  [self notifyBubbleClosed];

  // Force the field editor to resign the first responder so that it'll
  // be removed from the view hierarchy and its delegate be set to nil.
  [[self window] endEditingFor:nameTextField_];

  [super windowWillClose:notification];
}

// Override -[BaseBubbleController showWindow:] to tweak bubble location and
// set up UI elements.
- (void)showWindow:(id)sender {
  NSWindow* window = [self window];  // Force load the NIB.
  NSWindow* parentWindow = self.parentWindow;
  BrowserWindowController* bwc =
      [BrowserWindowController browserWindowControllerForWindow:parentWindow];

  InfoBubbleView* bubble = self.bubble;
  [bubble setArrowLocation:info_bubble::kTopTrailing];

  // Insure decent positioning even in the absence of a browser controller,
  // which will occur for some unit tests.
  NSPoint arrowTip = bwc ? [bwc bookmarkBubblePoint] :
      NSMakePoint([window frame].size.width, [window frame].size.height);
  arrowTip = ui::ConvertPointFromWindowToScreen(parentWindow, arrowTip);
  NSPoint bubbleArrowTip = [bubble arrowTip];
  bubbleArrowTip = [bubble convertPoint:bubbleArrowTip toView:nil];
  arrowTip.y -= bubbleArrowTip.y;
  arrowTip.x -= bubbleArrowTip.x;
  [window setFrameOrigin:arrowTip];

  // Default is IDS_BOOKMARK_BUBBLE_PAGE_BOOKMARK; "Bookmark".
  // If adding for the 1st time the string becomes "Bookmark Added!"
  if (!alreadyBookmarked_) {
    NSString* title =
        l10n_util::GetNSString(IDS_BOOKMARK_BUBBLE_PAGE_BOOKMARKED);
    [bigTitle_ setStringValue:title];
  }

  [self adjustForRTLIfNecessary];

  [self fillInFolderList];

  // Ping me when things change out from under us.  Unlike a normal
  // dialog, the bookmark bubble's cancel: means "don't add this as a
  // bookmark", not "cancel editing".  We must take extra care to not
  // touch the bookmark in this selector.
  bookmarkObserver_.reset(new BookmarkModelObserverForCocoa(model_, ^{
    [self dismissWithoutEditing:nil];
  }));
  bookmarkObserver_->StartObservingNode(node_);

  [parentWindow addChildWindow:window ordered:NSWindowAbove];
  [window makeKeyAndOrderFront:self];
  [self registerKeyStateEventTap];

  bookmarkBubbleObserver_->OnBookmarkBubbleShown(node_);

  [self decorationForBubble]->SetActive(true);
}

- (void)close {
  [[BrowserWindowController browserWindowControllerForWindow:self.parentWindow]
      releaseToolbarVisibilityForOwner:self
                         withAnimation:YES];

  [super close];
}

- (NSTouchBar*)makeTouchBar {
  if (!base::FeatureList::IsEnabled(features::kDialogTouchBar))
    return nil;

  base::scoped_nsobject<NSTouchBar> touchBar([[ui::NSTouchBar() alloc] init]);
  [touchBar
      setCustomizationIdentifier:ui::GetTouchBarId(kBookmarkBubbleTouchBarId)];
  [touchBar setDelegate:self];

  NSArray* dialogItems = @[
    ui::GetTouchBarItemId(kBookmarkBubbleTouchBarId, kRemoveTouchBarId),
    ui::GetTouchBarItemId(kBookmarkBubbleTouchBarId, kEditTouchBarId),
    ui::GetTouchBarItemId(kBookmarkBubbleTouchBarId, kDoneTouchBarId)
  ];

  [touchBar setDefaultItemIdentifiers:dialogItems];
  [touchBar setCustomizationAllowedItemIdentifiers:dialogItems];
  return touchBar.autorelease();
}

- (NSTouchBarItem*)touchBar:(NSTouchBar*)touchBar
      makeItemForIdentifier:(NSTouchBarItemIdentifier)identifier
    API_AVAILABLE(macos(10.12.2)) {
  NSButton* button = nil;
  if ([identifier hasSuffix:kRemoveTouchBarId]) {
    button = [NSButton buttonWithTitle:l10n_util::GetNSString(
                                           IDS_BOOKMARK_BUBBLE_REMOVE_BOOKMARK)
                                target:self
                                action:@selector(remove:)];
  } else if ([identifier hasSuffix:kEditTouchBarId]) {
    button = [NSButton
        buttonWithTitle:l10n_util::GetNSString(IDS_BOOKMARK_BUBBLE_OPTIONS)
                 target:self
                 action:@selector(edit:)];
  } else if ([identifier hasSuffix:kDoneTouchBarId]) {
    button = ui::GetBlueTouchBarButton(l10n_util::GetNSString(IDS_DONE), self,
                                       @selector(ok:));
  } else {
    return nil;
  }

  base::scoped_nsobject<NSCustomTouchBarItem> item(
      [[ui::NSCustomTouchBarItem() alloc] initWithIdentifier:identifier]);
  [item setView:button];
  return item.autorelease();
}

// Delegate method: see |NSWindowDelegate| protocol.
- (id)windowWillReturnFieldEditor:(NSWindow*)sender toObject:(id)obj {
  if (!base::FeatureList::IsEnabled(features::kDialogTouchBar))
    return nil;

  if (obj != nameTextField_)
    return nil;

  if (!textFieldEditor_)
    textFieldEditor_.reset([[DialogTextFieldEditor alloc] init]);

  return textFieldEditor_.get();
}

// Shows the bookmark editor sheet for more advanced editing.
- (void)showEditor {
  [self ok:self];
  // Send the action up through the responder chain.
  [NSApp sendAction:@selector(editBookmarkNode:) to:nil from:self];
}

- (IBAction)edit:(id)sender {
  base::RecordAction(UserMetricsAction("BookmarkBubble_Edit"));
  [self showEditor];
}

- (IBAction)ok:(id)sender {
  [self updateBookmarkNode];
  [self close];
}

// By implementing this, ESC causes the window to go away. If clicking the
// star was what prompted this bubble to appear (i.e., not already bookmarked),
// remove the bookmark.
- (IBAction)cancel:(id)sender {
  if (!alreadyBookmarked_) {
    // |-remove:| calls |-close| so don't do it.
    [self remove:sender];
  } else {
    [self dismissWithoutEditing:nil];
  }
}

- (IBAction)remove:(id)sender {
  bookmarks::RemoveAllBookmarks(model_, node_->url());
  base::RecordAction(UserMetricsAction("BookmarkBubble_Unstar"));
  node_ = NULL;  // no longer valid
  [self ok:sender];
}

// The controller is  the target of the pop up button box action so it can
// handle when "choose another folder" was picked.
- (IBAction)folderChanged:(id)sender {
  DCHECK([sender isEqual:folderPopUpButton_]);
  // It is possible that due to model change our parent window has been closed
  // but the popup is still showing and able to notify the controller of a
  // folder change.  We ignore the sender in this case.
  if (!self.parentWindow)
    return;
  NSMenuItem* selected = [folderPopUpButton_ selectedItem];
  if ([selected representedObject] ==
      [[self class] chooseAnotherFolderObject]) {
    base::RecordAction(UserMetricsAction("BookmarkBubble_EditFromCombobox"));
    [self showEditor];
  }
}

// The controller is the delegate of the window so it receives did resign key
// notifications. When key is resigned mirror Windows behavior and close the
// window.
- (void)windowDidResignKey:(NSNotification*)notification {
  NSWindow* window = [self window];
  DCHECK_EQ([notification object], window);
  if ([window isVisible]) {
    // If the window isn't visible, it is already closed, and this notification
    // has been sent as part of the closing operation, so no need to close.
    [self ok:self];
  }
}

// Look at the dialog; if the user has changed anything, update the
// bookmark node to reflect this.
- (void)updateBookmarkNode {
  if (!node_) return;

  // First the title...
  NSString* oldTitle = base::SysUTF16ToNSString(node_->GetTitle());
  NSString* newTitle = [nameTextField_ stringValue];
  if (![oldTitle isEqual:newTitle]) {
    model_->SetTitle(node_, base::SysNSStringToUTF16(newTitle));
    base::RecordAction(UserMetricsAction("BookmarkBubble_ChangeTitleInBubble"));
  }
  // Then the parent folder.
  const BookmarkNode* oldParent = node_->parent();
  NSMenuItem* selectedItem = [folderPopUpButton_ selectedItem];
  id representedObject = [selectedItem representedObject];
  if (representedObject == [[self class] chooseAnotherFolderObject]) {
    // "Choose another folder..."
    return;
  }
  const BookmarkNode* newParent =
      static_cast<const BookmarkNode*>([representedObject pointerValue]);
  DCHECK(newParent);
  if (oldParent != newParent) {
    int index = newParent->child_count();
    model_->Move(node_, newParent, index);
    base::RecordAction(UserMetricsAction("BookmarkBubble_ChangeParent"));
  }
}

// Fill in all information related to the folder pop up button.
- (void)fillInFolderList {
  [nameTextField_ setStringValue:base::SysUTF16ToNSString(node_->GetTitle())];
  DCHECK([folderPopUpButton_ numberOfItems] == 0);
  [self addFolderNodes:model_->root_node()
         toPopUpButton:folderPopUpButton_
           indentation:0];
  NSMenu* menu = [folderPopUpButton_ menu];
  [menu addItem:[NSMenuItem separatorItem]];
  NSString* title = [[self class] chooseAnotherFolderString];
  NSMenuItem *item = [menu addItemWithTitle:title
                                     action:NULL
                              keyEquivalent:@""];
  [item setRepresentedObject:[[self class] chooseAnotherFolderObject]];
  // Finally, select the current parent.
  NSValue* parentValue = [NSValue valueWithPointer:node_->parent()];
  NSInteger idx = [menu indexOfItemWithRepresentedObject:parentValue];
  [folderPopUpButton_ selectItemAtIndex:idx];
}

- (LocationBarDecoration*)decorationForBubble {
  BrowserWindowController* browserWindowController = [BrowserWindowController
      browserWindowControllerForWindow:[self parentWindow]];
  LocationBarViewMac* locationBar = [browserWindowController locationBarBridge];
  return locationBar ? locationBar->star_decoration() : nullptr;
}

- (void)adjustForRTLIfNecessary {
  // Info bubble view to:
  // - Fix the leading margin on the title.
  // - Flip containers.
  cocoa_l10n_util::FlipAllSubviewsIfNecessary([bigTitle_ superview]);
  // Margin on the labels.
  cocoa_l10n_util::FlipAllSubviewsIfNecessary(fieldLabelsContainer_);
  // Margin on the fields.
  cocoa_l10n_util::FlipAllSubviewsIfNecessary([nameTextField_ superview]);
  // Relative order of the done and options buttons.
  cocoa_l10n_util::FlipAllSubviewsIfNecessary(trailingButtonContainer_);
  if (cocoa_l10n_util::ShouldDoExperimentalRTLLayout()) {
    // Fix up pop-up button from the nib.
    [folderPopUpButton_ setUserInterfaceLayoutDirection:
                            NSUserInterfaceLayoutDirectionRightToLeft];
    [folderPopUpButton_ setAlignment:NSNaturalTextAlignment];
  }
}

@end  // BookmarkBubbleController


@implementation BookmarkBubbleController (ExposedForUnitTesting)

- (NSView*)syncPromoPlaceholder {
  return syncPromoPlaceholder_;
}

- (bookmarks::BookmarkBubbleObserver*)bookmarkBubbleObserver {
  return bookmarkBubbleObserver_;
}

+ (NSString*)chooseAnotherFolderString {
  return l10n_util::GetNSStringWithFixup(
      IDS_BOOKMARK_BUBBLE_CHOOSER_ANOTHER_FOLDER);
}

// For the given folder node, walk the tree and add folder names to
// the given pop up button.
- (void)addFolderNodes:(const BookmarkNode*)parent
         toPopUpButton:(NSPopUpButton*)button
           indentation:(int)indentation {
  if (!model_->is_root_node(parent)) {
    NSString* title = base::SysUTF16ToNSString(parent->GetTitle());
    NSMenu* menu = [button menu];
    NSMenuItem* item = [menu addItemWithTitle:title
                                       action:NULL
                                keyEquivalent:@""];
    [item setRepresentedObject:[NSValue valueWithPointer:parent]];
    [item setIndentationLevel:indentation];
    ++indentation;
  }
  for (int i = 0; i < parent->child_count(); i++) {
    const BookmarkNode* child = parent->GetChild(i);
    if (child->is_folder() && child->IsVisible() &&
        managedBookmarkService_->CanBeEditedByUser(child)) {
      [self addFolderNodes:child
             toPopUpButton:button
               indentation:indentation];
    }
  }
}

- (void)setTitle:(NSString*)title parentFolder:(const BookmarkNode*)parent {
  [nameTextField_ setStringValue:title];
  [self setParentFolderSelection:parent];
}

// Pick a specific parent node in the selection by finding the right
// pop up button index.
- (void)setParentFolderSelection:(const BookmarkNode*)parent {
  // Expectation: There is a parent mapping for all items in the
  // folderPopUpButton except the last one ("Choose another folder...").
  NSMenu* menu = [folderPopUpButton_ menu];
  NSValue* parentValue = [NSValue valueWithPointer:parent];
  NSInteger idx = [menu indexOfItemWithRepresentedObject:parentValue];
  DCHECK(idx != -1);
  [folderPopUpButton_ selectItemAtIndex:idx];
}

- (NSPopUpButton*)folderPopUpButton {
  return folderPopUpButton_;
}

@end  // implementation BookmarkBubbleController(ExposedForUnitTesting)
