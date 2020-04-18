// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/toolbar/buttons/toolbar_button_visibility_configuration.h"

#import "ios/chrome/browser/ui/toolbar/public/toolbar_controller_base_feature.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation ToolbarButtonVisibilityConfiguration

@synthesize type = _type;

- (instancetype)initWithType:(ToolbarType)type {
  self = [super init];
  if (self) {
    _type = type;
  }
  return self;
}

- (ToolbarComponentVisibility)backButtonVisibility {
  switch (self.type) {
    case PRIMARY:
      return ToolbarComponentVisibilityAlways &
             ~ToolbarComponentVisibilitySplit;
    case SECONDARY:
      return ToolbarComponentVisibilitySplit;
    case LEGACY:
      return ToolbarComponentVisibilityAlways;
  }
}

- (ToolbarComponentVisibility)forwardButtonVisibility {
  switch (self.type) {
    case PRIMARY:
      return ToolbarComponentVisibilityAlways &
             ~ToolbarComponentVisibilitySplit;
    case SECONDARY:
      return ToolbarComponentVisibilitySplit;
    case LEGACY:
      return ToolbarComponentVisibilityOnlyWhenEnabled |
             ToolbarComponentVisibilityRegularWidthRegularHeight;
  }
}

- (ToolbarComponentVisibility)tabGridButtonVisibility {
  switch (self.type) {
    case PRIMARY:
      return ToolbarComponentVisibilityCompactWidthCompactHeight |
             ToolbarComponentVisibilityRegularWidthCompactHeight;
    case SECONDARY:
      return ToolbarComponentVisibilitySplit;
    case LEGACY:
      return ToolbarComponentVisibilityIPhoneOnly;
  }
}

- (ToolbarComponentVisibility)toolsMenuButtonVisibility {
  switch (self.type) {
    case PRIMARY:
      return ToolbarComponentVisibilityAlways &
             ~ToolbarComponentVisibilitySplit;
    case SECONDARY:
      return ToolbarComponentVisibilitySplit;
    case LEGACY:
      return ToolbarComponentVisibilityAlways;
  }
}

- (ToolbarComponentVisibility)shareButtonVisibility {
  switch (self.type) {
    case PRIMARY:
      return ToolbarComponentVisibilityAlways &
             ~ToolbarComponentVisibilitySplit;
    case SECONDARY:
      return ToolbarComponentVisibilityNone;
    case LEGACY:
      return ToolbarComponentVisibilityRegularWidthRegularHeight;
  }
}

- (ToolbarComponentVisibility)reloadButtonVisibility {
  switch (self.type) {
    case PRIMARY:
      return ToolbarComponentVisibilityAlways &
             ~ToolbarComponentVisibilitySplit;
    case SECONDARY:
      return ToolbarComponentVisibilityNone;
    case LEGACY:
      return ToolbarComponentVisibilityRegularWidthRegularHeight;
  }
}

- (ToolbarComponentVisibility)stopButtonVisibility {
  switch (self.type) {
    case PRIMARY:
      return ToolbarComponentVisibilityAlways &
             ~ToolbarComponentVisibilitySplit;
    case SECONDARY:
      return ToolbarComponentVisibilityNone;
    case LEGACY:
      return ToolbarComponentVisibilityRegularWidthRegularHeight;
  }
}

- (ToolbarComponentVisibility)bookmarkButtonVisibility {
  switch (self.type) {
    case PRIMARY:
      return ToolbarComponentVisibilityRegularWidthRegularHeight;
    case SECONDARY:
      return ToolbarComponentVisibilityNone;
    case LEGACY:
      return ToolbarComponentVisibilityRegularWidthCompactHeight |
             ToolbarComponentVisibilityRegularWidthRegularHeight;
  }
}

- (ToolbarComponentVisibility)voiceSearchButtonVisibility {
  switch (self.type) {
    case PRIMARY:
      return ToolbarComponentVisibilityRegularWidthRegularHeight;
    case SECONDARY:
      return ToolbarComponentVisibilityNone;
    case LEGACY:
      return ToolbarComponentVisibilityRegularWidthCompactHeight |
             ToolbarComponentVisibilityRegularWidthRegularHeight;
  }
}

- (ToolbarComponentVisibility)contractButtonVisibility {
  switch (self.type) {
    case PRIMARY:
      return ToolbarComponentVisibilityNone;
    case SECONDARY:
      return ToolbarComponentVisibilityNone;
    case LEGACY:
      return ToolbarComponentVisibilityAlways;
  }
}

- (ToolbarComponentVisibility)omniboxButtonVisibility {
  switch (self.type) {
    case PRIMARY:
      return ToolbarComponentVisibilityNone;
    case SECONDARY:
      return ToolbarComponentVisibilitySplit;
    case LEGACY:
      return ToolbarComponentVisibilityNone;
  }
}

- (ToolbarComponentVisibility)locationBarLeadingButtonVisibility {
  switch (self.type) {
    case PRIMARY:
      return ToolbarComponentVisibilityAlways;
    case SECONDARY:
      return ToolbarComponentVisibilityNone;
    case LEGACY:
      return ToolbarComponentVisibilityAlways;
  }
}

@end
