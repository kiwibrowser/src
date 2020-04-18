// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "components/autofill/ios/browser/fake_autofill_agent.h"

#import "base/mac/bind_objc_block.h"
#include "ios/web/public/web_thread.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation FakeAutofillAgent {
  NSMutableDictionary<NSString*, NSMutableArray<FormSuggestion*>*>*
      _suggestionsByFormAndFieldName;
  NSMutableDictionary<NSString*, FormSuggestion*>*
      _selectedSuggestionByFormAndFieldName;
}

- (instancetype)initWithPrefService:(PrefService*)prefService
                           webState:(web::WebState*)webState {
  self = [super initWithPrefService:prefService webState:webState];
  if (self) {
    _suggestionsByFormAndFieldName = [NSMutableDictionary dictionary];
    _selectedSuggestionByFormAndFieldName = [NSMutableDictionary dictionary];
  }
  return self;
}

#pragma mark - Public Methods

- (void)addSuggestion:(FormSuggestion*)suggestion
          forFormName:(NSString*)formName
      fieldIdentifier:(NSString*)fieldIdentifier {
  NSString* key =
      [self keyForFormName:formName fieldIdentifier:fieldIdentifier];
  NSMutableArray* suggestions = _suggestionsByFormAndFieldName[key];
  if (!suggestions) {
    suggestions = [NSMutableArray array];
    _suggestionsByFormAndFieldName[key] = suggestions;
  }
  [suggestions addObject:suggestion];
}

- (FormSuggestion*)selectedSuggestionForFormName:(NSString*)formName
                                 fieldIdentifier:(NSString*)fieldIdentifier {
  NSString* key =
      [self keyForFormName:formName fieldIdentifier:fieldIdentifier];
  return _selectedSuggestionByFormAndFieldName[key];
}

#pragma mark - FormSuggestionProvider

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
  web::WebThread::PostTask(
      web::WebThread::UI, FROM_HERE, base::BindBlockArc(^{
        NSString* key =
            [self keyForFormName:formName fieldIdentifier:fieldIdentifier];
        completion([_suggestionsByFormAndFieldName[key] count] ? YES : NO);
      }));
}

- (void)retrieveSuggestionsForForm:(NSString*)formName
                         fieldName:(NSString*)fieldName
                   fieldIdentifier:(NSString*)fieldIdentifier
                         fieldType:(NSString*)fieldType
                              type:(NSString*)type
                        typedValue:(NSString*)typedValue
                          webState:(web::WebState*)webState
                 completionHandler:(SuggestionsReadyCompletion)completion {
  web::WebThread::PostTask(
      web::WebThread::UI, FROM_HERE, base::BindBlockArc(^{
        NSString* key =
            [self keyForFormName:formName fieldIdentifier:fieldIdentifier];
        completion(_suggestionsByFormAndFieldName[key], self);
      }));
}

- (void)didSelectSuggestion:(FormSuggestion*)suggestion
                  fieldName:(NSString*)fieldName
            fieldIdentifier:(NSString*)fieldIdentifier
                       form:(NSString*)formName
          completionHandler:(SuggestionHandledCompletion)completion {
  web::WebThread::PostTask(
      web::WebThread::UI, FROM_HERE, base::BindBlockArc(^{
        NSString* key =
            [self keyForFormName:formName fieldIdentifier:fieldIdentifier];
        _selectedSuggestionByFormAndFieldName[key] = suggestion;
        completion();
      }));
}

#pragma mark - Private Methods

- (NSString*)keyForFormName:(NSString*)formName
            fieldIdentifier:(NSString*)fieldIdentifier {
  // Uniqueness ensured because spaces are not allowed in html name attributes.
  return [NSString stringWithFormat:@"%@ %@", formName, fieldIdentifier];
}

@end
