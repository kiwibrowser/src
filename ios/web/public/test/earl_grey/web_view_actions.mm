// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web/public/test/earl_grey/web_view_actions.h"

#include "base/callback_helpers.h"
#include "base/logging.h"
#import "base/mac/bind_objc_block.h"
#include "base/strings/stringprintf.h"
#import "base/test/ios/wait_util.h"
#include "base/values.h"
#import "ios/testing/wait_util.h"
#import "ios/web/public/test/earl_grey/web_view_matchers.h"
#import "ios/web/public/test/web_view_interaction_test_util.h"
#import "ios/web/web_state/web_state_impl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using web::test::ExecuteJavaScript;

namespace {

// Long press duration to trigger context menu.  EarlGrey LongPress action uses
// 0.7 secs.  Use the same number to be consistent.
const NSTimeInterval kContextMenuLongPressDuration = 0.7;

// Duration to wait for verification of JavaScript action.
// TODO(crbug.com/670910): Reduce duration if the time required for verification
// is reduced on devices.
const NSTimeInterval kWaitForVerificationTimeout = 8.0;

// Callback prefix for injected verifiers.
const std::string CallbackPrefixForElementId(const std::string& element_id) {
  return "__web_test_" + element_id + "_interaction";
}

// Generic verification injector. Injects one-time mousedown verification into
// |web_state| that will set the boolean pointed to by |verified| to true when
// |web_state|'s webview registers the mousedown event.
// RemoveVerifierForElementWithId() should be called after this to ensure
// future tests can add verifiers with the same prefix.
bool AddVerifierToElementWithId(web::WebState* web_state,
                                const std::string& element_id,
                                bool* verified) {
  const std::string kCallbackPrefix = CallbackPrefixForElementId(element_id);
  const char kCallbackCommand[] = "verified";
  const std::string kCallbackInvocation =
      kCallbackPrefix + '.' + kCallbackCommand;

  const char kAddInteractionVerifierScriptTemplate[] =
      "(function() {"
      // First template param: element ID.
      "  var elementId = '%1$s';"
      "  var element = document.getElementById(elementId);"
      "  if (!element)"
      "    return 'Element ' + elementId + ' not found';"
      "  var invokeType = typeof __gCrWeb.message;"
      "  if (invokeType != 'object')"
      "    return 'Host invocation not installed (' + invokeType + ')';"
      "  var options = {'capture' : true, 'once' : true, 'passive' : true};"
      "  element.addEventListener('mousedown', function(event) {"
      "      __gCrWeb.message.invokeOnHost("
      // Second template param: callback command.
      "          {'command' : '%2$s' });"
      "  }, options);"
      "  return true;"
      "})();";

  const std::string kAddVerifierScript =
      base::StringPrintf(kAddInteractionVerifierScriptTemplate,
                         element_id.c_str(), kCallbackInvocation.c_str());

  bool success =
      testing::WaitUntilConditionOrTimeout(testing::kWaitForUIElementTimeout, ^{
        bool verifier_added = false;
        std::unique_ptr<base::Value> value =
            web::test::ExecuteJavaScript(web_state, kAddVerifierScript);
        if (value) {
          std::string error;
          if (value->GetAsString(&error)) {
            DLOG(ERROR) << "Verifier injection failed: " << error
                        << ", retrying.";
          } else if (value->GetAsBoolean(&verifier_added)) {
            return true;
          }
        }
        return false;
      });

  if (!success)
    return false;

  // The callback doesn't care about any of the parameters, just whether it is
  // called or not.
  auto callback = base::BindBlockArc(
      ^bool(const base::DictionaryValue& /* json */,
            const GURL& /* origin_url */, bool /* user_is_interacting */) {
        *verified = true;
        return true;
      });

  static_cast<web::WebStateImpl*>(web_state)->AddScriptCommandCallback(
      callback, kCallbackPrefix);
  return true;
}

// Removes the injected callback.
void RemoveVerifierForElementWithId(web::WebState* web_state,
                                    const std::string& element_id) {
  static_cast<web::WebStateImpl*>(web_state)->RemoveScriptCommandCallback(
      CallbackPrefixForElementId(element_id));
}

// Returns a no element found error.
id<GREYAction> WebViewElementNotFound(const std::string& element_id) {
  NSString* description = [NSString
      stringWithFormat:@"Couldn't locate a bounding rect for element_id %s; "
                       @"either it isn't there or it has no area.",
                       element_id.c_str()];
  GREYPerformBlock throw_error =
      ^BOOL(id /* element */, __strong NSError** error) {
        NSDictionary* user_info = @{NSLocalizedDescriptionKey : description};
        *error = [NSError errorWithDomain:kGREYInteractionErrorDomain
                                     code:kGREYInteractionActionFailedErrorCode
                                 userInfo:user_info];
        return NO;
      };
  return [GREYActionBlock actionWithName:@"Locate element bounds"
                            performBlock:throw_error];
}

}  // namespace

