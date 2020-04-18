// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/mac/cfbundle_blocker.h"
#include "chrome/common/mac/cfbundle_blocker_private.h"

#include <CoreFoundation/CoreFoundation.h>
#import <Foundation/Foundation.h>
#include <stddef.h>

#include "base/logging.h"
#include "base/mac/foundation_util.h"
#include "base/mac/scoped_cftyperef.h"
#include "base/mac/scoped_nsautorelease_pool.h"
#import "base/mac/scoped_nsobject.h"
#include "base/macros.h"
#include "base/strings/sys_string_conversions.h"
#include "third_party/mach_override/mach_override.h"

namespace chrome {
namespace common {
namespace mac {

// Call this to execute the original implementation of
// _CFBundleLoadExecutableAndReturnError.
_CFBundleLoadExecutableAndReturnError_Type
    g_original_underscore_cfbundle_load_executable_and_return_error;

namespace {

// Returns an autoreleased array of paths that contain plugins that should be
// forbidden to load. Each element of the array will be a string containing
// an absolute pathname ending in '/'.
NSArray* BlockedPaths() {
  NSMutableArray* blocked_paths;

  {
    base::mac::ScopedNSAutoreleasePool autorelease_pool;

    // ~/Library, /Library, and /Network/Library. Things in /System/Library
    // aren't blacklisted.
    NSArray* blocked_prefixes =
       NSSearchPathForDirectoriesInDomains(NSLibraryDirectory,
                                           NSUserDomainMask |
                                               NSLocalDomainMask |
                                               NSNetworkDomainMask,
                                           YES);

    // Everything in the suffix list has a trailing slash so as to only block
    // loading things contained in these directories.
    NSString* const blocked_suffixes[] = {
      // Don't load third-party scripting additions either. Scripting
      // additions are loaded by AppleScript from within AEProcessAppleEvent
      // in response to an Apple Event.
      @"ScriptingAdditions/"

      // This list is intentionally incomplete. For example, it doesn't block
      // printer drivers or Internet plugins.
    };

    NSUInteger blocked_paths_count = [blocked_prefixes count] *
                                     arraysize(blocked_suffixes);

    // Not autoreleased here, because the enclosing pool is scoped too
    // narrowly.
    blocked_paths =
        [[NSMutableArray alloc] initWithCapacity:blocked_paths_count];

    // Build a flat list by adding each suffix to each prefix.
    for (NSString* blocked_prefix in blocked_prefixes) {
      for (size_t blocked_suffix_index = 0;
           blocked_suffix_index < arraysize(blocked_suffixes);
           ++blocked_suffix_index) {
        NSString* blocked_suffix = blocked_suffixes[blocked_suffix_index];
        NSString* blocked_path =
            [blocked_prefix stringByAppendingPathComponent:blocked_suffix];

        [blocked_paths addObject:blocked_path];
      }
    }

    DCHECK_EQ([blocked_paths count], blocked_paths_count);
  }

  return [blocked_paths autorelease];
}

// Returns true if bundle_path identifies a path within a blocked directory.
// Blocked directories are those returned by BlockedPaths().
bool IsBundlePathBlocked(NSString* bundle_path) {
  static NSArray* blocked_paths = [BlockedPaths() retain];

  for (NSString* blocked_path in blocked_paths) {
    NSUInteger blocked_path_length = [blocked_path length];

    // Do a case-insensitive comparison because most users will be on
    // case-insensitive HFS+ filesystems and it's cheaper than asking the
    // disk. This is like [bundle_path hasPrefix:blocked_path] but is
    // case-insensitive.
    if ([bundle_path length] >= blocked_path_length &&
        [bundle_path compare:blocked_path
                     options:NSCaseInsensitiveSearch
                       range:NSMakeRange(0, blocked_path_length)] ==
        NSOrderedSame) {
      // If bundle_path is inside blocked_path (it has blocked_path as a
      // prefix), refuse to load it.
      return true;
    }
  }

  // bundle_path is not inside any blocked_path from blocked_paths.
  return false;
}

Boolean ChromeCFBundleLoadExecutableAndReturnError(CFBundleRef bundle,
                                                   Boolean force_global,
                                                   CFErrorRef* error) {
  base::mac::ScopedNSAutoreleasePool autorelease_pool;

  DCHECK(g_original_underscore_cfbundle_load_executable_and_return_error);

  base::ScopedCFTypeRef<CFURLRef> url_cf(CFBundleCopyBundleURL(bundle));
  base::scoped_nsobject<NSString> path(base::mac::CFToNSCast(
      CFURLCopyFileSystemPath(url_cf, kCFURLPOSIXPathStyle)));

  NSString* bundle_id = base::mac::CFToNSCast(CFBundleGetIdentifier(bundle));

  NSDictionary* bundle_dictionary =
      base::mac::CFToNSCast(CFBundleGetInfoDictionary(bundle));
  NSString* version = [bundle_dictionary objectForKey:
      base::mac::CFToNSCast(kCFBundleVersionKey)];
  if (![version isKindOfClass:[NSString class]]) {
    // Deal with pranksters.
    version = nil;
  }

  if (IsBundlePathBlocked(path) && !IsBundleAllowed(bundle_id, version)) {
    NSString* bundle_id_print = bundle_id ? bundle_id : @"(nil)";
    NSString* version_print = version ? version : @"(nil)";

    // Provide a hint for the user (or module developer) to figure out
    // that the bundle was blocked.
    LOG(INFO) << "Blocking attempt to load bundle "
              << [bundle_id_print UTF8String]
              << " version "
              << [version_print UTF8String]
              << " at "
              << [path fileSystemRepresentation];

    if (error) {
      base::ScopedCFTypeRef<CFStringRef> app_bundle_id(
          base::SysUTF8ToCFStringRef(base::mac::BaseBundleID()));

      // 0xb10c10ad = "block load"
      const CFIndex kBundleLoadBlocked = 0xb10c10ad;

      NSMutableDictionary* error_dict =
          [NSMutableDictionary dictionaryWithCapacity:4];
      if (bundle_id) {
        [error_dict setObject:bundle_id forKey:@"bundle_id"];
      }
      if (version) {
        [error_dict setObject:version forKey:@"version"];
      }
      if (path) {
        [error_dict setObject:path forKey:@"path"];
      }
      NSURL* url_ns = base::mac::CFToNSCast(url_cf);
      NSString* url_absolute_string = [url_ns absoluteString];
      if (url_absolute_string) {
        [error_dict setObject:url_absolute_string forKey:@"url"];
      }

      *error = CFErrorCreate(NULL,
                             app_bundle_id,
                             kBundleLoadBlocked,
                             base::mac::NSToCFCast(error_dict));
    }

    return FALSE;
  }

  // Not blocked. Call through to the original implementation.
  return g_original_underscore_cfbundle_load_executable_and_return_error(
      bundle, force_global, error);
}

}  // namespace

bool EnableCFBundleBlocker() {
  mach_error_t err = mach_override_ptr(
      reinterpret_cast<void*>(_CFBundleLoadExecutableAndReturnError),
      reinterpret_cast<void*>(ChromeCFBundleLoadExecutableAndReturnError),
      reinterpret_cast<void**>(
          &g_original_underscore_cfbundle_load_executable_and_return_error));
  if (err != err_none) {
    DLOG(WARNING) << "mach_override _CFBundleLoadExecutableAndReturnError: "
                  << err;
    return false;
  }
  return true;
}

namespace {

struct AllowedBundle {
  // The bundle identifier to permit. These are matched with a case-sensitive
  // literal comparison. "Children" of the declared bundle ID are permitted:
  // if bundle_id here is @"org.chromium", it would match both @"org.chromium"
  // and @"org.chromium.Chromium".
  NSString* bundle_id;

