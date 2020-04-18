// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <UIKit/UIKit.h>

#include <memory>

#include "base/ios/ios_util.h"
#include "base/json/json_reader.h"
#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#import "base/test/ios/wait_util.h"
#include "base/values.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_registry_simple.h"
#include "ios/chrome/browser/browser_state/test_chrome_browser_state.h"
#import "ios/chrome/browser/ui/contextual_search/contextual_search_controller.h"
#import "ios/chrome/browser/ui/contextual_search/js_contextual_search_manager.h"
#import "ios/chrome/browser/ui/contextual_search/touch_to_search_permissions_mediator+testing.h"
#import "ios/chrome/browser/web/chrome_web_test.h"
#import "ios/web/public/web_state/js/crw_js_injection_manager.h"
#import "ios/web/public/web_state/js/crw_js_injection_receiver.h"
#import "ios/web/public/web_state/web_state.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/gtest_mac.h"
#import "third_party/ocmock/OCMock/OCMock.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

// Unit tests for the resources/contextualsearch.js JavaScript file.

struct ContextualSearchStruct {
  std::string url;
  std::string selectedText;
  std::string surroundingText;
  int offsetStart;
  int offsetEnd;
};

@interface JsContextualSearchAdditionsManager : CRWJSInjectionManager
@end

@implementation JsContextualSearchAdditionsManager : CRWJSInjectionManager
- (NSString*)scriptPath {
  return @"contextualsearch_unittest";
}

- (NSArray*)dependencies {
  return @[ [JsContextualSearchManager class] ];
}

@end

namespace {

// HTML that contains script.
NSString* kHTMLSentenceWithScript =
    @"<html><body>"
     "This is <span id='taphere'>the</span> <script>function ignore() "
     "{};</script>sentence to select."
     "</body></html>";

// HTML that contains label to be tapped.
NSString* kHTMLSentenceWithLabel =
    @"<html><body>"
     "<label>Click <span id='taphere'>me</span> <input type='text'/></label>."
     "A sentence to select."
     "</body></html>";

// HTML that contains italic element.
NSString* kHTMLWithItalic =
    @"<html><body>"
     "This is <span id='taphere'>an</span> <i>italic</i> element."
     "</body></html>";

// HTML that contains a table.
NSString* kHTMLWithTable =
    @"<html><body>"
     "<table><tr><td>Left <span id='taphere'>cell</span></td>"
     "<td>right cell</td></tr></table>"
     "</body></html>";

// HTML that will trigger an unrelated DOM mutation on tap.
NSString* kHTMLWithUnrelatedDOMMutation =
    @"<html><body>"
     "<span>This <span style='margin-left:50px' id='taphere'>is</span>"
     " sentence</span> <span>with <span id='test' attr='before'>mutation</span>"
     "</span>.</body></html>";

// HTML that will trigger a related DOM mutation on tap.
NSString* kHTMLWithRelatedDOMMutation =
    @"<html><body>"
     "This <span id='test' attr='before'>"
     "<span style='margin-left:50px' id='taphere'>is</span></span>"
     "a sentence with mutation."
     "</body></html>";

// HTML that will trigger a related DOM mutation on tap.
NSString* kHTMLWithRelatedTextMutation =
    @"<html><body>"
     "This <span style='margin-left:50px' id='taphere'> text "
     "<span id='test'>mutation is </span> inside </span>"
     "a sentence with mutation."
     "</body></html>";

// HTML that contain a div in the middle of a sentence.
NSString* kHTMLWithDiv =
    @"<html><body>"
     "This<div>is</div>a <span id='taphere'>sentence</span>."
     "</body></html>";

// HTML that contain a div in the middle of a sentence.
NSString* kHTMLWithSpaceDiv =
    @"<html><body>"
     "This <div>is</div>a <span id='taphere'>sentence</span>."
     "</body></html>";

// HTML that contains prevent default.
NSString* kHTMLWithPreventDefault = @"<html><body>"
                                     "<div id='interceptor'>"
                                     "<span id='taphere'>is</span>"
                                     "</div></body></html>";

NSString* kStringWith50Chars =
    @"Sentence $INDEX containing exactly 50 characters. ";

class ContextualSearchJsTest : public ChromeWebTest {
 public:
  // Loads the given HTML, then loads the |contextualSearch| JavaScript.
  void LoadHtml(NSString* html) {
    ChromeWebTest::LoadHtml(html);
    [jsUnittestsAdditions_ inject];
  }

