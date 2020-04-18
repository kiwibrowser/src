// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/extensions/extension_install_view_controller.h"

#include <stddef.h>

#include <utility>

#include "base/auto_reset.h"
#include "base/i18n/rtl.h"
#include "base/mac/bundle_locations.h"
#include "base/mac/mac_util.h"
#include "base/strings/string_util.h"
#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#import "chrome/browser/ui/cocoa/chrome_style.h"
#include "chrome/browser/ui/scoped_tabbed_browser_displayer.h"
#include "chrome/common/extensions/extension_constants.h"
#include "chrome/grit/generated_resources.h"
#include "content/public/browser/page_navigator.h"
#include "extensions/common/extension.h"
#include "extensions/common/extension_urls.h"
#include "skia/ext/skia_utils_mac.h"
#import "third_party/google_toolbox_for_mac/src/AppKit/GTMUILocalizerAndLayoutTweaker.h"
#import "ui/base/cocoa/a11y_util.h"
#import "ui/base/cocoa/controls/hyperlink_button_cell.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/l10n/l10n_util_mac.h"
#include "ui/gfx/image/image_skia_util_mac.h"

using base::SysUTF16ToNSString;
using content::OpenURLParams;
using content::Referrer;

namespace {

// A collection of attributes (bitmask) for how to draw a cell, the expand
// marker and the text in the cell.
enum CellAttributesMask {
  kBoldText                = 1 << 0,
  kNoExpandMarker          = 1 << 1,
  kUseBullet               = 1 << 2,
  kAutoExpandCell          = 1 << 3,
  kUseCustomLinkCell       = 1 << 4,
  kCanExpand               = 1 << 5,
};

typedef NSUInteger CellAttributes;

}  // namespace.

@interface ExtensionInstallViewController ()
- (BOOL)hasWebstoreData;
- (void)appendRatingStar:(const gfx::ImageSkia*)skiaImage;
- (void)onOutlineViewRowCountDidChange;
- (NSDictionary*)buildItemWithTitle:(NSString*)title
                     cellAttributes:(CellAttributes)cellAttributes
                           children:(NSArray*)children;
- (NSDictionary*)buildDetailToggleItem:(size_t)type
                 permissionsDetailIndex:(size_t)index;
- (NSArray*)buildWarnings:(const ExtensionInstallPrompt::Prompt&)prompt;
// Adds permissions of |type| from |prompt| to |children| and returns the
// the appropriate permissions header. If no permissions are found, NULL is
// returned.
- (NSString*)
appendPermissionsForPrompt:(const ExtensionInstallPrompt::Prompt&)prompt
                  children:(NSMutableArray*)children;
- (void)updateViewFrame:(NSRect)frame;
@end

@interface DetailToggleHyperlinkButtonCell : HyperlinkButtonCell {
  NSUInteger permissionsDetailIndex_;
  ExtensionInstallPrompt::DetailsType permissionsDetailType_;
  SEL linkClickedAction_;
}

@property(assign, nonatomic) NSUInteger permissionsDetailIndex;
@property(assign, nonatomic)
    ExtensionInstallPrompt::DetailsType permissionsDetailType;
@property(assign, nonatomic) SEL linkClickedAction;

@end

