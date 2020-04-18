// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/contextual_search/contextual_search_controller.h"

#include <memory>
#include <utility>

#include "base/json/json_reader.h"
#include "base/logging.h"
#import "base/mac/bind_objc_block.h"
#include "base/mac/foundation_util.h"
#include "base/mac/scoped_block.h"
#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "base/values.h"
#include "components/google/core/browser/google_util.h"
#include "components/search_engines/template_url_service.h"
#include "ios/chrome/browser/application_context.h"
#import "ios/chrome/browser/procedural_block_types.h"
#import "ios/chrome/browser/tabs/tab.h"
#include "ios/chrome/browser/ui/contextual_search/contextual_search_context.h"
#include "ios/chrome/browser/ui/contextual_search/contextual_search_delegate.h"
#import "ios/chrome/browser/ui/contextual_search/contextual_search_highlighter_view.h"
#import "ios/chrome/browser/ui/contextual_search/contextual_search_metrics.h"
#import "ios/chrome/browser/ui/contextual_search/js_contextual_search_manager.h"
#import "ios/chrome/browser/ui/contextual_search/touch_to_search_permissions_mediator.h"
#import "ios/chrome/browser/ui/contextual_search/window_gesture_observer.h"
#include "ios/chrome/browser/ui/ui_util.h"
#import "ios/chrome/browser/ui/uikit_ui_util.h"
#import "ios/chrome/browser/web/dom_altering_lock.h"
#include "ios/chrome/common/string_util.h"
#include "ios/chrome/grit/ios_strings.h"
#import "ios/third_party/material_components_ios/src/components/Snackbar/src/MaterialSnackbar.h"
#include "ios/web/public/browser_state.h"
#include "ios/web/public/load_committed_details.h"
#include "ios/web/public/referrer.h"
#import "ios/web/public/web_state/js/crw_js_injection_receiver.h"
#import "ios/web/public/web_state/ui/crw_web_view_proxy.h"
#import "ios/web/public/web_state/ui/crw_web_view_scroll_view_proxy.h"
#include "ios/web/public/web_state/web_state.h"
#include "ios/web/public/web_state/web_state_observer.h"
#import "ios/web/public/web_state/web_state_observer_bridge.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/l10n/l10n_util_mac.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
// command prefix for injected JavaScript.
const std::string kCommandPrefix = "contextualSearch";

// Distance from edges of frame when scrolling to show selection.
const CGFloat kYScrollMargin = 30.0;
const CGFloat kXScrollMargin = 10.0;

// Delay to check if there is a DOM modification (in second).
// If delay is too short, JavaScript won't have time to handle the event and the
// DOM tree will be modified after the highlight.
// If delay is too long, the user experience will be degraded.
// Experiments on some websites (e.g. belgianrails.be) show that delay must be
// over 0.35 second.
// Timeline is as follow:
// t: tap happens,
// t + doubleTapDelay (t1): tap is triggered
// t1: JavaScript handles tap,
// t1 + delta1: DOM may be modified,
// t1 + kDOMModificationDelaySeconds: |handleTapFrom:| starts handling tap,
// t1 + kDOMModificationDelaySeconds + delta2: JavaScript handleTap is called.
//
// The delay between DOM mutation and contextual search tap handling is really
// kDOMModificationDelaySeconds + delta2 - delta1.
// To handle this random delta value, the timeout passed to JavaScript is
// doubled
// The body touch event timeout must include the double tap delay.
const CGFloat kDOMModificationDelaySeconds = 0.1;
const CGFloat kDOMModificationDelayForJavaScriptMilliseconds =
    2 * 1000 * kDOMModificationDelaySeconds;

// The touch delay disables CS on many sites, so for now it is disabled.
// The previous value used was:
//        kDOMModificationDelayForJavaScriptMilliseconds + 300
const CGFloat kBodyTouchDelayForJavaScriptMilliseconds = 0;

// After a dismiss, do not allow retrigger before a delay to prevent triggering
// on double tap, and to prevent retrigger on the same event.
const NSTimeInterval kPreventTriggerAfterDismissDelaySeconds = 0.3;

CGRect StringValueToRect(NSString* rectString) {
  double rectTop, rectBottom, rectLeft, rectRight;
  NSArray* items = [rectString componentsSeparatedByString:@" "];
  if ([items count] != 4) {
    return CGRectNull;
  }
  rectTop = [[items objectAtIndex:0] doubleValue];
  rectBottom = [[items objectAtIndex:1] doubleValue];
  rectLeft = [[items objectAtIndex:2] doubleValue];
  rectRight = [[items objectAtIndex:3] doubleValue];
  if (isnan(rectTop) || isinf(rectTop) || isnan(rectBottom) ||
      isinf(rectBottom) || isnan(rectLeft) || isinf(rectLeft) ||
      isnan(rectRight) || isinf(rectRight) || rectRight <= rectLeft ||
      rectBottom <= rectTop) {
    return CGRectNull;
  }
  CGRect rect =
      CGRectMake(rectLeft, rectTop, rectRight - rectLeft, rectBottom - rectTop);
  return rect;
}

NSArray* StringValueToRectArray(const std::string& list) {
  NSString* nsList = base::SysUTF8ToNSString(list);
  NSMutableArray* rectsArray = [[NSMutableArray alloc] init];
  NSArray* items = [nsList componentsSeparatedByString:@","];
  for (NSString* item : items) {
    CGRect rect = StringValueToRect(item);
    if (CGRectIsNull(rect)) {
      return nil;
    }
    [rectsArray addObject:[NSValue valueWithCGRect:rect]];
  }
  return rectsArray;
}

}  // namespace

@interface ContextualSearchController ()<DOMAltering,
                                         CRWWebViewScrollViewProxyObserver,
                                         CRWWebStateObserver,
                                         UIGestureRecognizerDelegate,
                                         ContextualSearchHighlighterDelegate,
                                         TouchToSearchPermissionsChangeAudience>

// Permissions interface for this feature. Property is readwrite for testing.
@property(nonatomic, readwrite, strong)
    TouchToSearchPermissionsMediator* permissions;

// Synchronous method executed by -asynchronouslyEnableContextualSearch:
- (void)doEnableContextualSearch:(BOOL)enabled;

