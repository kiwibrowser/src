// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// The order of these includes is important.
#include <windows.h>
#include <unknwn.h>
#include <intshcut.h>
#include <objbase.h>
#include <propvarutil.h>
#include <shlguid.h>
#include <shlobj.h>
#include <stddef.h>
#include <stdint.h>
#include <urlhist.h>
#include <wrl/client.h>

#include <algorithm>
#include <vector>

#include "base/bind.h"
#include "base/compiler_specific.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "base/strings/string16.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_restrictions.h"
#include "base/win/registry.h"
#include "base/win/scoped_propvariant.h"
#include "base/win/windows_version.h"
#include "chrome/browser/importer/external_process_importer_host.h"
#include "chrome/browser/importer/importer_progress_observer.h"
#include "chrome/browser/importer/importer_unittest_utils.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/importer/ie_importer_utils_win.h"
#include "chrome/common/importer/imported_bookmark_entry.h"
#include "chrome/common/importer/importer_bridge.h"
#include "chrome/common/importer/importer_data_types.h"
#include "chrome/common/importer/importer_test_registry_overrider_win.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/testing_profile.h"
#include "components/autofill/core/common/password_form.h"
#include "components/favicon_base/favicon_usage_data.h"
#include "components/os_crypt/ie7_password_win.h"
#include "components/search_engines/template_url.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

const BookmarkInfo kIEBookmarks[] = {
  {true, 2, {"Links", "SubFolderOfLinks"},
    L"SubLink",
    "http://www.links-sublink.com/"},
  {true, 1, {"Links"},
    L"TheLink",
    "http://www.links-thelink.com/"},
  {false, 0, {},
    L"Google Home Page",
    "http://www.google.com/"},
  {false, 0, {},
    L"TheLink",
    "http://www.links-thelink.com/"},
  {false, 1, {"SubFolder"},
    L"Title",
    "http://www.link.com/"},
  {false, 0, {},
    L"WithPortAndQuery",
    "http://host:8080/cgi?q=query"},
  {false, 1, {"a"},
    L"\x4E2D\x6587",
    "http://chinese-title-favorite/"},
  {false, 0, {},
    L"SubFolder",
    "http://www.subfolder.com/"},
};

const BookmarkInfo kIESortedBookmarks[] = {
  {false, 0, {}, L"a", "http://www.google.com/0"},
  {false, 1, {"b"}, L"a", "http://www.google.com/1"},
  {false, 1, {"b"}, L"b", "http://www.google.com/2"},
  {false, 0, {}, L"c", "http://www.google.com/3"},
};

const base::char16 kIEIdentifyUrl[] =
    L"http://A79029D6-753E-4e27-B807-3D46AB1545DF.com:8080/path?key=value";
const base::char16 kIEIdentifyTitle[] =
    L"Unittest GUID";
const base::char16 kIECacheItemUrl[] =
    L"http://B2EF40C8-2569-4D7E-97EA-BAD9DF468D9C.com";
const base::char16 kIECacheItemTitle[] =
    L"Unittest Cache Item GUID";

const base::char16 kFaviconStreamSuffix[] = L"url:favicon:$DATA";
const char kDummyFaviconImageData[] =
    "\x42\x4D"          // Magic signature 'BM'
    "\x1E\x00\x00\x00"  // File size
    "\x00\x00\x00\x00"  // Reserved
    "\x1A\x00\x00\x00"  // Offset of the pixel data
    "\x0C\x00\x00\x00"  // Header Size
    "\x01\x00\x01\x00"  // Size: 1x1
    "\x01\x00"          // Reserved
    "\x18\x00"          // 24-bits
    "\x00\xFF\x00\x00"; // The pixel

struct FaviconGroup {
  const base::char16* favicon_url;
  const base::char16* site_url[2];
};

const FaviconGroup kIEFaviconGroup[2] = {
  {L"http://www.google.com/favicon.ico",
    {L"http://www.google.com/",
      L"http://www.subfolder.com/"}},
  {L"http://example.com/favicon.ico",
    {L"http://host:8080/cgi?q=query",
      L"http://chinese-title-favorite/"}},
};

