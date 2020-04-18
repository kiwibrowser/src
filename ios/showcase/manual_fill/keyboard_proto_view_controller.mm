// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/showcase/manual_fill/keyboard_proto_view_controller.h"

#import <WebKit/WebKit.h>

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace manualfill {

void AddSameConstraints(UIView* sourceView, UIView* destinationView) {
  [NSLayoutConstraint activateConstraints:@[
    [sourceView.leadingAnchor
        constraintEqualToAnchor:destinationView.leadingAnchor],
    [sourceView.trailingAnchor
        constraintEqualToAnchor:destinationView.trailingAnchor],
    [sourceView.bottomAnchor
        constraintEqualToAnchor:destinationView.bottomAnchor],
    [sourceView.topAnchor constraintEqualToAnchor:destinationView.topAnchor],
  ]];
}

UIView* GetFirstResponderSubview(UIView* view) {
  if ([view isFirstResponder])
    return view;

  for (UIView* subview in [view subviews]) {
    UIView* firstResponder = GetFirstResponderSubview(subview);
    if (firstResponder)
      return firstResponder;
  }

  return nil;
}

}  // namespace manualfill

@interface KeyboardProtoViewController ()

// The last recorded active field identifier, used to interact with the web
// view (i.e. add CSS focus to the element).
@property(nonatomic, strong) NSString* activeFieldID;

@end

@implementation KeyboardProtoViewController

@synthesize activeFieldID = _activeFieldID;
@synthesize lastFirstResponder = _lastFirstResponder;
@synthesize webView = _webView;

- (void)viewDidLoad {
  [super viewDidLoad];

  WKWebViewConfiguration* configuration = [[WKWebViewConfiguration alloc] init];
  _webView = [[WKWebView alloc] initWithFrame:self.view.bounds
                                configuration:configuration];
  [self.view addSubview:self.webView];
  self.webView.translatesAutoresizingMaskIntoConstraints = NO;
  manualfill::AddSameConstraints(self.webView, self.view);
}

- (void)viewWillAppear:(BOOL)animated {
  [super viewWillAppear:animated];

  NSURL* sigupURL = [NSURL URLWithString:@"https://appleid.apple.com/account"];
  NSURLRequest* request = [NSURLRequest requestWithURL:sigupURL];
  [self.webView loadRequest:request];
}

#pragma mark - ManualFillContentDelegate

- (void)userDidPickContent:(NSString*)content {
  // No-op. Subclasess can override.
}

#pragma mark - Document Interaction

- (void)updateActiveFieldID {
  __weak KeyboardProtoViewController* weakSelf = self;
  NSString* javaScriptQuery = @"document.activeElement.id";
  [self.webView evaluateJavaScript:javaScriptQuery
                 completionHandler:^(id result, NSError* error) {
                   NSLog(@"result: %@", [result description]);
                   weakSelf.activeFieldID = result;
                   [weakSelf callFocusOnLastActiveField];
                 }];
}

- (void)fillLastSelectedFieldWithString:(NSString*)string {
  if ([self.lastFirstResponder conformsToProtocol:@protocol(UIKeyInput)]) {
    [(id<UIKeyInput>)self.lastFirstResponder insertText:string];
  }
}

- (void)callFocusOnLastActiveField {
  NSString* javaScriptQuery =
      [NSString stringWithFormat:@"document.getElementById(\"%@\").focus()",
                                 self.activeFieldID];
  [self.webView evaluateJavaScript:javaScriptQuery completionHandler:nil];
}

@end
