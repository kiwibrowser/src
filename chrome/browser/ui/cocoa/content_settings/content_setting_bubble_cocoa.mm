// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/content_settings/content_setting_bubble_cocoa.h"

#include <stddef.h>

#include "base/command_line.h"
#include "base/logging.h"
#include "base/mac/availability.h"
#import "base/mac/sdk_forward_declarations.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/plugins/plugin_finder.h"
#include "chrome/browser/plugins/plugin_metadata.h"
#import "chrome/browser/ui/cocoa/content_settings/blocked_plugin_bubble_controller.h"
#import "chrome/browser/ui/cocoa/info_bubble_view.h"
#import "chrome/browser/ui/cocoa/l10n_util.h"
#import "chrome/browser/ui/cocoa/location_bar/content_setting_decoration.h"
#import "chrome/browser/ui/cocoa/subresource_filter/subresource_filter_bubble_controller.h"
#include "chrome/browser/ui/content_settings/content_setting_bubble_model.h"
#include "chrome/browser/ui/content_settings/content_setting_media_menu_model.h"
#include "chrome/common/chrome_features.h"
#include "chrome/grit/generated_resources.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/strings/grit/components_strings.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/plugin_service.h"
#include "content/public/browser/web_contents_observer.h"
#include "skia/ext/skia_utils_mac.h"
#import "third_party/google_toolbox_for_mac/src/AppKit/GTMUILocalizerAndLayoutTweaker.h"
#import "ui/base/cocoa/controls/hyperlink_button_cell.h"
#import "ui/base/cocoa/touch_bar_util.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/events/cocoa/cocoa_event_utils.h"

using content::PluginService;

namespace {

// Height of one link in the popup list.
const int kLinkHeight = 16;

// Space between two popup links.
const int kLinkPadding = 4;

// Space taken in total by one popup link.
const int kLinkLineHeight = kLinkHeight + kLinkPadding;

// Space between popup list and surrounding UI elements.
const int kLinkOuterPadding = 8;

// Height of each of the labels in the geolocation bubble.
const int kGeoLabelHeight = 14;

// Height of the "Clear" button in the geolocation bubble.
const int kGeoClearButtonHeight = 17;

// General padding between elements in the geolocation bubble.
const int kGeoPadding = 8;

// Padding between host names in the geolocation bubble.
const int kGeoHostPadding = 4;

// Minimal padding between "Manage" and "Done" buttons.
const int kManageDonePadding = 8;

// Padding between radio buttons and media menus buttons in the media bubble.
const int kMediaMenuVerticalPadding = 25;

// Padding between media menu elements in the media bubble.
const int kMediaMenuElementVerticalPadding = 5;

// The amount of horizontal space between the media menu title and the border.
const int kMediaMenuTitleHorizontalPadding = 10;

// The minimum width of the media menu buttons.
const CGFloat kMinMediaMenuButtonWidth = 100;

// Height of each of the labels in the MIDI bubble.
const int kMIDISysExLabelHeight = 14;

// Height of the "Clear" button in the MIDI bubble.
const int kMIDISysExClearButtonHeight = 17;

// General padding between elements in the MIDI bubble.
const int kMIDISysExPadding = 8;

// Padding between host names in the MIDI bubble.
const int kMIDISysExHostPadding = 4;

// Touch bar identifier.
NSString* const kContentSettingsBubbleTouchBarId = @"content-settings-bubble";

// Touch bar item identifiers.
NSString* const kManageTouchBarId = @"MANAGE";
NSString* const kDoneTouchBarId = @"DONE";

void SetControlSize(NSControl* control, NSControlSize controlSize) {
  CGFloat fontSize = [NSFont systemFontSizeForControlSize:controlSize];
  NSCell* cell = [control cell];
  [cell setFont:[NSFont systemFontOfSize:fontSize]];
  [cell setControlSize:controlSize];
}

// Returns an autoreleased NSTextField that is configured to look like a Label
// looks in Interface Builder.
NSTextField* LabelWithFrame(NSString* text, const NSRect& frame) {
  NSTextField* label = [[NSTextField alloc] initWithFrame:frame];
  [label setStringValue:text];
  [label setSelectable:NO];
  [label setBezeled:NO];
  [label setAlignment:NSNaturalTextAlignment];
  return [label autorelease];
}

// Sets the title for the popup button.
void SetTitleForPopUpButton(NSPopUpButton* button, NSString* title) {
  base::scoped_nsobject<NSMenuItem> titleItem([[NSMenuItem alloc] init]);
  [titleItem setTitle:title];
  [[button cell] setUsesItemFromMenu:NO];
  [[button cell] setMenuItem:titleItem.get()];
}

// Builds the popup button menu from the menu model and returns the width of the
// longgest item as the width of the popup menu.
CGFloat BuildPopUpMenuFromModel(NSPopUpButton* button,
                                ContentSettingMediaMenuModel* model,
                                const std::string& title,
                                bool disabled) {
  [[button cell] setControlSize:NSSmallControlSize];
  [[button cell] setArrowPosition:NSPopUpArrowAtBottom];
  [button setFont:[NSFont systemFontOfSize:[NSFont smallSystemFontSize]]];
  [button setButtonType:NSMomentaryPushInButton];
  [button setAlignment:NSLeftTextAlignment];
  [button setAutoresizingMask:NSViewMinXMargin];
  [button setAction:@selector(mediaMenuChanged:)];
  [button sizeToFit];

  CGFloat menuWidth = 0;
  for (int i = 0; i < model->GetItemCount(); ++i) {
    NSString* itemTitle =
        base::SysUTF16ToNSString(model->GetLabelAt(i));
    [button addItemWithTitle:itemTitle];
    [[button lastItem] setTag:i];

    if (base::UTF16ToUTF8(model->GetLabelAt(i)) == title)
      [button selectItemWithTag:i];

    // Determine the largest possible size for this button.
    NSDictionary* textAttributes =
        [NSDictionary dictionaryWithObject:[button font]
                                    forKey:NSFontAttributeName];
    NSSize size = [itemTitle sizeWithAttributes:textAttributes];
    NSRect buttonFrame = [button frame];
    NSRect titleRect = [[button cell] titleRectForBounds:buttonFrame];
    CGFloat width = size.width + NSWidth(buttonFrame) - NSWidth(titleRect) +
        kMediaMenuTitleHorizontalPadding;
    menuWidth = std::max(menuWidth, width);
  }

  if (!model->GetItemCount()) {
    // Show a "None available" title and grey out the menu when there is no
    // available device.
    SetTitleForPopUpButton(
        button, l10n_util::GetNSString(IDS_MEDIA_MENU_NO_DEVICE_TITLE));
    [button setEnabled:NO];
  } else {
    SetTitleForPopUpButton(button, base::SysUTF8ToNSString(title));

    // Disable the device selection when the website is managing the devices
    // itself.
    if (disabled)
      [button setEnabled:NO];
  }

  return menuWidth;
}

}  // namespace