bool CreateOrderBlob(const base::FilePath& favorites_folder,
                     const base::string16& path,
                     const std::vector<base::string16>& entries) {
  if (entries.size() > 255)
    return false;

  // Create a binary sequence for setting a specific order of favorites.
  // The format depends on the version of Shell32.dll, so we cannot embed
  // a binary constant here.
  std::vector<uint8_t> blob(20, 0);
  blob[16] = static_cast<uint8_t>(entries.size());

  for (size_t i = 0; i < entries.size(); ++i) {
    PIDLIST_ABSOLUTE id_list_full = ILCreateFromPath(
        favorites_folder.Append(path).Append(entries[i]).value().c_str());
    PUITEMID_CHILD id_list = ILFindLastID(id_list_full);
    // Include the trailing zero-length item id.  Don't include the single
    // element array.
    size_t id_list_size = id_list->mkid.cb + sizeof(id_list->mkid.cb);

    blob.resize(blob.size() + 8);
    uint32_t total_size = id_list_size + 8;
    memcpy(&blob[blob.size() - 8], &total_size, 4);
    uint32_t sort_index = i;
    memcpy(&blob[blob.size() - 4], &sort_index, 4);
    blob.resize(blob.size() + id_list_size);
    memcpy(&blob[blob.size() - id_list_size], id_list, id_list_size);
    ILFree(id_list_full);
  }

  base::string16 key_path(importer::GetIEFavoritesOrderKey());
  if (!path.empty())
    key_path += L"\\" + path;
  base::win::RegKey key;
  if (key.Create(HKEY_CURRENT_USER, key_path.c_str(), KEY_WRITE) !=
      ERROR_SUCCESS) {
    return false;
  }
  if (key.WriteValue(L"Order", &blob[0], blob.size(), REG_BINARY) !=
      ERROR_SUCCESS) {
    return false;
  }
  return true;
}

bool CreateUrlFileWithFavicon(const base::FilePath& file,
                              const base::string16& url,
                              const base::string16& favicon_url) {
  Microsoft::WRL::ComPtr<IUniformResourceLocator> locator;
  HRESULT result =
      ::CoCreateInstance(CLSID_InternetShortcut, NULL, CLSCTX_INPROC_SERVER,
                         IID_PPV_ARGS(&locator));
  if (FAILED(result))
    return false;
  Microsoft::WRL::ComPtr<IPersistFile> persist_file;
  result = locator.CopyTo(persist_file.GetAddressOf());
  if (FAILED(result))
    return false;
  result = locator->SetURL(url.c_str(), 0);
  if (FAILED(result))
    return false;

  // Write favicon url if specified.
  if (!favicon_url.empty()) {
    Microsoft::WRL::ComPtr<IPropertySetStorage> property_set_storage;
    if (FAILED(locator.CopyTo(property_set_storage.GetAddressOf())))
      return false;
    Microsoft::WRL::ComPtr<IPropertyStorage> property_storage;
    if (FAILED(property_set_storage->Open(FMTID_Intshcut, STGM_WRITE,
                                          property_storage.GetAddressOf()))) {
      return false;
    }
    PROPSPEC properties[] = {{PRSPEC_PROPID, {PID_IS_ICONFILE}}};
    // WriteMultiple takes an array of PROPVARIANTs, but since this code only
    // needs an array of size 1: a pointer to |pv_icon| is equivalent.
    base::win::ScopedPropVariant pv_icon;
    if (FAILED(InitPropVariantFromString(favicon_url.c_str(),
                                         pv_icon.Receive())) ||
        FAILED(
            property_storage->WriteMultiple(1, properties, pv_icon.ptr(), 0))) {
      return false;
    }
  }

  // Save the .url file.
  result = persist_file->Save(file.value().c_str(), TRUE);
  if (FAILED(result))
    return false;

  // Write dummy favicon image data in NTFS alternate data stream.
  return favicon_url.empty() || (base::WriteFile(
      file.ReplaceExtension(kFaviconStreamSuffix), kDummyFaviconImageData,
      sizeof kDummyFaviconImageData) != -1);
}

bool CreateUrlFile(const base::FilePath& file, const base::string16& url) {
  return CreateUrlFileWithFavicon(file, url, base::string16());
}

