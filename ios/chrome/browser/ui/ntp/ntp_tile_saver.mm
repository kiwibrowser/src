// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/ui/ntp/ntp_tile_saver.h"

#include "base/bind.h"
#import "base/mac/bind_objc_block.h"
#include "base/md5.h"
#include "base/strings/sys_string_conversions.h"
#include "base/task_scheduler/post_task.h"
#include "base/threading/thread_restrictions.h"
#include "components/favicon/core/fallback_url_util.h"
#include "components/ntp_tiles/ntp_tile.h"
#import "ios/chrome/browser/ui/favicon/favicon_attributes.h"
#import "ios/chrome/browser/ui/favicon/favicon_attributes_provider.h"
#import "ios/chrome/browser/ui/ntp/ntp_tile.h"
#include "ios/chrome/common/app_group/app_group_constants.h"
#import "net/base/mac/url_conversions.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace ntp_tile_saver {

// Write the |most_visited_sites| to disk.
void WriteSavedMostVisited(NSDictionary<NSURL*, NTPTile*>* most_visited_sites);

// Checks if every site in |tiles| has had its favicons fetched. If so, writes
// the info to disk, saving the favicons to |favicons_directory|.
void WriteToDiskIfComplete(NSDictionary<NSURL*, NTPTile*>* tiles,
                           NSURL* favicons_directory);

// Gets a name for the favicon file.
NSString* GetFaviconFileName(const GURL& url);

// If the sites currently saved include one with |tile|'s url, replace it by
// |tile|.
void WriteSingleUpdatedTileToDisk(NTPTile* tile);

// Get the favicons using |favicon_provider| and writes them to disk.
void GetFaviconsAndSave(const ntp_tiles::NTPTilesVector& most_visited_data,
                        FaviconAttributesProvider* favicon_provider,
                        NSURL* favicons_directory);

// Updates the list of tiles that must be displayed in the content suggestion
// widget.
void UpdateTileList(const ntp_tiles::NTPTilesVector& most_visited_data);

// Deletes icons contained in |favicons_directory| and corresponding to no URL
// in |most_visited_data|.
void ClearOutdatedIcons(const ntp_tiles::NTPTilesVector& most_visited_data,
                        NSURL* favicons_directory);

}  // namespace ntp_tile_saver

namespace ntp_tile_saver {

void UpdateTileList(const ntp_tiles::NTPTilesVector& most_visited_data) {
  NSMutableDictionary<NSURL*, NTPTile*>* tiles =
      [[NSMutableDictionary alloc] init];
  NSDictionary<NSURL*, NTPTile*>* old_tiles = ReadSavedMostVisited();
  for (size_t i = 0; i < most_visited_data.size(); i++) {
    const ntp_tiles::NTPTile& ntp_tile = most_visited_data[i];
    NSURL* ns_url = net::NSURLWithGURL(ntp_tile.url);
    NTPTile* tile =
        [[NTPTile alloc] initWithTitle:base::SysUTF16ToNSString(ntp_tile.title)
                                   URL:ns_url
                              position:i];
    tile.faviconFileName = GetFaviconFileName(ntp_tile.url);
    NTPTile* old_tile = [old_tiles objectForKey:ns_url];
    if (old_tile) {
      // Keep fallback data.
      tile.fallbackMonogram = old_tile.fallbackMonogram;
      tile.fallbackTextColor = old_tile.fallbackTextColor;
      tile.fallbackIsDefaultColor = old_tile.fallbackIsDefaultColor;
      tile.fallbackBackgroundColor = old_tile.fallbackBackgroundColor;
    }
    [tiles setObject:tile forKey:tile.URL];
  }
  WriteSavedMostVisited(tiles);
}

NSString* GetFaviconFileName(const GURL& url) {
  return [base::SysUTF8ToNSString(base::MD5String(url.spec()))
      stringByAppendingString:@".png"];
}

void GetFaviconsAndSave(const ntp_tiles::NTPTilesVector& most_visited_data,
                        FaviconAttributesProvider* favicon_provider,
                        NSURL* favicons_directory) {
  for (size_t i = 0; i < most_visited_data.size(); i++) {
    const GURL& gurl = most_visited_data[i].url;
    UpdateSingleFavicon(gurl, favicon_provider, favicons_directory);
  }
}

void ClearOutdatedIcons(const ntp_tiles::NTPTilesVector& most_visited_data,
                        NSURL* favicons_directory) {
  NSMutableSet<NSString*>* allowed_files_name = [[NSMutableSet alloc] init];
  for (size_t i = 0; i < most_visited_data.size(); i++) {
    const ntp_tiles::NTPTile& ntp_tile = most_visited_data[i];
    NSString* favicon_file_name = GetFaviconFileName(ntp_tile.url);
    [allowed_files_name addObject:favicon_file_name];
  }
  [[NSFileManager defaultManager] createDirectoryAtURL:favicons_directory
                           withIntermediateDirectories:YES
                                            attributes:nil
                                                 error:nil];
  NSArray<NSURL*>* existing_files = [[NSFileManager defaultManager]
        contentsOfDirectoryAtURL:favicons_directory
      includingPropertiesForKeys:nil
                         options:0
                           error:nil];
  for (NSURL* file : existing_files) {
    if (![allowed_files_name containsObject:[file lastPathComponent]]) {
      [[NSFileManager defaultManager] removeItemAtURL:file error:nil];
    }
  }
}

void SaveMostVisitedToDisk(const ntp_tiles::NTPTilesVector& most_visited_data,
                           FaviconAttributesProvider* favicon_provider,
                           NSURL* favicons_directory) {
  if (favicons_directory == nil) {
    return;
  }
  UpdateTileList(most_visited_data);

  base::PostTaskWithTraitsAndReply(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::BACKGROUND},
      base::BindOnce(&ClearOutdatedIcons, most_visited_data,
                     favicons_directory),
      base::Bind(base::BindBlockArc(
                     ^(const ntp_tiles::NTPTilesVector& most_visited_data) {
                       GetFaviconsAndSave(most_visited_data, favicon_provider,
                                          favicons_directory);
                     }),
                 most_visited_data));
}

