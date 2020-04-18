// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/find_bar/find_bar_controller_ios.h"

#include "base/format_macros.h"
#include "base/i18n/rtl.h"
#include "base/mac/bundle_locations.h"
#include "base/mac/foundation_util.h"
#include "base/strings/sys_string_conversions.h"
#include "components/strings/grit/components_strings.h"
#import "ios/chrome/browser/find_in_page/find_in_page_controller.h"
#import "ios/chrome/browser/find_in_page/find_in_page_model.h"
#import "ios/chrome/browser/ui/UIView+SizeClassSupport.h"
#import "ios/chrome/browser/ui/commands/browser_commands.h"
#import "ios/chrome/browser/ui/find_bar/find_bar_view.h"
#include "ios/chrome/browser/ui/ui_util.h"
#include "ui/base/l10n/l10n_util_mac.h"
#include "ui/base/resource/resource_bundle.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

NSString* const kFindInPageContainerViewId = @"kFindInPageContainerViewId";

namespace {

// Find Bar height.
// Right padding on iPad find bar.
const CGFloat kFindBarIPhoneHeight = 56;
const CGFloat kFindBarIPadHeight = 62;

// Padding added by the invisible background.
const CGFloat kBackgroundPadding = 6;

// Find Bar animation drop down duration.
const CGFloat kAnimationDuration = 0.15;

// For the first |kSearchDelayChars| characters, delay by |kSearchLongDelay|
// For the remaining characters, delay by |kSearchShortDelay|.
const NSUInteger kSearchDelayChars = 3;
const NSTimeInterval kSearchLongDelay = 1.0;
const NSTimeInterval kSearchShortDelay = 0.100;

}  // anonymous namespace

#pragma mark - FindBarControllerIOS

@interface FindBarControllerIOS ()<UITextFieldDelegate>

// Set up iPad UI
- (void)setUpIPad;
// Set up iPhone UI
- (void)setUpIPhone;
// Animate find bar to iPad top right, or, when possible, to align find bar
// horizontally with |alignmentFrame|.
- (void)showIPadFindBarView:(BOOL)animate
                   intoView:(UIView*)parentView
                  withFrame:(CGRect)targetFrame
             alignWithFrame:(CGRect)alignmentFrame
                 selectText:(BOOL)selectText;
// Animate find bar over iPhone toolbar.
- (void)showIPhoneFindBarView:(BOOL)animate
                     intoView:(UIView*)parentView
                    withFrame:(CGRect)targetFrame
                   selectText:(BOOL)selectText;
// Returns the appropriate variant of the image for |image_name| based on
// |_isIncognito| and device idiom.
- (UIImage*)imageWithName:(NSString*)image_name;
// Responds to touches that make editing changes on the text field, triggering
// find-in-page searches for the field's current value.
- (void)editingChanged;
// Return the expected find bar height. This will include the status bar height
// when running iOS7 on an iPhone.
- (CGFloat)findBarHeight;
// Selects text in such way that selection menu does not appear and
// a11y label is read. When -[UITextField selectAll:] is used, iOS
// will read "Select All" instead of a11y label.
- (void)selectAllText;

// Redefined to be readwrite. This view acts as background for |findBarView| and
// contains it as a subview.
@property(nonatomic, readwrite, strong) UIView* view;
// The view containing all the buttons and textfields that is common between
// iPhone and iPad.
@property(nonatomic, strong) FindBarView* findBarView;
// Typing delay timer.
@property(nonatomic, strong) NSTimer* delayTimer;
// Yes if incognito.
@property(nonatomic, assign) BOOL isIncognito;
@end

@implementation FindBarControllerIOS

@synthesize view = _view;
@synthesize findBarView = _findBarView;
@synthesize delayTimer = _delayTimer;
@synthesize isIncognito = _isIncognito;
@synthesize dispatcher = _dispatcher;

#pragma mark - Lifecycle

- (instancetype)initWithIncognito:(BOOL)isIncognito {
  self = [super init];
  if (self) {
    _isIncognito = isIncognito;
  }
  return self;
}

#pragma mark View Setup & Teardown

