// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/autofill/form_input_accessory_view_controller.h"

#include <memory>

#include "base/ios/block_types.h"
#include "base/mac/foundation_util.h"
#include "base/mac/scoped_block.h"
#import "components/autofill/core/browser/keyboard_accessory_metrics_logger.h"
#import "components/autofill/ios/browser/js_suggestion_manager.h"
#import "ios/chrome/browser/autofill/form_input_accessory_view.h"
#import "ios/chrome/browser/autofill/form_suggestion_view.h"
#import "ios/chrome/browser/passwords/password_generation_utils.h"
#include "ios/chrome/browser/ui/ui_util.h"
#import "ios/chrome/browser/ui/util/constraints_ui_util.h"
#import "ios/web/public/url_scheme_util.h"
#include "ios/web/public/web_state/form_activity_params.h"
#import "ios/web/public/web_state/js/crw_js_injection_receiver.h"
#import "ios/web/public/web_state/ui/crw_web_view_proxy.h"
#include "ios/web/public/web_state/url_verification_constants.h"
#include "ios/web/public/web_state/web_state.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace autofill {
NSString* const kFormSuggestionAssistButtonPreviousElement = @"previousTap";
NSString* const kFormSuggestionAssistButtonNextElement = @"nextTap";
NSString* const kFormSuggestionAssistButtonDone = @"done";
CGFloat const kInputAccessoryHeight = 44.0f;
}  // namespace autofill

namespace {

// Finds all views of a particular kind if class |klass| in the subview
// hierarchy of the given |root| view.
NSArray* FindDescendantsOfClass(UIView* root, Class klass) {
  DCHECK(root);
  NSMutableArray* viewsToExamine = [NSMutableArray arrayWithObject:root];
  NSMutableArray* descendants = [NSMutableArray array];

  while ([viewsToExamine count]) {
    UIView* view = [viewsToExamine lastObject];
    if ([view isKindOfClass:klass])
      [descendants addObject:view];

    [viewsToExamine removeLastObject];
    [viewsToExamine addObjectsFromArray:[view subviews]];
  }

  return descendants;
}

// Returns true if |item|'s action name contains |actionName|.
bool ItemActionMatchesName(UIBarButtonItem* item, NSString* actionName) {
  SEL itemAction = [item action];
  if (!itemAction)
    return false;
  NSString* itemActionName = NSStringFromSelector(itemAction);

  // We don't do a strict string match for the action name.
  return [itemActionName rangeOfString:actionName].location != NSNotFound;
}

// Finds all UIToolbarItems associated with a given UIToolbar |toolbar| with
// action selectors with a name that containts the action name specified by
// |actionName|.
NSArray* FindToolbarItemsForActionName(UIToolbar* toolbar,
                                       NSString* actionName) {
  NSMutableArray* toolbarItems = [NSMutableArray array];

  for (UIBarButtonItem* item in [toolbar items]) {
    if (ItemActionMatchesName(item, actionName))
      [toolbarItems addObject:item];
  }

  return toolbarItems;
}

// Finds all UIToolbarItem(s) with action selectors of the name specified by
// |actionName| in any UIToolbars in the view hierarchy below |root|.
NSArray* FindDescendantToolbarItemsForActionName(UIView* root,
                                                 NSString* actionName) {
  NSMutableArray* descendants = [NSMutableArray array];

  NSArray* toolbars = FindDescendantsOfClass(root, [UIToolbar class]);
  for (UIToolbar* toolbar in toolbars) {
    [descendants
        addObjectsFromArray:FindToolbarItemsForActionName(toolbar, actionName)];
  }

  return descendants;
}

NSArray* FindDescendantToolbarItemsForActionName(
    UITextInputAssistantItem* inputAssistantItem,
    NSString* actionName) {
  NSMutableArray* toolbarItems = [NSMutableArray array];

  NSMutableArray* buttonGroupsGroup = [[NSMutableArray alloc] init];
  if (inputAssistantItem.leadingBarButtonGroups)
    [buttonGroupsGroup addObject:inputAssistantItem.leadingBarButtonGroups];
  if (inputAssistantItem.trailingBarButtonGroups)
    [buttonGroupsGroup addObject:inputAssistantItem.trailingBarButtonGroups];
  for (NSArray* buttonGroups in buttonGroupsGroup) {
    for (UIBarButtonItemGroup* group in buttonGroups) {
      NSArray* items = group.barButtonItems;
      for (UIBarButtonItem* item in items) {
        if (ItemActionMatchesName(item, actionName))
          [toolbarItems addObject:item];
      }
    }
  }

  return toolbarItems;
}

}  // namespace