void WriteSingleUpdatedTileToDisk(NTPTile* tile) {
  NSMutableDictionary* tiles = [ReadSavedMostVisited() mutableCopy];
  if (![tiles objectForKey:tile.URL]) {
    return;
  }
  [tiles setObject:tile forKey:tile.URL];
  WriteSavedMostVisited(tiles);
}

void WriteSavedMostVisited(NSDictionary<NSURL*, NTPTile*>* most_visited_data) {
  NSData* data = [NSKeyedArchiver archivedDataWithRootObject:most_visited_data];
  NSUserDefaults* sharedDefaults = app_group::GetGroupUserDefaults();
  [sharedDefaults setObject:data forKey:app_group::kSuggestedItems];

  // TODO(crbug.com/750673): Update the widget's visibility depending on
  // availability of sites.
}

NSDictionary* ReadSavedMostVisited() {
  NSUserDefaults* sharedDefaults = app_group::GetGroupUserDefaults();

  return [NSKeyedUnarchiver
      unarchiveObjectWithData:[sharedDefaults
                                  objectForKey:app_group::kSuggestedItems]];
}

void UpdateSingleFavicon(const GURL& site_url,
                         FaviconAttributesProvider* favicon_provider,
                         NSURL* favicons_directory) {
  NSURL* siteNSURL = net::NSURLWithGURL(site_url);

  void (^faviconAttributesBlock)(FaviconAttributes*) =
      ^(FaviconAttributes* attributes) {
        if (attributes.faviconImage) {
          // Update the available icon.
          // If we have a fallback icon, do not remove it. The favicon will have
          // priority, and should anything happen to the image, the fallback
          // icon will be a nicer fallback.
          NSString* faviconFileName =
              GetFaviconFileName(net::GURLWithNSURL(siteNSURL));
          NSURL* fileURL =
              [favicons_directory URLByAppendingPathComponent:faviconFileName];
          NSData* imageData = UIImagePNGRepresentation(attributes.faviconImage);

          base::OnceCallback<void()> writeImage = base::BindBlockArc(^{
            base::AssertBlockingAllowed();
            [imageData writeToURL:fileURL atomically:YES];
          });

          base::PostTaskWithTraits(
              FROM_HERE, {base::MayBlock(), base::TaskPriority::BACKGROUND},
              std::move(writeImage));
        } else {
          NSDictionary* tiles = ReadSavedMostVisited();
          NTPTile* tile = [tiles objectForKey:siteNSURL];
          if (!tile) {
            return;
          }
          tile.fallbackTextColor = attributes.textColor;
          tile.fallbackBackgroundColor = attributes.backgroundColor;
          tile.fallbackIsDefaultColor = attributes.defaultBackgroundColor;
          tile.fallbackMonogram = attributes.monogramString;
          WriteSingleUpdatedTileToDisk(tile);
          // Favicon is outdated. Delete it.
          NSString* faviconFileName =
              GetFaviconFileName(net::GURLWithNSURL(siteNSURL));
          NSURL* fileURL =
              [favicons_directory URLByAppendingPathComponent:faviconFileName];
          base::OnceCallback<void()> removeImage = base::BindBlockArc(^{
            base::AssertBlockingAllowed();
            [[NSFileManager defaultManager] removeItemAtURL:fileURL error:nil];
          });

          base::PostTaskWithTraits(
              FROM_HERE, {base::MayBlock(), base::TaskPriority::BACKGROUND},
              std::move(removeImage));
        }
      };

  [favicon_provider fetchFaviconAttributesForURL:site_url
                                      completion:faviconAttributesBlock];
}
}  // namespace ntp_tile_saver