namespace content_setting_bubble {

MediaMenuParts::MediaMenuParts(content::MediaStreamType type,
                               NSTextField* label)
    : type(type),
      label(label) {}
MediaMenuParts::~MediaMenuParts() {}

}  // namespace content_setting_bubble

class ContentSettingBubbleWebContentsObserverBridge
    : public content::WebContentsObserver {
 public:
  ContentSettingBubbleWebContentsObserverBridge(
      content::WebContents* web_contents,
      ContentSettingBubbleController* controller)
      : content::WebContentsObserver(web_contents),
        controller_(controller) {
  }

 protected:
  // WebContentsObserver:
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override {
    if (!navigation_handle->IsInMainFrame() ||
        !navigation_handle->HasCommitted()) {
      return;
    }
    // Content settings are based on the main frame, so if it switches then
    // close up shop.
    [controller_ closeBubble:nil];
  }

 private:
  ContentSettingBubbleController* controller_;  // weak

  DISALLOW_COPY_AND_ASSIGN(ContentSettingBubbleWebContentsObserverBridge);
};

@interface ContentSettingBubbleController (Private)
- (id)initWithModel:(ContentSettingBubbleModel*)settingsBubbleModel
        webContents:(content::WebContents*)webContents
       parentWindow:(NSWindow*)parentWindow
         decoration:(ContentSettingDecoration*)decoration
         anchoredAt:(NSPoint)anchoredAt;
+ (NSString*)getNibPathForModel:(ContentSettingBubbleModel*)model;
+ (ContentSettingBubbleController*)allocControllerForModel:
    (ContentSettingBubbleModel*)model;
- (NSButton*)hyperlinkButtonWithFrame:(NSRect)frame
                                title:(NSString*)title
                                 icon:(NSImage*)icon
                       referenceFrame:(NSRect)referenceFrame;
- (void)initializeTitle;
- (void)initializeMessage;
- (void)initializeRadioGroup;
- (void)initializeItemList;
- (void)initializeGeoLists;
- (void)initializeMediaMenus;
- (void)initializeMIDISysExLists;
- (void)initManageDoneButtons;
- (void)removeInfoButton;
- (void)popupLinkClicked:(id)sender;
- (void)clearGeolocationForCurrentHost:(id)sender;
- (void)clearMIDISysExForCurrentHost:(id)sender;
- (void)adjustFrameHeight:(int)delta;

// if |row| is negative, append the subview to the end.
- (void)addSubViewForListItem:(bool)hasLink
                        title:(NSString*)title
                        image:(NSImage*)image
                          row:(int)row;
@end

class ContentSettingBubbleModelOwnerBridge
    : public ContentSettingBubbleModel::Owner {
 public:
  ContentSettingBubbleModelOwnerBridge(
      std::unique_ptr<ContentSettingBubbleModel> model,
      ContentSettingBubbleController* controller)
      : model_(std::move(model)), controller_(controller) {
    model_->set_owner(this);
  }
  ~ContentSettingBubbleModelOwnerBridge() override = default;

  ContentSettingBubbleModel* model() const { return model_.get(); }

 private:
  void OnListItemAdded(
      const ContentSettingBubbleModel::ListItem& item) override {
    [controller_ adjustFrameHeight:kLinkLineHeight];

    bool hasLink = item.has_link;
    NSString* title = base::SysUTF16ToNSString(item.title);
    NSImage* image = hasLink ? item.image.AsNSImage() : nil;
    [controller_ addSubViewForListItem:hasLink title:title image:image row:-1];
  }

  void OnListItemRemovedAt(int index) override {
    // Do nothing. If a list item is removed from popup blocker,
    // this bubble will disappear.
  }

  std::unique_ptr<ContentSettingBubbleModel> model_;

  // |controller_| owns this object and can therefore be a raw pointer.
  ContentSettingBubbleController* controller_;

  DISALLOW_COPY_AND_ASSIGN(ContentSettingBubbleModelOwnerBridge);
};

@implementation ContentSettingBubbleController

+ (ContentSettingBubbleController*)
showForModel:(ContentSettingBubbleModel*)contentSettingBubbleModel
 webContents:(content::WebContents*)webContents
parentWindow:(NSWindow*)parentWindow
  decoration:(ContentSettingDecoration*)decoration
  anchoredAt:(NSPoint)anchor {
  // Autoreleases itself on bubble close.
  ContentSettingBubbleController* controller =
      [self allocControllerForModel:contentSettingBubbleModel];

  DCHECK(controller);

  return [controller initWithModel:contentSettingBubbleModel
                       webContents:webContents
                      parentWindow:parentWindow
                        decoration:decoration
                        anchoredAt:anchor];
}