namespace {

// Padding above the warnings separator, we must also subtract this when hiding
// it.
const CGFloat kWarningsSeparatorPadding = 14;

// The left padding for the link cell.
const CGFloat kLinkCellPaddingLeft = 3;

// Maximum height we will adjust controls to when trying to accomodate their
// contents.
const CGFloat kMaxControlHeight = 250;

NSString* const kTitleKey = @"title";
NSString* const kChildrenKey = @"children";
NSString* const kCellAttributesKey = @"cellAttributes";
NSString* const kPermissionsDetailIndex = @"permissionsDetailIndex";
NSString* const kPermissionsDetailType = @"permissionsDetailType";

// Computes the |control|'s desired height to fit its contents, constrained to
// be kMaxControlHeight at most.
CGFloat ComputeDesiredControlHeight(NSControl* control) {
  NSRect rect = [control frame];
  rect.size.height = kMaxControlHeight;
  return [[control cell] cellSizeForBounds:rect].height;
}

// Adjust the |control|'s height so that its content is not clipped.
// This also adds the change in height to the |total_offset| and shifts the
// control down by that amount.
void OffsetControlVerticallyToFitContent(NSControl* control,
                                         CGFloat* total_offset) {
  // Adjust the control's height so that its content is not clipped.
  NSRect current_rect = [control frame];
  CGFloat desired_height = ComputeDesiredControlHeight(control);
  CGFloat offset = desired_height - NSHeight(current_rect);

  [control setFrameSize:NSMakeSize(NSWidth(current_rect),
                                   NSHeight(current_rect) + offset)];

  *total_offset += offset;

  // Move the control vertically by the new total offset.
  NSPoint origin = [control frame].origin;
  origin.y -= *total_offset;
  [control setFrameOrigin:origin];
}

// Adjust the |view|'s height so that its subviews are not clipped.
// This also adds the change in height to the |total_offset| and shifts the
// control down by that amount.
void OffsetViewVerticallyToFitContent(NSView* view, CGFloat* total_offset) {
  // Adjust the view's height so that its subviews are not clipped.
  CGFloat desired_height = 0;
  for (NSView* subview in [view subviews]) {
    int required_height = NSMaxY([subview frame]);
    if (required_height > desired_height)
      desired_height = required_height;
  }
  NSRect current_rect = [view frame];
  CGFloat offset = desired_height - NSHeight(current_rect);

  [view setFrameSize:NSMakeSize(NSWidth(current_rect),
                                NSHeight(current_rect) + offset)];

  *total_offset += offset;

  // Move the view vertically by the new total offset.
  NSPoint origin = [view frame].origin;
  origin.y -= *total_offset;
  [view setFrameOrigin:origin];
}

// Gets the desired height of |outline_view|. Simply using the view's frame
// doesn't work if an animation is pending.
CGFloat GetDesiredOutlineViewHeight(NSOutlineView* outline_view) {
  CGFloat height = 0;
  for (NSInteger i = 0; i < [outline_view numberOfRows]; ++i)
    height += NSHeight([outline_view rectOfRow:i]);
  return height;
}

void OffsetOutlineViewVerticallyToFitContent(NSOutlineView* outline_view,
                                             CGFloat* total_offset) {
  NSScrollView* scroll_view = [outline_view enclosingScrollView];
  NSRect frame = [scroll_view frame];
  CGFloat desired_height = GetDesiredOutlineViewHeight(outline_view);
  if (desired_height > kMaxControlHeight)
    desired_height = kMaxControlHeight;
  CGFloat offset = desired_height - NSHeight(frame);
  frame.size.height += offset;

  *total_offset += offset;

  // Move the control vertically by the new total offset.
  frame.origin.y -= *total_offset;
  [scroll_view setFrame:frame];
}

void AppendRatingStarsShim(const gfx::ImageSkia* skia_image, void* data) {
  ExtensionInstallViewController* controller =
      static_cast<ExtensionInstallViewController*>(data);
  [controller appendRatingStar:skia_image];
}

void DrawBulletInFrame(NSRect frame) {
  NSRect rect;
  rect.size.width = std::min(NSWidth(frame), NSHeight(frame)) * 0.25;
  rect.size.height = NSWidth(rect);
  rect.origin.x = frame.origin.x + (NSWidth(frame) - NSWidth(rect)) / 2.0;
  rect.origin.y = frame.origin.y + (NSHeight(frame) - NSHeight(rect)) / 2.0;
  rect = NSIntegralRect(rect);

  [[NSColor colorWithCalibratedWhite:0.0 alpha:0.42] set];
  [[NSBezierPath bezierPathWithOvalInRect:rect] fill];
}

bool HasAttribute(id item, CellAttributesMask attributeMask) {
  return [[item objectForKey:kCellAttributesKey] intValue] & attributeMask;
}

}  // namespace

@implementation ExtensionInstallViewController