- (UIView*)constructFindBarView {
  BOOL isIPad = IsIPadIdiom();
  UIView* findBarBackground = nil;
  if (isIPad) {
    // Future self.view. Contains only |contentView|. Is an image view that is
    // typecast elsewhere but always is exposed as a UIView.
    findBarBackground =
        [[UIImageView alloc] initWithFrame:CGRectMake(0, 0, 345, 62)];
    findBarBackground.backgroundColor = [UIColor clearColor];
    findBarBackground.userInteractionEnabled = YES;
  } else {
    findBarBackground =
        [[UIView alloc] initWithFrame:CGRectMake(0, 0, 320, 56)];
    findBarBackground.backgroundColor = [UIColor whiteColor];
  }

  self.findBarView = [[FindBarView alloc]
      initWithDarkAppearance:self.isIncognito && !IsIPadIdiom()];
  [findBarBackground addSubview:self.findBarView];
  self.findBarView.translatesAutoresizingMaskIntoConstraints = NO;
  NSMutableArray* constraints = [[NSMutableArray alloc] init];
  [constraints addObjectsFromArray:@[
    [self.findBarView.trailingAnchor
        constraintEqualToAnchor:findBarBackground.trailingAnchor],
    [self.findBarView.leadingAnchor
        constraintEqualToAnchor:findBarBackground.leadingAnchor],
    [self.findBarView.heightAnchor constraintEqualToConstant:56.0f]
  ]];

  if (isIPad) {
    [constraints
        addObject:[self.findBarView.centerYAnchor
                      constraintEqualToAnchor:findBarBackground.centerYAnchor
                                     constant:-2]];
  } else {
    [constraints
        addObject:[self.findBarView.bottomAnchor
                      constraintEqualToAnchor:findBarBackground.bottomAnchor]];
  }

  [NSLayoutConstraint activateConstraints:constraints];

  self.findBarView.inputField.delegate = self;
  [self.findBarView.inputField addTarget:self
                                  action:@selector(editingChanged)
                        forControlEvents:UIControlEventEditingChanged];
  [self.findBarView.nextButton addTarget:self.dispatcher
                                  action:@selector(findNextStringInPage)
                        forControlEvents:UIControlEventTouchUpInside];
  [self.findBarView.nextButton addTarget:self
                                  action:@selector(hideKeyboard:)
                        forControlEvents:UIControlEventTouchUpInside];
  [self.findBarView.previousButton addTarget:self.dispatcher
                                      action:@selector(findPreviousStringInPage)
                            forControlEvents:UIControlEventTouchUpInside];
  [self.findBarView.previousButton addTarget:self
                                      action:@selector(hideKeyboard:)
                            forControlEvents:UIControlEventTouchUpInside];
  [self.findBarView.closeButton addTarget:self.dispatcher
                                   action:@selector(closeFindInPage)
                         forControlEvents:UIControlEventTouchUpInside];

  return findBarBackground;
}

- (void)setupViewInView:(UIView*)view {
  self.view = [self constructFindBarView];

  // Idiom specific setup.
  if ([self shouldShowCompactSearchBarInView:view])
    [self setUpIPhone];
  else
    [self setUpIPad];

  self.view.accessibilityIdentifier = kFindInPageContainerViewId;
}

- (void)teardownView {
  [self.view removeFromSuperview];
  self.view = nil;
}

- (void)setUpIPhone {
  CGRect frame = self.view.frame;
  frame.size.height = [self findBarHeight];
  self.view.frame = frame;

  if (self.isIncognito) {
    [self.view setBackgroundColor:[UIColor colorWithWhite:115 / 255.0 alpha:1]];
  }
}

- (void)setUpIPad {
  self.view.autoresizingMask = UIViewAutoresizingFlexibleLeftMargin;
  UIEdgeInsets backgroundInsets = UIEdgeInsetsMake(6, 9, 10, 8);
  UIImage* bgImage = [UIImage imageNamed:@"find_bg"];
  bgImage = [bgImage resizableImageWithCapInsets:backgroundInsets];
  UIImageView* bgView = (UIImageView*)self.view;
  [bgView setImage:bgImage];
}

#pragma mark - Public

- (NSString*)searchTerm {
  return [self.findBarView.inputField text];
}

- (BOOL)isFindInPageShown {
  return self.view != nil;
}

- (BOOL)isFocused {
  return [self.findBarView.inputField isFirstResponder];
}

- (void)updateResultsCount:(FindInPageModel*)model {
  [self updateWithMatchNumber:model.currentIndex
                   matchCount:model.matches
                   searchText:model.text];
}