  bool GetContextFromId(NSString* elementID,
                        ContextualSearchStruct* searchContext) {
    id javaScriptResult = ExecuteJavaScript([NSString
        stringWithFormat:@"document.getElementById('%@').scrollIntoView();"
                          "__gCrWeb.contextualSearch.tapOnElement('%@');",
                         elementID, elementID]);
    if (!javaScriptResult)
      return false;
    const std::string json = base::SysNSStringToUTF8(javaScriptResult);
    std::unique_ptr<base::Value> parsedResult(
        base::JSONReader::Read(json, false));
    if (!parsedResult.get() || !parsedResult->is_dict()) {
      return false;
    }

    base::DictionaryValue* resultDict =
        static_cast<base::DictionaryValue*>(parsedResult.get());
    const base::DictionaryValue* context = nullptr;
    if (!resultDict->GetDictionary("context", &context)) {
      return false;
    }

    std::string error;
    context->GetString("error", &error);
    if (!error.empty()) {
      LOG(ERROR) << "GetContext error: " << error;
      return false;
    }

    std::string url, selectedText;
    context->GetString("url", &searchContext->url);
    context->GetString("selectedText", &searchContext->selectedText);
    context->GetString("surroundingText", &searchContext->surroundingText);
    context->GetInteger("offsetStart", &searchContext->offsetStart);
    context->GetInteger("offsetEnd", &searchContext->offsetEnd);
    return true;
  }

  id expandHighlight(int start, int end) {
    return ExecuteJavaScript([NSString
        stringWithFormat:@"__gCrWeb.contextualSearch.expandHighlight(%d, %d);"
                          "__gCrWeb.contextualSearch.retrieveHighlighted();",
                         start, end]);
  }

  id highlight() {
    return ExecuteJavaScript(
        @"__gCrWeb.contextualSearch.setHighlighting(true);"
         "__gCrWeb.contextualSearch.retrieveHighlighted();");
  }

  NSInteger GetMutatedNodeCount() {
    id output = ExecuteJavaScript(
        @"__gCrWeb.contextualSearch.getMutatedElementCount();");
    return [output integerValue];
  }

  void CheckContextOffsets(const ContextualSearchStruct& searchContext,
                           const std::string& surroundingText) {
    EXPECT_EQ(searchContext.surroundingText, surroundingText);
    EXPECT_EQ(searchContext.selectedText,
              searchContext.surroundingText.substr(
                  searchContext.offsetStart,
                  searchContext.offsetEnd - searchContext.offsetStart));
  }

  NSString* BuildLongTestString(int startIndex,
                                int endIndex,
                                int tapOn,
                                bool newLine) {
    NSString* longString = @"";
    for (int i = startIndex; i < endIndex; i++) {
      NSString* index = [NSString stringWithFormat:@"%06d", i];
      if (i == tapOn) {
        index =
            [NSString stringWithFormat:@"<span id='taphere'>%@</span>", index];
      }
      NSString* sentence =
          [kStringWith50Chars stringByReplacingOccurrencesOfString:@"$INDEX"
                                                        withString:index];
      longString = [longString stringByAppendingString:sentence];
      if (newLine) {
        longString = [longString stringByAppendingString:@"<br/>"];
      }
    }
    return longString;
  }

  void SetUp() override {
    ChromeWebTest::SetUp();
    jsUnittestsAdditions_ = static_cast<JsContextualSearchAdditionsManager*>(
        [web_state()->GetJSInjectionReceiver()
            instanceOfClass:[JsContextualSearchAdditionsManager class]]);
    TestChromeBrowserState::Builder test_cbs_builder;
    chrome_browser_state_ = test_cbs_builder.Build();
    controller_ = [[ContextualSearchController alloc]
        initWithBrowserState:chrome_browser_state_.get()];
    [controller_
        setPermissions:[[MockTouchToSearchPermissionsMediator alloc]
                           initWithBrowserState:chrome_browser_state_.get()]];
    [controller_ setWebState:web_state()];
    [controller_ enableContextualSearch:YES];
  }