@synthesize iconView = iconView_;
@synthesize titleField = titleField_;
@synthesize itemsField = itemsField_;
@synthesize cancelButton = cancelButton_;
@synthesize okButton = okButton_;
@synthesize outlineView = outlineView_;
@synthesize warningsSeparator = warningsSeparator_;
@synthesize ratingStars = ratingStars_;
@synthesize ratingCountField = ratingCountField_;
@synthesize userCountField = userCountField_;
@synthesize storeLinkButton = storeLinkButton_;

- (id)initWithProfile:(Profile*)profile
            navigator:(content::PageNavigator*)navigator
             delegate:(ExtensionInstallViewDelegate*)delegate
               prompt:(std::unique_ptr<ExtensionInstallPrompt::Prompt>)prompt {
  // We use a different XIB in the case of installs with webstore data, or no
  // permission warnings. These are laid out nicely for the data they display.
  NSString* nibName = nil;
  if (prompt->has_webstore_data()) {
    nibName = @"ExtensionInstallPromptWebstoreData";
  } else if (!prompt->ShouldShowPermissions() &&
             prompt->GetRetainedFileCount() == 0 &&
             prompt->GetRetainedDeviceCount() == 0) {
    nibName = @"ExtensionInstallPromptNoWarnings";
  } else {
    nibName = @"ExtensionInstallPrompt";
  }

  if ((self = [super initWithNibName:nibName
                              bundle:base::mac::FrameworkBundle()])) {
    profile_ = profile;
    navigator_ = navigator;
    delegate_ = delegate;
    prompt_ = std::move(prompt);
    warnings_.reset([[self buildWarnings:*prompt_] retain]);
  }
  return self;
}

- (IBAction)storeLinkClicked:(id)sender {
  GURL store_url(extension_urls::GetWebstoreItemDetailURLPrefix() +
                 prompt_->extension()->id());
  OpenURLParams params(store_url, Referrer(),
                       WindowOpenDisposition::NEW_FOREGROUND_TAB,
                       ui::PAGE_TRANSITION_LINK, false);
  if (navigator_) {
    navigator_->OpenURL(params);
  } else {
    chrome::ScopedTabbedBrowserDisplayer displayer(profile_);
    displayer.browser()->OpenURL(params);
  }

  delegate_->OnStoreLinkClicked();
}

- (IBAction)cancel:(id)sender {
  delegate_->OnCancelButtonClicked();
}

- (IBAction)ok:(id)sender {
  delegate_->OnOkButtonClicked();
}