- (void)updateView:(FindInPageModel*)model
     initialUpdate:(BOOL)initialUpdate
    focusTextfield:(BOOL)focusTextfield {
  [self.delayTimer invalidate];
  self.delayTimer = nil;

  if (initialUpdate) {
    // Set initial text and first search.
    [self.findBarView.inputField setText:model.text];
    [self editingChanged];
  }

  // Focus input field if necessary.
  if (focusTextfield) {
    [self.findBarView.inputField becomeFirstResponder];
  } else {
    [self.findBarView.inputField resignFirstResponder];
  }

  [self updateWithMatchNumber:model.currentIndex
                   matchCount:model.matches
                   searchText:model.text];
}

- (void)updateWithMatchNumber:(NSUInteger)matchNumber
                   matchCount:(NSUInteger)matchCount
                   searchText:(NSString*)searchText {
  NSString* text = nil;
  if (searchText.length != 0) {
    NSString* indexStr = [NSString stringWithFormat:@"%" PRIdNS, matchNumber];
    NSString* matchesStr = [NSString stringWithFormat:@"%" PRIdNS, matchCount];
    text = l10n_util::GetNSStringF(IDS_FIND_IN_PAGE_COUNT,
                                   base::SysNSStringToUTF16(indexStr),
                                   base::SysNSStringToUTF16(matchesStr));
  }
  [self.findBarView updateResultsLabelWithText:text];

  BOOL enabled = matchCount != 0;
  self.findBarView.nextButton.enabled = enabled;
  self.findBarView.previousButton.enabled = enabled;
}

- (void)addFindBarView:(BOOL)animate
              intoView:(UIView*)view
             withFrame:(CGRect)frame
        alignWithFrame:(CGRect)omniboxFrame
            selectText:(BOOL)selectText {
  // If already showing find bar, nothing to do.
  if (self.view) {
    return;
  }
  if ([self shouldShowCompactSearchBarInView:view]) {
    [self showIPhoneFindBarView:animate
                       intoView:view
                      withFrame:frame
                     selectText:selectText];
  } else {
    [self showIPadFindBarView:animate
                     intoView:view
                    withFrame:frame
               alignWithFrame:omniboxFrame
                   selectText:selectText];
  }
}

- (void)hideFindBarView:(BOOL)animate {
  // If view is nil, nothing to hide.
  if (!self.view) {
    return;
  }

  self.findBarView.inputField.selectedTextRange = nil;
  [self.delayTimer invalidate];
  self.delayTimer = nil;

  if (animate) {
    [UIView animateWithDuration:kAnimationDuration
        animations:^{
          CGRect frame = self.view.frame;
          frame.size.height = 0;
          self.view.frame = frame;
        }
        completion:^(BOOL finished) {
          [self teardownView];
        }];
  } else {
    [self teardownView];
  }
}

- (void)hideKeyboard:(id)sender {
  [self.view endEditing:YES];
}

- (UIImage*)imageWithName:(NSString*)imageName {
  NSString* name = !IsIPadIdiom() && self.isIncognito
                       ? [imageName stringByAppendingString:@"_incognito"]
                       : imageName;
  return [UIImage imageNamed:name];
}

#pragma mark - Internal

- (void)selectAllText {
  UITextRange* wholeTextRange = [self.findBarView.inputField
      textRangeFromPosition:self.findBarView.inputField.beginningOfDocument
                 toPosition:self.findBarView.inputField.endOfDocument];
  self.findBarView.inputField.selectedTextRange = wholeTextRange;
}

- (BOOL)shouldShowCompactSearchBarInView:(UIView*)view {
  return !IsIPadIdiom();
}