class TestObserver : public ProfileWriter,
                     public importer::ImporterProgressObserver {
 public:
  enum TestIEVersion {
    IE6,
    IE7,
  };

  explicit TestObserver(uint16_t importer_items, TestIEVersion ie_version)
      : ProfileWriter(NULL),
        bookmark_count_(0),
        history_count_(0),
        password_count_(0),
        favicon_count_(0),
        homepage_count_(0),
        ie7_password_count_(0),
        importer_items_(importer_items),
        ie_version_(ie_version) {}

  // importer::ImporterProgressObserver:
  void ImportStarted() override {}
  void ImportItemStarted(importer::ImportItem item) override {}
  void ImportItemEnded(importer::ImportItem item) override {}
  void ImportEnded() override {
    base::RunLoop::QuitCurrentWhenIdleDeprecated();
    if (importer_items_ & importer::FAVORITES) {
      EXPECT_EQ(arraysize(kIEBookmarks), bookmark_count_);
      EXPECT_EQ(arraysize(kIEFaviconGroup), favicon_count_);
    }
    if (importer_items_ & importer::HISTORY)
      EXPECT_EQ(2u, history_count_);
    if (importer_items_ & importer::HOME_PAGE)
      EXPECT_EQ(1u, homepage_count_);
    if ((importer_items_ & importer::PASSWORDS) && (ie_version_ == IE7))
      EXPECT_EQ(1u, ie7_password_count_);
    // We need to test the IE6 password importer code.
    // https://crbug.com/257100
    // EXPECT_EQ(1, password_count_);
  }

  // ProfileWriter:
  bool BookmarkModelIsLoaded() const override {
    // Profile is ready for writing.
    return true;
  }

  bool TemplateURLServiceIsLoaded() const override {
    return true;
  }

  void AddPasswordForm(const autofill::PasswordForm& form) override {
    // Importer should obtain this password form only.
    EXPECT_EQ(GURL("http://localhost:8080/security/index.htm"), form.origin);
    EXPECT_EQ("http://localhost:8080/", form.signon_realm);
    EXPECT_EQ(L"user", form.username_element);
    EXPECT_EQ(L"1", form.username_value);
    EXPECT_EQ(L"", form.password_element);
    EXPECT_EQ(L"2", form.password_value);
    EXPECT_EQ("", form.action.spec());
    ++password_count_;
  }

  void AddHistoryPage(const history::URLRows& page,
                      history::VisitSource visit_source) override {
    bool cache_item_found = false;
    bool history_item_found = false;
    // Importer should read the specified URL.
    for (size_t i = 0; i < page.size(); ++i) {
      if (page[i].title() == kIEIdentifyTitle &&
          page[i].url() == GURL(kIEIdentifyUrl)) {
        EXPECT_FALSE(page[i].hidden());
        history_item_found = true;
        ++history_count_;
      }
      if (page[i].title() == kIECacheItemTitle &&
          page[i].url() == GURL(kIECacheItemUrl)) {
        EXPECT_TRUE(page[i].hidden());
        cache_item_found = true;
        ++history_count_;
      }
    }
    EXPECT_TRUE(history_item_found);
    EXPECT_TRUE(cache_item_found);
    EXPECT_EQ(history::SOURCE_IE_IMPORTED, visit_source);
  }

  void AddBookmarks(const std::vector<ImportedBookmarkEntry>& bookmarks,
                    const base::string16& top_level_folder_name) override {
    ASSERT_LE(bookmark_count_ + bookmarks.size(), arraysize(kIEBookmarks));
    // Importer should import the IE Favorites folder the same as the list,
    // in the same order.
    for (size_t i = 0; i < bookmarks.size(); ++i) {
      EXPECT_NO_FATAL_FAILURE(
          TestEqualBookmarkEntry(bookmarks[i],
                                 kIEBookmarks[bookmark_count_])) << i;
      ++bookmark_count_;
    }
  }

  void AddFavicons(const favicon_base::FaviconUsageDataList& usage) override {
    // Importer should group the favicon information for each favicon URL.
    for (size_t i = 0; i < arraysize(kIEFaviconGroup); ++i) {
      GURL favicon_url(kIEFaviconGroup[i].favicon_url);
      std::set<GURL> urls;
      for (size_t j = 0; j < arraysize(kIEFaviconGroup[i].site_url); ++j)
        urls.insert(GURL(kIEFaviconGroup[i].site_url[j]));

      SCOPED_TRACE(testing::Message() << "Expected Favicon: " << favicon_url);

      bool expected_favicon_url_found = false;
      for (size_t j = 0; j < usage.size(); ++j) {
        if (usage[j].favicon_url == favicon_url) {
          EXPECT_EQ(urls, usage[j].urls);
          expected_favicon_url_found = true;
          break;
        }
      }
      EXPECT_TRUE(expected_favicon_url_found);
    }

    favicon_count_ += usage.size();
  }

  void AddIE7PasswordInfo(const IE7PasswordInfo& info) override {
    // This function also gets called for the IEImporter test. Ignore.
    if (ie_version_ == IE7) {
      EXPECT_EQ(L"Test1", info.url_hash);
      EXPECT_EQ(1, info.encrypted_data[0]);
      EXPECT_EQ(4u, info.encrypted_data.size());
      ++ie7_password_count_;
    }
  }

  void AddHomepage(const GURL& homepage) override {
    EXPECT_EQ(homepage.spec(), "http://www.test.com/");
    ++homepage_count_;
  }

 private:
  ~TestObserver() override {}

  size_t bookmark_count_;
  size_t history_count_;
  size_t password_count_;
  size_t favicon_count_;
  size_t homepage_count_;
  size_t ie7_password_count_;
  uint16_t importer_items_;
  TestIEVersion ie_version_;
};