- (void)awakeFromNib {
  // Since linking to 10.10, |outlineView_| needs an explicit background to
  // ensure subpixel antialiasing is enabled for the permissions text. At the
  // same time, the animation that shows the prompt breaks whenever the scroll
  // view is present. Giving the scroll view a layer restores the animation, and
  // since its contents has an opaque background, subpixel AA isn't affected.
  [[outlineView_ enclosingScrollView] setWantsLayer:YES];
  [outlineView_ setBackgroundColor:[NSColor whiteColor]];

  // Set control labels.
  [titleField_ setStringValue:base::SysUTF16ToNSString(
      prompt_->GetDialogTitle())];
  NSRect okButtonRect;
  base::string16 acceptButtonLabel = prompt_->GetAcceptButtonLabel();
  if (!acceptButtonLabel.empty()) {
    [okButton_ setTitle:base::SysUTF16ToNSString(acceptButtonLabel)];
  } else {
    [okButton_ removeFromSuperview];
    okButtonRect = [okButton_ frame];
    okButton_ = nil;
  }
  [cancelButton_ setTitle:base::SysUTF16ToNSString(
      prompt_->GetAbortButtonLabel())];
  if ([self hasWebstoreData]) {
    prompt_->AppendRatingStars(AppendRatingStarsShim, self);
    [ratingCountField_ setStringValue:base::SysUTF16ToNSString(
        prompt_->GetRatingCount())];
    [userCountField_ setStringValue:base::SysUTF16ToNSString(
        prompt_->GetUserCount())];
    [[storeLinkButton_ cell] setUnderlineBehavior:
        hyperlink_button_cell::UnderlineBehavior::ON_HOVER];
    [[storeLinkButton_ cell] setTextColor:
        skia::SkColorToCalibratedNSColor(chrome_style::GetLinkColor())];
  }

  [iconView_ setImage:prompt_->icon().ToNSImage()];
  // The icon does not add any additional information for VoiceOver beyond what
  // the title already gives. Ignore the icon in VoiceOver.
  ui::a11y_util::HideImageFromAccessibilityOrder(iconView_);

  // The dialog is laid out in the NIB exactly how we want it assuming that
  // each label fits on one line. However, for each label, we want to allow
  // wrapping onto multiple lines. So we accumulate an offset by measuring how
  // big each label wants to be, and comparing it to how big it actually is.
  // Then we shift each label down and resize by the appropriate amount, then
  // finally resize the window.
  CGFloat totalOffset = 0.0;

  OffsetControlVerticallyToFitContent(titleField_, &totalOffset);

  if ([self hasWebstoreData]) {
    OffsetControlVerticallyToFitContent(ratingCountField_, &totalOffset);
    OffsetViewVerticallyToFitContent(ratingStars_, &totalOffset);
    OffsetControlVerticallyToFitContent(userCountField_, &totalOffset);
    OffsetControlVerticallyToFitContent(storeLinkButton_, &totalOffset);
    NSPoint separatorOrigin = [warningsSeparator_ frame].origin;
    separatorOrigin.y -= totalOffset;
    [warningsSeparator_ setFrameOrigin:separatorOrigin];
  }

  // Resize |okButton_| and |cancelButton_| to fit the button labels, but keep
  // them right-aligned.
  NSSize buttonDelta;
  if (okButton_) {
    buttonDelta = [GTMUILocalizerAndLayoutTweaker sizeToFitView:okButton_];
    if (buttonDelta.width) {
      [okButton_ setFrame:NSOffsetRect([okButton_ frame],
                                       -buttonDelta.width, 0)];
      [cancelButton_ setFrame:NSOffsetRect([cancelButton_ frame],
                                           -buttonDelta.width, 0)];
    }
  } else {
    // Make |cancelButton_| right-aligned in the absence of |okButton_|.
    NSRect cancelButtonRect = [cancelButton_ frame];
    cancelButtonRect.origin.x =
        NSMaxX(okButtonRect) - NSWidth(cancelButtonRect);
    [cancelButton_ setFrame:cancelButtonRect];
  }
  buttonDelta = [GTMUILocalizerAndLayoutTweaker sizeToFitView:cancelButton_];
  if (buttonDelta.width) {
    [cancelButton_ setFrame:NSOffsetRect([cancelButton_ frame],
                                         -buttonDelta.width, 0)];
  }

  // If there are any warnings, retained devices or retained files, then we
  // have to do some special layout.
  if (prompt_->ShouldShowPermissions() || prompt_->GetRetainedFileCount() > 0) {
    NSSize spacing = [outlineView_ intercellSpacing];
    spacing.width += 2;
    spacing.height += 2;
    [outlineView_ setIntercellSpacing:spacing];
    [[[[outlineView_ tableColumns] objectAtIndex:0] dataCell] setWraps:YES];
    for (id item in warnings_.get())
      [self expandItemAndChildren:item];

    // Adjust the outline view to fit the warnings.
    OffsetOutlineViewVerticallyToFitContent(outlineView_, &totalOffset);
  } else if ([self hasWebstoreData]) {
    // Installs with webstore data that don't have a permissions section need to
    // hide controls related to that and shrink the window by the space they
    // take up.
    NSRect hiddenRect = NSUnionRect([warningsSeparator_ frame],
                                    [[outlineView_ enclosingScrollView] frame]);
    [warningsSeparator_ setHidden:YES];
    [[outlineView_ enclosingScrollView] setHidden:YES];
    totalOffset -= NSHeight(hiddenRect) + kWarningsSeparatorPadding;
  }

  // If necessary, adjust the window size.
  if (totalOffset) {
    NSRect currentRect = [[self view] bounds];
    currentRect.size.height += totalOffset;
    [self updateViewFrame:currentRect];
  }
}

- (BOOL)hasWebstoreData {
  return prompt_->has_webstore_data();
}

