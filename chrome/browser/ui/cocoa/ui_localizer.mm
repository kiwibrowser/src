// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/ui_localizer.h"

#import <Foundation/Foundation.h>

#include <stdlib.h>

#include "base/logging.h"
#include "base/strings/sys_string_conversions.h"
#include "chrome/grit/chromium_strings.h"
#include "chrome/grit/generated_resources.h"
#include "components/strings/grit/components_strings.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/l10n/l10n_util_mac.h"
#include "ui/strings/grit/ui_strings.h"

struct UILocalizerResourceMap {
  const char* const name;
  unsigned int label_id;
  unsigned int label_arg_id;
};


namespace {

// Utility function for bsearch on a ResourceMap table
int ResourceMapCompare(const void* utf8Void,
                       const void* resourceMapVoid) {
  const char* utf8_key = reinterpret_cast<const char*>(utf8Void);
  const UILocalizerResourceMap* res_map =
      reinterpret_cast<const UILocalizerResourceMap*> (resourceMapVoid);
  return strcmp(utf8_key, res_map->name);
}

}  // namespace

@interface GTMUILocalizer (PrivateAdditions)
- (void)localizedObjects;
@end

@implementation GTMUILocalizer (PrivateAdditions)
- (void)localizedObjects {
  // The ivars are private, so this method lets us trigger the localization
  // from -[ChromeUILocalizer awakeFromNib].
  [self localizeObject:owner_ recursively:YES];
  [self localizeObject:otherObjectToLocalize_ recursively:YES];
  [self localizeObject:yetAnotherObjectToLocalize_ recursively:YES];
}
 @end

@implementation ChromeUILocalizer

- (void)awakeFromNib {
  // The GTM base is bundle based, since don't need the bundle, use this
  // override to bypass the bundle lookup and directly do the localization
  // calls.
  [self localizedObjects];
}

- (NSString *)localizedStringForString:(NSString *)string {

  // Include the table here so it is a local static.  This header provides
  // kUIResources and kUIResourcesSize.
#include "chrome/app/nibs/localizer_table.h"

  // Look up the string for the resource id to fetch.
  const char* utf8_key = [string UTF8String];
  if (utf8_key) {
    const void* valVoid = bsearch(utf8_key,
                                  kUIResources,
                                  kUIResourcesSize,
                                  sizeof(UILocalizerResourceMap),
                                  ResourceMapCompare);
    const UILocalizerResourceMap* val =
        reinterpret_cast<const UILocalizerResourceMap*>(valVoid);
    if (val) {
      // Do we need to build the string, or just fetch it?
      if (val->label_arg_id != 0) {
        const base::string16 label_arg(
            l10n_util::GetStringUTF16(val->label_arg_id));
        return l10n_util::GetNSStringFWithFixup(val->label_id, label_arg);
      }

      return l10n_util::GetNSStringWithFixup(val->label_id);
    }

    // Sanity check, there shouldn't be any strings with this id that aren't
    // in our map.
    DLOG_IF(WARNING, [string hasPrefix:@"^ID"]) << "Key '" << utf8_key
        << "' wasn't in the resource map?";
  }

  // If we didn't find anything, this string doesn't need localizing.
  return nil;
}

@end
