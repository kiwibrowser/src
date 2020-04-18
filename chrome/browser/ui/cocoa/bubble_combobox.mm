// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/bubble_combobox.h"

#include "base/strings/sys_string_conversions.h"
#include "ui/base/models/combobox_model.h"

@implementation BubbleCombobox

- (id)initWithFrame:(NSRect)frame
          pullsDown:(BOOL)pullsDown
              model:(ui::ComboboxModel*)model {
  if ((self = [super initWithFrame:frame pullsDown:pullsDown])) {
    [self setFont:[NSFont systemFontOfSize:[NSFont smallSystemFontSize]]];
    [self setBordered:YES];
    [[self cell] setControlSize:NSSmallControlSize];

    for (int i = 0; i < model->GetItemCount(); ++i) {
      if (model->IsItemSeparatorAt(i))
        [[self menu] addItem:[NSMenuItem separatorItem]];
      else
        [self addItemWithTitle:base::SysUTF16ToNSString(model->GetItemAt(i))];
    }

    [self selectItemAtIndex:model->GetDefaultIndex()];
  }
  return self;
}

@end