namespace web {

id<GREYAction> WebViewVerifiedActionOnElement(WebState* state,
                                              id<GREYAction> action,
                                              const std::string& element_id) {
  NSString* action_name =
      [NSString stringWithFormat:@"Verified action (%@) on webview element %s.",
                                 action.name, element_id.c_str()];

  GREYPerformBlock verified_tap = ^BOOL(id element, __strong NSError** error) {
    // A pointer to |verified| is passed into AddVerifierToElementWithId() so
    // the verifier can update its value, but |verified| also needs to be marked
    // as __block so that waitUntilCondition(), below, can access it by
    // reference.
    __block bool verified = false;

    // Ensure that RemoveVerifierForElementWithId() is run regardless of how
    // the block exits.
    base::ScopedClosureRunner cleanup(
        base::Bind(&RemoveVerifierForElementWithId, state, element_id));

    // Inject the verifier.
    bool verifier_added =
        AddVerifierToElementWithId(state, element_id, &verified);
    if (!verifier_added) {
      NSString* description = [NSString
          stringWithFormat:@"It wasn't possible to add the verification "
                           @"javascript for element_id %s",
                           element_id.c_str()];
      NSDictionary* user_info = @{NSLocalizedDescriptionKey : description};
      *error = [NSError errorWithDomain:kGREYInteractionErrorDomain
                                   code:kGREYInteractionActionFailedErrorCode
                               userInfo:user_info];
      return NO;
    }

    // Run the action.
    [[EarlGrey selectElementWithMatcher:WebViewInWebState(state)]
        performAction:action
                error:error];

    if (*error) {
      return NO;
    }

    // Wait for the verified to trigger and set |verified|.
    NSString* verification_timeout_message =
        [NSString stringWithFormat:@"The action (%@) on element_id %s wasn't "
                                   @"verified before timing out.",
                                   action.name, element_id.c_str()];
    GREYAssert(testing::WaitUntilConditionOrTimeout(
                   kWaitForVerificationTimeout,
                   ^{
                     return verified;
                   }),
               verification_timeout_message);

    // If |verified| is not true, the wait condition should have already exited
    // this control flow, so sanity check that it has in fact been set to
    // true by this point.
    DCHECK(verified);
    return YES;
  };

  return [GREYActionBlock actionWithName:action_name
                             constraints:WebViewInWebState(state)
                            performBlock:verified_tap];
}

id<GREYAction> WebViewLongPressElementForContextMenu(
    WebState* state,
    const std::string& element_id,
    bool triggers_context_menu) {
  CGRect rect = web::test::GetBoundingRectOfElementWithId(state, element_id);
  if (CGRectIsEmpty(rect)) {
    return WebViewElementNotFound(element_id);
  }
  CGPoint point = CGPointMake(CGRectGetMidX(rect), CGRectGetMidY(rect));
  id<GREYAction> longpress =
      grey_longPressAtPointWithDuration(point, kContextMenuLongPressDuration);
  if (triggers_context_menu) {
    return longpress;
  }
  return WebViewVerifiedActionOnElement(state, longpress, element_id);
}

id<GREYAction> WebViewTapElement(WebState* state,
                                 const std::string& element_id) {
  CGRect rect = web::test::GetBoundingRectOfElementWithId(state, element_id);
  CGPoint point = CGPointMake(CGRectGetMidX(rect), CGRectGetMidY(rect));
  return CGRectIsEmpty(rect) ? WebViewElementNotFound(element_id)
                             : WebViewVerifiedActionOnElement(
                                   state, grey_tapAtPoint(point), element_id);
}

}  // namespace web