  // If bundle_id should only be permitted as of a certain minimum version,
  // this string defines that version, which will be compared to the bundle's
  // version with a numeric comparison. If bundle_id may be permitted at any
  // version, set minimum_version to nil.
  NSString* minimum_version;
};

}  // namespace

bool IsBundleAllowed(NSString* bundle_id, NSString* version) {
  // The list of bundles that are allowed to load. Before adding an entry to
  // this list, be sure that it's well-behaved. Specifically, anything that
  // uses mach_override
  // (https://github.com/rentzsch/mach_star/tree/master/mach_override) must
  // use version 51ae3d199463fa84548f466d649f0821d579fdaf (July 22, 2011) or
  // newer. Products added to the list must not cause crashes. Entries should
  // include the name of the product, URL, and the name and e-mail address of
  // someone responsible for the product's engineering. To add items to this
  // list, file a bug at http://crbug.com/new using the "Defect on Mac OS"
  // template, and provide the bundle ID (or IDs) and minimum CFBundleVersion
  // that's safe for Chrome to load, along with the necessary product and
  // contact information. Whitelisted bundles in this list may be removed if
  // they are found to cause instability or otherwise behave badly. With
  // proper contact information, Chrome developers may try to contact
  // maintainers to resolve any problems.
  const AllowedBundle kAllowedBundles[] = {
    // Google Authenticator BT
    // Dave MacLachlan <dmaclach@google.com>
    { @"com.google.osax.Google_Authenticator_BT", nil },

    // Default Folder X, http://www.stclairsoft.com/DefaultFolderX/
    // Jon Gotow <gotow@stclairsoft.com>
    { @"com.stclairsoft.DefaultFolderX", @"4.4.3" },

    // MySpeed, http://www.enounce.com/myspeed
    // Edward Bianchi <ejbianchi@enounce.com>
    { @"com.enounce.MySpeed.osax", @"1201" },

    // SIMBL (fork), https://github.com/albertz/simbl
    // Albert Zeyer <albzey@googlemail.com>
    { @"net.culater.SIMBL", nil },

    // Smart Scroll, http://marcmoini.com/sx_en.html
    // Marc Moini <marc@a9ff.com>
    { @"com.marcmoini.SmartScroll", @"3.9" },

    // Bartender for Mac, http://www.macbartender.com
    // Ben Surtees <ben@surteesstudios.com>
    { @"com.surteesstudios.BartenderHelper", @"1.2.20" },
    { @"com.surteesstudios.BartenderHelperSeventy", @"1.2.20" },
    { @"com.surteesstudios.BartenderHelperBundle", @"1.2.20" },
  };

  for (size_t index = 0; index < arraysize(kAllowedBundles); ++index) {
    const AllowedBundle& allowed_bundle = kAllowedBundles[index];
    NSString* allowed_bundle_id = allowed_bundle.bundle_id;
    NSUInteger allowed_bundle_id_length = [allowed_bundle_id length];

    // Permit bundle identifiers that are exactly equal to the allowed
    // identifier, as well as "children" of the allowed identifier.
    if ([bundle_id isEqualToString:allowed_bundle_id] ||
        ([bundle_id length] > allowed_bundle_id_length &&
         [bundle_id characterAtIndex:allowed_bundle_id_length] == '.' &&
         [bundle_id hasPrefix:allowed_bundle_id])) {
      NSString* minimum_version = allowed_bundle.minimum_version;
      if (!minimum_version) {
        // If the rule didn't declare any version requirement, the bundle is
        // allowed to load.
        return true;
      }

      if (!version) {
        // If there wasn't any version but one was required, the bundle isn't
        // allowed to load.
        return false;
      }

      // A numeric search is appropriate for comparing version numbers.
      NSComparisonResult result = [version compare:minimum_version
                                           options:NSNumericSearch];
      return result != NSOrderedAscending;
    }
  }

  // Nothing matched.
  return false;
}

}  // namespace mac
}  // namespace common
}  // namespace chrome
