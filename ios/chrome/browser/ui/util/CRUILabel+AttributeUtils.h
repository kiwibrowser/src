// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_UTIL_CRUILABEL_ATTRIBUTEUTILS_H_
#define IOS_CHROME_BROWSER_UI_UTIL_CRUILABEL_ATTRIBUTEUTILS_H_

#import <UIKit/UIKit.h>

@interface UILabel (CRUILabelAttributeUtils)
// The height of the label.
// Make sure to create a LabelObserver for this label and start observing before
// setting this property.
@property(nonatomic, assign, setter=cr_setLineHeight:) CGFloat cr_lineHeight;

@end

#endif  // IOS_CHROME_BROWSER_UI_UTIL_CRUILABEL_ATTRIBUTEUTILS_H_