- (void)appendRatingStar:(const gfx::ImageSkia*)skiaImage {
  NSImage* image = gfx::NSImageFromImageSkiaWithColorSpace(
      *skiaImage, base::mac::GetSystemColorSpace());
  NSRect frame = NSMakeRect(0, 0, skiaImage->width(), skiaImage->height());
  base::scoped_nsobject<NSImageView> view(
      [[NSImageView alloc] initWithFrame:frame]);
  [view setImage:image];

  // Add this star after all the other ones
  CGFloat maxStarRight = 0;
  if ([[ratingStars_ subviews] count]) {
    maxStarRight = NSMaxX([[[ratingStars_ subviews] lastObject] frame]);
  }
  NSRect starBounds = NSMakeRect(maxStarRight, 0,
                                 skiaImage->width(), skiaImage->height());
  [view setFrame:starBounds];
  [ratingStars_ addSubview:view];
}

- (void)onOutlineViewRowCountDidChange {
  // Force the outline view to update.
  [outlineView_ reloadData];

  CGFloat totalOffset = 0.0;
  OffsetOutlineViewVerticallyToFitContent(outlineView_, &totalOffset);
  if (totalOffset) {
    NSRect currentRect = [[self view] bounds];
    currentRect.size.height += totalOffset;
    [self updateViewFrame:currentRect];
  }
}

- (id)outlineView:(NSOutlineView*)outlineView
            child:(NSInteger)index
           ofItem:(id)item {
  if (!item)
    return [warnings_ objectAtIndex:index];
  if ([item isKindOfClass:[NSDictionary class]])
    return [[item objectForKey:kChildrenKey] objectAtIndex:index];
  NOTREACHED();
  return nil;
}

- (BOOL)outlineView:(NSOutlineView*)outlineView
   isItemExpandable:(id)item {
  return [self outlineView:outlineView numberOfChildrenOfItem:item] > 0;
}

- (NSInteger)outlineView:(NSOutlineView*)outlineView
  numberOfChildrenOfItem:(id)item {
  if (!item)
    return [warnings_ count];

  if ([item isKindOfClass:[NSDictionary class]])
    return [[item objectForKey:kChildrenKey] count];

  NOTREACHED();
  return 0;
}

- (id)outlineView:(NSOutlineView*)outlineView
    objectValueForTableColumn:(NSTableColumn *)tableColumn
                       byItem:(id)item {
  return [item objectForKey:kTitleKey];
}

- (BOOL)outlineView:(NSOutlineView *)outlineView
   shouldExpandItem:(id)item {
  return HasAttribute(item, kCanExpand);
}

- (void)outlineViewItemDidExpand:sender {
  // Call via run loop to avoid animation glitches.
  [self performSelector:@selector(onOutlineViewRowCountDidChange)
             withObject:nil
             afterDelay:0];
}

- (void)outlineViewItemDidCollapse:sender {
  // Call via run loop to avoid animation glitches.
  [self performSelector:@selector(onOutlineViewRowCountDidChange)
             withObject:nil
             afterDelay:0];
}

- (CGFloat)outlineView:(NSOutlineView *)outlineView
     heightOfRowByItem:(id)item {
  // Prevent reentrancy due to the frameOfCellAtColumn:row: call below.
  if (isComputingRowHeight_)
    return 1;
  base::AutoReset<BOOL> reset(&isComputingRowHeight_, YES);

  NSCell* cell = [[[outlineView_ tableColumns] objectAtIndex:0] dataCell];
  [cell setStringValue:[item objectForKey:kTitleKey]];
  NSRect bounds = NSZeroRect;
  NSInteger row = [outlineView_ rowForItem:item];
  bounds.size.width = NSWidth([outlineView_ frameOfCellAtColumn:0 row:row]);
  bounds.size.height = kMaxControlHeight;

  return [cell cellSizeForBounds:bounds].height;
}

- (BOOL)outlineView:(NSOutlineView*)outlineView
    shouldShowOutlineCellForItem:(id)item {
  return !HasAttribute(item, kNoExpandMarker);
}

- (BOOL)outlineView:(NSOutlineView*)outlineView
    shouldTrackCell:(NSCell*)cell
     forTableColumn:(NSTableColumn*)tableColumn
               item:(id)item {
  return HasAttribute(item, kUseCustomLinkCell);
}

