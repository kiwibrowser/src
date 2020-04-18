// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_HAS_WEAK_BROWSER_POINTER_H_
#define CHROME_BROWSER_UI_COCOA_HAS_WEAK_BROWSER_POINTER_H_

// This allows reference counted objects holding a Browser* to be
// notified that the Browser will be destroyed so they can invalidate their
// Browser*, perform any necessary cleanup, and pass the notification onto any
// objects they retain that also implement this protocol. This helps to prevent
// use-after-free of Browser*, or anything else tied to the Browser's lifetime,
// by objects that may be retained beyond the lifetime of their associated
// Browser (e.g. NSViewControllers).
@protocol HasWeakBrowserPointer
@required
// This is usually called by BrowserWindowController but can be called by any
// object that's certain of Browser's destruction.
- (void)browserWillBeDestroyed;
@end

#endif  // CHROME_BROWSER_UI_COCOA_HAS_WEAK_BROWSER_POINTER_H_
