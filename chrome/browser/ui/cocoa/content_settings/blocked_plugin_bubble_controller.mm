// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/content_settings/blocked_plugin_bubble_controller.h"

#import "base/mac/scoped_nsobject.h"
#include "base/strings/sys_string_conversions.h"
#import "chrome/browser/ui/cocoa/info_bubble_window.h"
#include "chrome/browser/ui/cocoa/key_equivalent_constants.h"
#import "chrome/browser/ui/cocoa/l10n_util.h"
#include "chrome/browser/ui/content_settings/content_setting_bubble_model.h"
#include "chrome/grit/generated_resources.h"
#include "components/strings/grit/components_strings.h"
#import "ui/base/cocoa/controls/button_utils.h"
#import "ui/base/cocoa/controls/textfield_utils.h"
#import "ui/base/l10n/l10n_util_mac.h"

namespace {

// Vertical spacing between elements in this bubble. The nib uses several
// different values for this.
constexpr int kVerticalSpacing = 8;

// Horizontal margin at the edges of the bubble. The nib uses 17 for some views
// and 20 for others, but powers of two are much nicer, no?
constexpr int kHorizontalMargin = 16;

void SetControlSize(NSControl* control, NSControlSize control_size) {
  CGFloat font_size = [NSFont systemFontSizeForControlSize:control_size];
  NSCell* cell = [control cell];
  [cell setFont:[NSFont systemFontOfSize:font_size]];
  [cell setControlSize:control_size];
}

}  // namespace

@implementation BlockedPluginBubbleController

- (id)initWithModel:(ContentSettingBubbleModel*)model
        webContents:(content::WebContents*)webContents
       parentWindow:(NSWindow*)parentWindow
         decoration:(ContentSettingDecoration*)decoration
         anchoredAt:(NSPoint)anchoredAt {
  // The size of this bubble, and the other layout constants elsewhere in this
  // file, were lifted from ContentBlockedPlugins.xib
  base::scoped_nsobject<InfoBubbleWindow> window([[InfoBubbleWindow alloc]
      initWithContentRect:NSMakeRect(0, 0, 314, 145)
                styleMask:NSBorderlessWindowMask
                  backing:NSBackingStoreBuffered
                    defer:NO]);

  [window setAllowedAnimations:info_bubble::kAnimateNone];

  return [super initWithModel:model
                  webContents:webContents
                       window:window
                 parentWindow:parentWindow
                   decoration:decoration
                   anchoredAt:anchoredAt];
}

- (NSString*)manageButtonTitle {
  return base::SysUTF16ToNSString([super model]->bubble_content().manage_text);
}

- (NSString*)customLinkTitle {
  return base::SysUTF16ToNSString([super model]->bubble_content().custom_link);
}

// This method requires that the plugin items all be plain strings instead of
// links, which is how the model behaves in practice, but is not otherwise
// enforced.
- (NSArray*)pluginNames {
  const ContentSettingBubbleModel::ListItems& items =
      [super model]->bubble_content().list_items;

  NSMutableArray* names = [NSMutableArray arrayWithCapacity:items.size()];
  for (const auto& item : items) {
    DCHECK(!item.has_link);
    [names addObject:base::SysUTF16ToNSString(item.title)];
  }

  return names;
}

- (void)awakeFromNib {
  [self loadView];
  [super awakeFromNib];
}

// This method constructs a view containing the names of all the blocked
// plugins. At the moment, it clips names that don't fit, but it could either
// scroll or grow outside |frame|.
- (NSView*)makePluginsBox:(NSRect)frame {
  NSView* plugins = [[[NSView alloc] initWithFrame:frame] autorelease];
  CGFloat y = NSHeight([plugins frame]);

  for (NSString* name in [self pluginNames]) {
    NSTextField* label = [TextFieldUtils labelWithString:name];
    SetControlSize(label, NSSmallControlSize);

    // The sizing is a bit fiddly. First, make the frame the same width as the
    // plugins box is:
    [label setFrameSize:NSMakeSize(NSWidth(frame), NSHeight([label frame]))];

    // Then, wrap the label if needed, which may change the label's height:
    cocoa_l10n_util::WrapOrSizeToFit(label);

    // Then stick the label in, and leave kVerticalSpacing below it.
    y -= NSHeight([label frame]);
    [label setFrameOrigin:NSMakePoint(0, y)];
    [plugins addSubview:label];
    y -= kVerticalSpacing;
  }

  return plugins;
}

