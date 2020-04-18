// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/mac/availability.h"
#import "base/mac/scoped_nsobject.h"
#import "base/mac/sdk_forward_declarations.h"
#include "base/strings/sys_string_conversions.h"
#import "ui/base/cocoa/touch_bar_forward_declarations.h"
#include "ui/base/models/dialog_model.h"
#import "ui/views/cocoa/bridged_content_view.h"
#include "ui/views/widget/widget_delegate.h"
#include "ui/views/window/dialog_client_view.h"
#include "ui/views/window/dialog_delegate.h"

namespace {

NSString* const kTouchBarDialogButtonsGroupId =
    @"com.google.chrome-DIALOG-BUTTONS-GROUP";
NSString* const kTouchBarOKId = @"com.google.chrome-OK";
NSString* const kTouchBarCancelId = @"com.google.chrome-CANCEL";

}  // namespace

@interface BridgedContentView (TouchBarAdditions)<NSTouchBarDelegate>
- (void)touchBarButtonAction:(id)sender;
@end

@implementation BridgedContentView (TouchBarAdditions)

- (void)touchBarButtonAction:(id)sender {
  if (!hostedView_)
    return;

  views::DialogDelegate* dialog =
      hostedView_->GetWidget()->widget_delegate()->AsDialogDelegate();
  DCHECK(dialog);
  views::DialogClientView* client = dialog->GetDialogClientView();

  if ([sender tag] == ui::DIALOG_BUTTON_OK) {
    client->AcceptWindow();
    return;
  }

  DCHECK_EQ([sender tag], ui::DIALOG_BUTTON_CANCEL);
  client->CancelWindow();
}

// NSTouchBarDelegate protocol implementation.

- (NSTouchBarItem*)touchBar:(NSTouchBar*)touchBar
      makeItemForIdentifier:(NSTouchBarItemIdentifier)identifier
    API_AVAILABLE(macos(10.12.2)) {
  if (!hostedView_)
    return nil;

  if ([identifier isEqualToString:kTouchBarDialogButtonsGroupId]) {
    NSMutableArray* items = [NSMutableArray arrayWithCapacity:2];
    for (NSTouchBarItemIdentifier i in @[ kTouchBarCancelId, kTouchBarOKId ]) {
      NSTouchBarItem* item = [self touchBar:touchBar makeItemForIdentifier:i];
      if (item)
        [items addObject:item];
    }
    if ([items count] == 0)
      return nil;
    return [NSClassFromString(@"NSGroupTouchBarItem")
        groupItemWithIdentifier:identifier
                          items:items];
  }

  ui::DialogButton type = ui::DIALOG_BUTTON_NONE;
  if ([identifier isEqualToString:kTouchBarOKId])
    type = ui::DIALOG_BUTTON_OK;
  else if ([identifier isEqualToString:kTouchBarCancelId])
    type = ui::DIALOG_BUTTON_CANCEL;
  else
    return nil;

  ui::DialogModel* model =
      hostedView_->GetWidget()->widget_delegate()->AsDialogDelegate();
  if (!model || !(model->GetDialogButtons() & type))
    return nil;

  base::scoped_nsobject<NSCustomTouchBarItem> item([[NSClassFromString(
      @"NSCustomTouchBarItem") alloc] initWithIdentifier:identifier]);
  NSString* title = base::SysUTF16ToNSString(model->GetDialogButtonLabel(type));
  NSButton* button =
      [NSButton buttonWithTitle:title
                         target:self
                         action:@selector(touchBarButtonAction:)];
  if (type == model->GetDefaultDialogButton()) {
    // NSAlert uses a private NSButton subclass (_NSTouchBarGroupButton) with
    // more bells and whistles. It doesn't use -setBezelColor: directly, but
    // this gives an appearance matching the default _NSTouchBarGroupButton.
    [button setBezelColor:[NSColor colorWithSRGBRed:0.168
                                              green:0.51
                                               blue:0.843
                                              alpha:1.0]];
  }
  [button setEnabled:model->IsDialogButtonEnabled(type)];
  [button setTag:type];
  [item setView:button];
  return item.autorelease();
}

// NSTouchBarProvider protocol implementation (via NSResponder category).

- (NSTouchBar*)makeTouchBar {
  if (!hostedView_)
    return nil;

  ui::DialogModel* model =
      hostedView_->GetWidget()->widget_delegate()->AsDialogDelegate();
  if (!model || !model->GetDialogButtons())
    return nil;

  base::scoped_nsobject<NSTouchBar> bar(
      [[NSClassFromString(@"NSTouchBar") alloc] init]);
  [bar setDelegate:self];

  // Use a group rather than individual items so they can be centered together.
  [bar setDefaultItemIdentifiers:@[ kTouchBarDialogButtonsGroupId ]];

  // Setting the group as principal will center it in the TouchBar.
  [bar setPrincipalItemIdentifier:kTouchBarDialogButtonsGroupId];
  return bar.autorelease();
}

@end