// Handler for injected JavaScript callbacks.
- (BOOL)handleScriptCommand:(const base::DictionaryValue&)JSONCommand;

// Handle the selection change event if the DOM lock is acquired.
// |selection| is the currently selected text in the webview.
// if |updated| is true, then the selection changed by the user moving one of
// the selection handles (not making a new selection).
// If |selectionValid| is false, the selection contains invalid chars or element
// and TTS should be dismissed. If selection is invalid, |selection| is empty.
- (void)handleSelectionChanged:(const std::string&)selection
              selectionUpdated:(BOOL)update
                selectionValid:(BOOL)selectionValid;

// Action for the tap gesture recognizer.
- (void)handleTapFrom:(UIGestureRecognizer*)gestureRecognizer;

// Handle a tap on a web view at |point|, extracting contextual search
// information from the tapped word and surrounding text.
- (void)handleTapAtPoint:(CGPoint)point;

// Initialize the contextual search JavaScript.
- (void)initializeWebViewForContextualSearch;

// Update the webViewProxy for the current tab to enable/disable scroll view
// observation.
- (void)updateWebViewProxy:(id<CRWWebViewProxy>)webViewProxy;

// Updates the UI to match the current state, setting the text label content
// if there is a current search context, and setting the panel state.
- (void)updateUI;

// Updates the UI for a resolved search.
- (void)updateForResolvedSearch:
    (ContextualSearchDelegate::SearchResolution)resolution;

// State changes.
// Set the state of the panel, given |reason|. Handles metrics updates.
- (void)setState:(ContextualSearch::PanelState)state
          reason:(ContextualSearch::StateChangeReason)reason;

// Dismiss pane for |reason|, invoking |completionHandler|, if any, after
// clearing any existing highlighted text in the webview, and finally releasing
// the DOM lock.
- (void)
dismissPaneWithJavascriptCompletionHandler:(ProceduralBlock)completionHandler
                                    reason:(ContextualSearch::StateChangeReason)
                                               reason;

// Clean-up the web state (release lock, clear highlight...) in case of a
// dismiss.
- (void)cleanUpWebStateForDismissWithCompletion:
    (ProceduralBlock)completionHandler;

// Convenience method for dismissing the pane with no completion handler.
- (void)dismissPane:(ContextualSearch::StateChangeReason)reason;

// Peek (show at the bottom of the window) the pane for |reason|.
- (void)peekPane:(ContextualSearch::StateChangeReason)reason;

// Preview the pane (covering kPreviewingDisplayRatio of the webview) for
// |reason|.
- (void)previewPane:(ContextualSearch::StateChangeReason)reason;

// Cover the pane (covering the entire webview) for |reason|.
- (void)coverPane:(ContextualSearch::StateChangeReason)reason;

// Scroll the webview to show the highlighted text.
// Scroll the minimal distance to put |_highlightBoundingRect| at
// |kYScrollMargin| from top and bottom edges and |kXScrollMargin| from left and
// right edges.
// Overflow policy :
// - horizontal: center |_highlightBoundingRect|,
// - vertical: put |_highlightBoundingRect| at |kYScrollMargin| from top edge.
- (void)scrollToShowSelection:(CRWWebViewScrollViewProxy*)scrollView;

@end

@implementation ContextualSearchController {
  // Permissions interface for this feature.
  TouchToSearchPermissionsMediator* _permissions;

  // WebState for the tab this object is attached to.
  web::WebState* _webState;

  // Access to the web view from |_webState|.
  id<CRWWebViewProxy> _webViewProxy;

  // Observer for |_webState|.
  std::unique_ptr<web::WebStateObserverBridge> _webStateObserver;

  // Object that manages find_in_page.js injection into the web view.
  __weak JsContextualSearchManager* _contextualSearchJsManager;

  // Gesture reccognizer for contextual search taps.
  UITapGestureRecognizer* _tapRecognizer;

  // Gesture reccognizer for double tap. It is used to prevent |_tapRecognizer|
  // from firing if there is a double tap on the web view. It is disabled when
  // the panel is displayed, since any tap will dismiss the panel in that case.
  UITapGestureRecognizer* _doubleTapRecognizer;

  // Gesture recognizer for long-tap copy.
  UILongPressGestureRecognizer* _copyGestureRecognizer;

  // Gesture recognizer to detect taps outside of the CS interface that would
  // cause it to dismiss.
  WindowGestureObserver* _dismissRecognizer;

  // Context information retrieved from a search tap.
  std::shared_ptr<ContextualSearchContext> _searchContext;

  // Resolved search information generated from the context or text selection.
  ContextualSearchDelegate::SearchResolution _resolvedSearch;

  // Delegate for fetching search information.
  std::unique_ptr<ContextualSearchDelegate> _delegate;

  // The view containing the highlighting of the search terms.
  __weak ContextualSearchHighlighterView* _contextualHighlightView;

  // Vertical constraints for layout of the search tab.
  NSArray* _searchTabVerticalConstraints;

  // The tab that should be used as the opener for the search tab.
  Tab* _opener;

  // YES if a cancel event was received since last tap, meaning the current tap
  // must not result in a search.
  BOOL _currentTapCancelled;

  // The current selection text.
  std::string _selectedText;

  // Boolean to track if the current WebState is enabled (has
  // gesture recognizers and DOM lock set up).
  BOOL _webStateEnabled;

  // Boolean to distinguish selection-clearing taps on the webview from
  // those on other UI elements.
  BOOL _webViewTappedWithSelection;

  // Metrics tracking variables and flags.
  // Time the tap handler fires. The delay of doubleTap is not counted.
  base::Time _tapTime;
  // Has the user entered the previewing/covering state yet for the
  // current search?
  BOOL _enteredPreviewing;
  BOOL _enteredCovering;
  // Has the search results content been visible for the current search?
  BOOL _resultsVisible;
  // Has the user exited the peeking/previewing/covering state yet for the
  // current search?
  BOOL _exitedPeeking;
  BOOL _exitedPreviewing;
  BOOL _exitedCovering;
  // Was the first run flow invoked during this search?
  BOOL _searchInvolvedFirstRun;
  // Did the first run panel become visible during this search?
  BOOL _firstRunPanelBecameVisible;
  // Was the search triggered by a long-press selection? Unlike other metrics-
  // related flags, this is not reset when a search ends; instead it is set
  // when a new search is started.
  BOOL _searchTriggeredBySelection;
  // Has the current search used SERP navigation (tapped on a link on the
  // search results page)?
  BOOL _usedSERPNavigation;
  // Boolean to track if the script has been injected in the current page. This
  // is a faster check than asking the JS controller.
  BOOL _isScriptInjected;

  // Boolean to track if the UIMenuControllerWillShowMenuNotification is
  // observed (to prevent double observation).
  BOOL _observingActionMenu;
  // Boolean to track if the current search is triggered by selection, and
  // action menu should be disabled.
  BOOL _preventActionMenu;
  // Boolean to track if a new text selection has been made (as opposed to an
  // existing one being changed) which will trigger the appearance of the
  // panel.
  BOOL _newSelectionDisplaying;

  // Boolean to temporarly disable preloading of search tab.
  BOOL _preventPreload;

  // Boolean to track if the search term has been resolved.
  BOOL _searchTermResolved;

  // True when closed has been called and contextual search controller
  // has been destroyed.
  BOOL _closed;

  // When view is resized, JavaScript and UIView sizes are not updated at the
  // same time. Computing a scroll delta to make selection visible in these
  // conditions will likely scroll to a random position.
  BOOL _preventScrollToShowSelection;

  // The time of the last dismiss.
  NSDate* _lastDismiss;
}

