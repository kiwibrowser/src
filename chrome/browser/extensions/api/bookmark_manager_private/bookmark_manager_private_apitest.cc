// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/bookmark_manager_private/bookmark_manager_private_api.h"

#include <memory>
#include <utility>

#include "base/command_line.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "chrome/browser/bookmarks/bookmark_model_factory.h"
#include "chrome/browser/bookmarks/managed_bookmark_service_factory.h"
#include "chrome/browser/extensions/extension_apitest.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/common/pref_names.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/common/bookmark_pref_names.h"
#include "components/bookmarks/managed/managed_bookmark_service.h"
#include "components/bookmarks/test/bookmark_test_helpers.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"

using bookmarks::BookmarkModel;
using bookmarks::BookmarkNode;

namespace extensions {

IN_PROC_BROWSER_TEST_F(ExtensionApiTest, BookmarkManager) {
  // Add managed bookmarks.
  Profile* profile = browser()->profile();
  BookmarkModel* model = BookmarkModelFactory::GetForBrowserContext(profile);
  bookmarks::ManagedBookmarkService* managed =
      ManagedBookmarkServiceFactory::GetForProfile(profile);
  bookmarks::test::WaitForBookmarkModelToLoad(model);

  base::ListValue list;
  std::unique_ptr<base::DictionaryValue> node(new base::DictionaryValue());
  node->SetString("name", "Managed Bookmark");
  node->SetString("url", "http://www.chromium.org");
  list.Append(std::move(node));
  node.reset(new base::DictionaryValue());
  node->SetString("name", "Managed Folder");
  node->Set("children", std::make_unique<base::ListValue>());
  list.Append(std::move(node));
  profile->GetPrefs()->Set(bookmarks::prefs::kManagedBookmarks, list);
  ASSERT_EQ(2, managed->managed_node()->child_count());

  ASSERT_TRUE(RunComponentExtensionTest("bookmark_manager/standard"))
      << message_;
}

IN_PROC_BROWSER_TEST_F(ExtensionApiTest, BookmarkManagerEditDisabled) {
  Profile* profile = browser()->profile();

  // Provide some testing data here, since bookmark editing will be disabled
  // within the extension.
  BookmarkModel* model = BookmarkModelFactory::GetForBrowserContext(profile);
  bookmarks::test::WaitForBookmarkModelToLoad(model);
  const BookmarkNode* bar = model->bookmark_bar_node();
  const BookmarkNode* folder =
      model->AddFolder(bar, 0, base::ASCIIToUTF16("Folder"));
  model->AddURL(bar, 1, base::ASCIIToUTF16("AAA"),
                GURL("http://aaa.example.com"));
  model->AddURL(folder, 0, base::ASCIIToUTF16("BBB"),
                GURL("http://bbb.example.com"));

  PrefService* prefs = user_prefs::UserPrefs::Get(profile);
  prefs->SetBoolean(bookmarks::prefs::kEditBookmarksEnabled, false);

  ASSERT_TRUE(RunComponentExtensionTest("bookmark_manager/edit_disabled"))
      << message_;
}

}  // namespace extensions