struct ContentTypeToNibPath {
  ContentSettingsType type;
  NSString* path;
};

const ContentTypeToNibPath kNibPaths[] = {
    {CONTENT_SETTINGS_TYPE_COOKIES, @"ContentBlockedCookies"},
    {CONTENT_SETTINGS_TYPE_IMAGES, @"ContentBlockedSimple"},
    {CONTENT_SETTINGS_TYPE_JAVASCRIPT, @"ContentBlockedSimple"},
    {CONTENT_SETTINGS_TYPE_PPAPI_BROKER, @"ContentBlockedSimple"},
    {CONTENT_SETTINGS_TYPE_SOUND, @"ContentBlockedSimple"},
    {CONTENT_SETTINGS_TYPE_POPUPS, @"ContentBlockedPopups"},
    {CONTENT_SETTINGS_TYPE_GEOLOCATION, @"ContentBlockedGeolocation"},
    {CONTENT_SETTINGS_TYPE_MIXEDSCRIPT, @"ContentBlockedMixedScript"},
    {CONTENT_SETTINGS_TYPE_PROTOCOL_HANDLERS, @"ContentProtocolHandlers"},
    {CONTENT_SETTINGS_TYPE_MIDI_SYSEX, @"ContentBlockedMIDISysEx"},
    {CONTENT_SETTINGS_TYPE_CLIPBOARD_READ, @"ContentBlockedSimple"},
    {CONTENT_SETTINGS_TYPE_SENSORS, @"ContentBlockedSimple"},
};

- (id)initWithModel:(ContentSettingBubbleModel*)contentSettingBubbleModel
        webContents:(content::WebContents*)webContents
       parentWindow:(NSWindow*)parentWindow
         decoration:(ContentSettingDecoration*)decoration
         anchoredAt:(NSPoint)anchoredAt {
  // This method takes ownership of |contentSettingBubbleModel| in all cases.
  std::unique_ptr<ContentSettingBubbleModel> model(contentSettingBubbleModel);
  DCHECK(model.get());
  observerBridge_.reset(
    new ContentSettingBubbleWebContentsObserverBridge(webContents, self));

  NSString* nibPath =
      [ContentSettingBubbleController getNibPathForModel:model.get()];

  DCHECK_NE(0u, [nibPath length]);

  if ((self = [super initWithWindowNibPath:nibPath
                              parentWindow:parentWindow
                                anchoredAt:anchoredAt])) {
    modelOwnerBridge_.reset(
        new ContentSettingBubbleModelOwnerBridge(std::move(model), self));
    decoration_ = decoration;
    [self showWindow:nil];
  }
  return self;
}

- (id)initWithModel:(ContentSettingBubbleModel*)contentSettingBubbleModel
        webContents:(content::WebContents*)webContents
             window:(NSWindow*)window
       parentWindow:(NSWindow*)parentWindow
         decoration:(ContentSettingDecoration*)decoration
         anchoredAt:(NSPoint)anchoredAt {
  // This method takes ownership of |contentSettingBubbleModel| in all cases.
  std::unique_ptr<ContentSettingBubbleModel> model(contentSettingBubbleModel);
  DCHECK(model.get());
  observerBridge_.reset(
      new ContentSettingBubbleWebContentsObserverBridge(webContents, self));

  modelOwnerBridge_.reset(
      new ContentSettingBubbleModelOwnerBridge(std::move(model), self));

  if ((self = [super initWithWindow:window
                       parentWindow:parentWindow
                         anchoredAt:anchoredAt])) {
    decoration_ = decoration;
    [self showWindow:nil];
  }
  return self;
}

+ (NSString*)getNibPathForModel:(ContentSettingBubbleModel*)model {
  NSString* nibPath = @"";

  ContentSettingSimpleBubbleModel* simple_bubble = model->AsSimpleBubbleModel();
  if (simple_bubble) {
    ContentSettingsType settingsType = simple_bubble->content_type();

    for (const ContentTypeToNibPath& type_to_path : kNibPaths) {
      if (settingsType == type_to_path.type) {
        nibPath = type_to_path.path;
        break;
      }
    }
  }

  if (model->AsMediaStreamBubbleModel())
    nibPath = @"ContentBlockedMedia";

  if (model->AsDownloadsBubbleModel())
    nibPath = @"ContentBlockedDownloads";
  return nibPath;
}

+ (ContentSettingBubbleController*)allocControllerForModel:
    (ContentSettingBubbleModel*)model {
  // Check if the view is expressed in xib file or not.
  NSString* nibPath = [self getNibPathForModel:model];

  // Autoreleases itself on bubble close.

  if ([nibPath length] > 0u)
    return [ContentSettingBubbleController alloc];

  if (model->AsSubresourceFilterBubbleModel())
    return [SubresourceFilterBubbleController alloc];

  if (model->AsSimpleBubbleModel() &&
      model->AsSimpleBubbleModel()->content_type() ==
          CONTENT_SETTINGS_TYPE_PLUGINS) {
    return [BlockedPluginBubbleController alloc];
  }

  return nil;
}

- (void)initializeTitle {
  if (!titleLabel_)
    return;

  NSString* label =
      base::SysUTF16ToNSString([self model]->bubble_content().title);
  [titleLabel_ setStringValue:label];

  // Layout title post-localization.
  CGFloat deltaY = [GTMUILocalizerAndLayoutTweaker
      sizeToFitFixedWidthTextField:titleLabel_];
  NSRect windowFrame = [[self window] frame];
  windowFrame.size.height += deltaY;
  [[self window] setFrame:windowFrame display:NO];
  NSRect titleFrame = [titleLabel_ frame];
  titleFrame.origin.y -= deltaY;
  [titleLabel_ setFrame:titleFrame];
  [titleLabel_ setAlignment:NSNaturalTextAlignment];
}

