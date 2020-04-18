// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_ELEMENTS_SELECTOR_PICKER_VIEW_CONTROLLER_H_
#define IOS_CHROME_BROWSER_UI_ELEMENTS_SELECTOR_PICKER_VIEW_CONTROLLER_H_

#import <UIKit/UIKit.h>

@protocol SelectorViewControllerDelegate;

// View controller for displaying a UIPickerView topped by a UINavigationBar
// displaying "Done" on the right and "Cancel" on the left.
@interface SelectorPickerViewController : UIViewController

// Initializer for view controller that will display |options|. |defaultOptions|
// will be selected initially, and pressing cancel will set |defaultOption| as
// the selected option.
- (instancetype)initWithOptions:(NSOrderedSet<NSString*>*)options
                        default:(NSString*)defaultOption
    NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithNibName:(NSString*)nibNameOrNil
                         bundle:(NSBundle*)nibBundleOrNil NS_UNAVAILABLE;
- (instancetype)initWithCoder:(NSCoder*)aDecoder NS_UNAVAILABLE;

@property(nonatomic, weak) id<SelectorViewControllerDelegate> delegate;

@end

#endif  // IOS_CHROME_BROWSER_UI_ELEMENTS_SELECTOR_PICKER_VIEW_CONTROLLER_H_
