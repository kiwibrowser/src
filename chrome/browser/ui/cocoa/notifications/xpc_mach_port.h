// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_NOTIFICATIONS_XPC_MACH_PORT_H_
#define CHROME_BROWSER_UI_COCOA_NOTIFICATIONS_XPC_MACH_PORT_H_

#import <Foundation/Foundation.h>

#include "base/mac/scoped_mach_port.h"

// An XPC transit type for Mach send rights. While this class conforms to
// NSSecureCoding, it is only valid to use the NSCoding methods for XPC.
//
// The public XPC APIs do not proivde any way to transfer Mach ports. Using the
// C-based API, the functions for setting/creating an XPC Mach port object can
// simply be forward-declared. But the Foundation API does not provide any way
// to pass a Mach port as part of a message. The OS_xpc_object Obj-C types
// do not conform to NSSecureCoding either, so they cannot be used to transport
// ports via the Foundation-level API.
//
// Instead, this class encodes Mach port OOL data using the private
// NSXPCEncoder and NSXPCDecoder APIs. These APIs expose methods by which
// xpc_objct_t's (and their OS_object-bridged-siblings) can be encoded into a
// Foundation-level XPC message.
@interface CrXPCMachPort : NSObject<NSSecureCoding>

// Creates a new transit Mach port by taking ownership of the |sendRight|.
- (instancetype)initWithMachSendRight:(base::mac::ScopedMachSendRight)sendRight;

// Relinquishes ownership of the Mach port to the caller.
- (base::mac::ScopedMachSendRight)takeRight;

@end

#endif  // CHROME_BROWSER_UI_COCOA_NOTIFICATIONS_XPC_MACH_PORT_H_