- (void)outlineView:(NSOutlineView*)outlineView
    willDisplayCell:(id)cell
     forTableColumn:(NSTableColumn *)tableColumn
               item:(id)item {
  if (HasAttribute(item, kBoldText))
    [cell setFont:[NSFont boldSystemFontOfSize:12.0]];
  else
    [cell setFont:[NSFont systemFontOfSize:12.0]];
}

- (void)outlineView:(NSOutlineView *)outlineView
    willDisplayOutlineCell:(id)cell
            forTableColumn:(NSTableColumn *)tableColumn
                      item:(id)item {
  if (HasAttribute(item, kNoExpandMarker)) {
    [cell setImagePosition:NSNoImage];
    return;
  }

  if (HasAttribute(item, kUseBullet)) {
    // Replace disclosure triangles with bullet lists for leaf nodes.
    [cell setImagePosition:NSNoImage];
    DrawBulletInFrame([outlineView_ frameOfOutlineCellAtRow:
        [outlineView_ rowForItem:item]]);
    return;
  }

  // Reset image to default value.
  [cell setImagePosition:NSImageOverlaps];
}

- (BOOL)outlineView:(NSOutlineView *)outlineView
   shouldSelectItem:(id)item {
  return false;
}

- (NSCell*)outlineView:(NSOutlineView*)outlineView
    dataCellForTableColumn:(NSTableColumn*)tableColumn
                  item:(id)item {
  if (HasAttribute(item, kUseCustomLinkCell)) {
    base::scoped_nsobject<DetailToggleHyperlinkButtonCell> cell(
        [[DetailToggleHyperlinkButtonCell alloc] initTextCell:@""]);
    [cell setTarget:self];
    [cell setLinkClickedAction:@selector(onToggleDetailsLinkClicked:)];
    [cell setAlignment:NSLeftTextAlignment];
    [cell setUnderlineBehavior:
        hyperlink_button_cell::UnderlineBehavior::ON_HOVER];
    [cell setTextColor:
        skia::SkColorToCalibratedNSColor(chrome_style::GetLinkColor())];

    size_t detailsIndex =
        [[item objectForKey:kPermissionsDetailIndex] unsignedIntegerValue];
    [cell setPermissionsDetailIndex:detailsIndex];

    ExtensionInstallPrompt::DetailsType detailsType =
        static_cast<ExtensionInstallPrompt::DetailsType>(
            [[item objectForKey:kPermissionsDetailType] unsignedIntegerValue]);
    [cell setPermissionsDetailType:detailsType];

    if (prompt_->GetIsShowingDetails(detailsType, detailsIndex)) {
      [cell setTitle:
          l10n_util::GetNSStringWithFixup(IDS_EXTENSIONS_HIDE_DETAILS)];
    } else {
      [cell setTitle:
          l10n_util::GetNSStringWithFixup(IDS_EXTENSIONS_SHOW_DETAILS)];
    }

    return cell.autorelease();
  } else {
    return [tableColumn dataCell];
  }
}

- (void)expandItemAndChildren:(id)item {
  if (HasAttribute(item, kAutoExpandCell))
    [outlineView_ expandItem:item expandChildren:NO];

  for (id child in [item objectForKey:kChildrenKey])
    [self expandItemAndChildren:child];
}

- (void)onToggleDetailsLinkClicked:(id)sender {
  size_t index = [sender permissionsDetailIndex];
  ExtensionInstallPrompt::DetailsType type = [sender permissionsDetailType];
  prompt_->SetIsShowingDetails(
      type, index, !prompt_->GetIsShowingDetails(type, index));

  warnings_.reset([[self buildWarnings:*prompt_] retain]);
  [outlineView_ reloadData];

  for (id item in warnings_.get())
    [self expandItemAndChildren:item];
}