@synthesize enabled = _enabled;
@synthesize webState = _webState;

- (instancetype)initWithBrowserState:(ios::ChromeBrowserState*)browserState {
  if ((self = [super init])) {
    _permissions = [[TouchToSearchPermissionsMediator alloc]
        initWithBrowserState:browserState];
    [_permissions setAudience:self];

    // Set up the web state observer. This lasts as long as this object does,
    // but it will observe and un-observe the web tabs as it changes over time.
    _webStateObserver = std::make_unique<web::WebStateObserverBridge>(self);

    _copyGestureRecognizer = [[UILongPressGestureRecognizer alloc]
        initWithTarget:self
                action:@selector(handleLongPressFrom:)];

    __weak ContextualSearchController* weakself = self;
    auto callback = base::BindBlockArc(
        ^(ContextualSearchDelegate::SearchResolution resolution) {
          [weakself updateForResolvedSearch:resolution];
        });

    _delegate.reset(new ContextualSearchDelegate(browserState, callback));
  }
  return self;
}

- (TouchToSearchPermissionsMediator*)permissions {
  return _permissions;
}

- (void)setPermissions:(TouchToSearchPermissionsMediator*)permissions {
  _permissions = permissions;
}

- (void)enableContextualSearch:(BOOL)enabled {
  // Asynchronously enables contextual search, so that some preferences
  // (UIAccessibilityIsVoiceOverRunning(), for example) have time to synchronize
  // with their own notifications.
  __weak ContextualSearchController* weakSelf = self;
  dispatch_async(dispatch_get_main_queue(), ^{
    [weakSelf doEnableContextualSearch:enabled];
  });
}

- (void)doEnableContextualSearch:(BOOL)enabled {
  enabled = enabled && [self.permissions canEnable];

  BOOL changing = _enabled != enabled;
  if (changing) {
    if (!enabled) {
      [self dismissPane:ContextualSearch::RESET];
    }
    _enabled = enabled;
    [self enableCurrentWebState];
  }
}

- (void)updateWebViewProxy:(id<CRWWebViewProxy>)webViewProxy {
  if (_webViewProxy) {
    [[_webViewProxy scrollViewProxy] removeObserver:self];
  }
  _webViewProxy = webViewProxy;
  if (_webViewProxy) {
    [[_webViewProxy scrollViewProxy] addObserver:self];
  }
}

- (void)setTab:(Tab*)tab {
  [self setWebState:tab.webState];
}

- (void)setWebState:(web::WebState*)webState {
  [self disconnectWebState];
  if (webState) {
    _contextualSearchJsManager = static_cast<JsContextualSearchManager*>(
        [webState->GetJSInjectionReceiver()
            instanceOfClass:[JsContextualSearchManager class]]);
    _webState = webState;
    _webState->AddObserver(_webStateObserver.get());
    [self updateWebViewProxy:webState->GetWebViewProxy()];
    [self enableCurrentWebState];
  } else {
    _webState = nullptr;
  }
}

- (void)enableCurrentWebState {
  if (![self webState])
    return;
  if (_enabled && [self webState]->ContentIsHTML()) {
    if (!_webStateEnabled) {
      DOMAlteringLock::CreateForWebState([self webState]);

      __weak ContextualSearchController* weakSelf = self;
      auto callback = base::BindBlockArc(
          ^bool(const base::DictionaryValue& JSON, const GURL& originURL,
                bool userIsInteracting) {
            ContextualSearchController* strongSelf = weakSelf;
            // |originURL| and |isInteracting| aren't used.
            return [strongSelf handleScriptCommand:JSON];
          });
      [self webState]->AddScriptCommandCallback(callback, kCommandPrefix);

      // |_doubleTapRecognizer| should be added to the web view before
      // |_tapRecognizer| so |_tapRecognizer| can require it to fail.
      _doubleTapRecognizer =
          [[UITapGestureRecognizer alloc] initWithTarget:self
                                                  action:@selector(ignoreTap:)];
      [_doubleTapRecognizer setDelegate:self];
      [_doubleTapRecognizer setNumberOfTapsRequired:2];
      [_webViewProxy addGestureRecognizer:_doubleTapRecognizer];

      _tapRecognizer = [[UITapGestureRecognizer alloc]
          initWithTarget:self
                  action:@selector(handleTapFrom:)];
      [_tapRecognizer setDelegate:self];
      [_webViewProxy addGestureRecognizer:_tapRecognizer];

      // Make sure that |_tapRecogngizer| doesn't fire if the web view's other
      // non-single-finger non-single-tap recognizers fire.
      for (UIGestureRecognizer* recognizer in
           [[_tapRecognizer view] gestureRecognizers]) {
        if ([recognizer isKindOfClass:[UITapGestureRecognizer class]] &&
            ([static_cast<UITapGestureRecognizer*>(recognizer)
                 numberOfTapsRequired] > 1 ||
             [static_cast<UITapGestureRecognizer*>(recognizer)
                 numberOfTouchesRequired] > 1)) {
          [_tapRecognizer requireGestureRecognizerToFail:recognizer];
        }
      }
      _webStateEnabled = YES;
    }

    [self initializeWebViewForContextualSearch];
  } else {
    [self disableCurrentWebState];
  }
}

