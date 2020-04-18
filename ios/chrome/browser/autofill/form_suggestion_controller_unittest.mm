// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/autofill/form_suggestion_controller.h"

#include <utility>
#include <vector>

#include "base/mac/foundation_util.h"
#include "base/path_service.h"
#include "base/strings/stringprintf.h"
#import "components/autofill/ios/browser/form_suggestion.h"
#import "components/autofill/ios/browser/form_suggestion_provider.h"
#import "ios/chrome/browser/autofill/form_input_accessory_view_controller.h"
#import "ios/chrome/browser/autofill/form_suggestion_view.h"
#include "ios/chrome/browser/ui/ui_util.h"
#import "ios/chrome/browser/web/chrome_web_test.h"
#import "ios/web/public/navigation_manager.h"
#include "ios/web/public/web_state/form_activity_params.h"
#import "ios/web/public/web_state/ui/crw_web_view_proxy.h"
#import "ios/web/public/web_state/web_state.h"
#import "testing/gtest_mac.h"
#import "third_party/ocmock/OCMock/OCMock.h"
#include "third_party/ocmock/gtest_support.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface FormInputAccessoryViewController (Testing)
- (instancetype)initWithWebState:(web::WebState*)webState
             JSSuggestionManager:(JsSuggestionManager*)JSSuggestionManager
                       providers:(NSArray*)providers;
@end

// Test provider that records invocations of its interface methods.
@interface TestSuggestionProvider : NSObject<FormSuggestionProvider>

@property(weak, nonatomic, readonly) FormSuggestion* suggestion;
@property(weak, nonatomic, readonly) NSString* formName;
@property(weak, nonatomic, readonly) NSString* fieldName;
@property(nonatomic, assign) BOOL selected;
@property(nonatomic, assign) BOOL askedIfSuggestionsAvailable;
@property(nonatomic, assign) BOOL askedForSuggestions;

- (instancetype)initWithSuggestions:(NSArray*)suggestions;

@end

@implementation TestSuggestionProvider {
  NSArray* _suggestions;
  NSString* _formName;
  NSString* _fieldName;
  NSString* _fieldIdentifier;
  FormSuggestion* _suggestion;
}

@synthesize selected = _selected;
@synthesize askedIfSuggestionsAvailable = _askedIfSuggestionsAvailable;
@synthesize askedForSuggestions = _askedForSuggestions;

- (instancetype)initWithSuggestions:(NSArray*)suggestions {
  self = [super init];
  if (self)
    _suggestions = [suggestions copy];
  return self;
}

- (NSString*)formName {
  return _formName;
}

- (NSString*)fieldName {
  return _fieldName;
}

- (FormSuggestion*)suggestion {
  return _suggestion;
}

- (void)checkIfSuggestionsAvailableForForm:(NSString*)formName
                                 fieldName:(NSString*)fieldName
                           fieldIdentifier:(NSString*)fieldIdentifier
                                 fieldType:(NSString*)fieldType
                                      type:(NSString*)type
                                typedValue:(NSString*)typedValue
                               isMainFrame:(BOOL)isMainFrame
                                  webState:(web::WebState*)webState
                         completionHandler:
                             (SuggestionsAvailableCompletion)completion {
  self.askedIfSuggestionsAvailable = YES;
  completion([_suggestions count] > 0);
}

- (void)retrieveSuggestionsForForm:(NSString*)formName
                         fieldName:(NSString*)fieldName
                   fieldIdentifier:(NSString*)fieldIdentifier
                         fieldType:(NSString*)fieldType
                              type:(NSString*)type
                        typedValue:(NSString*)typedValue
                          webState:(web::WebState*)webState
                 completionHandler:(SuggestionsReadyCompletion)completion {
  self.askedForSuggestions = YES;
  completion(_suggestions, self);
}

- (void)didSelectSuggestion:(FormSuggestion*)suggestion
                  fieldName:(NSString*)fieldName
            fieldIdentifier:(NSString*)fieldIdentifier
                       form:(NSString*)formName
          completionHandler:(SuggestionHandledCompletion)completion {
  self.selected = YES;
  _suggestion = suggestion;
  _formName = [formName copy];
  _fieldName = [fieldName copy];
  _fieldIdentifier = [fieldIdentifier copy];
  completion();
}

@end

namespace {

// Finds the FormSuggestionView in |parent|'s view hierarchy, if it exists.
FormSuggestionView* GetSuggestionView(UIView* parent) {
  if ([parent isKindOfClass:[FormSuggestionView class]])
    return base::mac::ObjCCastStrict<FormSuggestionView>(parent);
  for (UIView* child in parent.subviews) {
    UIView* suggestion_view = GetSuggestionView(child);
    if (suggestion_view)
      return base::mac::ObjCCastStrict<FormSuggestionView>(suggestion_view);
  }
  return nil;
}

// Test fixture for FormSuggestionController testing.
class FormSuggestionControllerTest : public ChromeWebTest {
 public:
  FormSuggestionControllerTest() {}