class MalformedFavoritesRegistryTestObserver
    : public ProfileWriter,
      public importer::ImporterProgressObserver {
 public:
  MalformedFavoritesRegistryTestObserver() : ProfileWriter(NULL) {
    bookmark_count_ = 0;
  }

  // importer::ImporterProgressObserver:
  void ImportStarted() override {}
  void ImportItemStarted(importer::ImportItem item) override {}
  void ImportItemEnded(importer::ImportItem item) override {}
  void ImportEnded() override {
    base::RunLoop::QuitCurrentWhenIdleDeprecated();
    EXPECT_EQ(arraysize(kIESortedBookmarks), bookmark_count_);
  }

  // ProfileWriter:
  bool BookmarkModelIsLoaded() const override { return true; }
  bool TemplateURLServiceIsLoaded() const override { return true; }

  void AddPasswordForm(const autofill::PasswordForm& form) override {}
  void AddHistoryPage(const history::URLRows& page,
                      history::VisitSource visit_source) override {}
  void AddKeywords(TemplateURLService::OwnedTemplateURLVector template_urls,
                   bool unique_on_host_and_path) override {}
  void AddBookmarks(const std::vector<ImportedBookmarkEntry>& bookmarks,
                    const base::string16& top_level_folder_name) override {
    ASSERT_LE(bookmark_count_ + bookmarks.size(),
              arraysize(kIESortedBookmarks));
    for (size_t i = 0; i < bookmarks.size(); ++i) {
      EXPECT_NO_FATAL_FAILURE(
          TestEqualBookmarkEntry(bookmarks[i],
                                 kIESortedBookmarks[bookmark_count_])) << i;
      ++bookmark_count_;
    }
  }

 private:
  ~MalformedFavoritesRegistryTestObserver() override {}

  size_t bookmark_count_;
};

}  // namespace

// These tests need to be browser tests in order to be able to run the OOP
// import (via ExternalProcessImporterHost) which launches a utility process.
class IEImporterBrowserTest : public InProcessBrowserTest {
 protected:
  void SetUp() override {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());

    // This will launch the browser test and thus needs to happen last.
    InProcessBrowserTest::SetUp();
  }

  base::ScopedTempDir temp_dir_;

  // Overrides the default registry key for IE registry keys like favorites,
  // settings, password store, etc.
  ImporterTestRegistryOverrider test_registry_overrider_;
};