- (void)disableCurrentWebState {
  if (_webStateEnabled) {
    if ([self webState]->ContentIsHTML()) {
      [self highlightRects:nil];
      [_contextualHighlightView removeFromSuperview];
      [_contextualSearchJsManager clearHighlight];
      [_contextualSearchJsManager disableListeners];
    }
    _webState->RemoveScriptCommandCallback(kCommandPrefix);
    DOMAlteringLock::FromWebState(_webState)->Release(self);
    [_webViewProxy removeGestureRecognizer:_tapRecognizer];
    [_webViewProxy removeGestureRecognizer:_doubleTapRecognizer];
    _webStateEnabled = NO;
  }
}

- (void)disconnectWebState {
  if (_webState) {
    _contextualSearchJsManager = nil;
    _webState->RemoveObserver(_webStateObserver.get());
    [self updateWebViewProxy:nil];
    [self disableCurrentWebState];
  }
}

- (void)dealloc {
  [self close];
}

- (void)handleWindowGesture:(UIGestureRecognizer*)recognizer {
  DCHECK(recognizer == _dismissRecognizer);
  [self dismissPane:ContextualSearch::BASE_PAGE_TAP];
}

- (BOOL)canExtractTapContext {
  web::URLVerificationTrustLevel trustLevel = web::kNone;
  GURL pageURL = [self webState]->GetCurrentURL(&trustLevel);
  return [self.permissions canExtractTapContextForURL:pageURL];
}

- (void)initializeWebViewForContextualSearch {
  DCHECK(_webStateEnabled);
  [_contextualSearchJsManager inject];
  _isScriptInjected = YES;
  [_contextualSearchJsManager
      enableEventListenersWithMutationDelay:
          kDOMModificationDelayForJavaScriptMilliseconds
                             bodyTouchDelay:
                                 kBodyTouchDelayForJavaScriptMilliseconds];
}

- (void)handleSelectionChanged:(const std::string&)selection
              selectionUpdated:(BOOL)updated
                selectionValid:(BOOL)selectionValid {
  if (!selectionValid) {
    [self dismissPane:ContextualSearch::INVALID_SELECTION];
    return;
  }
  std::string selectedText = CleanStringForDisplay(selection, true);

  if (selectedText == _selectedText)
    return;
  _newSelectionDisplaying = !updated && !selectedText.empty();
  _selectedText = selectedText;
  _searchContext.reset();
  [self highlightRects:nil];
  [_contextualSearchJsManager clearHighlight];
  _delegate->CancelSearchTermRequest();

  if (selectedText.length() == 0) {
    if (_webViewTappedWithSelection) {
      [self dismissPane:ContextualSearch::BASE_PAGE_TAP];
    }
  } else {
    // TODO(crbug.com/546220): Detect and use actual page encoding.
    std::string encoding = "UTF-8";

    _searchContext.reset(
        new ContextualSearchContext(selectedText, true, GURL(), encoding));
    _searchTriggeredBySelection = YES;

    _preventPreload = YES;
    _delegate->PostSearchTermRequest(_searchContext);

    ContextualSearch::RecordSelectionIsValid(true);
    _preventActionMenu = YES;
    if (!_observingActionMenu) {
      _observingActionMenu = YES;
    }
    [self peekPane:ContextualSearch::TEXT_SELECT_LONG_PRESS];
  }
  _webViewTappedWithSelection = NO;
}

- (BOOL)handleScriptCommand:(const base::DictionaryValue&)JSONCommand {
  std::string command;
  if (!JSONCommand.GetString("command", &command))
    return NO;
  if (command == "contextualSearch.selectionChanged") {
    std::string selectedText;
    if (!JSONCommand.GetString("text", &selectedText))
      return NO;
    bool selectionUpdated;
    if (!JSONCommand.GetBoolean("updated", &selectionUpdated))
      selectionUpdated = false;
    bool selectionValid;
    if (!JSONCommand.GetBoolean("valid", &selectionValid))
      selectionValid = true;
    __weak ContextualSearchController* weakSelf = self;
    ProceduralBlockWithBool lockAction = ^(BOOL lockAcquired) {
      if (lockAcquired) {
        [weakSelf handleSelectionChanged:selectedText
                        selectionUpdated:selectionUpdated
                          selectionValid:selectionValid];
      }
    };
    DOMAlteringLock::FromWebState([self webState])->Acquire(self, lockAction);
    return YES;
  }
  if (command == "contextualSearch.mutationEvent") {
    if (!_searchTermResolved) {
      [self dismissPane:ContextualSearch::UNKNOWN];
    }
    return YES;
  }
  return NO;
}

- (void)ignoreTap:(UIGestureRecognizer*)recognizer {
  // This method is intentionally empty. It is intended to ignore the tap.
}

- (void)handleTapFrom:(UIGestureRecognizer*)recognizer {
  DCHECK(recognizer == _tapRecognizer);
  // Taps will be triggered by long-presses to make a selection in the webview,
  // as well as 'regular' taps. Long-presses that create a selection will set
  // |_newSelectionDisplaying| as well as populating _selectedText (this happens
  // in -handleScriptCommand:).

  // If we just dismissed, do not consider this tap.
  NSTimeInterval dismissTimeout = [_lastDismiss timeIntervalSinceNow] +
                                  kPreventTriggerAfterDismissDelaySeconds;

  // If the panel is already displayed, just dismiss it and return, unless the
  // tap was from displaying a new selection.
  if (dismissTimeout > 0) {
    [self dismissPane:ContextualSearch::BASE_PAGE_TAP];
    return;
  }
  // Otherwise handle the tap.
  [_tapRecognizer setEnabled:NO];
  _currentTapCancelled = NO;
  _newSelectionDisplaying = NO;
  ProceduralBlockWithBool lockAction = ^(BOOL lockAcquired) {
    if (!lockAcquired || !_isScriptInjected || _currentTapCancelled ||
        [recognizer state] != UIGestureRecognizerStateEnded ||
        !_selectedText.empty()) {
      [_tapRecognizer setEnabled:YES];
      if (!_selectedText.empty())
        _webViewTappedWithSelection = YES;
      return;
    }

    // tapPoint is the coordinate of the tap in the webView.
    CGPoint tapPoint = [recognizer locationInView:recognizer.view];

    // Handle tap asynchronously to monitor DOM modifications. See comment
    // of |kDOMModificationDelaySeconds| for details.
    dispatch_time_t dispatch = dispatch_time(
        DISPATCH_TIME_NOW,
        static_cast<int64_t>(kDOMModificationDelaySeconds * NSEC_PER_SEC));
    __weak ContextualSearchController* weakSelf = self;
    dispatch_after(dispatch, dispatch_get_main_queue(), ^{
      [weakSelf handleTapAtPoint:tapPoint];
    });
  };
  DOMAlteringLock::FromWebState([self webState])->Acquire(self, lockAction);
}