@interface FormInputAccessoryViewController ()

// Allows injection of the JsSuggestionManager.
- (instancetype)initWithWebState:(web::WebState*)webState
             JSSuggestionManager:(JsSuggestionManager*)JSSuggestionManager
                       providers:(NSArray*)providers;

// Called when the keyboard will or did change frame.
- (void)keyboardWillOrDidChangeFrame:(NSNotification*)notification;

// Called when the keyboard is dismissed.
- (void)keyboardDidHide:(NSNotification*)notification;

// Hides the subviews in |accessoryView|.
- (void)hideSubviewsInOriginalAccessoryView:(UIView*)accessoryView;

// Attempts to execute/tap/send-an-event-to the iOS built-in "next" and
// "previous" form assist controls. Returns NO if this attempt failed, YES
// otherwise. [HACK]
- (BOOL)executeFormAssistAction:(NSString*)actionName;

// Asynchronously retrieves an accessory view from |_providers|.
- (void)retrieveAccessoryViewForForm:(const web::FormActivityParams&)params
                            webState:(web::WebState*)webState;

// Clears the current custom accessory view and restores the default.
- (void)reset;

@end

@implementation FormInputAccessoryViewController {
  // The WebState this instance is observing. Will be null after
  // -webStateDestroyed: has been called.
  web::WebState* _webState;

  // Bridge to observe the web state from Objective-C.
  std::unique_ptr<web::WebStateObserverBridge> _webStateObserverBridge;

  // Last registered keyboard rectangle.
  CGRect _keyboardFrame;

  // The custom view that should be shown in the input accessory view.
  FormInputAccessoryView* _customAccessoryView;

  // The JS manager for interacting with the underlying form.
  JsSuggestionManager* _JSSuggestionManager;

  // The original subviews in keyboard accessory view that were originally not
  // hidden but were hidden when showing Autofill suggestions.
  NSMutableArray* _hiddenOriginalSubviews;

  // The objects that can provide a custom input accessory view while filling
  // forms.
  NSArray* _providers;

  // Whether suggestions have previously been shown.
  BOOL _suggestionsHaveBeenShown;

  // The object that manages the currently-shown custom accessory view.
  __weak id<FormInputAccessoryViewProvider> _currentProvider;

  // Logs UMA metrics for the keyboard accessory.
  std::unique_ptr<autofill::KeyboardAccessoryMetricsLogger>
      _keyboardAccessoryMetricsLogger;
}

- (instancetype)initWithWebState:(web::WebState*)webState
                       providers:(NSArray*)providers {
  JsSuggestionManager* suggestionManager =
      base::mac::ObjCCastStrict<JsSuggestionManager>(
          [webState->GetJSInjectionReceiver()
              instanceOfClass:[JsSuggestionManager class]]);
  return [self initWithWebState:webState
            JSSuggestionManager:suggestionManager
                      providers:providers];
}

- (instancetype)initWithWebState:(web::WebState*)webState
             JSSuggestionManager:(JsSuggestionManager*)JSSuggestionManager
                       providers:(NSArray*)providers {
  self = [super init];
  if (self) {
    DCHECK(webState);
    _webState = webState;
    _JSSuggestionManager = JSSuggestionManager;
    _hiddenOriginalSubviews = [[NSMutableArray alloc] init];
    _webStateObserverBridge =
        std::make_unique<web::WebStateObserverBridge>(self);
    _webState->AddObserver(_webStateObserverBridge.get());
    _providers = [providers copy];
    _suggestionsHaveBeenShown = NO;
    _keyboardAccessoryMetricsLogger.reset(
        new autofill::KeyboardAccessoryMetricsLogger());
  }
  return self;
}