IN_PROC_BROWSER_TEST_F(IEImporterBrowserTest, IEImporter) {
  base::ScopedAllowBlockingForTesting allow_blocking;
  // Sets up a favorites folder.
  base::FilePath path = temp_dir_.GetPath().AppendASCII("Favorites");
  CreateDirectory(path.value().c_str(), NULL);
  CreateDirectory(path.AppendASCII("SubFolder").value().c_str(), NULL);
  base::FilePath links_path = path.AppendASCII("Links");
  CreateDirectory(links_path.value().c_str(), NULL);
  CreateDirectory(links_path.AppendASCII("SubFolderOfLinks").value().c_str(),
                  NULL);
  CreateDirectory(path.AppendASCII("\x0061").value().c_str(), NULL);
  ASSERT_TRUE(CreateUrlFileWithFavicon(path.AppendASCII("Google Home Page.url"),
                                       L"http://www.google.com/",
                                       L"http://www.google.com/favicon.ico"));
  ASSERT_TRUE(CreateUrlFile(path.AppendASCII("SubFolder\\Title.url"),
                            L"http://www.link.com/"));
  ASSERT_TRUE(CreateUrlFileWithFavicon(path.AppendASCII("SubFolder.url"),
                                       L"http://www.subfolder.com/",
                                       L"http://www.google.com/favicon.ico"));
  ASSERT_TRUE(CreateUrlFile(path.AppendASCII("TheLink.url"),
                            L"http://www.links-thelink.com/"));
  ASSERT_TRUE(CreateUrlFileWithFavicon(path.AppendASCII("WithPortAndQuery.url"),
                                       L"http://host:8080/cgi?q=query",
                                       L"http://example.com/favicon.ico"));
  ASSERT_TRUE(CreateUrlFileWithFavicon(
      path.AppendASCII("\x0061").Append(L"\x4E2D\x6587.url"),
      L"http://chinese-title-favorite/",
      L"http://example.com/favicon.ico"));
  ASSERT_TRUE(CreateUrlFile(links_path.AppendASCII("TheLink.url"),
                            L"http://www.links-thelink.com/"));
  ASSERT_TRUE(CreateUrlFile(
      links_path.AppendASCII("SubFolderOfLinks").AppendASCII("SubLink.url"),
      L"http://www.links-sublink.com/"));
  ASSERT_TRUE(CreateUrlFile(path.AppendASCII("IEDefaultLink.url"),
                            L"http://go.microsoft.com/fwlink/?linkid=140813"));
  base::WriteFile(path.AppendASCII("InvalidUrlFile.url"), "x", 1);
  base::WriteFile(path.AppendASCII("PlainTextFile.txt"), "x", 1);

  const base::char16* root_links[] = {
    L"Links",
    L"Google Home Page.url",
    L"TheLink.url",
    L"SubFolder",
    L"WithPortAndQuery.url",
    L"a",
    L"SubFolder.url",
  };
  ASSERT_TRUE(CreateOrderBlob(
      base::FilePath(path), L"",
      std::vector<base::string16>(root_links,
                                  root_links + arraysize(root_links))));

  // Sets up a special history link.
  Microsoft::WRL::ComPtr<IUrlHistoryStg2> url_history_stg2;
  ASSERT_EQ(S_OK,
            ::CoCreateInstance(CLSID_CUrlHistory, NULL, CLSCTX_INPROC_SERVER,
                               IID_PPV_ARGS(&url_history_stg2)));
  // Usage of ADDURL_ADDTOHISTORYANDCACHE and ADDURL_ADDTOCACHE flags
  // is explained in the article:
  // http://msdn.microsoft.com/ru-ru/aa767730
  ASSERT_EQ(S_OK, url_history_stg2->AddUrl(kIEIdentifyUrl, kIEIdentifyTitle,
                                           ADDURL_ADDTOHISTORYANDCACHE));
  ASSERT_EQ(S_OK, url_history_stg2->AddUrl(kIECacheItemUrl, kIECacheItemTitle,
                                           ADDURL_ADDTOCACHE));

  // Starts to import the above settings.
  // Deletes itself.
  ExternalProcessImporterHost* host = new ExternalProcessImporterHost;
  TestObserver* observer = new TestObserver(
      importer::HISTORY | importer::PASSWORDS | importer::FAVORITES,
      TestObserver::IE6);
  host->set_observer(observer);

  importer::SourceProfile source_profile;
  source_profile.importer_type = importer::TYPE_IE;
  source_profile.source_path = temp_dir_.GetPath();

  host->StartImportSettings(
      source_profile,
      browser()->profile(),
      importer::HISTORY | importer::PASSWORDS | importer::FAVORITES,
      observer);
  base::RunLoop().Run();

  // Cleans up.
  url_history_stg2->DeleteUrl(kIEIdentifyUrl, 0);
  url_history_stg2->DeleteUrl(kIECacheItemUrl, 0);
  url_history_stg2.Reset();
}