- (void)handleLongPressFrom:(UIGestureRecognizer*)recognizer {
  DCHECK(recognizer == _copyGestureRecognizer);
  if (recognizer.state != UIGestureRecognizerStateEnded)
    return;

  // Put the resolved search term (or the current selected text) into the
  // pasteboard.
  std::string text;
  if (!_resolvedSearch.display_text.empty()) {
    text = _resolvedSearch.display_text;
  }

  if (!text.empty()) {
    UIPasteboard* pasteboard = [UIPasteboard generalPasteboard];
    pasteboard.string = base::SysUTF8ToNSString(_resolvedSearch.display_text);
    // Let the user know.
    NSString* messageText = l10n_util::GetNSString(IDS_IOS_SEARCH_COPIED);
    MDCSnackbarMessage* message =
        [MDCSnackbarMessage messageWithText:messageText];
    message.duration = 1.0;
    message.category = @"search term copied";
    [MDCSnackbarManager showMessage:message];
  }
}

- (void)handleTapAtPoint:(CGPoint)point {
  _tapTime = base::Time::Now();
  if (_currentTapCancelled) {
    [_tapRecognizer setEnabled:YES];
    return;
  }

  _searchTriggeredBySelection = NO;

  // TODO(crbug.com/546220): Detect and use actual page encoding.
  std::string encoding = "UTF-8";

  CGPoint relativeTapPoint = point;
  CGSize contentSize = [_webViewProxy scrollViewProxy].contentSize;
  relativeTapPoint.x += [_webViewProxy scrollViewProxy].contentOffset.x;
  relativeTapPoint.y += [_webViewProxy scrollViewProxy].contentOffset.y;

  relativeTapPoint.x /= contentSize.width;
  relativeTapPoint.y /= contentSize.height;

  __weak id<CRWWebViewProxy> weakWebViewProxy = _webViewProxy;
  void (^handler)(NSString*) = ^(NSString* result) {
    [_tapRecognizer setEnabled:YES];
    // If there has been an error in the javascript, return can be nil.
    if (!result || _currentTapCancelled)
      return;

    // Parse JSON.
    const std::string json = base::SysNSStringToUTF8(result);
    std::unique_ptr<base::Value> parsedResult(
        base::JSONReader::Read(json, false));
    if (!parsedResult.get() || !parsedResult->is_dict()) {
      return;
    }

    base::DictionaryValue* resultDict =
        static_cast<base::DictionaryValue*>(parsedResult.get());
    const base::DictionaryValue* context = nullptr;
    BOOL contextError = NO;
    if (!resultDict->GetDictionary("context", &context)) {
      // No context returned -- the tap wasn't on a word.
      DVLOG(1) << "Contextual search results did not include a context.";
      contextError = YES;
    } else {
      std::string error;
      context->GetString("error", &error);
      if (!error.empty()) {
        // Something went wrong!
        DVLOG(0) << "Contextual search error: " << error;
        contextError = YES;
      }
    }

    if (contextError) {
      _searchContext.reset();
      [self updateUI];
      // The JavaScript will have taken care of clearing the highlighting.
      return;
    }

    // Marshall the retrieved context.
    std::string url, selectedText;
    BOOL marshallingOK = YES;
    GURL sentUrl;
    if ([self.permissions canSendPageURLs]) {
      marshallingOK = marshallingOK && context->GetString("url", &url);
      sentUrl = GURL(url);
    }
    marshallingOK =
        marshallingOK && context->GetString("selectedText", &selectedText);

    if (!marshallingOK) {
      _searchContext.reset();
      [self updateUI];
      // The JavaScript will have taken care of clearing the highlighting.
      return;
    }
    _searchContext.reset(
        new ContextualSearchContext(selectedText, true, sentUrl, encoding));

    if ([self canExtractTapContext]) {
      marshallingOK =
          marshallingOK &&
          context->GetString("surroundingText",
                             &_searchContext->surrounding_text) &&
          context->GetInteger("offsetStart", &_searchContext->start_offset) &&
          context->GetInteger("offsetEnd", &_searchContext->end_offset);
    }

    if (!marshallingOK) {
      _searchContext.reset();
      [self updateUI];
      // The JavaScript will have taken care of clearing the highlighting.
      return;
    }

    DVLOG(1) << "Contextual search results:\n"
             << "    URL: " << _searchContext->page_url.spec() << "\n"
             << "    selectedText: " << _searchContext->selected_text << "\n"
             << "    offsets: " << _searchContext->start_offset << "-"
             << _searchContext->end_offset << "\n"
             << "    surroundingText: " << _searchContext->surrounding_text;

    std::string rects;
    if (!context->GetString("rects", &rects)) {
      _searchContext.reset();
      [self updateUI];
      return;
    }
    NSArray* rectsArray = StringValueToRectArray(rects);
    if (!rectsArray) {
      _searchContext.reset();
      [self updateUI];
      return;
    }
    [self highlightRects:rectsArray];

    [self scrollToShowSelection:[weakWebViewProxy scrollViewProxy]];

    // Update the content view and the state of the UI.
    [self updateUI];
    _preventPreload = NO;

    _delegate->PostSearchTermRequest(_searchContext);
    _searchTriggeredBySelection = NO;
  };
  [_contextualSearchJsManager fetchContextFromSelectionAtPoint:relativeTapPoint
                                             completionHandler:handler];
}

