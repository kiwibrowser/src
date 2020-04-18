// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/omnibox/clipping_textfield_container.h"

#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

class ClippingTextFieldContainerTest : public PlatformTest {
 protected:
  void SetUp() override {
    PlatformTest::SetUp();
    textField_ = [[ClippingTextField alloc] init];
    container_ = [[ClippingTextFieldContainer alloc]
        initWithClippingTextField:textField_];

    [[[UIApplication sharedApplication] keyWindow] addSubview:container_];
    // Width of 200 is enough to fit "http://www.google.com" and is
    // in the same ballpark as iPhone omnibox.
    container_.frame = CGRectMake(0, 0, 200, 20);
  };

  void TearDown() override { [container_ removeFromSuperview]; }

  ClippingTextFieldContainer* container_;
  ClippingTextField* textField_;

  UIView* LeftDecorationView() {
    // The decoration container contains the decoration views covering the
    // tail and the head.
    UIView* decorationContainer = container_.maskView;
    CGPoint leftMidPoint = CGPointMake(0, container_.bounds.size.height / 2);
    for (UIView* decorationView in decorationContainer.subviews) {
      if (CGRectContainsPoint(decorationView.frame, leftMidPoint) &&
          [decorationView.layer class] == [CAGradientLayer class]) {
        return decorationView;
      }
    }
    return nil;
  }

  UIView* RightDecorationView() {
    // The decoration container contains the decoration views covering the
    // tail and the head.
    UIView* decorationContainer = container_.maskView;
    // Substract 0.5pt from x coordinate because CGRectContainsPoint
    // returns NO for points on the maximum X edge.
    CGPoint rightMidPoint = CGPointMake(container_.bounds.size.width - 0.5,
                                        container_.bounds.size.height / 2);
    for (UIView* decorationView in decorationContainer.subviews) {
      if (CGRectContainsPoint(decorationView.frame, rightMidPoint) &&
          [decorationView.layer class] == [CAGradientLayer class]) {
        return decorationView;
      }
    }
    return nil;
  }
};

TEST_F(ClippingTextFieldContainerTest, DoesntClipWhenEntireURLFits) {
  NSString* text = @"http://www.google.com";
  [textField_ setText:text];

  // The textfield fits in the container.
  EXPECT_LE(textField_.frame.size.width, container_.frame.size.width);
}

TEST_F(ClippingTextFieldContainerTest, ClipsLongPrefix) {
  NSString* text = @"http://verylongprefixwaymorethan200pts.google.com";
  [textField_ setText:text];

  // The left edge is clipped, so the textfield is shifted left.
  EXPECT_TRUE(textField_.frame.origin.x < 0);
  // The amount it's shifted left by is just enough to fit the URL.
  EXPECT_EQ(textField_.frame.origin.x + textField_.frame.size.width,
            container_.frame.size.width);
}

TEST_F(ClippingTextFieldContainerTest, ClipsLongSuffix) {
  NSString* text = @"http://www.google.com/verylongsuffixwaymorethan200pts";
  [textField_ setText:text];

  // The left edge is not clipped.
  EXPECT_EQ(textField_.frame.origin.x, 0);
  // The right edge is clipped.
  EXPECT_GE(textField_.frame.size.width, container_.frame.size.width);
}

TEST_F(ClippingTextFieldContainerTest, ClipsPrefixAndSuffix) {
  NSString* text =
      @"http://verylongprefixwaymorethan200pts.google.com/"
      @"verylongsuffixwaymorethan200pts";
  [textField_ setText:text];
  CGSize textSize = [[textField_ attributedText] size];

  // The left edge is clipped.
  EXPECT_TRUE(textField_.frame.origin.x < 0);
  // The right edge is clipped.
  EXPECT_GE(textField_.frame.size.width + textField_.frame.origin.x,
            container_.frame.size.width);
  // The textfield expands to fit the text.
  EXPECT_EQ(textField_.frame.size.width, ceil(textSize.width));
}

TEST_F(ClippingTextFieldContainerTest, NoScheme) {
  NSString* text = @"www.google.com";
  [textField_ setText:text];

  // The textfield fits in the container.
  EXPECT_LE(textField_.frame.size.width, container_.frame.size.width);
  // The left edge is not clipped.
  EXPECT_EQ(textField_.frame.origin.x, 0);

  // Textfield's size is equal to container size and greater than text.
  CGSize textSize = [[textField_ attributedText] size];
  textSize.width = ceil(textSize.width);
  textSize.height = ceil(textSize.height);
  EXPECT_EQ(textField_.frame.size.width, container_.bounds.size.width);
  EXPECT_LE(textSize.width, textField_.bounds.size.width);
}

TEST_F(ClippingTextFieldContainerTest, NoHost) {
  NSString* text = @"http://";
  [textField_ setText:text];

  // The textfield fits in the container.
  EXPECT_LE(textField_.frame.size.width, container_.frame.size.width);
  // The left edge is not clipped.
  EXPECT_EQ(textField_.frame.origin.x, 0);

  // Textfield's size is equal to container size and greater than text.
  CGSize textSize = [[textField_ attributedText] size];
  textSize.width = ceil(textSize.width);
  textSize.height = ceil(textSize.height);
  EXPECT_EQ(textField_.frame.size.width, container_.bounds.size.width);
  EXPECT_LE(textSize.width, textField_.bounds.size.width);
}

TEST_F(ClippingTextFieldContainerTest, DoesntClipWhenFocused) {
  NSString* text =
      @"http://verylongprefixwaymorethan200pts.google.com/"
      @"verylongsuffixwaymorethan200pts";
  [textField_ setText:text];

  // The left edge is clipped.
  EXPECT_TRUE(textField_.frame.origin.x < 0);
  // The right edge is clipped.
  EXPECT_GE(textField_.frame.size.width + textField_.frame.origin.x,
            container_.frame.size.width);

  [textField_ becomeFirstResponder];
  [container_ layoutIfNeeded];

  // When focused, the textfield should take the container's space and
  // not be clipped.
  EXPECT_EQ(textField_.frame.origin.x, 0);
  EXPECT_EQ(textField_.frame.size.width, container_.frame.size.width);

  [textField_ resignFirstResponder];
  [container_ layoutIfNeeded];

  // The left edge is clipped.
  EXPECT_TRUE(textField_.frame.origin.x < 0);
  // The right edge is clipped.
  EXPECT_GE(textField_.frame.size.width + textField_.frame.origin.x,
            container_.frame.size.width);
}

TEST_F(ClippingTextFieldContainerTest, ShowsDecorationsWhenClipping) {
  NSString* text =
      @"http://verylongprefixwaymorethan200pts.google.com/"
      @"verylongsuffixwaymorethan200pts";
  [textField_ setText:text];

  // Tail and head are masked when the ends are clipped.
  EXPECT_FALSE(LeftDecorationView().hidden);
  EXPECT_FALSE(RightDecorationView().hidden);

  [textField_ becomeFirstResponder];
  [container_ layoutIfNeeded];

  // Even for long strings, the masks are disabled while editing.
  EXPECT_TRUE(LeftDecorationView().hidden);
  EXPECT_TRUE(RightDecorationView().hidden);

  [textField_ resignFirstResponder];
  [container_ layoutIfNeeded];

  // Deselecting textfield with a long string remasks the ends.
  EXPECT_FALSE(LeftDecorationView().hidden);
  EXPECT_FALSE(RightDecorationView().hidden);
}

}  // namespace