- (void)wasShown {
  // There is no defined relation on the timing of JavaScript events and
  // keyboard showing up. So it is necessary to listen to the keyboard
  // notification to make sure the keyboard is updated.
  if (IsIPadIdiom()) {
    [[NSNotificationCenter defaultCenter]
        addObserver:self
           selector:@selector(keyboardWillOrDidChangeFrame:)
               name:UIKeyboardWillChangeFrameNotification
             object:nil];
    [[NSNotificationCenter defaultCenter]
        addObserver:self
           selector:@selector(textInputDidBeginEditing:)
               name:UITextFieldTextDidBeginEditingNotification
             object:nil];
    [[NSNotificationCenter defaultCenter]
        addObserver:self
           selector:@selector(textInputDidBeginEditing:)
               name:UITextViewTextDidBeginEditingNotification
             object:nil];
  }
  [[NSNotificationCenter defaultCenter]
      addObserver:self
         selector:@selector(keyboardWillOrDidChangeFrame:)
             name:UIKeyboardDidChangeFrameNotification
           object:nil];
  [[NSNotificationCenter defaultCenter]
      addObserver:self
         selector:@selector(keyboardDidHide:)
             name:UIKeyboardDidHideNotification
           object:nil];
}

- (void)wasHidden {
  [_customAccessoryView removeFromSuperview];
  [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void)detachFromWebState {
  [self reset];
  if (_webState) {
    _webState->RemoveObserver(_webStateObserverBridge.get());
    _webStateObserverBridge.reset();
    _webState = nullptr;
  }
}

- (void)dealloc {
  if (_webState) {
    _webState->RemoveObserver(_webStateObserverBridge.get());
    _webStateObserverBridge.reset();
    _webState = nullptr;
  }
}

- (id<CRWWebViewProxy>)webViewProxy {
  return _webState ? _webState->GetWebViewProxy() : nil;
}

- (void)hideSubviewsInOriginalAccessoryView:(UIView*)accessoryView {
  for (UIView* subview in [accessoryView subviews]) {
    if (!subview.hidden) {
      [_hiddenOriginalSubviews addObject:subview];
      subview.hidden = YES;
    }
  }
}

- (void)showCustomInputAccessoryView:(UIView*)view {
  DCHECK(view);
  if (IsIPadIdiom()) {
    // On iPad, there's no inputAccessoryView available, so we attach the custom
    // view directly to the keyboard view instead.
    [_customAccessoryView removeFromSuperview];

    // If the keyboard isn't visible don't show the custom view.
    if (CGRectIntersection([UIScreen mainScreen].bounds, _keyboardFrame)
                .size.height == 0 ||
        CGRectEqualToRect(_keyboardFrame, CGRectZero)) {
      _customAccessoryView = nil;
      return;
    }

    // If this is a form suggestion view and no suggestions have been triggered
    // yet, don't show the custom view.
    FormSuggestionView* formSuggestionView =
        base::mac::ObjCCast<FormSuggestionView>(view);
    if (formSuggestionView) {
      int numSuggestions = [[formSuggestionView suggestions] count];
      if (!_suggestionsHaveBeenShown && numSuggestions == 0) {
        _customAccessoryView = nil;
        return;
      }
    }
    _suggestionsHaveBeenShown = YES;

    _customAccessoryView = [[FormInputAccessoryView alloc] init];
    [_customAccessoryView setUpWithCustomView:view];

    CGFloat height = autofill::kInputAccessoryHeight;
    CGRect contentFrame = self.webViewProxy.frame;
    _customAccessoryView.frame = CGRectMake(contentFrame.origin.x, -height,
                                            contentFrame.size.width, height);

    UIView* keyboardView = [self getKeyboardView];
    DCHECK(keyboardView);
    [keyboardView addSubview:_customAccessoryView];
  } else {
    // On iPhone, the custom view replaces the default UI of the
    // inputAccessoryView.
    [self restoreDefaultInputAccessoryView];
    UIView* inputAccessoryView = [self.webViewProxy keyboardAccessory];
    if (inputAccessoryView) {
      [self hideSubviewsInOriginalAccessoryView:inputAccessoryView];
      _customAccessoryView = [[FormInputAccessoryView alloc] init];
      [_customAccessoryView setUpWithNavigationDelegate:self customView:view];
      [inputAccessoryView addSubview:_customAccessoryView];
      AddSameConstraints(_customAccessoryView, inputAccessoryView);
    }
  }
}

- (void)restoreDefaultInputAccessoryView {
  [_customAccessoryView removeFromSuperview];
  _customAccessoryView = nil;
  for (UIView* subview in _hiddenOriginalSubviews) {
    subview.hidden = NO;
  }
  [_hiddenOriginalSubviews removeAllObjects];
}

- (void)closeKeyboardWithButtonPress {
  [self closeKeyboardWithoutButtonPress];
  if (_currentProvider && [_currentProvider getLogKeyboardAccessoryMetrics])
    _keyboardAccessoryMetricsLogger->OnCloseButtonPressed();
}

- (void)closeKeyboardWithoutButtonPress {
  BOOL performedAction =
      [self executeFormAssistAction:autofill::kFormSuggestionAssistButtonDone];

  if (!performedAction) {
    // We could not find the built-in form assist controls, so try to focus
    // the next or previous control using JavaScript.
    [_JSSuggestionManager closeKeyboard];
  }
}

- (BOOL)executeFormAssistAction:(NSString*)actionName {
  NSArray* descendants = nil;
  if (IsIPadIdiom()) {
    UITextInputAssistantItem* inputAssistantItem =
        [self.webViewProxy inputAssistantItem];
    if (!inputAssistantItem)
      return NO;
    descendants =
        FindDescendantToolbarItemsForActionName(inputAssistantItem, actionName);
  } else {
    UIView* inputAccessoryView = [self.webViewProxy keyboardAccessory];
    if (!inputAccessoryView)
      return NO;
    descendants =
        FindDescendantToolbarItemsForActionName(inputAccessoryView, actionName);
  }

  if (![descendants count])
    return NO;

  UIBarButtonItem* item = descendants[0];
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Warc-performSelector-leaks"
  [[item target] performSelector:[item action] withObject:item];
#pragma clang diagnostic pop
  return YES;
}

#pragma mark -
#pragma mark FormInputAccessoryViewDelegate

- (void)selectPreviousElementWithButtonPress {
  [self selectPreviousElementWithoutButtonPress];
  if (_currentProvider && [_currentProvider getLogKeyboardAccessoryMetrics])
    _keyboardAccessoryMetricsLogger->OnPreviousButtonPressed();
}

- (void)selectPreviousElementWithoutButtonPress {
  BOOL performedAction =
      [self executeFormAssistAction:
                autofill::kFormSuggestionAssistButtonPreviousElement];
  if (!performedAction) {
    // We could not find the built-in form assist controls, so try to focus
    // the next or previous control using JavaScript.
    [_JSSuggestionManager selectPreviousElement];
  }
}

- (void)selectNextElementWithButtonPress {
  [self selectNextElementWithoutButtonPress];
  if (_currentProvider && [_currentProvider getLogKeyboardAccessoryMetrics])
    _keyboardAccessoryMetricsLogger->OnNextButtonPressed();
}

- (void)selectNextElementWithoutButtonPress {
  BOOL performedAction = [self
      executeFormAssistAction:autofill::kFormSuggestionAssistButtonNextElement];

  if (!performedAction) {
    // We could not find the built-in form assist controls, so try to focus
    // the next or previous control using JavaScript.
    [_JSSuggestionManager selectNextElement];
  }
}

- (void)fetchPreviousAndNextElementsPresenceWithCompletionHandler:
        (void (^)(BOOL, BOOL))completionHandler {
  DCHECK(completionHandler);
  [_JSSuggestionManager
      fetchPreviousAndNextElementsPresenceWithCompletionHandler:
          completionHandler];
}

#pragma mark -
#pragma mark CRWWebStateObserver

- (void)webState:(web::WebState*)webState didLoadPageWithSuccess:(BOOL)success {
  DCHECK_EQ(_webState, webState);
  [self reset];
}

- (void)webState:(web::WebState*)webState
    didRegisterFormActivity:(const web::FormActivityParams&)params {
  DCHECK_EQ(_webState, webState);
  web::URLVerificationTrustLevel trustLevel;
  const GURL pageURL(webState->GetCurrentURL(&trustLevel));
  if (params.input_missing ||
      trustLevel != web::URLVerificationTrustLevel::kAbsolute ||
      !web::UrlHasWebScheme(pageURL) || !webState->ContentIsHTML()) {
    [self reset];
    return;
  }

  if (params.type == "blur" || params.type == "change" ||
      params.type == "form_changed") {
    return;
  }

  [self retrieveAccessoryViewForForm:params webState:webState];
}

- (void)webStateDestroyed:(web::WebState*)webState {
  DCHECK_EQ(_webState, webState);
  [self detachFromWebState];
}

- (void)reset {
  if (_currentProvider) {
    [_currentProvider inputAccessoryViewControllerDidReset:self];
    _currentProvider = nil;
  }
  [self restoreDefaultInputAccessoryView];

  _keyboardAccessoryMetricsLogger.reset(
      new autofill::KeyboardAccessoryMetricsLogger());
}

- (void)retrieveAccessoryViewForForm:(const web::FormActivityParams&)params
                            webState:(web::WebState*)webState {
  __weak FormInputAccessoryViewController* weakSelf = self;
  web::FormActivityParams strongParams = params;

  // Build a block for each provider that will invoke its completion with YES
  // if the provider can provide an accessory view for the specified form/field
  // and NO otherwise.
  NSMutableArray* findProviderBlocks = [[NSMutableArray alloc] init];
  for (NSUInteger i = 0; i < [_providers count]; i++) {
    passwords::PipelineBlock block =
        ^(void (^completion)(BOOL success)) {
          // Access all the providers through |self| to guarantee that both
          // |self| and all the providers exist when the block is executed.
          // |_providers| is immutable, so the subscripting is always valid.
          FormInputAccessoryViewController* strongSelf = weakSelf;
          if (!strongSelf)
            return;
          id<FormInputAccessoryViewProvider> provider =
              strongSelf->_providers[i];
          [provider checkIfAccessoryViewIsAvailableForForm:strongParams
                                                  webState:webState
                                         completionHandler:completion];
        };
    [findProviderBlocks addObject:block];
  }

  // Once the view is retrieved, update the UI.
  AccessoryViewReadyCompletion readyCompletion =
      ^(UIView* accessoryView, id<FormInputAccessoryViewProvider> provider) {
        FormInputAccessoryViewController* strongSelf = weakSelf;
        if (!strongSelf || !strongSelf->_currentProvider)
          return;
        DCHECK_EQ(strongSelf->_currentProvider, provider);
        [provider setAccessoryViewDelegate:strongSelf];
        [strongSelf showCustomInputAccessoryView:accessoryView];
      };

  // Once a provider is found, use it to retrieve the accessory view.
  passwords::PipelineCompletionBlock onProviderFound =
      ^(NSUInteger providerIndex) {
        if (providerIndex == NSNotFound) {
          [weakSelf reset];
          return;
        }
        FormInputAccessoryViewController* strongSelf = weakSelf;
        if (!strongSelf || !strongSelf->_webState)
          return;
        id<FormInputAccessoryViewProvider> provider =
            strongSelf->_providers[providerIndex];
        [strongSelf->_currentProvider
            inputAccessoryViewControllerDidReset:self];
        strongSelf->_currentProvider = provider;
        [strongSelf->_currentProvider
            retrieveAccessoryViewForForm:strongParams
                                webState:webState
                accessoryViewUpdateBlock:readyCompletion];
      };

  // Run all the blocks in |findProviderBlocks| until one invokes its
  // completion with YES. The first one to do so will be passed to
  // |onProviderFound|.
  passwords::RunSearchPipeline(findProviderBlocks, onProviderFound);
}

- (UIView*)getKeyboardView {
  NSArray* windows = [UIApplication sharedApplication].windows;
  if (windows.count < 2)
    return nil;

  UIWindow* window = windows[1];
  for (UIView* subview in window.subviews) {
    if ([NSStringFromClass([subview class]) rangeOfString:@"PeripheralHost"]
            .location != NSNotFound) {
      return subview;
    }
    if ([NSStringFromClass([subview class]) rangeOfString:@"SetContainer"]
            .location != NSNotFound) {
      for (UIView* subsubview in subview.subviews) {
        if ([NSStringFromClass([subsubview class]) rangeOfString:@"SetHost"]
                .location != NSNotFound) {
          return subsubview;
        }
      }
    }
  }

  return nil;
}

- (void)keyboardWillOrDidChangeFrame:(NSNotification*)notification {
  if (!_webState || !_currentProvider)
    return;
  CGRect keyboardFrame =
      [notification.userInfo[UIKeyboardFrameEndUserInfoKey] CGRectValue];
  // With iOS8 (beta) this method can be called even when the rect has not
  // changed. When this is detected we exit early.
  if (CGRectEqualToRect(CGRectIntegral(_keyboardFrame),
                        CGRectIntegral(keyboardFrame))) {
    return;
  }
  _keyboardFrame = keyboardFrame;
  [_currentProvider resizeAccessoryView];
}

// On iPads running iOS 9 or later, when any text field or text view (e.g.
// omnibox, settings, card unmask dialog) begins editing, reset ourselves so
// that we don't present our custom view over the keyboard.
- (void)textInputDidBeginEditing:(NSNotification*)notification {
  [self reset];
}

- (void)keyboardDidHide:(NSNotification*)notification {
  _keyboardFrame = CGRectZero;
}

@end