- (void)handleHighlightJSResult:(id)result withError:(NSError*)error {
  if (error) {
    [self highlightRects:nil];
    [_contextualSearchJsManager clearHighlight];
    return;
  }
  std::string JSON(
      base::SysNSStringToUTF8(base::mac::ObjCCastStrict<NSString>(result)));
  // |json| is a JSON dicionary containing at list 2 entries:
  // - 'rects': containing a list of rect dictionaries representing the zone of
  //            the page to highlight as a string in the format
  //            top1 bottom1 left1 right1,top2 bottom2 left2 right2,...,
  // - 'size':  containing a dictionary containing the size of the document as
  //            seen in JavaScript.
  // As the 'rects' coordinates are based on a document which size is contained
  // in 'size', if the web content view does not have the same size, they should
  // not be considered.

  std::unique_ptr<base::Value> parsedResult(
      base::JSONReader::Read(JSON, false));
  if (!parsedResult.get() || !parsedResult->is_dict()) {
    return;
  }
  base::DictionaryValue* resultDict =
      static_cast<base::DictionaryValue*>(parsedResult.get());

  CGSize contentSize = [_webViewProxy scrollViewProxy].contentSize;
  const base::DictionaryValue* contentSizeDict;
  if (resultDict->GetDictionary("size", &contentSizeDict)) {
    double width, height;
    if (!contentSizeDict->GetDouble("height", &height) ||
        !contentSizeDict->GetDouble("width", &width)) {
      // Value is not correctly formatted. Early return.
      return;
    }
    width *= [_webViewProxy scrollViewProxy].zoomScale;
    height *= [_webViewProxy scrollViewProxy].zoomScale;
    if (fabsl(contentSize.width - width) > 2 ||
        fabsl(contentSize.height - height) > 2) {
      // The coords in of the UIView and in JavaScript are not synced. A scroll
      // now would be almost random.
      _preventScrollToShowSelection = YES;
      dispatch_after(
          dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.1 * NSEC_PER_SEC)),
          dispatch_get_main_queue(), ^{
            [self updateHighlight];
          });
      return;
    }
    _preventScrollToShowSelection = NO;
  }

  std::string rectsList;
  if (resultDict->GetString("rects", &rectsList)) {
    NSArray* rects = StringValueToRectArray(rectsList);
    if (rects) {
      [self highlightRects:rects];
      [self scrollToShowSelection:[_webViewProxy scrollViewProxy]];
    }
  }
}

- (void)updateForResolvedSearch:
    (ContextualSearchDelegate::SearchResolution)resolution {
  _resolvedSearch = resolution;

  DVLOG(1) << "is invalid: " << _resolvedSearch.is_invalid << "\n"
           << "response code: " << _resolvedSearch.response_code << "\n"
           << "search term: " << _resolvedSearch.search_term << "\n"
           << "search term: " << _resolvedSearch.alternate_term << "\n"
           << "display text: " << _resolvedSearch.display_text << "\n"
           << "stop preload: " << _resolvedSearch.prevent_preload;

  if (_resolvedSearch.is_invalid) {
    [self dismissPane:ContextualSearch::UNKNOWN];
  } else {
    _searchTermResolved = YES;
    if (_resolvedSearch.start_offset != -1 &&
        _resolvedSearch.end_offset != -1) {
      __weak ContextualSearchController* weakSelf = self;
      [_contextualSearchJsManager
          expandHighlightToStartOffset:_resolvedSearch.start_offset
                             endOffset:_resolvedSearch.end_offset
                     completionHandler:^(id result, NSError* error) {
                       [weakSelf handleHighlightJSResult:result
                                               withError:error];
                     }];
    }
    GURL url = _delegate->GetURLForResolvedSearch(_resolvedSearch, true);
    // Record the tap-to-search interval.
    ContextualSearch::RecordTimeToSearch(base::Time::Now() - _tapTime);
  }
}

- (void)updateUI {
  if (_searchContext) {
    ContextualSearch::RecordSelectionIsValid(true);
    [self peekPane:ContextualSearch::TEXT_SELECT_TAP];
    _searchInvolvedFirstRun =
        self.permissions.preferenceState == TouchToSearch::UNDECIDED;
  } else {
    ContextualSearch::RecordSelectionIsValid(false);
    [self dismissPane:ContextualSearch::INVALID_SELECTION];
  }
}
- (void)scrollToShowSelection:(CRWWebViewScrollViewProxy*)scrollView {
  if (!scrollView || _preventScrollToShowSelection)
    return;
  if (!_contextualHighlightView) {
    return;
  }
  CGRect highlightBoundingRect = [_contextualHighlightView boundingRect];
  if (CGRectIsNull(highlightBoundingRect)) {
    return;
  }

  // Do the maths without the insets.
  CGPoint scrollPoint = [scrollView contentOffset];
  scrollPoint.y += scrollView.contentInset.top;
  scrollPoint.x += scrollView.contentInset.left;

  // Coordinates of the bounding box to show.
  CGFloat top = CGRectGetMinY(highlightBoundingRect);
  CGFloat bottom = CGRectGetMaxY(highlightBoundingRect);
  CGFloat left = CGRectGetMinX(highlightBoundingRect);
  CGFloat right = CGRectGetMaxX(highlightBoundingRect);

  CGSize displaySize = [_contextualHighlightView frame].size;

  CGFloat panelHeight =
      CGRectGetHeight(CGRectIntersection(CGRectZero, CGRectZero));

  displaySize.height -= scrollView.contentInset.top +
                        scrollView.contentInset.bottom + panelHeight;
  displaySize.width -=
      scrollView.contentInset.left + scrollView.contentInset.right;

  // Coordinates of the displayed frame in the same coordinates system.
  CGFloat frameTop = scrollPoint.y;
  CGFloat frameBottom = frameTop + displaySize.height;
  CGFloat frameLeft = scrollPoint.x;
  CGFloat frameRight = frameLeft + displaySize.width;

  CGSize contentSize = scrollView.contentSize;
  CGFloat maxOffsetY = MAX(contentSize.height - displaySize.height, 0);
  CGFloat maxOffsetX = MAX(contentSize.width - displaySize.width, 0);

  if (highlightBoundingRect.size.width + 2 * kXScrollMargin >
      displaySize.width) {
    // Selection does not fit in the screen. Center horizontal scroll.
    if (contentSize.width > displaySize.width) {
      scrollPoint.x = (left + right - displaySize.width) / 2;
    }
  } else {
    // Make sure right is visible.
    if (right + kXScrollMargin > frameRight) {
      scrollPoint.x = right + kXScrollMargin - displaySize.width;
    }

    // Make sure left is visible.
    if (left - kXScrollMargin < frameLeft) {
      scrollPoint.x = left - kXScrollMargin;
    }
  }

  // Make sure bottom is visible.
  if (bottom + kYScrollMargin > frameBottom) {
    scrollPoint.y = bottom + kYScrollMargin - displaySize.height;
  }

  // Make sure top is visible.
  if (top - kYScrollMargin < frameTop) {
    scrollPoint.y = top - kYScrollMargin;
  }

  if (scrollPoint.x < 0)
    scrollPoint.x = 0;
  if (scrollPoint.x > maxOffsetX) {
    scrollPoint.x = maxOffsetX;
  }
  if (scrollPoint.y < 0)
    scrollPoint.y = 0;
  if (scrollPoint.y > maxOffsetY)
    scrollPoint.y = maxOffsetY;

  scrollPoint.y -= scrollView.contentInset.top;
  scrollPoint.x -= scrollView.contentInset.left;
  [scrollView setContentOffset:scrollPoint animated:YES];
}

