// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_SHOWCASE_MANUAL_FILL_IPAD_KEYBOARD_PROTO_VIEW_CONTROLLER_H_
#define IOS_SHOWCASE_MANUAL_FILL_IPAD_KEYBOARD_PROTO_VIEW_CONTROLLER_H_

#import <UIKit/UIKit.h>

#import "ios/showcase/manual_fill/keyboard_proto_view_controller.h"

// This class allows the user to manual fill data on an iPad. It uses the
// `inputAssistantItem` of the first responder and pop overs to show the
// available options.
@interface IPadKeyboardProtoViewController : KeyboardProtoViewController
@end

#endif  // IOS_SHOWCASE_MANUAL_FILL_IPAD_KEYBOARD_PROTO_VIEW_CONTROLLER_H_