- (void)initializeMessage {
  if (!messageLabel_)
    return;

  NSString* label =
      base::SysUTF16ToNSString([self model]->bubble_content().message);
  [messageLabel_ setStringValue:label];

  CGFloat deltaY = [GTMUILocalizerAndLayoutTweaker
      sizeToFitFixedWidthTextField:messageLabel_];
  NSRect windowFrame = [[self window] frame];
  windowFrame.size.height += deltaY;
  [[self window] setFrame:windowFrame display:NO];
  NSRect messageFrame = [messageLabel_ frame];
  messageFrame.origin.y -= deltaY;
  [messageLabel_ setFrame:messageFrame];
  [messageLabel_ setAlignment:NSNaturalTextAlignment];
}

- (void)initializeRadioGroup {
  // NOTE! Tags in the xib files must match the order of the radio buttons
  // passed in the radio_group and be 1-based, not 0-based.
  const ContentSettingBubbleModel::BubbleContent& bubble_content =
      [self model]->bubble_content();
  const ContentSettingBubbleModel::RadioGroup& radio_group =
      bubble_content.radio_group;

  // Xcode 5.1 Interface Builder doesn't allow a font property to be set for
  // NSMatrix. The implementation of GTMUILocalizerAndLayoutTweaker assumes that
  // the font for each of the cells in a NSMatrix is identical, and is the font
  // of the NSMatrix. This logic sets the font of NSMatrix to be that of its
  // cells.
  NSFont* font = nil;
  for (NSCell* cell in [allowBlockRadioGroup_ cells]) {
    if (!font)
      font = [cell font];
    DCHECK([font isEqual:[cell font]]);
  }
  [allowBlockRadioGroup_ setFont:font];
  [allowBlockRadioGroup_ setEnabled:bubble_content.radio_group_enabled];

  // Select appropriate radio button.
  [allowBlockRadioGroup_ selectCellWithTag: radio_group.default_item + 1];

  const ContentSettingBubbleModel::RadioItems& radio_items =
      radio_group.radio_items;
  for (size_t ii = 0; ii < radio_group.radio_items.size(); ++ii) {
    NSCell* radioCell = [allowBlockRadioGroup_ cellWithTag:ii + 1];
    [radioCell setTitle:base::SysUTF16ToNSString(radio_items[ii])];
  }

  // Layout radio group labels post-localization.
  [GTMUILocalizerAndLayoutTweaker
      wrapRadioGroupForWidth:allowBlockRadioGroup_];
  CGFloat radioDeltaY = [GTMUILocalizerAndLayoutTweaker
      sizeToFitView:allowBlockRadioGroup_].height;
  NSRect windowFrame = [[self window] frame];
  windowFrame.size.height += radioDeltaY;
  [[self window] setFrame:windowFrame display:NO];

  // NSMatrix-based radio buttons don't get automatically flipped for
  // RTL. Setting the user interface layout direction explicitly
  // doesn't affect rendering, so set image position and text alignment
  // manually.
  if (cocoa_l10n_util::ShouldDoExperimentalRTLLayout())
    for (NSButtonCell* cell in [allowBlockRadioGroup_ cells]) {
      [cell setAlignment:NSNaturalTextAlignment];
      [cell setImagePosition:cocoa_l10n_util::LeadingCellImagePosition()];
      // Why not?
      [cell setUserInterfaceLayoutDirection:
                NSUserInterfaceLayoutDirectionRightToLeft];
    }
}

- (NSButton*)hyperlinkButtonWithFrame:(NSRect)frame
                                title:(NSString*)title
                                 icon:(NSImage*)icon
                       referenceFrame:(NSRect)referenceFrame {
  base::scoped_nsobject<HyperlinkButtonCell> cell(
      [[HyperlinkButtonCell alloc] initTextCell:title]);
  [cell.get() setAlignment:NSNaturalTextAlignment];
  if (icon) {
    [cell.get() setImagePosition:NSImageLeft];
    [cell.get() setImage:icon];
  } else {
    [cell.get() setImagePosition:NSNoImage];
  }
  [cell.get() setControlSize:NSSmallControlSize];

  NSButton* button = [[[NSButton alloc] initWithFrame:frame] autorelease];
  // Cell must be set immediately after construction.
  [button setCell:cell.get()];

  // Size to fit the button and add a little extra padding for the small-text
  // hyperlink button, which sizeToFit gets wrong.
  [GTMUILocalizerAndLayoutTweaker sizeToFitView:button];
  NSRect buttonFrame = [button frame];
  buttonFrame.size.width += 2;

  // If the link text is too long, clamp it.
  int maxWidth = NSWidth([[self bubble] frame]) - 2 * NSMinX(referenceFrame);
  if (NSWidth(buttonFrame) > maxWidth)
    buttonFrame.size.width = maxWidth;

  [button setFrame:buttonFrame];
  [button setTarget:self];
  [button setAction:@selector(popupLinkClicked:)];
  return button;
}

- (void)initializeItemList {
  // I didn't put the buttons into a NSMatrix because then they are only one
  // entity in the key view loop. This way, one can tab through all of them.
  const ContentSettingBubbleModel::ListItems& listItems =
      [self model]->bubble_content().list_items;

  // Get the pre-resize frame of the radio group. Its origin is where the
  // popup list should go.
  NSRect radioFrame = [allowBlockRadioGroup_ frame];
  topLinkY_ = NSMaxY(radioFrame) - kLinkHeight;

  // Make room for the popup list. The bubble view and its subviews autosize
  // themselves when the window is enlarged.
  // Heading and radio box are already 1 * kLinkOuterPadding apart in the nib,
  // so only 1 * kLinkOuterPadding more is needed.
  int delta =
      listItems.size() * kLinkLineHeight - kLinkPadding + kLinkOuterPadding;
  [self adjustFrameHeight:delta];

  // Create item list.
  int row = 0;
  for (const ContentSettingBubbleModel::ListItem& listItem : listItems) {
    bool hasLink = listItem.has_link;
    NSString* title = base::SysUTF16ToNSString(listItem.title);
    NSImage* image = hasLink ? listItem.image.AsNSImage() : nil;
    [self addSubViewForListItem:hasLink title:title image:image row:row];
    row++;
  }
}

