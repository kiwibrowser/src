// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/utility/importer/safari_importer.h"

#include <stddef.h>
#include <stdint.h>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/macros.h"
#include "base/path_service.h"
#include "base/strings/string16.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/importer/imported_bookmark_entry.h"
#include "chrome/common/importer/importer_bridge.h"
#include "chrome/common/importer/safari_importer_utils.h"
#include "chrome/utility/importer/safari_importer.h"
#include "components/favicon_base/favicon_usage_data.h"
#include "sql/connection.h"
#include "testing/platform_test.h"

using base::ASCIIToUTF16;

// In order to test the Safari import functionality effectively, we store a
// simulated Library directory containing dummy data files in the same
// structure as ~/Library in the Chrome test data directory.
// This function returns the path to that directory.
base::FilePath GetTestSafariLibraryPath(const std::string& suffix) {
  base::FilePath test_dir;
  base::PathService::Get(chrome::DIR_TEST_DATA, &test_dir);

  // Our simulated ~/Library directory
  return
      test_dir.AppendASCII("import").AppendASCII("safari").AppendASCII(suffix);
}

class SafariImporterTest : public PlatformTest {
 public:
  SafariImporter* GetSafariImporter() {
    return GetSafariImporterWithPathSuffix("default");
  }

  SafariImporter* GetSafariImporterWithPathSuffix(const std::string& suffix) {
    base::FilePath test_library_dir = GetTestSafariLibraryPath(suffix);
    CHECK(base::PathExists(test_library_dir));
    return new SafariImporter(test_library_dir);
  }
};

TEST_F(SafariImporterTest, HistoryImport) {
  scoped_refptr<SafariImporter> importer(GetSafariImporter());

  std::vector<ImporterURLRow> history_items;
  importer->ParseHistoryItems(&history_items);

  // Should be 2 history items.
  ASSERT_EQ(history_items.size(), 2U);

  ImporterURLRow& it1 = history_items[0];
  EXPECT_EQ(it1.url, GURL("http://www.firsthistoryitem.com/"));
  EXPECT_EQ(it1.title, base::UTF8ToUTF16("First History Item Title"));
  EXPECT_EQ(it1.visit_count, 1);
  EXPECT_EQ(it1.hidden, 0);
  EXPECT_EQ(it1.typed_count, 0);
  EXPECT_EQ(it1.last_visit.ToDoubleT(),
      importer->HistoryTimeToEpochTime(@"270598264.4"));

  ImporterURLRow& it2 = history_items[1];
  std::string second_item_title("http://www.secondhistoryitem.com/");
  EXPECT_EQ(it2.url, GURL(second_item_title));
  // The second item lacks a title so we expect the URL to be substituted.
  EXPECT_EQ(base::UTF16ToUTF8(it2.title), second_item_title.c_str());
  EXPECT_EQ(it2.visit_count, 55);
  EXPECT_EQ(it2.hidden, 0);
  EXPECT_EQ(it2.typed_count, 0);
  EXPECT_EQ(it2.last_visit.ToDoubleT(),
      importer->HistoryTimeToEpochTime(@"270598231.4"));
}