- (NSDictionary*)buildItemWithTitle:(NSString*)title
                     cellAttributes:(CellAttributes)cellAttributes
                           children:(NSArray*)children {
  if (!children || ([children count] == 0 && cellAttributes & kUseBullet)) {
    // Add a dummy child even though this is a leaf node. This will cause
    // the outline view to show a disclosure triangle for this item.
    // This is later overriden in willDisplayOutlineCell: to draw a bullet
    // instead. (The bullet could be placed in the title instead but then
    // the bullet wouldn't line up with disclosure triangles of sibling nodes.)
    children = [NSArray arrayWithObject:[NSDictionary dictionary]];
  } else {
    cellAttributes = cellAttributes | kCanExpand;
  }

  return @{
    kTitleKey : title,
    kChildrenKey : children,
    kCellAttributesKey : [NSNumber numberWithInt:cellAttributes],
    kPermissionsDetailIndex : @0ul,
    kPermissionsDetailType : @0ul,
  };
}

- (NSDictionary*)buildDetailToggleItem:(size_t)type
                permissionsDetailIndex:(size_t)index {
  return @{
    kTitleKey : @"",
    kChildrenKey : @[ @{} ],
    kCellAttributesKey : [NSNumber numberWithInt:kUseCustomLinkCell |
                                                 kNoExpandMarker],
    kPermissionsDetailIndex : [NSNumber numberWithUnsignedInteger:index],
    kPermissionsDetailType : [NSNumber numberWithUnsignedInteger:type],
  };
}

- (NSArray*)buildWarnings:(const ExtensionInstallPrompt::Prompt&)prompt {
  NSMutableArray* warnings = [NSMutableArray array];
  NSString* heading = nil;

  bool hasPermissions = prompt.GetPermissionCount(
      ExtensionInstallPrompt::PermissionsType::ALL_PERMISSIONS);
  CellAttributes warningCellAttributes =
      kBoldText | kAutoExpandCell | kNoExpandMarker;
  if (prompt.ShouldShowPermissions()) {
    NSMutableArray* children = [NSMutableArray array];

    heading =
        [self appendPermissionsForPrompt:prompt
                                children:children];

    if (!hasPermissions) {
      [children addObject:
          [self buildItemWithTitle:
              l10n_util::GetNSString(IDS_EXTENSION_NO_SPECIAL_PERMISSIONS)
                    cellAttributes:kUseBullet
                          children:nil]];
      heading = @"";
    }

    if (heading) {
      [warnings addObject:[self buildItemWithTitle:heading
                                    cellAttributes:warningCellAttributes
                                          children:children]];
    }
  }

  if (prompt.GetRetainedFileCount() > 0) {
    const ExtensionInstallPrompt::DetailsType type =
        ExtensionInstallPrompt::RETAINED_FILES_DETAILS;

    NSMutableArray* children = [NSMutableArray array];

    if (prompt.GetIsShowingDetails(type, 0)) {
      for (size_t i = 0; i < prompt.GetRetainedFileCount(); ++i) {
        NSString* title = SysUTF16ToNSString(prompt.GetRetainedFile(i));
        [children addObject:[self buildItemWithTitle:title
                                      cellAttributes:kUseBullet
                                            children:nil]];
      }
    }

    NSString* title = SysUTF16ToNSString(prompt.GetRetainedFilesHeading());
    [warnings addObject:[self buildItemWithTitle:title
                                  cellAttributes:warningCellAttributes
                                        children:children]];

    // Add a row for the link.
    [warnings addObject:
        [self buildDetailToggleItem:type permissionsDetailIndex:0]];
  }

  if (prompt.GetRetainedDeviceCount() > 0) {
    const ExtensionInstallPrompt::DetailsType type =
        ExtensionInstallPrompt::RETAINED_DEVICES_DETAILS;

    NSMutableArray* children = [NSMutableArray array];

    if (prompt.GetIsShowingDetails(type, 0)) {
      for (size_t i = 0; i < prompt.GetRetainedDeviceCount(); ++i) {
        NSString* title =
            SysUTF16ToNSString(prompt.GetRetainedDeviceMessageString(i));
        [children addObject:[self buildItemWithTitle:title
                                      cellAttributes:kUseBullet
                                            children:nil]];
      }
    }

    NSString* title = SysUTF16ToNSString(prompt.GetRetainedDevicesHeading());
    [warnings addObject:[self buildItemWithTitle:title
                                  cellAttributes:warningCellAttributes
                                        children:children]];

    // Add a row for the link.
    [warnings
        addObject:[self buildDetailToggleItem:type permissionsDetailIndex:0]];
  }

  return warnings;
}