- (void)initializeGeoLists {
  // Cocoa has its origin in the lower left corner. This means elements are
  // added from bottom to top, which explains why loops run backwards and the
  // order of operations is the other way than on Linux/Windows.
  const ContentSettingBubbleModel::BubbleContent& content =
      [self model]->bubble_content();
  NSRect containerFrame = [contentsContainer_ frame];
  NSRect frame = NSMakeRect(0, 0, NSWidth(containerFrame), kGeoLabelHeight);

  // "Clear" button / text field.
  if (!content.custom_link.empty()) {
    base::scoped_nsobject<NSControl> control;
    if(content.custom_link_enabled) {
      NSRect buttonFrame = NSMakeRect(0, 0,
                                      NSWidth(containerFrame),
                                      kGeoClearButtonHeight);
      NSButton* button = [[NSButton alloc] initWithFrame:buttonFrame];
      control.reset(button);
      [button setTitle:base::SysUTF16ToNSString(content.custom_link)];
      [button setTarget:self];
      [button setAction:@selector(clearGeolocationForCurrentHost:)];
      [button setBezelStyle:NSRoundRectBezelStyle];
      SetControlSize(button, NSSmallControlSize);
      [button sizeToFit];
    } else {
      // Add the notification that settings will be cleared on next reload.
      control.reset([LabelWithFrame(
          base::SysUTF16ToNSString(content.custom_link), frame) retain]);
      SetControlSize(control.get(), NSSmallControlSize);
    }

    // If the new control is wider than the container, widen the window.
    CGFloat controlWidth = NSWidth([control frame]);
    if (controlWidth > NSWidth(containerFrame)) {
      NSRect windowFrame = [[self window] frame];
      windowFrame.size.width += controlWidth - NSWidth(containerFrame);
      [[self window] setFrame:windowFrame display:NO];
      // Fetch the updated sizes.
      containerFrame = [contentsContainer_ frame];
      frame = NSMakeRect(0, 0, NSWidth(containerFrame), kGeoLabelHeight);
    }

    DCHECK(control);
    [contentsContainer_ addSubview:control];
    frame.origin.y = NSMaxY([control frame]) + kGeoPadding;
  }

  for (auto i = content.domain_lists.rbegin();
       i != content.domain_lists.rend(); ++i) {
    // Add all hosts in the current domain list.
    for (auto j = i->hosts.rbegin(); j != i->hosts.rend(); ++j) {
      NSTextField* title = LabelWithFrame(base::SysUTF8ToNSString(*j), frame);
      SetControlSize(title, NSSmallControlSize);
      [contentsContainer_ addSubview:title];

      frame.origin.y = NSMaxY(frame) + kGeoHostPadding +
          [GTMUILocalizerAndLayoutTweaker sizeToFitFixedWidthTextField:title];
    }
    if (!i->hosts.empty())
      frame.origin.y += kGeoPadding - kGeoHostPadding;

    // Add the domain list's title.
    NSTextField* title =
        LabelWithFrame(base::SysUTF16ToNSString(i->title), frame);
    SetControlSize(title, NSSmallControlSize);
    [contentsContainer_ addSubview:title];

    frame.origin.y = NSMaxY(frame) + kGeoPadding +
        [GTMUILocalizerAndLayoutTweaker sizeToFitFixedWidthTextField:title];
  }

  CGFloat containerHeight = frame.origin.y;
  // Undo last padding.
  if (!content.domain_lists.empty())
    containerHeight -= kGeoPadding;

  // Resize container to fit its subviews, and window to fit the container.
  NSRect windowFrame = [[self window] frame];
  windowFrame.size.height += containerHeight - NSHeight(containerFrame);
  [[self window] setFrame:windowFrame display:NO];
  containerFrame.size.height = containerHeight;
  [contentsContainer_ setFrame:containerFrame];
}