  void SetUp() override {
    ChromeWebTest::SetUp();

    // Mock out the JsSuggestionManager.
    mock_js_suggestion_manager_ =
        [OCMockObject niceMockForClass:[JsSuggestionManager class]];

    // Set up a fake keyboard accessory view. It is expected to have two
    // subviews.
    input_accessory_view_ = [[UIView alloc] init];
    UIView* fake_view_1 = [[UIView alloc] init];
    [input_accessory_view_ addSubview:fake_view_1];
    UIView* fake_view_2 = [[UIView alloc] init];
    [input_accessory_view_ addSubview:fake_view_2];

    // Return the fake keyboard accessory view from the mock CRWWebViewProxy.
    mock_web_view_proxy_ =
        [OCMockObject niceMockForProtocol:@protocol(CRWWebViewProxy)];
    [[[mock_web_view_proxy_ stub] andReturn:input_accessory_view_]
        keyboardAccessory];
  }

  void TearDown() override {
    [suggestion_controller_ detachFromWebState];
    ChromeWebTest::TearDown();
  }

  // Sets |url| to be current for WebState.
  void SetCurrentUrl(const std::string& url) {
    LoadHtml(@"<html></html>", GURL(url));
  }

 protected:
  // Sets up |suggestion_controller_| with the specified array of
  // FormSuggestionProviders.
  void SetUpController(NSArray* providers) {
    suggestion_controller_ = [[FormSuggestionController alloc]
           initWithWebState:web_state()
                  providers:providers
        JsSuggestionManager:mock_js_suggestion_manager_];
    [suggestion_controller_ setWebViewProxy:mock_web_view_proxy_];
    @autoreleasepool {
      accessory_controller_ = [[FormInputAccessoryViewController alloc]
             initWithWebState:web_state()
          JSSuggestionManager:mock_js_suggestion_manager_
                    providers:@[
                      [suggestion_controller_ accessoryViewProvider]
                    ]];
    }
    // Mock out the FormInputAccessoryViewController so it can use the fake
    // CRWWebViewProxy
    id mock_accessory_controller =
        [OCMockObject partialMockForObject:accessory_controller_];
    [[[mock_accessory_controller stub] andReturn:mock_web_view_proxy_]
        webViewProxy];

    // On iPad devices, the suggestion view is added directly to the
    // keyboard view instead of to the input accessory view which is no longer
    // available on iPad devices. The following code mocks out the methods on
    // FormInputAccessoryViewController that add and remove the suggestion view.
    // The mocks now just add and remove it directly to and from
    // input_accessory_view_ so that the tests can locate it with
    // GetSuggestionView (defined above).
    // TODO(crbug.com/661622): Revisit this to see if there's a better way to
    // test the iPad case. At a minimum, the name 'input_accessory_view_' should
    // be made more generic.
    if (IsIPadIdiom()) {
      void (^mockShow)(NSInvocation*) = ^(NSInvocation* invocation) {
        __unsafe_unretained UIView* view;
        [invocation getArgument:&view atIndex:2];
        for (UIView* view in [input_accessory_view_ subviews]) {
          [view removeFromSuperview];
        }
        [input_accessory_view_ addSubview:view];
      };
      [[[mock_accessory_controller stub] andDo:mockShow]
          showCustomInputAccessoryView:[OCMArg any]];

      void (^mockRestore)(NSInvocation*) = ^(NSInvocation* invocation) {
        for (UIView* view in [input_accessory_view_ subviews]) {
          [view removeFromSuperview];
        }
      };
      [[[mock_accessory_controller stub] andDo:mockRestore]
          restoreDefaultInputAccessoryView];
    }
  }

  // The FormSuggestionController under test.
  FormSuggestionController* suggestion_controller_;

  // A fake keyboard accessory view.
  UIView* input_accessory_view_;

  // Mock JsSuggestionManager for verifying interactions.
  id mock_js_suggestion_manager_;

  // Mock CRWWebViewProxy for verifying interactions.
  id mock_web_view_proxy_;

  // Accessory view controller.
  FormInputAccessoryViewController* accessory_controller_;