- (void)highlightRects:(NSArray*)rects {
  if (![self webState]) {
    return;
  }
  if (!_contextualHighlightView && [rects count]) {
    CGRect frame = [[self webState]->GetWebViewProxy() frame];
    ContextualSearchHighlighterView* highlightView =
        [[ContextualSearchHighlighterView alloc] initWithFrame:frame
                                                      delegate:self];
    _contextualHighlightView = highlightView;
    [[self webState]->GetWebViewProxy() addSubview:highlightView];
  }
  CGPoint scroll = [[_webViewProxy scrollViewProxy] contentOffset];
  [_contextualHighlightView
      highlightRects:rects
          withOffset:0
                zoom:[[_webViewProxy scrollViewProxy] zoomScale]
              scroll:scroll];
}

- (void)close {
  if (_closed)
    return;

  _closed = YES;
  [self disableCurrentWebState];
  [self setWebState:nil];
  _delegate.reset();
}

#pragma mark - Promo view management

- (void)userOptedInFromPromo:(BOOL)optIn {
  if (optIn) {
    self.permissions.preferenceState = TouchToSearch::ENABLED;
  } else {
    [self dismissPane:ContextualSearch::OPTOUT];
    self.permissions.preferenceState = TouchToSearch::DISABLED;
  }
  ContextualSearch::RecordFirstRunFlowOutcome(self.permissions.preferenceState);
}

#pragma mark - State change methods

- (void)setState:(ContextualSearch::PanelState)state
          reason:(ContextualSearch::StateChangeReason)reason {
  ContextualSearch::PanelState fromState = ContextualSearch::DISMISSED;

  // If we're moving to PEEKING as a result of text selection, that's starting
  // a new search.
  BOOL startingSearch = state == ContextualSearch::PEEKING &&
                        (reason == ContextualSearch::TEXT_SELECT_TAP ||
                         reason == ContextualSearch::TEXT_SELECT_LONG_PRESS);
  // If we're showing anything, then there's an ongoing search.
  BOOL ongoingSearch = fromState > ContextualSearch::DISMISSED;
  // If there's an ongoing search and we're dismissing or starting a search,
  // then we're ending a search.
  BOOL endingSearch =
      ongoingSearch && (state == ContextualSearch::DISMISSED || startingSearch);
  // If we're starting a search while there's one already there, it's chained.
  BOOL chained = startingSearch && endingSearch;

  BOOL sameState = fromState == state;
  BOOL firstExitFromPeeking = fromState == ContextualSearch::PEEKING &&
                              !_exitedPeeking && (!sameState || startingSearch);
  BOOL firstExitFromPreviewing = fromState == ContextualSearch::PREVIEWING &&
                                 !_exitedPreviewing && !sameState;
  BOOL firstExitFromCovering =
      fromState == ContextualSearch::COVERING && !_exitedCovering && !sameState;

  if (endingSearch) {
    if (_searchInvolvedFirstRun) {
      // If the first run panel might have been shown, did the user see it?
      ContextualSearch::RecordFirstRunPanelSeen(_firstRunPanelBecameVisible);
    }
    // Record if the user saw the search results.
    if (_searchTriggeredBySelection) {
      ContextualSearch::RecordSelectionResultsSeen(_resultsVisible);
    } else {
      ContextualSearch::RecordTapResultsSeen(_resultsVisible);
    }
  }

  // Log state change. We only log the first transition to a state within a
  // contextual search. Note that when a user clicks on a link on the search
  // content view, this will trigger a transition to COVERING (SERP_NAVIGATION)
  // followed by a transition to DISMISSED (TAB_PROMOTION). For the purpose of
  // logging, the reason for the second transition is reinterpreted to
  // SERP_NAVIGATION, in order to distinguish it from a tab promotion caused
  // when tapping on the header when the panel is maximized.
  ContextualSearch::StateChangeReason loggedReason =
      _usedSERPNavigation ? ContextualSearch::SERP_NAVIGATION : reason;
  if (startingSearch || endingSearch ||
      (!sameState && !_enteredPreviewing &&
       state == ContextualSearch::PREVIEWING) ||
      (!sameState && !_enteredCovering &&
       state == ContextualSearch::COVERING)) {
    ContextualSearch::RecordFirstStateEntry(fromState, state, loggedReason);
  }
  if ((startingSearch && !chained) || firstExitFromPeeking ||
      firstExitFromPreviewing || firstExitFromCovering) {
    ContextualSearch::RecordFirstStateExit(fromState, state, loggedReason);
  }

  if (firstExitFromPeeking) {
    _exitedPeeking = YES;
  } else if (firstExitFromPreviewing) {
    _exitedPreviewing = YES;
  } else if (firstExitFromCovering) {
    _exitedCovering = YES;
  }

  // If the panel is now visible, enable the window-tap detector.

  if (state == ContextualSearch::PREVIEWING) {
    _enteredPreviewing = YES;
  } else if (state == ContextualSearch::COVERING) {
    _enteredCovering = YES;
  }

  if (reason == ContextualSearch::SERP_NAVIGATION) {
    _usedSERPNavigation = YES;
  }

  if (endingSearch) {
    _enteredPreviewing = NO;
    _enteredCovering = NO;
    _resultsVisible = NO;
    _exitedPeeking = NO;
    _exitedPreviewing = NO;
    _exitedCovering = NO;
    _searchInvolvedFirstRun = NO;
    _firstRunPanelBecameVisible = NO;
    _searchTermResolved = NO;
    _usedSERPNavigation = NO;
  }
}

