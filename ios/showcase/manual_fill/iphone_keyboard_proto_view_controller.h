// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_SHOWCASE_MANUAL_FILL_IPHONE_KEYBOARD_PROTO_VIEW_CONTROLLER_H_
#define IOS_SHOWCASE_MANUAL_FILL_IPHONE_KEYBOARD_PROTO_VIEW_CONTROLLER_H_

#import "ios/showcase/manual_fill/keyboard_proto_view_controller.h"

#import "ios/showcase/manual_fill/keyboard_accessory_view.h"

// Subclass of `KeyboardProtoViewController` with the code that is specific for
// iPhone.
@interface IPhoneKeyboardProtoViewController
    : KeyboardProtoViewController<KeyboardAccessoryViewDelegate>
@end

#endif  // IOS_SHOWCASE_MANUAL_FILL_IPHONE_KEYBOARD_PROTO_VIEW_CONTROLLER_H_