// Animate find bar to iPad top right.
- (void)showIPadFindBarView:(BOOL)animate
                   intoView:(UIView*)parentView
                  withFrame:(CGRect)targetFrame
             alignWithFrame:(CGRect)omniboxFrame
                 selectText:(BOOL)selectText {
  DCHECK(IsIPadIdiom());
  [self setupViewInView:parentView];
  UIView* view = self.view;
  CGRect frame = view.frame;
  frame.size.width =
      MIN(CGRectGetWidth(parentView.bounds), CGRectGetWidth(frame));
  frame.origin.y = targetFrame.origin.y;
  frame.size.height = 0;

  CGFloat containerWidth = parentView.bounds.size.width;

  // On iPad, there are three possible frames for the Search bar:
  // 1. In Regular width size class, it is short, right-aligned to the omnibox's
  //   right edge.
  // 2. In Compact size class, if the short bar width is less than the omnibox,
  //   stretch and align the search bar to the omnibox.
  // 3. Finally, if the short bar width is more than the omnibox, fill the
  //   container view from edge to edge, ignoring the omnibox.
  if (view.cr_widthSizeClass == REGULAR) {
    if (base::i18n::IsRTL()) {
      frame.origin.x = CGRectGetMinX(omniboxFrame) - kBackgroundPadding;
    } else {
      frame.origin.x = CGRectGetMinX(omniboxFrame) +
                       CGRectGetWidth(omniboxFrame) - frame.size.width +
                       kBackgroundPadding;
    }
  } else {
    // Compact size class.
    CGRect visibleFrame = CGRectInset(frame, kBackgroundPadding, 0);
    if (omniboxFrame.size.width > visibleFrame.size.width) {
      visibleFrame.origin.x = omniboxFrame.origin.x;
      visibleFrame.size.width = omniboxFrame.size.width;
      frame = CGRectInset(visibleFrame, -kBackgroundPadding, 0);
    } else {
      frame.origin.x = 0;
      frame.size.width = containerWidth;
    }
  }

  view.frame = frame;
  [parentView addSubview:view];

  CGFloat duration = (animate) ? kAnimationDuration : 0;
  [UIView animateWithDuration:duration
      animations:^{
        CGRect frame = view.frame;
        frame.size.height = [self findBarHeight];
        view.frame = frame;
      }
      completion:^(BOOL finished) {
        if (selectText)
          [self selectAllText];
      }];
}

// Animate find bar over iPhone toolbar.
- (void)showIPhoneFindBarView:(BOOL)animate
                     intoView:(UIView*)parentView
                    withFrame:(CGRect)targetFrame
                   selectText:(BOOL)selectText {
  [self setupViewInView:parentView];
  UIView* view = self.view;
  CGRect frame = view.frame;
  frame.size.width = targetFrame.size.width;
  frame.origin.y = 0 - frame.size.height;
  frame.origin.x = 0;
  view.frame = frame;
  [parentView addSubview:view];

  CGFloat duration = (animate) ? kAnimationDuration : 0;
  [UIView animateWithDuration:duration
      animations:^{
        CGRect frame = view.frame;
        frame.origin.y = 0;
        view.frame = frame;
      }
      completion:^(BOOL finished) {
        if (selectText)
          [self selectAllText];
      }];
}

- (void)editingChanged {
  [self.delayTimer invalidate];
  NSUInteger length = [[self searchTerm] length];
  if (length == 0) {
    [self.dispatcher searchFindInPage];
    return;
  }

  // Delay delivery of the search text event to give time for a user to type out
  // a longer word.  Use a longer delay when the input length is short, as short
  // words are currently very inefficient and lock up the web view.
  NSTimeInterval delay =
      (length > kSearchDelayChars) ? kSearchShortDelay : kSearchLongDelay;
  self.delayTimer =
      [NSTimer scheduledTimerWithTimeInterval:delay
                                       target:self.dispatcher
                                     selector:@selector(searchFindInPage)
                                     userInfo:nil
                                      repeats:NO];
}

- (CGFloat)findBarHeight {
  if (IsIPadIdiom())
    return kFindBarIPadHeight;
  return StatusBarHeight() + kFindBarIPhoneHeight;
}

#pragma mark - UITextFieldDelegate

- (BOOL)textFieldShouldBeginEditing:(UITextField*)textField {
  DCHECK(textField == self.findBarView.inputField);
  [[NSNotificationCenter defaultCenter]
      postNotificationName:kFindBarTextFieldWillBecomeFirstResponderNotification
                    object:self];
  return YES;
}

- (void)textFieldDidEndEditing:(UITextField*)textField {
  DCHECK(textField == self.findBarView.inputField);
  [[NSNotificationCenter defaultCenter]
      postNotificationName:kFindBarTextFieldDidResignFirstResponderNotification
                    object:self];
}

- (BOOL)textFieldShouldReturn:(UITextField*)textField {
  DCHECK(textField == self.findBarView.inputField);
  [self.findBarView.inputField resignFirstResponder];
  return YES;
}

@end