- (void)
dismissPaneWithJavascriptCompletionHandler:(ProceduralBlock)completionHandler
                                    reason:(ContextualSearch::StateChangeReason)
                                               reason {
  [self cleanUpWebStateForDismissWithCompletion:completionHandler];
  [self setState:ContextualSearch::DISMISSED reason:reason];
}

- (void)cleanUpWebStateForDismissWithCompletion:
    (ProceduralBlock)completionHandler {
  _lastDismiss = [NSDate date];
  _currentTapCancelled = YES;
  if ([self webState]) {
    DOMAlteringLock* lock = DOMAlteringLock::FromWebState([self webState]);
    if (lock) {
      lock->Release(self);
    }
  }
  if (completionHandler)
    completionHandler();
}

- (void)dismissPane:(ContextualSearch::StateChangeReason)reason {
  [self dismissPaneWithJavascriptCompletionHandler:nil reason:reason];
}

- (void)peekPane:(ContextualSearch::StateChangeReason)reason {
  [self setState:ContextualSearch::PEEKING reason:reason];
  [_doubleTapRecognizer setEnabled:NO];
  [self scrollToShowSelection:[_webViewProxy scrollViewProxy]];
}

- (void)previewPane:(ContextualSearch::StateChangeReason)reason {
  if (_searchInvolvedFirstRun) {
    _firstRunPanelBecameVisible = YES;
  }
  [self setState:ContextualSearch::PREVIEWING reason:reason];
  [_doubleTapRecognizer setEnabled:NO];
  [self scrollToShowSelection:[_webViewProxy scrollViewProxy]];
  _delegate->StartPendingSearchTermRequest();
}

- (void)coverPane:(ContextualSearch::StateChangeReason)reason {
  [self setState:ContextualSearch::COVERING reason:reason];
}

- (void)movePanelOffscreen {
  [self dismissPane:ContextualSearch::RESET];
}

#pragma mark - CRWWebStateObserver methods

- (void)webState:(web::WebState*)webState didLoadPageWithSuccess:(BOOL)success {
  DCHECK_EQ(self.webState, webState);
  if (!success)
    return;

  [self movePanelOffscreen];
  _isScriptInjected = NO;
  [self enableCurrentWebState];
}

- (void)webStateDestroyed:(web::WebState*)webState {
  DCHECK_EQ(self.webState, webState);
  [self updateWebViewProxy:nil];
  [self close];
}

#pragma mark - UIGestureRecognizerDelegate Methods

// Ensures that |_tapRecognizer| and |_doubleTapRecognizer| cooperate with all
// other gesture recognizers.
- (BOOL)gestureRecognizer:(UIGestureRecognizer*)gestureRecognizer
    shouldRecognizeSimultaneouslyWithGestureRecognizer:
        (UIGestureRecognizer*)otherGestureRecognizer {
  return gestureRecognizer == _tapRecognizer ||
         gestureRecognizer == _doubleTapRecognizer;
}

#pragma mark - CRWWebViewScrollViewObserver methods

- (void)webViewScrollViewWillBeginDragging:
    (CRWWebViewScrollViewProxy*)webViewScrollViewProxy {
  [self dismissPane:ContextualSearch::BASE_PAGE_SCROLL];
  [_tapRecognizer setEnabled:NO];
}

- (void)webViewScrollViewDidEndDragging:
            (CRWWebViewScrollViewProxy*)webViewScrollViewProxy
                         willDecelerate:(BOOL)decelerate {
  if (!decelerate)
    [_tapRecognizer setEnabled:YES];
}

- (void)webViewScrollViewDidEndDecelerating:
    (CRWWebViewScrollViewProxy*)webViewScrollViewProxy {
  [_tapRecognizer setEnabled:YES];
}

- (void)webViewScrollViewDidScroll:
    (CRWWebViewScrollViewProxy*)webViewScrollViewProxy {
  _currentTapCancelled = YES;
  [_contextualHighlightView setScroll:[webViewScrollViewProxy contentOffset]
                                 zoom:[webViewScrollViewProxy zoomScale]
                               offset:0];
}

- (void)webViewScrollViewDidZoom:
    (CRWWebViewScrollViewProxy*)webViewScrollViewProxy {
  _currentTapCancelled = YES;
  [_contextualHighlightView setScroll:[webViewScrollViewProxy contentOffset]
                                 zoom:[webViewScrollViewProxy zoomScale]
                               offset:0];
  [self scrollToShowSelection:webViewScrollViewProxy];
}

#pragma mark - DOMAltering methods

- (BOOL)canReleaseDOMLock {
  return YES;
}

- (void)releaseDOMLockWithCompletionHandler:(ProceduralBlock)completionHandler {
  [self dismissPaneWithJavascriptCompletionHandler:completionHandler
                                            reason:ContextualSearch::RESET];
}

#pragma mark - TouchToSearchPermissionsChangeAudience methods

- (void)touchToSearchDidChangePreferenceState:
    (TouchToSearch::TouchToSearchPreferenceState)preferenceState {
  if (preferenceState != TouchToSearch::UNDECIDED) {
    ContextualSearch::RecordPreferenceChanged(preferenceState ==
                                              TouchToSearch::ENABLED);
  }
}

- (void)touchToSearchPermissionsUpdated {
  // This method is already invoked asynchronously, so it's safe to
  // synchronously attempt to enable the feature.
  [self enableContextualSearch:YES];
}

#pragma mark - ContextualSearchHighlighterDelegate methods

- (void)updateHighlight {
  __weak ContextualSearchController* weakSelf = self;
  [_contextualSearchJsManager
      highlightRectsWithCompletionHandler:^void(id result, NSError* error) {
        [weakSelf handleHighlightJSResult:result withError:error];
      }];
}

@end
