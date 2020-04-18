// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_PUBLIC_PROVIDER_CHROME_BROWSER_UI_TEXT_FIELD_STYLING_H_
#define IOS_PUBLIC_PROVIDER_CHROME_BROWSER_UI_TEXT_FIELD_STYLING_H_

#import <Foundation/Foundation.h>

@protocol TextFieldValidation;

// Types of styling controlling layout of the placeholder.
typedef NS_ENUM(NSInteger, TextFieldStylingPlaceholder) {
  // Effectively the same placeholder as a UITextField.
  TextFieldStylingPlaceholderDefault = 0,
  // The placeholder text will float above the field when there is content or
  // the field is being edited.
  TextFieldStylingPlaceholderFloatingPlaceholder,
};

// The TextFieldStyling protocol works with UITextField to add methods related
// to styling and input validation.
@protocol TextFieldStyling

// The placement for the placeholder text (if there is any).
@property(nonatomic, assign) TextFieldStylingPlaceholder placeholderStyle;

// The object used to validate text in this text field.  If nil, no validation
// is performed.
@property(nonatomic, weak) id<TextFieldValidation> textValidator;

// If |error| is YES, display this field using error colors and styling.
- (void)setUseErrorStyling:(BOOL)error;

@end

// The TextFieldValidation protocol is used to validate the contents of a text
// field.
@protocol TextFieldValidation

// Returns an error string to display if the contents of the given |field| are
// invalid.  If this method returns nil, the contents of the field are presumed
// to be valid and no error needs to displayed.  This method is called whenever
// the field's text changes.
- (NSString*)validationErrorForTextField:(id<TextFieldStyling>)field;

@end

#endif  // IOS_PUBLIC_PROVIDER_CHROME_BROWSER_UI_TEXT_FIELD_STYLING_H_
