// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_HISTORY_CLEAR_BROWSING_BAR_H_
#define IOS_CHROME_BROWSER_UI_HISTORY_CLEAR_BROWSING_BAR_H_

#import <UIKit/UIKit.h>

// View at the bottom of the history panel that presents options to clear
// browsing data or enter edit mode. When in edit mode, the bar displays a
// delete button and a cancel button instead.
@interface ClearBrowsingBar : UIView

// Yes if in edit mode. Setting to |editing| ClearBrowsingBar for edit
// mode or non-edit mode accordingly.
@property(nonatomic, getter=isEditing) BOOL editing;
// Yes if the edit button is enabled. Setting |editButtonEnabled| enables or
// disables the edit button accordingly.
@property(nonatomic, getter=isEditButtonEnabled) BOOL editButtonEnabled;
// Yes if the delete button is enabled. Setting |deleteButtonEnabled| enables or
// disables the delete button accordingly.
@property(nonatomic, getter=isDeleteButtonEnabled) BOOL deleteButtonEnabled;

// Sets the target/action of the "Clear Browsing Data..." button.
- (void)setClearBrowsingDataTarget:(id)target action:(SEL)action;
// Sets the target/action of the "Edit" button.
- (void)setEditTarget:(id)target action:(SEL)action;
// Sets the target/action of the "Delete" button.
- (void)setDeleteTarget:(id)taret action:(SEL)action;
// Sets the target/action of the "Cancel" button.
- (void)setCancelTarget:(id)target action:(SEL)action;
// Updates the height of the ClearBrowsingBar.
- (void)updateHeight;

@end
#endif  // IOS_CHROME_BROWSER_UI_HISTORY_CLEAR_BROWSING_BAR_H_