- (void)initializeMediaMenus {
  const ContentSettingBubbleModel::MediaMenuMap& media_menus =
      [self model]->bubble_content().media_menus;

  // Calculate the longest width of the labels and menus menus to avoid
  // truncation by the window's edge.
  CGFloat maxLabelWidth = 0;
  CGFloat maxMenuWidth = 0;
  CGFloat maxMenuHeight = 0;
  NSRect radioFrame = [allowBlockRadioGroup_ frame];
  for (const std::pair<content::MediaStreamType,
                       ContentSettingBubbleModel::MediaMenu>& map_entry :
       media_menus) {
    // |labelFrame| will be resized later on in this function.
    NSRect labelFrame = NSMakeRect(NSMinX(radioFrame), 0, 0, 0);
    NSTextField* label = LabelWithFrame(
        base::SysUTF16ToNSString(map_entry.second.label), labelFrame);
    SetControlSize(label, NSSmallControlSize);
    NSCell* cell = [label cell];
    [cell setAlignment:NSRightTextAlignment];
    [GTMUILocalizerAndLayoutTweaker sizeToFitView:label];
    maxLabelWidth = std::max(maxLabelWidth, [label frame].size.width);
    [[self bubble] addSubview:label];

    // |buttonFrame| will be resized and repositioned later on.
    NSRect buttonFrame = NSMakeRect(NSMinX(radioFrame), 0, 0, 0);
    base::scoped_nsobject<NSPopUpButton> button(
        [[NSPopUpButton alloc] initWithFrame:buttonFrame]);
    [button setTarget:self];

    // Set the map_entry's key value to |button| tag.
    // MediaMenuPartsMap uses this value to order its elements.
    [button setTag:static_cast<NSInteger>(map_entry.first)];

    // Store the |label| and |button| into MediaMenuParts struct and build
    // the popup menu from the menu model.
    content_setting_bubble::MediaMenuParts* menuParts =
        new content_setting_bubble::MediaMenuParts(map_entry.first, label);
    menuParts->model.reset(new ContentSettingMediaMenuModel(
        map_entry.first, [self model],
        ContentSettingMediaMenuModel::MenuLabelChangedCallback()));
    mediaMenus_[button] = base::WrapUnique(menuParts);
    CGFloat width = BuildPopUpMenuFromModel(
        button, menuParts->model.get(), map_entry.second.selected_device.name,
        map_entry.second.disabled);
    maxMenuWidth = std::max(maxMenuWidth, width);

    [[self bubble] addSubview:button
                   positioned:NSWindowBelow
                   relativeTo:nil];

    maxMenuHeight = std::max(maxMenuHeight, [button frame].size.height);
  }

  // Make room for the media menu(s) and enlarge the windows to fit the views.
  // The bubble view and its subviews autosize themselves when the window is
  // enlarged.
  int delta = media_menus.size() * maxMenuHeight +
      (media_menus.size() - 1) * kMediaMenuElementVerticalPadding;
  NSSize deltaSize =
      [[[self window] contentView] convertSize:NSMakeSize(0, delta) toView:nil];
  NSRect windowFrame = [[self window] frame];
  windowFrame.size.height += deltaSize.height;
  // If the media menus are wider than the window, widen the window.
  CGFloat widthNeeded = maxLabelWidth + maxMenuWidth + 2 * NSMinX(radioFrame);
  if (widthNeeded > windowFrame.size.width)
    windowFrame.size.width = widthNeeded;
  [[self window] setFrame:windowFrame display:NO];

  // The radio group lies above the media menus, move the radio group up.
  radioFrame.origin.y += delta;
  [allowBlockRadioGroup_ setFrame:radioFrame];

  // Resize and reposition the media menus layout.
  CGFloat topMenuY = NSMinY(radioFrame) - kMediaMenuVerticalPadding;
  maxMenuWidth = std::max(maxMenuWidth, kMinMediaMenuButtonWidth);
  for (const auto& map_entry : mediaMenus_) {
    NSRect labelFrame = [map_entry.second->label frame];
    // Align the label text with the button text.
    labelFrame.origin.y =
        topMenuY + (maxMenuHeight - labelFrame.size.height) / 2 + 1;
    labelFrame.size.width = maxLabelWidth;
    [map_entry.second->label setFrame:labelFrame];
    NSRect menuFrame = [map_entry.first frame];
    menuFrame.origin.y = topMenuY;
    menuFrame.origin.x = NSMinX(radioFrame) + maxLabelWidth;
    menuFrame.size.width = maxMenuWidth;
    menuFrame.size.height = maxMenuHeight;
    [map_entry.first setFrame:menuFrame];
    topMenuY -= (maxMenuHeight + kMediaMenuElementVerticalPadding);
  }
}

- (void)initializeMIDISysExLists {
  const ContentSettingBubbleModel::BubbleContent& content =
      [self model]->bubble_content();
  NSRect containerFrame = [contentsContainer_ frame];
  NSRect frame =
      NSMakeRect(0, 0, NSWidth(containerFrame), kMIDISysExLabelHeight);

  // "Clear" button / text field.
  if (!content.custom_link.empty()) {
    base::scoped_nsobject<NSControl> control;
    if (content.custom_link_enabled) {
      NSRect buttonFrame = NSMakeRect(0, 0,
                                      NSWidth(containerFrame),
                                      kMIDISysExClearButtonHeight);
      NSButton* button = [[NSButton alloc] initWithFrame:buttonFrame];
      control.reset(button);
      [button setTitle:base::SysUTF16ToNSString(content.custom_link)];
      [button setTarget:self];
      [button setAction:@selector(clearMIDISysExForCurrentHost:)];
      [button setBezelStyle:NSRoundRectBezelStyle];
      SetControlSize(button, NSSmallControlSize);
      [button sizeToFit];
    } else {
      // Add the notification that settings will be cleared on next reload.
      control.reset([LabelWithFrame(
          base::SysUTF16ToNSString(content.custom_link), frame) retain]);
      SetControlSize(control.get(), NSSmallControlSize);
    }

    // If the new control is wider than the container, widen the window.
    CGFloat controlWidth = NSWidth([control frame]);
    if (controlWidth > NSWidth(containerFrame)) {
      NSRect windowFrame = [[self window] frame];
      windowFrame.size.width += controlWidth - NSWidth(containerFrame);
      [[self window] setFrame:windowFrame display:NO];
      // Fetch the updated sizes.
      containerFrame = [contentsContainer_ frame];
      frame = NSMakeRect(0, 0, NSWidth(containerFrame), kMIDISysExLabelHeight);
    }

    DCHECK(control);
    [contentsContainer_ addSubview:control];
    frame.origin.y = NSMaxY([control frame]) + kMIDISysExPadding;
  }

  for (auto i = content.domain_lists.rbegin();
       i != content.domain_lists.rend(); ++i) {
    // Add all hosts in the current domain list.
    for (auto j = i->hosts.rbegin(); j != i->hosts.rend(); ++j) {
      NSTextField* title = LabelWithFrame(base::SysUTF8ToNSString(*j), frame);
      SetControlSize(title, NSSmallControlSize);
      [contentsContainer_ addSubview:title];

      frame.origin.y = NSMaxY(frame) + kMIDISysExHostPadding +
          [GTMUILocalizerAndLayoutTweaker sizeToFitFixedWidthTextField:title];
    }
    if (!i->hosts.empty())
      frame.origin.y += kMIDISysExPadding - kMIDISysExHostPadding;

    // Add the domain list's title.
    NSTextField* title =
        LabelWithFrame(base::SysUTF16ToNSString(i->title), frame);
    SetControlSize(title, NSSmallControlSize);
    [contentsContainer_ addSubview:title];

    frame.origin.y = NSMaxY(frame) + kMIDISysExPadding +
        [GTMUILocalizerAndLayoutTweaker sizeToFitFixedWidthTextField:title];
  }

  CGFloat containerHeight = frame.origin.y;
  // Undo last padding.
  if (!content.domain_lists.empty())
    containerHeight -= kMIDISysExPadding;

  // Resize container to fit its subviews, and window to fit the container.
  NSRect windowFrame = [[self window] frame];
  windowFrame.size.height += containerHeight - NSHeight(containerFrame);
  [[self window] setFrame:windowFrame display:NO];
  containerFrame.size.height = containerHeight;
  [contentsContainer_ setFrame:containerFrame];
}