  void TearDown() override {
    [controller_ close];
    // Need to tear down the controller so it deregisters its JS handlers
    // before |webController_| is destroyed.
    controller_ = nil;
    ChromeWebTest::TearDown();
  }

  std::unique_ptr<TestChromeBrowserState> chrome_browser_state_;
  __weak JsContextualSearchAdditionsManager* jsUnittestsAdditions_ = nil;
  ContextualSearchController* controller_;
};

// Test that ignored elements do not trigger CS when tapped.
TEST_F(ContextualSearchJsTest, TestIgnoreTapsOnElements) {
  LoadHtml(kHTMLSentenceWithLabel);
  ContextualSearchStruct searchContext;
  ASSERT_FALSE(GetContextFromId(@"taphere", &searchContext));
};

// Test that ingnored element are not included in highlight or surrounding text.
TEST_F(ContextualSearchJsTest, TestIgnoreScript) {
  LoadHtml(kHTMLSentenceWithScript);
  ContextualSearchStruct searchContext;
  ASSERT_TRUE(GetContextFromId(@"taphere", &searchContext));

  CheckContextOffsets(searchContext, "This is the sentence to select.");
  id highlighted = expandHighlight(0, searchContext.surroundingText.size());
  EXPECT_NSEQ(highlighted, @"This is the sentence to select.");
};

// Test that all span element are correctly highlighted.
TEST_F(ContextualSearchJsTest, TestHighlightThroughSpan) {
  LoadHtml(kHTMLWithItalic);
  ContextualSearchStruct searchContext;
  ASSERT_TRUE(GetContextFromId(@"taphere", &searchContext));

  CheckContextOffsets(searchContext, "This is an italic element.");
  NSString* highlighted =
      expandHighlight(0, searchContext.surroundingText.size());
  EXPECT_NSEQ(highlighted, @"This is an italic element.");
};

// Test that all block element are highlighted. Spaces separating element must
// be ignored.
TEST_F(ContextualSearchJsTest, TestHighlightThroughBlock) {
  LoadHtml(kHTMLWithTable);
  ContextualSearchStruct searchContext;
  ASSERT_TRUE(GetContextFromId(@"taphere", &searchContext));

  CheckContextOffsets(searchContext, "Left cell right cell");
  NSString* highlighted =
      expandHighlight(0, searchContext.surroundingText.size());
  EXPECT_NSEQ(highlighted, @"Left cell right cell");
};

// Test that blocks add spaces if there are not around it.
TEST_F(ContextualSearchJsTest, TestHighlightBlockAddsSpace) {
  LoadHtml(kHTMLWithDiv);
  ContextualSearchStruct searchContext;
  ASSERT_TRUE(GetContextFromId(@"taphere", &searchContext));
  CheckContextOffsets(searchContext, "This is a sentence.");
};

// Test that blocks don't add spaces if there are around it.
TEST_F(ContextualSearchJsTest, TestHighlightBlockDontAddsSpace) {
  LoadHtml(kHTMLWithSpaceDiv);
  ContextualSearchStruct searchContext;
  ASSERT_TRUE(GetContextFromId(@"taphere", &searchContext));
  CheckContextOffsets(searchContext, "This is a sentence.");
};

// Test that related DOM mutation cancels contextual search.
TEST_F(ContextualSearchJsTest, TestHighlightRelatedDOMMutation) {
  // TODO(crbug.com/736989): Test failures in GetMutatedNodeCount.
  if (base::ios::IsRunningOnIOS11OrLater())
    return;
  LoadHtml(kHTMLWithRelatedDOMMutation);
  ExecuteJavaScript(
      @"document.getElementById('test').setAttribute('attr', 'after');");
  ASSERT_EQ(1, GetMutatedNodeCount());
  ContextualSearchStruct searchContext;
  ASSERT_FALSE(GetContextFromId(@"taphere", &searchContext));
};

// Test that unrelated DOM mutation does not cancel contextual search.
TEST_F(ContextualSearchJsTest, TestHighlightIgnoreUnrelatedDOMMutation) {
  LoadHtml(kHTMLWithUnrelatedDOMMutation);
  ExecuteJavaScript(
      @"document.getElementById('test').setAttribute('attr', 'after');");
  ASSERT_EQ(1, GetMutatedNodeCount());
  ContextualSearchStruct searchContext;

  ASSERT_TRUE(GetContextFromId(@"taphere", &searchContext));
};

// Test that DOM mutation with same value does not cancel contextual search.
// Mutation should not mark a node as mutated.
TEST_F(ContextualSearchJsTest, TestHighlightIgnoreDOMMutationSameAttribute) {
  LoadHtml(kHTMLWithRelatedDOMMutation);
  ExecuteJavaScript(
      @"document.getElementById('test').setAttribute('attr', 'before');");
  ASSERT_EQ(0, GetMutatedNodeCount());
  ContextualSearchStruct searchContext;

  ASSERT_TRUE(GetContextFromId(@"taphere", &searchContext));
};

// Test that DOM mutation between different false values does not cancel
// contextual search. Mutation should not mark a node as mutated.
TEST_F(ContextualSearchJsTest, TestHighlightIgnoreDOMMutationBothFalse) {
  LoadHtml(kHTMLWithRelatedDOMMutation);
  ExecuteJavaScript(
      @"document.getElementById('test').setAttribute('non_attr', '');");
  ASSERT_EQ(0, GetMutatedNodeCount());
  ContextualSearchStruct searchContext;

  ASSERT_TRUE(GetContextFromId(@"taphere", &searchContext));
};

// Test that DOM mutation with same text does not cancel contextual search.
// Mutation should not mark a node as mutated.
TEST_F(ContextualSearchJsTest, TestHighlightIgnoreDOMMutationSameText) {
  LoadHtml(kHTMLWithRelatedTextMutation);
  ExecuteJavaScript(
      @"document.getElementById('test').innerText = 'mutation is ';");
  ASSERT_EQ(0, GetMutatedNodeCount());
  ContextualSearchStruct searchContext;

  ASSERT_TRUE(GetContextFromId(@"taphere", &searchContext));
};

// Test that related text DOM mutation prevents contextual search.
TEST_F(ContextualSearchJsTest, TestHighlightRelatedDOMMutationText) {
  // TODO(crbug.com/736989): Test failures in GetMutatedNodeCount.
  if (base::ios::IsRunningOnIOS11OrLater())
    return;
  LoadHtml(kHTMLWithRelatedTextMutation);
  ExecuteJavaScript(@"document.getElementById('test').innerText = 'mutated';");
  ASSERT_EQ(1, GetMutatedNodeCount());
  ContextualSearchStruct searchContext;

  ASSERT_FALSE(GetContextFromId(@"taphere", &searchContext));
};

// Test that unrelated text DOM mutation doesn't prevent contextual search.
TEST_F(ContextualSearchJsTest, TestHighlightUnrelatedDOMMutationTextIgnored) {
  // TODO(crbug.com/736989): Test failures in GetMutatedNodeCount.
  if (base::ios::IsRunningOnIOS11OrLater())
    return;
  LoadHtml(kHTMLWithUnrelatedDOMMutation);
  ExecuteJavaScript(@"document.getElementById('test').innerText = 'mutated';");
  ASSERT_EQ(1, GetMutatedNodeCount());
  ContextualSearchStruct searchContext;

  ASSERT_TRUE(GetContextFromId(@"taphere", &searchContext));
};

// Test that two related DOM mutations prevent contextual search.
TEST_F(ContextualSearchJsTest, TestHighlightTwoDOMMutations) {
  // TODO(crbug.com/736989): Test failures in GetMutatedNodeCount.
  if (base::ios::IsRunningOnIOS11OrLater())
    return;

  LoadHtml(kHTMLWithRelatedDOMMutation);
  ASSERT_EQ(0, GetMutatedNodeCount());
  ExecuteJavaScript(
      @"document.getElementById('taphere').innerText = 'mutated';"
       "document.getElementById('test').setAttribute('attr', 'after');");
  ASSERT_EQ(2, GetMutatedNodeCount());
  ContextualSearchStruct searchContext;

  ASSERT_FALSE(GetContextFromId(@"taphere", &searchContext));
};

// Test that two related DOM mutations with only one change prevent contextual
// search.
TEST_F(ContextualSearchJsTest, TestHighlightTwoDOMMutationOneChanging) {
  // TODO(crbug.com/736989): Test failures in GetMutatedNodeCount.
  if (base::ios::IsRunningOnIOS11OrLater())
    return;

  LoadHtml(kHTMLWithRelatedDOMMutation);
  ExecuteJavaScript(
      @"document.getElementById('taphere').innerText = 'mutated';"
       "document.getElementById('test').setAttribute('attr', 'before');");
  ASSERT_EQ(1, GetMutatedNodeCount());
  ContextualSearchStruct searchContext;

  ASSERT_FALSE(GetContextFromId(@"taphere", &searchContext));
};

// Test that two DOM mutations with same value does not cancel contextual
// search.
TEST_F(ContextualSearchJsTest, TestHighlightTwoDOMMutationNoChange) {
  LoadHtml(kHTMLWithRelatedDOMMutation);
  ExecuteJavaScript(
      @"document.getElementById('taphere').innerText = 'is';"
       "document.getElementById('test').setAttribute('attr', 'before');");
  ASSERT_EQ(0, GetMutatedNodeCount());
  ContextualSearchStruct searchContext;

  ASSERT_TRUE(GetContextFromId(@"taphere", &searchContext));
};

// Test that non bubbling event does not trigger contextual search.
TEST_F(ContextualSearchJsTest, TestHighlightIgnorePreventDefault) {
  LoadHtml(kHTMLWithPreventDefault);
  // Enable touch delay.
  ExecuteJavaScript(
      @"__gCrWeb.contextualSearch.setBodyTouchListenerDelay(200);");
  ContextualSearchStruct searchContext;
  ExecuteJavaScript(@"document.getElementById('interceptor')."
                    @"addEventListener('touchend', function(event) "
                    @"{event.preventDefault();return false;}, false);");

  ASSERT_FALSE(GetContextFromId(@"taphere", &searchContext));
};

TEST_F(ContextualSearchJsTest, Test1500CharactersCenter) {
  // String should extend 15 sentences to the left and right.
  NSString* stringHTML = BuildLongTestString(0, 50, 25, true);
  NSString* expectedHTML = BuildLongTestString(10, 41, -1, false);
  LoadHtml(stringHTML);
  ContextualSearchStruct searchContext;
  ASSERT_TRUE(GetContextFromId(@"taphere", &searchContext));
  CheckContextOffsets(searchContext, base::SysNSStringToUTF8(expectedHTML));
};

TEST_F(ContextualSearchJsTest, Test1500CharactersRight) {
  // There is not enough chars on the left, so string should extend on the
  // right.
  NSString* stringHTML = BuildLongTestString(0, 50, 5, true);
  NSString* expectedHTML = BuildLongTestString(0, 30, -1, false);
  LoadHtml(
      [NSString stringWithFormat:@"<html><body>%@</body></html>", stringHTML]);
  ContextualSearchStruct searchContext;
  ASSERT_TRUE(GetContextFromId(@"taphere", &searchContext));
  CheckContextOffsets(searchContext, base::SysNSStringToUTF8(expectedHTML));
};

TEST_F(ContextualSearchJsTest, Test1500CharactersLeft) {
  // There is not enough chars on the right, so string should extend on the
  // left.
  NSString* stringHTML = BuildLongTestString(0, 50, 45, true);
  NSString* expectedHTML = BuildLongTestString(20, 50, -1, false);
  LoadHtml(
      [NSString stringWithFormat:@"<html><body>%@</body></html>", stringHTML]);
  ContextualSearchStruct searchContext;
  ASSERT_TRUE(GetContextFromId(@"taphere", &searchContext));
  CheckContextOffsets(searchContext, base::SysNSStringToUTF8(expectedHTML));
};

TEST_F(ContextualSearchJsTest, Test1500CharactersToShort) {
  // String is too short so the whole string should be in the context.
  NSString* stringHTML = BuildLongTestString(0, 10, 5, true);
  NSString* expectedHTML = BuildLongTestString(0, 10, -1, false);
  // LoadHtml will trim the last space.
  LoadHtml(
      [NSString stringWithFormat:@"<html><body>%@</body></html>", stringHTML]);
  ContextualSearchStruct searchContext;
  ASSERT_TRUE(GetContextFromId(@"taphere", &searchContext));
  CheckContextOffsets(searchContext, base::SysNSStringToUTF8(expectedHTML));
};
}  // namespace