  DISALLOW_COPY_AND_ASSIGN(FormSuggestionControllerTest);
};

// Tests that pages whose URLs don't have a web scheme aren't processed.
TEST_F(FormSuggestionControllerTest, PageLoadShouldBeIgnoredWhenNotWebScheme) {
  SetUpController(@[]);
  SetCurrentUrl("data:text/html;charset=utf8;base64,");
  [suggestion_controller_ webState:web_state() didLoadPageWithSuccess:YES];

  EXPECT_FALSE(GetSuggestionView(input_accessory_view_));
  EXPECT_OCMOCK_VERIFY(mock_js_suggestion_manager_);
}

// Tests that pages whose content isn't HTML aren't processed.
TEST_F(FormSuggestionControllerTest, PageLoadShouldBeIgnoredWhenNotHtml) {
  SetUpController(@[]);

  // Construct file:// URL for a PDF file.
  base::FilePath path;
  base::PathService::Get(base::DIR_MODULE, &path);
  const char kPdfFilePath[] = "ios/testing/data/http_server_files/testpage.pdf";
  path = path.Append(FILE_PATH_LITERAL(kPdfFilePath));
  GURL url(base::StringPrintf("file://%s", path.value().c_str()));

  // Load PDF file URL.
  web::NavigationManager::WebLoadParams params(url);
  web_state()->GetNavigationManager()->LoadURLWithParams(params);
  WaitForCondition(^{
    return !web_state()->IsLoading();
  });

  ASSERT_EQ("application/pdf", web_state()->GetContentsMimeType());
  EXPECT_FALSE(GetSuggestionView(input_accessory_view_));
  EXPECT_OCMOCK_VERIFY(mock_js_suggestion_manager_);
}

// Tests that the keyboard accessory view is reset and JavaScript is injected
// when a page is loaded.
TEST_F(FormSuggestionControllerTest,
       PageLoadShouldRestoreKeyboardAccessoryViewAndInjectJavaScript) {
  SetUpController(@[]);
  SetCurrentUrl("http://foo.com");

  // Load the page. The JS should be injected.
  [[mock_js_suggestion_manager_ expect] inject];
  [suggestion_controller_ webState:web_state() didLoadPageWithSuccess:YES];
  EXPECT_OCMOCK_VERIFY(mock_js_suggestion_manager_);

  // Trigger form activity, which should set up the suggestions view.
  web::FormActivityParams params;
  params.form_name = "form";
  params.field_name = "field";
  params.field_identifier = "field_id";
  params.field_type = "text";
  params.type = "type";
  params.value = "value";
  params.input_missing = false;
  [accessory_controller_ webState:web_state() didRegisterFormActivity:params];
  EXPECT_TRUE(GetSuggestionView(input_accessory_view_));

  // Trigger another page load. The suggestions accessory view should
  // not be present.
  [accessory_controller_ webState:web_state() didLoadPageWithSuccess:YES];
  EXPECT_FALSE(GetSuggestionView(input_accessory_view_));
}

// Tests that "blur" events are ignored.
TEST_F(FormSuggestionControllerTest, FormActivityBlurShouldBeIgnored) {
  web::FormActivityParams params;
  params.form_name = "form";
  params.field_name = "field";
  params.field_identifier = "field_id";
  params.field_type = "text";
  params.type = "blur";  // blur!
  params.value = "value";
  params.input_missing = false;
  [accessory_controller_ webState:web_state() didRegisterFormActivity:params];
  EXPECT_FALSE(GetSuggestionView(input_accessory_view_));
}

// Tests that no suggestions are displayed when no providers are registered.
TEST_F(FormSuggestionControllerTest,
       FormActivityShouldRetrieveSuggestions_NoProvidersAvailable) {
  // Set up the controller without any providers.
  SetUpController(@[]);
  SetCurrentUrl("http://foo.com");
  web::FormActivityParams params;
  params.form_name = "form";
  params.field_name = "field";
  params.field_identifier = "field_id";
  params.field_type = "text";
  params.type = "type";
  params.value = "value";
  params.input_missing = false;
  [accessory_controller_ webState:web_state() didRegisterFormActivity:params];

  // The suggestions accessory view should be empty.
  FormSuggestionView* suggestionView = GetSuggestionView(input_accessory_view_);
  EXPECT_TRUE(suggestionView);
  EXPECT_EQ(0U, [suggestionView.suggestions count]);
}

// Tests that, when no providers have suggestions to offer for a form/field,
// they aren't asked and no suggestions are displayed.
TEST_F(FormSuggestionControllerTest,
       FormActivityShouldRetrieveSuggestions_NoSuggestionsAvailable) {
  // Set up the controller with some providers, but none of them will
  // have suggestions available.
  TestSuggestionProvider* provider1 =
      [[TestSuggestionProvider alloc] initWithSuggestions:@[]];
  TestSuggestionProvider* provider2 =
      [[TestSuggestionProvider alloc] initWithSuggestions:@[]];
  SetUpController(@[ provider1, provider2 ]);
  SetCurrentUrl("http://foo.com");

  web::FormActivityParams params;
  params.form_name = "form";
  params.field_name = "field";
  params.field_identifier = "field_id";
  params.field_type = "text";
  params.type = "type";
  params.value = "value";
  params.input_missing = false;
  [accessory_controller_ webState:web_state() didRegisterFormActivity:params];

  // The providers should each be asked if they have suggestions for the
  // form in question.
  EXPECT_TRUE([provider1 askedIfSuggestionsAvailable]);
  EXPECT_TRUE([provider2 askedIfSuggestionsAvailable]);

  // Since none of the providers had suggestions available, none of them
  // should have been asked for suggestions.
  EXPECT_FALSE([provider1 askedForSuggestions]);
  EXPECT_FALSE([provider2 askedForSuggestions]);

  // The accessory view should be empty.
  FormSuggestionView* suggestionView = GetSuggestionView(input_accessory_view_);
  EXPECT_TRUE(suggestionView);
  EXPECT_EQ(0U, [suggestionView.suggestions count]);
}

// Tests that, once a provider is asked if it has suggestions for a form/field,
// it and only it is asked to provide them, and that they are then displayed
// in the keyboard accessory view.
TEST_F(FormSuggestionControllerTest,
       FormActivityShouldRetrieveSuggestions_SuggestionsAddedToAccessoryView) {
  // Set up the controller with some providers, one of which can provide
  // suggestions.
  NSArray* suggestions = @[
    [FormSuggestion suggestionWithValue:@"foo"
                     displayDescription:nil
                                   icon:@""
                             identifier:0],
    [FormSuggestion suggestionWithValue:@"bar"
                     displayDescription:nil
                                   icon:@""
                             identifier:1]
  ];
  TestSuggestionProvider* provider1 =
      [[TestSuggestionProvider alloc] initWithSuggestions:suggestions];
  TestSuggestionProvider* provider2 =
      [[TestSuggestionProvider alloc] initWithSuggestions:@[]];
  SetUpController(@[ provider1, provider2 ]);
  SetCurrentUrl("http://foo.com");

  web::FormActivityParams params;
  params.form_name = "form";
  params.field_name = "field";
  params.field_identifier = "field_id";
  params.field_type = "text";
  params.type = "type";
  params.value = "value";
  params.input_missing = false;
  [accessory_controller_ webState:web_state() didRegisterFormActivity:params];

  // Since the first provider has suggestions available, it and only it
  // should have been asked.
  EXPECT_TRUE([provider1 askedIfSuggestionsAvailable]);
  EXPECT_FALSE([provider2 askedIfSuggestionsAvailable]);

  // Since the first provider said it had suggestions, it and only it
  // should have been asked to provide them.
  EXPECT_TRUE([provider1 askedForSuggestions]);
  EXPECT_FALSE([provider2 askedForSuggestions]);

  // The accessory view should show the suggestions.
  FormSuggestionView* suggestionView = GetSuggestionView(input_accessory_view_);
  EXPECT_TRUE(suggestionView);
  EXPECT_NSEQ(suggestions, suggestionView.suggestions);
}

// Tests that selecting a suggestion from the accessory view informs the
// specified delegate for that suggestion.
TEST_F(FormSuggestionControllerTest, SelectingSuggestionShouldNotifyDelegate) {
  // Send some suggestions to the controller and then tap one.
  NSArray* suggestions = @[
    [FormSuggestion suggestionWithValue:@"foo"
                     displayDescription:nil
                                   icon:@""
                             identifier:0],
  ];
  TestSuggestionProvider* provider =
      [[TestSuggestionProvider alloc] initWithSuggestions:suggestions];
  SetUpController(@[ provider ]);
  SetCurrentUrl("http://foo.com");
  web::FormActivityParams params;
  params.form_name = "form";
  params.field_name = "field";
  params.field_identifier = "field_id";
  params.field_type = "text";
  params.type = "type";
  params.value = "value";
  params.input_missing = false;
  [accessory_controller_ webState:web_state() didRegisterFormActivity:params];

  // Selecting a suggestion should notify the delegate.
  [suggestion_controller_ didSelectSuggestion:suggestions[0]];
  EXPECT_TRUE([provider selected]);
  EXPECT_NSEQ(@"form", [provider formName]);
  EXPECT_NSEQ(@"field", [provider fieldName]);
  EXPECT_NSEQ(suggestions[0], [provider suggestion]);
}

}  // namespace
