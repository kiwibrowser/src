// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/toolbar/buttons/tools_menu_button_observer_bridge.h"

#include <memory>

#include "base/logging.h"
#include "components/reading_list/core/reading_list_model.h"
#import "ios/chrome/browser/ui/toolbar/buttons/toolbar_tools_menu_button.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface ToolsMenuButtonObserverBridge ()
- (void)updateButtonWithModel:(const ReadingListModel*)model;
- (void)buttonPressed:(UIButton*)sender;
@end

@implementation ToolsMenuButtonObserverBridge {
  ToolbarToolsMenuButton* _button;
  ReadingListModel* _model;
  std::unique_ptr<ReadingListModelBridge> _modelBridge;
}

- (instancetype)initWithModel:(ReadingListModel*)readingListModel
                toolbarButton:(ToolbarToolsMenuButton*)button {
  self = [super init];
  if (self) {
    _button = button;
    _model = readingListModel;
    [_button addTarget:self
                  action:@selector(buttonPressed:)
        forControlEvents:UIControlEventTouchUpInside];
    _modelBridge = std::make_unique<ReadingListModelBridge>(self, _model);
  }
  return self;
}

- (void)updateButtonWithModel:(const ReadingListModel*)model {
  DCHECK(model == _model);
  BOOL readingListContainsUnseenItems = model->GetLocalUnseenFlag();
  [_button setReadingListContainsUnseenItems:readingListContainsUnseenItems];
}

- (void)buttonPressed:(UIButton*)sender {
  if (_model) {
    _model->ResetLocalUnseenFlag();
  }
  [_button setReadingListContainsUnseenItems:NO];
}

#pragma mark - ReadingListModelBridgeObserver

- (void)readingListModelLoaded:(const ReadingListModel*)model {
  [self updateButtonWithModel:model];
}

- (void)readingListModelDidApplyChanges:(const ReadingListModel*)model {
  [self updateButtonWithModel:model];
}

- (void)readingListModelBeingDeleted:(const ReadingListModel*)model {
  DCHECK(model == _model);
  _modelBridge.reset();
  _model = nullptr;
}

- (void)readingListModel:(const ReadingListModel*)model
             didAddEntry:(const GURL&)url
             entrySource:(reading_list::EntrySource)source {
  if (source == reading_list::ADDED_VIA_CURRENT_APP)
    [_button triggerAnimation];
}

@end