TEST_F(SafariImporterTest, BookmarkImport) {
  // Expected results
  const struct {
    bool in_toolbar;
    GURL url;
    // We store the path with levels of nesting delimited by forward slashes.
    base::string16 path;
    base::string16 title;
  } kImportedBookmarksData[] = {
    {
      true,
      GURL("http://www.apple.com/"),
      ASCIIToUTF16("Toolbar/"),
      ASCIIToUTF16("Apple")
    },
    {
      true,
      GURL("http://www.yahoo.com/"),
      ASCIIToUTF16("Toolbar/"),
      ASCIIToUTF16("Yahoo!")
    },
    {
      true,
      GURL("http://www.cnn.com/"),
      ASCIIToUTF16("Toolbar/News"),
      ASCIIToUTF16("CNN")
    },
    {
      true,
      GURL("http://www.nytimes.com/"),
      ASCIIToUTF16("Toolbar/News"),
      ASCIIToUTF16("The New York Times")
    },
    {
      false,
      GURL("http://www.reddit.com/"),
      base::string16(),
      ASCIIToUTF16("reddit.com: what's new online!")
    },
    {
      false,
      GURL(),
      base::string16(),
      ASCIIToUTF16("Empty Folder")
    },
    {
      false,
      GURL("http://www.webkit.org/blog/"),
      base::string16(),
      ASCIIToUTF16("Surfin' Safari - The WebKit Blog")
    },
  };

  scoped_refptr<SafariImporter> importer(GetSafariImporter());
  std::vector<ImportedBookmarkEntry> bookmarks;
  importer->ParseBookmarks(ASCIIToUTF16("Toolbar"), &bookmarks);
  size_t num_bookmarks = bookmarks.size();
  ASSERT_EQ(arraysize(kImportedBookmarksData), num_bookmarks);

  for (size_t i = 0; i < num_bookmarks; ++i) {
    ImportedBookmarkEntry& entry = bookmarks[i];
    EXPECT_EQ(kImportedBookmarksData[i].in_toolbar, entry.in_toolbar);
    EXPECT_EQ(kImportedBookmarksData[i].url, entry.url);

    std::vector<base::string16> path = base::SplitString(
        kImportedBookmarksData[i].path, ASCIIToUTF16("/"),
        base::KEEP_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
    ASSERT_EQ(path.size(), entry.path.size());
    for (size_t j = 0; j < path.size(); ++j) {
      EXPECT_EQ(path[j], entry.path[j]);
    }

    EXPECT_EQ(kImportedBookmarksData[i].title, entry.title);
  }
}

TEST_F(SafariImporterTest, BookmarkImportWithEmptyBookmarksMenu) {
  // Expected results.
  const struct {
    bool in_toolbar;
    GURL url;
    // We store the path with levels of nesting delimited by forward slashes.
    base::string16 path;
    base::string16 title;
  } kImportedBookmarksData[] = {
    {
      true,
      GURL("http://www.apple.com/"),
      ASCIIToUTF16("Toolbar/"),
      ASCIIToUTF16("Apple")
    },
    {
      true,
      GURL("http://www.yahoo.com/"),
      ASCIIToUTF16("Toolbar/"),
      ASCIIToUTF16("Yahoo!")
    },
    {
      true,
      GURL("http://www.cnn.com/"),
      ASCIIToUTF16("Toolbar/News"),
      ASCIIToUTF16("CNN")
    },
    {
      true,
      GURL("http://www.nytimes.com/"),
      ASCIIToUTF16("Toolbar/News"),
      ASCIIToUTF16("The New York Times")
    },
    {
      false,
      GURL("http://www.webkit.org/blog/"),
      base::string16(),
      ASCIIToUTF16("Surfin' Safari - The WebKit Blog")
    },
  };

  scoped_refptr<SafariImporter> importer(
      GetSafariImporterWithPathSuffix("empty_bookmarks_menu"));
  std::vector<ImportedBookmarkEntry> bookmarks;
  importer->ParseBookmarks(ASCIIToUTF16("Toolbar"), &bookmarks);
  size_t num_bookmarks = bookmarks.size();
  ASSERT_EQ(arraysize(kImportedBookmarksData), num_bookmarks);

  for (size_t i = 0; i < num_bookmarks; ++i) {
    ImportedBookmarkEntry& entry = bookmarks[i];
    EXPECT_EQ(kImportedBookmarksData[i].in_toolbar, entry.in_toolbar);
    EXPECT_EQ(kImportedBookmarksData[i].url, entry.url);

    std::vector<base::string16> path = base::SplitString(
        kImportedBookmarksData[i].path, ASCIIToUTF16("/"),
        base::KEEP_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
    ASSERT_EQ(path.size(), entry.path.size());
    for (size_t j = 0; j < path.size(); ++j) {
      EXPECT_EQ(path[j], entry.path[j]);
    }

    EXPECT_EQ(kImportedBookmarksData[i].title, entry.title);
  }
}

TEST_F(SafariImporterTest, FaviconImport) {
  scoped_refptr<SafariImporter> importer(GetSafariImporter());
  sql::Connection db;
  ASSERT_TRUE(importer->OpenDatabase(&db));

  SafariImporter::FaviconMap favicon_map;
  importer->ImportFaviconURLs(&db, &favicon_map);

  favicon_base::FaviconUsageDataList favicons;
  importer->LoadFaviconData(&db, favicon_map, &favicons);

  size_t num_favicons = favicons.size();
  ASSERT_EQ(num_favicons, 2U);

  favicon_base::FaviconUsageData& fav0 = favicons[0];
  EXPECT_EQ("http://s.ytimg.com/yt/favicon-vfl86270.ico",
            fav0.favicon_url.spec());
  EXPECT_GT(fav0.png_data.size(), 0U);
  EXPECT_EQ(fav0.urls.size(), 1U);
  EXPECT_TRUE(fav0.urls.find(GURL("http://www.youtube.com/"))
      != fav0.urls.end());

  favicon_base::FaviconUsageData& fav1 = favicons[1];
  EXPECT_EQ("http://www.opensearch.org/favicon.ico",
            fav1.favicon_url.spec());
  EXPECT_GT(fav1.png_data.size(), 0U);
  EXPECT_EQ(fav1.urls.size(), 2U);
  EXPECT_TRUE(fav1.urls.find(GURL("http://www.opensearch.org/Home"))
      != fav1.urls.end());

  EXPECT_TRUE(fav1.urls.find(
      GURL("http://www.opensearch.org/Special:Search?search=lalala&go=Search"))
          != fav1.urls.end());
}

TEST_F(SafariImporterTest, CanImport) {
  uint16_t items = importer::NONE;
  EXPECT_TRUE(SafariImporterCanImport(
      GetTestSafariLibraryPath("default"), &items));
  EXPECT_EQ(items, importer::HISTORY | importer::FAVORITES);
  EXPECT_EQ(items & importer::COOKIES, importer::NONE);
  EXPECT_EQ(items & importer::PASSWORDS, importer::NONE);
  EXPECT_EQ(items & importer::SEARCH_ENGINES, importer::NONE);
  EXPECT_EQ(items & importer::HOME_PAGE, importer::NONE);

  // Check that we don't import anything from a bogus library directory.
  base::ScopedTempDir fake_library_dir;
  ASSERT_TRUE(fake_library_dir.CreateUniqueTempDir());
  EXPECT_FALSE(SafariImporterCanImport(fake_library_dir.GetPath(), &items));
}
