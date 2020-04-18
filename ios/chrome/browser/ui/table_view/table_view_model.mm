// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/table_view/table_view_model.h"

#include "base/logging.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

NSString* const kTableViewModelCollapsedKey =
    @"ChromeTableViewModelCollapsedSections";

@interface TableViewModel ()
@property(strong, nonatomic) NSMutableDictionary* collapsedKeys;
@end

@implementation TableViewModel
@synthesize collapsedKeys = _collapsedKeys;

#pragma mark - UITableViewDataSource

// Override numberOfItemsInSection to return 0 if the section is collapsed.
- (NSInteger)numberOfItemsInSection:(NSInteger)section {
  DCHECK_LT(section, [self numberOfSections]);
  NSInteger sectionIdentifier = [self sectionIdentifierForSection:section];
  // Check if the sectionType is collapsed. If sectionType is collapsed
  // return 0.
  if ([self sectionIsCollapsed:sectionIdentifier]) {
    return 0;
  } else {
    return [super numberOfItemsInSection:section];
  }
}

#pragma mark - Collapsing methods.

- (void)setSectionIdentifier:(NSInteger)sectionIdentifier
                collapsedKey:(NSString*)collapsedKey {
  // Check that the sectionIdentifier exists.
  DCHECK([self hasSectionForSectionIdentifier:sectionIdentifier]);
  // Check that the collapsedKey is not being used already.
  DCHECK(![self.collapsedKeys objectForKey:collapsedKey]);
  [self.collapsedKeys setObject:collapsedKey forKey:@(sectionIdentifier)];
}

- (void)setSection:(NSInteger)sectionIdentifier collapsed:(BOOL)collapsed {
  // TODO(crbug.com/419346): Store in the browser state preference instead of
  // NSUserDefaults.
  DCHECK([self hasSectionForSectionIdentifier:sectionIdentifier]);
  NSString* sectionKey = [self.collapsedKeys objectForKey:@(sectionIdentifier)];
  DCHECK(sectionKey);
  NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];
  NSDictionary* collapsedSections =
      [defaults dictionaryForKey:kTableViewModelCollapsedKey];
  NSMutableDictionary* newCollapsedSection =
      [NSMutableDictionary dictionaryWithDictionary:collapsedSections];
  NSNumber* value = [NSNumber numberWithBool:collapsed];
  [newCollapsedSection setValue:value forKey:sectionKey];
  [defaults setObject:newCollapsedSection forKey:kTableViewModelCollapsedKey];
}

- (BOOL)sectionIsCollapsed:(NSInteger)sectionIdentifier {
  // TODO(crbug.com/419346): Store in the profile's preference instead of the
  // NSUserDefaults.
  DCHECK([self hasSectionForSectionIdentifier:sectionIdentifier]);
  NSString* sectionKey = [self.collapsedKeys objectForKey:@(sectionIdentifier)];
  if (!sectionKey)
    return NO;
  NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];
  NSDictionary* collapsedSections =
      [defaults dictionaryForKey:kTableViewModelCollapsedKey];
  NSNumber* value = (NSNumber*)[collapsedSections valueForKey:sectionKey];
  return [value boolValue];
}

// |self.collapsedKeys| lazy instantiation.
- (NSMutableDictionary*)collapsedKeys {
  if (!_collapsedKeys) {
    _collapsedKeys = [[NSMutableDictionary alloc] init];
  }
  return _collapsedKeys;
}

@end