- (void)initManageDoneButtons {
  if (!manageButton_ && !doneButton_)
    return;

  const ContentSettingBubbleModel::BubbleContent& content =
      [self model]->bubble_content();

  CGFloat requiredWidthForManageButton = 0.0;
  if (manageButton_) {
    [manageButton_ setTitle:base::SysUTF16ToNSString(content.manage_text)];
    [GTMUILocalizerAndLayoutTweaker sizeToFitView:[manageButton_ superview]];
    requiredWidthForManageButton =
        NSMaxX([manageButton_ frame]) + kManageDonePadding;
  }

  if (!doneButton_)
    return;

  NSString* doneLabel = base::SysUTF16ToNSString(content.done_button_text);
  if ([doneLabel length] > 0u)
    [doneButton_ setTitle:doneLabel];

  CGFloat actualWidth = NSWidth([[[self window] contentView] frame]);
  CGFloat requiredWidth = requiredWidthForManageButton +
                          NSWidth([[doneButton_ superview] frame]) -
                          NSMinX([doneButton_ frame]);
  if (requiredWidth <= actualWidth || !doneButton_ || !manageButton_)
    return;

  // Resize window, autoresizing takes care of the rest.
  NSSize size = NSMakeSize(requiredWidth - actualWidth, 0);
  size = [[[self window] contentView] convertSize:size toView:nil];
  NSRect frame = [[self window] frame];
  frame.origin.x -= size.width;
  frame.size.width += size.width;
  [[self window] setFrame:frame display:NO];
}

- (void)awakeFromNib {
  [super awakeFromNib];
  [self layoutView];
}

- (NSTouchBar*)makeTouchBar {
  if (!base::FeatureList::IsEnabled(features::kDialogTouchBar))
    return nil;

  if (!manageButton_ && !doneButton_)
    return nil;

  base::scoped_nsobject<NSTouchBar> touchBar([[ui::NSTouchBar() alloc] init]);
  [touchBar setCustomizationIdentifier:ui::GetTouchBarId(
                                           kContentSettingsBubbleTouchBarId)];
  [touchBar setDelegate:self];

  NSMutableArray* dialogItems = [NSMutableArray array];
  if (manageButton_) {
    [dialogItems
        addObject:ui::GetTouchBarItemId(kContentSettingsBubbleTouchBarId,
                                        kManageTouchBarId)];
  }

  if (doneButton_) {
    [dialogItems
        addObject:ui::GetTouchBarItemId(kContentSettingsBubbleTouchBarId,
                                        kDoneTouchBarId)];
  }

  [touchBar setDefaultItemIdentifiers:dialogItems];
  [touchBar setCustomizationAllowedItemIdentifiers:dialogItems];
  return touchBar.autorelease();
}

- (NSTouchBarItem*)touchBar:(NSTouchBar*)touchBar
      makeItemForIdentifier:(NSTouchBarItemIdentifier)identifier
    API_AVAILABLE(macos(10.12.2)) {
  NSButton* button = nil;
  if ([identifier hasSuffix:kManageTouchBarId]) {
    NSString* title =
        base::SysUTF16ToNSString([self model]->bubble_content().manage_text);
    button = [NSButton buttonWithTitle:title
                                target:self
                                action:@selector(manageBlocking:)];
  } else if ([identifier hasSuffix:kDoneTouchBarId]) {
    button = ui::GetBlueTouchBarButton(l10n_util::GetNSString(IDS_DONE), self,
                                       @selector(closeBubble:));
  } else {
    return nil;
  }

  base::scoped_nsobject<NSCustomTouchBarItem> item(
      [[ui::NSCustomTouchBarItem() alloc] initWithIdentifier:identifier]);
  [item setView:button];
  return item.autorelease();
}