- (void)loadView {
  // This method constructs the layout of the bubble from the bottom (ie,
  // numerically lowest y-coordinate) upwards, stacking elements together,
  // separated by kVerticalSpacing.
  int fullWidth =
      NSWidth([self.window.contentView frame]) - 2 * kHorizontalMargin;

  // The manage button is in the bottom left. This button may not be present, if
  // the "manage" link is not set.
  NSString* manageButtonTitle = [self manageButtonTitle];
  if ([manageButtonTitle length]) {
    NSButton* manageButton =
        [ButtonUtils buttonWithTitle:manageButtonTitle
                              action:@selector(manageBlocking:)
                              target:self];
    SetControlSize(manageButton, NSSmallControlSize);
    cocoa_l10n_util::WrapOrSizeToFit(manageButton);
    [manageButton
        setFrameOrigin:NSMakePoint(kHorizontalMargin, kVerticalSpacing)];
    [self.window.contentView addSubview:manageButton];
  }

  // The done button is in the bottom right.
  NSButton* doneButton =
      [ButtonUtils buttonWithTitle:l10n_util::GetNSString(IDS_DONE)
                            action:@selector(closeBubble:)
                            target:self];
  SetControlSize(doneButton, NSSmallControlSize);
  cocoa_l10n_util::WrapOrSizeToFit(doneButton);
  [doneButton
      setFrameOrigin:NSMakePoint(NSWidth([self.window.contentView frame]) -
                                     NSWidth([doneButton frame]) -
                                     kHorizontalMargin,
                                 kVerticalSpacing)];
  [doneButton setKeyEquivalent:kKeyEquivalentReturn];
  [self.window.contentView addSubview:doneButton];

  // The separator is stacked above those two buttons.
  base::scoped_nsobject<NSBox> bottomSeparator([[NSBox alloc]
      initWithFrame:NSMakeRect(kHorizontalMargin,
                               NSMaxY([doneButton frame]) + kVerticalSpacing,
                               fullWidth, 4)]);
  [bottomSeparator setBoxType:NSBoxSeparator];
  [self.window.contentView addSubview:bottomSeparator];

  CGFloat learnMoreLinkY = NSMaxY([bottomSeparator frame]) + kVerticalSpacing;
  // The custom link is stacked above the separator. Note that this link is
  // sometimes not present, depending on the model.
  NSString* customLinkTitle = [self customLinkTitle];
  if ([customLinkTitle length]) {
    NSButton* customLink = [ButtonUtils linkWithTitle:customLinkTitle
                                               action:@selector(load:)
                                               target:self];
    [customLink
        setFrame:NSMakeRect(kHorizontalMargin,
                            NSMaxY([bottomSeparator frame]) + kVerticalSpacing,
                            fullWidth, 18)];
    SetControlSize(customLink, NSSmallControlSize);
    cocoa_l10n_util::WrapOrSizeToFit(customLink);
    [self.window.contentView addSubview:customLink];

    learnMoreLinkY = NSMaxY([customLink frame]) + kVerticalSpacing;
  }

  // The learn more link is stacked above the load all link, or directly above
  // the separator, if there is no learn more link.
  NSButton* learnMoreLink =
      [ButtonUtils linkWithTitle:l10n_util::GetNSString(IDS_LEARN_MORE)
                          action:@selector(learnMoreLinkClicked:)
                          target:self];
  [learnMoreLink
      setFrame:NSMakeRect(kHorizontalMargin, learnMoreLinkY, fullWidth, 18)];
  SetControlSize(learnMoreLink, NSSmallControlSize);
  cocoa_l10n_util::WrapOrSizeToFit(learnMoreLink);
  [self.window.contentView addSubview:learnMoreLink];

  // The title is attached to the top of the bubble, not the bottom. Note the
  // extra vertical spacing, to help the title stand out.
  NSTextField* title = [TextFieldUtils
      labelWithString:l10n_util::GetNSString(IDS_BLOCKED_PLUGINS_TITLE)];
  [title setFrameSize:NSMakeSize(fullWidth, NSHeight([title frame]))];
  cocoa_l10n_util::WrapOrSizeToFit(title);
  CGFloat titleHeight = NSMaxY([self.window.contentView bounds]) -
                        NSHeight([title frame]) - (2 * kVerticalSpacing);
  [title setFrameOrigin:NSMakePoint(kHorizontalMargin, titleHeight)];
  [self.window.contentView addSubview:title];

  // The plugins view fills all the remaining space in the bubble.
  CGFloat pluginsY = NSMaxY([learnMoreLink frame]) + kVerticalSpacing;
  CGFloat pluginsHeight = NSMinY([title frame]) - kVerticalSpacing - pluginsY;
  NSView* pluginsBox =
      [self makePluginsBox:NSMakeRect(kHorizontalMargin, pluginsY, fullWidth,
                                      pluginsHeight)];
  [self.window.contentView addSubview:pluginsBox];

  cocoa_l10n_util::FlipAllSubviewsIfNecessary(self.window.contentView);
}

- (void)layoutView {
  // Deliberately do not update the layout. This bubble's layout is entirely
  // static.
}

@end
