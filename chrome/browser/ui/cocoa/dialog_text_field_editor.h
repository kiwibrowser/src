// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_DIALOG_TEXT_FIELD__EDITOR_H_
#define CHROME_BROWSER_UI_COCOA_DIALOG_TEXT_FIELD__EDITOR_H_

#import <Cocoa/Cocoa.h>

// This class is used to customize the field editors for dialog textfields.
// It intercepts touch bar methods to prevent the textfield's touch bar from
// overriding the dialog's touch bar.
//
// TODO(spqchan): Add the textfield's candidate list to the touch bar. I've
// tried to combine the parent's touch bar with the dialog's, but the result
// was buggy.
@interface DialogTextFieldEditor : NSTextView

@end

#endif  // CHROME_BROWSER_UI_COCOA_DIALOG_TEXT_FIELD__EDITOR_H_