IN_PROC_BROWSER_TEST_F(IEImporterBrowserTest,
                       IEImporterMalformedFavoritesRegistry) {
  // Sets up a favorites folder.
  base::FilePath path = temp_dir_.GetPath().AppendASCII("Favorites");
  CreateDirectory(path.value().c_str(), NULL);
  CreateDirectory(path.AppendASCII("b").value().c_str(), NULL);
  ASSERT_TRUE(CreateUrlFile(path.AppendASCII("a.url"),
                            L"http://www.google.com/0"));
  ASSERT_TRUE(CreateUrlFile(path.AppendASCII("b").AppendASCII("a.url"),
                            L"http://www.google.com/1"));
  ASSERT_TRUE(CreateUrlFile(path.AppendASCII("b").AppendASCII("b.url"),
                            L"http://www.google.com/2"));
  ASSERT_TRUE(CreateUrlFile(path.AppendASCII("c.url"),
                            L"http://www.google.com/3"));

  struct BadBinaryData {
    const char* data;
    int length;
  };
  static const BadBinaryData kBadBinary[] = {
    // number_of_items field is truncated
    {"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
     "\x00\xff\xff\xff", 17},
    // number_of_items = 0xffff, but the byte sequence is too short.
    {"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
     "\xff\xff\x00\x00", 20},
    // number_of_items = 1, size_of_item is too big.
    {"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
     "\x01\x00\x00\x00"
     "\xff\xff\x00\x00\x00\x00\x00\x00"
     "\x00\x00\x00\x00", 32},
    // number_of_items = 1, size_of_item = 16, size_of_shid is too big.
    {"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
     "\x01\x00\x00\x00"
     "\x10\x00\x00\x00\x00\x00\x00\x00"
     "\xff\x7f\x00\x00" "\x00\x00\x00\x00", 36},
    // number_of_items = 1, size_of_item = 16, size_of_shid is too big.
    {"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
     "\x01\x00\x00\x00"
     "\x10\x00\x00\x00\x00\x00\x00\x00"
     "\x06\x00\x00\x00" "\x00\x00\x00\x00", 36},
  };

  // Verify malformed registry data are safely ignored and alphabetical
  // sort is performed.
  for (size_t i = 0; i < arraysize(kBadBinary); ++i) {
    base::string16 key_path(importer::GetIEFavoritesOrderKey());
    base::win::RegKey key;
    ASSERT_EQ(ERROR_SUCCESS,
              key.Create(HKEY_CURRENT_USER, key_path.c_str(), KEY_WRITE));
    ASSERT_EQ(ERROR_SUCCESS,
              key.WriteValue(L"Order", kBadBinary[i].data, kBadBinary[i].length,
                             REG_BINARY));

    // Starts to import the above settings.
    // Deletes itself.
    ExternalProcessImporterHost* host = new ExternalProcessImporterHost;
    MalformedFavoritesRegistryTestObserver* observer =
        new MalformedFavoritesRegistryTestObserver();
    host->set_observer(observer);

    importer::SourceProfile source_profile;
    source_profile.importer_type = importer::TYPE_IE;
    source_profile.source_path = temp_dir_.GetPath();

    host->StartImportSettings(
        source_profile,
        browser()->profile(),
        importer::FAVORITES,
        observer);
    base::RunLoop().Run();
  }
}

IN_PROC_BROWSER_TEST_F(IEImporterBrowserTest, IE7ImporterPasswordsTest) {
  // Starts to import the IE7 passwords.
  // Deletes itself.
  ExternalProcessImporterHost* host = new ExternalProcessImporterHost;
  TestObserver* observer = new TestObserver(importer::PASSWORDS,
                                            TestObserver::IE7);
  host->set_observer(observer);

  base::string16 key_path(importer::GetIE7PasswordsKey());
  base::win::RegKey key;
  ASSERT_EQ(ERROR_SUCCESS,
            key.Create(HKEY_CURRENT_USER, key_path.c_str(), KEY_WRITE));
  key.WriteValue(L"Test1", 1);

  importer::SourceProfile source_profile;
  source_profile.importer_type = importer::TYPE_IE;
  source_profile.source_path = temp_dir_.GetPath();

  host->StartImportSettings(
      source_profile,
      browser()->profile(),
      importer::PASSWORDS,
      observer);
  base::RunLoop().Run();
}

IN_PROC_BROWSER_TEST_F(IEImporterBrowserTest, IEImporterHomePageTest) {
  // Starts to import the IE home page.
  // Deletes itself.
  ExternalProcessImporterHost* host = new ExternalProcessImporterHost;
  TestObserver* observer = new TestObserver(importer::HOME_PAGE,
                                            TestObserver::IE6);
  host->set_observer(observer);

  base::string16 key_path(importer::GetIESettingsKey());
  base::win::RegKey key;
  ASSERT_EQ(ERROR_SUCCESS,
            key.Create(HKEY_CURRENT_USER, key_path.c_str(), KEY_WRITE));
  key.WriteValue(L"Start Page", L"http://www.test.com/");

  importer::SourceProfile source_profile;
  source_profile.importer_type = importer::TYPE_IE;
  source_profile.source_path = temp_dir_.GetPath();

  host->StartImportSettings(
      source_profile,
      browser()->profile(),
      importer::HOME_PAGE,
      observer);
  base::RunLoop().Run();
}