- (NSString*)
appendPermissionsForPrompt:(const ExtensionInstallPrompt::Prompt&)prompt
                 children:(NSMutableArray*)children {
  size_t permissionsCount = prompt.GetPermissionCount();
  if (permissionsCount == 0)
    return NULL;

  for (size_t i = 0; i < permissionsCount; ++i) {
    NSDictionary* item =
        [self buildItemWithTitle:SysUTF16ToNSString(prompt.GetPermission(i))
                  cellAttributes:kUseBullet
                        children:nil];
    [children addObject:item];

    // If there are additional details, add them below this item.
    if (!prompt.GetPermissionsDetails(i).empty()) {
      if (prompt.GetIsShowingDetails(
              ExtensionInstallPrompt::PERMISSIONS_DETAILS, i)) {
        item = [self buildItemWithTitle:SysUTF16ToNSString(
                                            prompt.GetPermissionsDetails(i))
                         cellAttributes:kNoExpandMarker
                               children:nil];
        [children addObject:item];
      }

      // Add a row for the link.
      [children addObject:
          [self buildDetailToggleItem:type permissionsDetailIndex:i]];
    }
  }

  return SysUTF16ToNSString(prompt.GetPermissionsHeading());
}

- (void)updateViewFrame:(NSRect)frame {
  NSWindow* window = [[self view] window];
  [window setFrame:[window frameRectForContentRect:frame] display:YES];
  [[self view] setFrame:frame];
}

@end


@implementation DetailToggleHyperlinkButtonCell

@synthesize permissionsDetailIndex = permissionsDetailIndex_;
@synthesize permissionsDetailType = permissionsDetailType_;
@synthesize linkClickedAction = linkClickedAction_;

+ (BOOL)prefersTrackingUntilMouseUp {
  return YES;
}

- (NSRect)drawingRectForBounds:(NSRect)rect {
  NSRect rectInset = NSMakeRect(rect.origin.x + kLinkCellPaddingLeft,
                                rect.origin.y,
                                rect.size.width - kLinkCellPaddingLeft,
                                rect.size.height);
  return [super drawingRectForBounds:rectInset];
}

- (NSUInteger)hitTestForEvent:(NSEvent*)event
                       inRect:(NSRect)cellFrame
                       ofView:(NSView*)controlView {
  NSUInteger hitTestResult =
      [super hitTestForEvent:event inRect:cellFrame ofView:controlView];
  if ((hitTestResult & NSCellHitContentArea) != 0)
    hitTestResult |= NSCellHitTrackableArea;
  return hitTestResult;
}

- (void)handleLinkClicked {
  [NSApp sendAction:linkClickedAction_ to:[self target] from:self];
}

- (BOOL)trackMouse:(NSEvent*)event
            inRect:(NSRect)cellFrame
            ofView:(NSView*)controlView
      untilMouseUp:(BOOL)flag {
  BOOL result = YES;
  NSUInteger hitTestResult =
      [self hitTestForEvent:event inRect:cellFrame ofView:controlView];
  if ((hitTestResult & NSCellHitContentArea) != 0) {
    result = [super trackMouse:event
                        inRect:cellFrame
                        ofView:controlView
                  untilMouseUp:flag];
    event = [NSApp currentEvent];
    hitTestResult =
        [self hitTestForEvent:event inRect:cellFrame ofView:controlView];
    if ((hitTestResult & NSCellHitContentArea) != 0)
      [self handleLinkClicked];
  }
  return result;
}

- (NSArray*)accessibilityActionNames {
  return [[super accessibilityActionNames]
      arrayByAddingObject:NSAccessibilityPressAction];
}

- (void)accessibilityPerformAction:(NSString*)action {
  if ([action isEqualToString:NSAccessibilityPressAction])
    [self handleLinkClicked];
  else
    [super accessibilityPerformAction:action];
}

@end