- (void)layoutView {
  ContentSettingSimpleBubbleModel* simple_bubble =
      [self model]->AsSimpleBubbleModel();

  [[self bubble] setArrowLocation:info_bubble::kTopTrailing];

  // Adapt window size to bottom buttons. Do this before all other layouting.
  if ((simple_bubble && !simple_bubble->bubble_content().manage_text.empty()) ||
      [self model]->AsDownloadsBubbleModel() ||
      [self model]->AsSubresourceFilterBubbleModel()) {
    [self initManageDoneButtons];
  }

  [self initializeTitle];
  [self initializeMessage];

  if (allowBlockRadioGroup_)  // Some xibs do not bind |allowBlockRadioGroup_|.
    [self initializeRadioGroup];

  if (simple_bubble) {
    ContentSettingsType type = simple_bubble->content_type();

    if (type == CONTENT_SETTINGS_TYPE_POPUPS)
      [self initializeItemList];
    if (type == CONTENT_SETTINGS_TYPE_GEOLOCATION)
      [self initializeGeoLists];
    if (type == CONTENT_SETTINGS_TYPE_MIDI_SYSEX)
      [self initializeMIDISysExLists];

    // For plugins, many controls are now removed. Remove them after the item
    // list has been placed to preserve the existing layout logic.
    if (type == CONTENT_SETTINGS_TYPE_PLUGINS) {
      // The radio group is no longer applicable to plugins.
      int delta = NSHeight([allowBlockRadioGroup_ frame]);
      [allowBlockRadioGroup_ removeFromSuperview];

      // Remove the "Load All" button if the model specifes it as empty.
      if (simple_bubble->bubble_content().custom_link.empty()) {
        delta += NSHeight([loadButton_ frame]);
        [loadButton_ removeFromSuperview];
      }

      // Remove the "Manage" button if the model specifies it as empty.
      if (simple_bubble->bubble_content().manage_text.empty())
        [manageButton_ removeFromSuperview];

      NSRect frame = [[self window] frame];
      frame.size.height -= delta;
      [[self window] setFrame:frame display:NO];
    }
  }

  if ([self model]->AsMediaStreamBubbleModel())
    [self initializeMediaMenus];

  // RTL-ize NIBS:
  if (cocoa_l10n_util::ShouldDoExperimentalRTLLayout()) {
    cocoa_l10n_util::FlipAllSubviewsIfNecessary([self bubble]);

    // Some NIBs have the manage/done buttons outside of the bubble.
    cocoa_l10n_util::FlipAllSubviewsIfNecessary([[self bubble] superview]);
    cocoa_l10n_util::FlipAllSubviewsIfNecessary(contentsContainer_);

    // These buttons are inside |GTMWidthBasedTweaker|s, so fix margins.
    cocoa_l10n_util::FlipAllSubviewsIfNecessary([infoButton_ superview]);
    cocoa_l10n_util::FlipAllSubviewsIfNecessary([doneButton_ superview]);
    cocoa_l10n_util::FlipAllSubviewsIfNecessary([manageButton_ superview]);
  }
}

///////////////////////////////////////////////////////////////////////////////
// Actual application logic

- (IBAction)allowBlockToggled:(id)sender {
  NSButtonCell *selectedCell = [sender selectedCell];
  [self model]->OnRadioClicked([selectedCell tag] - 1);
}

- (void)popupLinkClicked:(id)sender {
  content_setting_bubble::PopupLinks::iterator i(popupLinks_.find(sender));
  DCHECK(i != popupLinks_.end());
  const int event_flags = ui::EventFlagsFromModifiers([NSEvent modifierFlags]);
  [self model]->OnListItemClicked(i->second, event_flags);
}

- (void)clearGeolocationForCurrentHost:(id)sender {
  [self model]->OnCustomLinkClicked();
  [self close];
}

- (void)clearMIDISysExForCurrentHost:(id)sender {
  [self model]->OnCustomLinkClicked();
  [self close];
}

- (void)adjustFrameHeight:(int)delta {
  topLinkY_ += delta;

  NSSize deltaSize =
      [[[self window] contentView] convertSize:NSMakeSize(0, delta) toView:nil];
  NSRect windowFrame = [[self window] frame];
  windowFrame.size.height += deltaSize.height;
  windowFrame.origin.y -= deltaSize.height;
  [[self window] setFrame:windowFrame display:NO];
}

- (void)addSubViewForListItem:(bool)hasLink
                        title:(NSString*)title
                        image:(NSImage*)image
                          row:(int)row {
  if (row < 0)
    row = [self model]->bubble_content().list_items.size() - 1;

  NSRect referenceFrame = [allowBlockRadioGroup_ frame];
  NSRect frame =
      NSMakeRect(NSMinX(referenceFrame), topLinkY_ - kLinkLineHeight * row, 200,
                 kLinkHeight);
  if (hasLink) {
    NSButton* button = [self hyperlinkButtonWithFrame:frame
                                                title:title
                                                 icon:image
                                       referenceFrame:referenceFrame];
    [button setAutoresizingMask:NSViewMinYMargin];
    [[self bubble] addSubview:button];
    popupLinks_[button] = row;
  } else {
    NSTextField* label = LabelWithFrame(title, frame);
    SetControlSize(label, NSSmallControlSize);
    [label setAutoresizingMask:NSViewMinYMargin];
    [[self bubble] addSubview:label];
  }
}

- (IBAction)showMoreInfo:(id)sender {
  [self model]->OnCustomLinkClicked();
  [self close];
}

- (IBAction)load:(id)sender {
  [self model]->OnCustomLinkClicked();
  [self close];
}

- (IBAction)learnMoreLinkClicked:(id)sender {
  [self model]->OnLearnMoreClicked();
}

- (IBAction)manageBlocking:(id)sender {
  [self model]->OnManageButtonClicked();
}

- (IBAction)closeBubble:(id)sender {
  [self model]->OnDoneClicked();
  [self close];
}

- (IBAction)mediaMenuChanged:(id)sender {
  NSPopUpButton* button = static_cast<NSPopUpButton*>(sender);
  auto it = mediaMenus_.find(sender);
  DCHECK(it != mediaMenus_.end());
  NSInteger index = [[button selectedItem] tag];

  SetTitleForPopUpButton(
      button, base::SysUTF16ToNSString(it->second->model->GetLabelAt(index)));

  it->second->model->ExecuteCommand(index, 0);
}

- (ContentSettingBubbleModel*)model {
  return modelOwnerBridge_->model();
}

- (content_setting_bubble::MediaMenuPartsMap*)mediaMenus {
  return &mediaMenus_;
}

- (LocationBarDecoration*)decorationForBubble {
  return decoration_;
}

@end  // ContentSettingBubbleController
