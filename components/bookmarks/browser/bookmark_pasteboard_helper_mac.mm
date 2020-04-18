// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/bookmarks/browser/bookmark_pasteboard_helper_mac.h"

#import <Cocoa/Cocoa.h>
#include <stddef.h>
#include <stdint.h>

#include "base/files/file_path.h"
#include "base/strings/sys_string_conversions.h"
#include "components/bookmarks/browser/bookmark_node.h"
#include "ui/base/clipboard/clipboard.h"
#include "ui/base/clipboard/clipboard_util_mac.h"

NSString* const kBookmarkDictionaryListPboardType =
    @"com.google.chrome.BookmarkDictionaryListPboardType";

namespace bookmarks {

namespace {

// Pasteboard type used to store profile path to determine which profile
// a set of bookmarks came from.
NSString* const kChromiumProfilePathPboardType =
    @"com.google.chrome.ChromiumProfilePathPboardType";

// Internal bookmark ID for a bookmark node.  Used only when moving inside
// of one profile.
NSString* const kChromiumBookmarkId = @"ChromiumBookmarkId";

// Internal bookmark meta info dictionary for a bookmark node.
NSString* const kChromiumBookmarkMetaInfo = @"ChromiumBookmarkMetaInfo";

// Keys for the type of node in BookmarkDictionaryListPboardType.
NSString* const kWebBookmarkType = @"WebBookmarkType";

NSString* const kWebBookmarkTypeList = @"WebBookmarkTypeList";

NSString* const kWebBookmarkTypeLeaf = @"WebBookmarkTypeLeaf";

BookmarkNode::MetaInfoMap MetaInfoMapFromDictionary(NSDictionary* dictionary) {
  BookmarkNode::MetaInfoMap meta_info_map;

  for (NSString* key in dictionary) {
    meta_info_map[base::SysNSStringToUTF8(key)] =
        base::SysNSStringToUTF8([dictionary objectForKey:key]);
  }

  return meta_info_map;
}

void ConvertPlistToElements(NSArray* input,
                            std::vector<BookmarkNodeData::Element>& elements) {
  NSUInteger len = [input count];
  for (NSUInteger i = 0; i < len; ++i) {
    NSDictionary* pboardBookmark = [input objectAtIndex:i];
    std::unique_ptr<BookmarkNode> new_node(new BookmarkNode(GURL()));
    int64_t node_id =
        [[pboardBookmark objectForKey:kChromiumBookmarkId] longLongValue];
    new_node->set_id(node_id);

    NSDictionary* metaInfoDictionary =
        [pboardBookmark objectForKey:kChromiumBookmarkMetaInfo];
    if (metaInfoDictionary)
      new_node->SetMetaInfoMap(MetaInfoMapFromDictionary(metaInfoDictionary));

    BOOL is_folder = [[pboardBookmark objectForKey:kWebBookmarkType]
        isEqualToString:kWebBookmarkTypeList];
    if (is_folder) {
      new_node->set_type(BookmarkNode::FOLDER);
      NSString* title = [pboardBookmark objectForKey:@"Title"];
      new_node->SetTitle(base::SysNSStringToUTF16(title));
    } else {
      new_node->set_type(BookmarkNode::URL);
      NSDictionary* uriDictionary =
          [pboardBookmark objectForKey:@"URIDictionary"];
      NSString* title = [uriDictionary objectForKey:@"title"];
      NSString* urlString = [pboardBookmark objectForKey:@"URLString"];
      new_node->SetTitle(base::SysNSStringToUTF16(title));
      new_node->set_url(GURL(base::SysNSStringToUTF8(urlString)));
    }
    BookmarkNodeData::Element e = BookmarkNodeData::Element(new_node.get());
    if (is_folder) {
      ConvertPlistToElements([pboardBookmark objectForKey:@"Children"],
                             e.children);
    }
    elements.push_back(e);
  }
}

bool ReadBookmarkDictionaryListPboardType(
    NSPasteboard* pb,
    std::vector<BookmarkNodeData::Element>& elements) {
  NSString* uti = ui::ClipboardUtil::UTIForPasteboardType(
      kBookmarkDictionaryListPboardType);
  NSArray* bookmarks = [pb propertyListForType:uti];
  if (!bookmarks)
    return false;
  ConvertPlistToElements(bookmarks, elements);
  return true;
}

bool ReadWebURLsWithTitlesPboardType(
    NSPasteboard* pb,
    std::vector<BookmarkNodeData::Element>& elements) {
  NSArray* urlsArr = nil;
  NSArray* titlesArr = nil;
  if (!ui::ClipboardUtil::URLsAndTitlesFromPasteboard(pb, &urlsArr, &titlesArr))
    return false;

  NSUInteger len = [titlesArr count];
  for (NSUInteger i = 0; i < len; ++i) {
    base::string16 title =
        base::SysNSStringToUTF16([titlesArr objectAtIndex:i]);
    std::string url = base::SysNSStringToUTF8([urlsArr objectAtIndex:i]);
    if (!url.empty()) {
      BookmarkNodeData::Element element;
      element.is_url = true;
      element.url = GURL(url);
      element.title = title;
      elements.push_back(element);
    }
  }
  return true;
}

NSDictionary* DictionaryFromBookmarkMetaInfo(
    const BookmarkNode::MetaInfoMap& meta_info_map) {
  NSMutableDictionary* dictionary = [NSMutableDictionary dictionary];

  for (BookmarkNode::MetaInfoMap::const_iterator it = meta_info_map.begin();
      it != meta_info_map.end(); ++it) {
    [dictionary setObject:base::SysUTF8ToNSString(it->second)
                   forKey:base::SysUTF8ToNSString(it->first)];
  }

  return dictionary;
}

NSArray* GetPlistForBookmarkList(
    const std::vector<BookmarkNodeData::Element>& elements) {
  NSMutableArray* plist = [NSMutableArray array];
  for (size_t i = 0; i < elements.size(); ++i) {
    BookmarkNodeData::Element element = elements[i];
    NSDictionary* metaInfoDictionary =
        DictionaryFromBookmarkMetaInfo(element.meta_info_map);
    if (element.is_url) {
      NSString* title = base::SysUTF16ToNSString(element.title);
      NSString* url = base::SysUTF8ToNSString(element.url.spec());
      int64_t elementId = element.id();
      NSNumber* idNum = [NSNumber numberWithLongLong:elementId];
      NSDictionary* uriDictionary = [NSDictionary dictionaryWithObjectsAndKeys:
              title, @"title", nil];
      NSDictionary* object = [NSDictionary dictionaryWithObjectsAndKeys:
          uriDictionary, @"URIDictionary",
          url, @"URLString",
          kWebBookmarkTypeLeaf, kWebBookmarkType,
          idNum, kChromiumBookmarkId,
          metaInfoDictionary, kChromiumBookmarkMetaInfo,
          nil];
      [plist addObject:object];
    } else {
      NSString* title = base::SysUTF16ToNSString(element.title);
      NSArray* children = GetPlistForBookmarkList(element.children);
      int64_t elementId = element.id();
      NSNumber* idNum = [NSNumber numberWithLongLong:elementId];
      NSDictionary* object = [NSDictionary dictionaryWithObjectsAndKeys:
          title, @"Title",
          children, @"Children",
          kWebBookmarkTypeList, kWebBookmarkType,
          idNum, kChromiumBookmarkId,
          metaInfoDictionary, kChromiumBookmarkMetaInfo,
          nil];
      [plist addObject:object];
    }
  }
  return plist;
}

void WriteBookmarkDictionaryListPboardType(
    NSPasteboardItem* item,
    const std::vector<BookmarkNodeData::Element>& elements) {
  NSArray* plist = GetPlistForBookmarkList(elements);
  NSString* uti = ui::ClipboardUtil::UTIForPasteboardType(
      kBookmarkDictionaryListPboardType);
  [item setPropertyList:plist forType:uti];
}

void FillFlattenedArraysForBookmarks(
    const std::vector<BookmarkNodeData::Element>& elements,
    NSMutableArray* url_titles,
    NSMutableArray* urls,
    NSMutableArray* toplevel_string_data) {
  for (const BookmarkNodeData::Element& element : elements) {
    NSString* title = base::SysUTF16ToNSString(element.title);
    if (element.is_url) {
      NSString* url = base::SysUTF8ToNSString(element.url.spec());
      [url_titles addObject:title];
      [urls addObject:url];
      if (toplevel_string_data)
        [toplevel_string_data addObject:url];
    } else {
      if (toplevel_string_data)
        [toplevel_string_data addObject:title];
      FillFlattenedArraysForBookmarks(element.children, url_titles, urls, nil);
    }
  }
}

base::scoped_nsobject<NSPasteboardItem> WriteSimplifiedBookmarkTypes(
    const std::vector<BookmarkNodeData::Element>& elements) {
  NSMutableArray* url_titles = [NSMutableArray array];
  NSMutableArray* urls = [NSMutableArray array];
  NSMutableArray* toplevel_string_data = [NSMutableArray array];
  FillFlattenedArraysForBookmarks(
      elements, url_titles, urls, toplevel_string_data);

  base::scoped_nsobject<NSPasteboardItem> item;
  if ([urls count] > 0) {
    if ([urls count] == 1) {
      item = ui::ClipboardUtil::PasteboardItemFromUrl([urls firstObject],
                                                      [url_titles firstObject]);
    } else {
      item = ui::ClipboardUtil::PasteboardItemFromUrls(urls, url_titles);
    }
  }

  if (!item) {
    item.reset([[NSPasteboardItem alloc] init]);
  }

  [item setString:[toplevel_string_data componentsJoinedByString:@"\n"]
          forType:NSPasteboardTypeString];
  return item;
}

NSPasteboard* PasteboardFromType(ui::ClipboardType type) {
  NSString* type_string = nil;
  switch (type) {
    case ui::CLIPBOARD_TYPE_COPY_PASTE:
      type_string = NSGeneralPboard;
      break;
    case ui::CLIPBOARD_TYPE_DRAG:
      type_string = NSDragPboard;
      break;
    case ui::CLIPBOARD_TYPE_SELECTION:
      NOTREACHED();
      break;
  }

  return [NSPasteboard pasteboardWithName:type_string];
}

}  // namespace

NSPasteboardItem* PasteboardItemFromBookmarks(
    const std::vector<BookmarkNodeData::Element>& elements,
    const base::FilePath& profile_path) {
  base::scoped_nsobject<NSPasteboardItem> item =
      WriteSimplifiedBookmarkTypes(elements);

  WriteBookmarkDictionaryListPboardType(item, elements);

  NSString* uti =
      ui::ClipboardUtil::UTIForPasteboardType(kChromiumProfilePathPboardType);
  [item setString:base::SysUTF8ToNSString(profile_path.value()) forType:uti];
  return [[item retain] autorelease];
}

void WriteBookmarksToPasteboard(
    ui::ClipboardType type,
    const std::vector<BookmarkNodeData::Element>& elements,
    const base::FilePath& profile_path) {
  if (elements.empty())
    return;

  NSPasteboardItem* item = PasteboardItemFromBookmarks(elements, profile_path);
  NSPasteboard* pb = PasteboardFromType(type);
  [pb clearContents];
  [pb writeObjects:@[ item ]];
}

bool ReadBookmarksFromPasteboard(
    ui::ClipboardType type,
    std::vector<BookmarkNodeData::Element>& elements,
    base::FilePath* profile_path) {
  NSPasteboard* pb = PasteboardFromType(type);

  elements.clear();
  NSString* uti =
      ui::ClipboardUtil::UTIForPasteboardType(kChromiumProfilePathPboardType);
  NSString* profile = [pb stringForType:uti];
  *profile_path = base::FilePath(base::SysNSStringToUTF8(profile));
  return ReadBookmarkDictionaryListPboardType(pb, elements) ||
         ReadWebURLsWithTitlesPboardType(pb, elements);
}

bool PasteboardContainsBookmarks(ui::ClipboardType type) {
  NSPasteboard* pb = PasteboardFromType(type);

  NSArray* availableTypes = @[
    ui::ClipboardUtil::UTIForWebURLsAndTitles(),
    ui::ClipboardUtil::UTIForPasteboardType(kBookmarkDictionaryListPboardType)
  ];
  return [pb availableTypeFromArray:availableTypes] != nil;
}

}  // namespace bookmarks
