// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/drive/chromeos/search_metadata.h"

#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <vector>

#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/i18n/string_search.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/drive/chromeos/drive_test_util.h"
#include "components/drive/chromeos/fake_free_disk_space_getter.h"
#include "components/drive/chromeos/file_cache.h"
#include "components/drive/drive_api_util.h"
#include "components/drive/file_system_core_util.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace drive {
namespace internal {

namespace {

const int kDefaultAtMostNumMatches = 10;

// A simple wrapper for testing FindAndHighlightWrapper(). It just converts the
// query text parameter to FixedPatternStringSearchIgnoringCaseAndAccents.
bool FindAndHighlightWrapper(
    const std::string& text,
    const std::string& query_text,
    std::string* highlighted_text) {
  std::vector<std::unique_ptr<
      base::i18n::FixedPatternStringSearchIgnoringCaseAndAccents>>
      queries;
  queries.push_back(std::make_unique<
                    base::i18n::FixedPatternStringSearchIgnoringCaseAndAccents>(
      base::UTF8ToUTF16(query_text)));
  return FindAndHighlight(text, queries, highlighted_text);
}

}  // namespace

class SearchMetadataTest : public testing::Test {
 protected:
  void SetUp() override {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    fake_free_disk_space_getter_.reset(new FakeFreeDiskSpaceGetter);

    metadata_storage_.reset(new ResourceMetadataStorage(
        temp_dir_.GetPath(), base::ThreadTaskRunnerHandle::Get().get()));
    ASSERT_TRUE(metadata_storage_->Initialize());

    cache_.reset(new FileCache(metadata_storage_.get(), temp_dir_.GetPath(),
                               base::ThreadTaskRunnerHandle::Get().get(),
                               fake_free_disk_space_getter_.get()));
    ASSERT_TRUE(cache_->Initialize());

    resource_metadata_.reset(
        new ResourceMetadata(metadata_storage_.get(),
                             cache_.get(),
                             base::ThreadTaskRunnerHandle::Get().get()));
    ASSERT_EQ(FILE_ERROR_OK, resource_metadata_->Initialize());

    AddEntriesToMetadata();
  }

  void AddEntriesToMetadata() {
    base::FilePath temp_file;
    EXPECT_TRUE(
        base::CreateTemporaryFileInDir(temp_dir_.GetPath(), &temp_file));
    const std::string temp_file_md5 = "md5";

    ResourceEntry entry;
    std::string local_id;

    // drive/root
    EXPECT_EQ(FILE_ERROR_OK, resource_metadata_->GetIdByPath(
        util::GetDriveMyDriveRootPath(), &local_id));
    const std::string root_local_id = local_id;

    // drive/root/Directory-1
    EXPECT_EQ(FILE_ERROR_OK,
              resource_metadata_->AddEntry(
                  GetDirectoryEntry("Directory-1", "dir1", 1, 1, root_local_id),
                  &local_id));
    const std::string dir1_local_id = local_id;

    // drive/root/Directory-1/SubDirectory File 1.txt
    EXPECT_EQ(FILE_ERROR_OK, resource_metadata_->AddEntry(
                                 GetFileEntry("SubDirectory File 1.txt",
                                              "file1a", 2, 99, dir1_local_id),
                                 &local_id));
    EXPECT_EQ(FILE_ERROR_OK, cache_->Store(
        local_id, temp_file_md5, temp_file, FileCache::FILE_OPERATION_COPY));

    // drive/root/Directory-1/Shared To The Account Owner.txt
    entry = GetFileEntry("Shared To The Account Owner.txt", "file1b", 3, 3,
                         dir1_local_id);
    entry.set_shared_with_me(true);
    EXPECT_EQ(FILE_ERROR_OK, resource_metadata_->AddEntry(entry, &local_id));

    // drive/root/Directory 2 excludeDir-test
    EXPECT_EQ(FILE_ERROR_OK,
              resource_metadata_->AddEntry(
                  GetDirectoryEntry("Directory 2 excludeDir-test", "dir2", 4, 4,
                                    root_local_id),
                  &local_id));

    // drive/root/Slash \xE2\x88\x95 in directory
    EXPECT_EQ(FILE_ERROR_OK,
              resource_metadata_->AddEntry(
                  GetDirectoryEntry("Slash \xE2\x88\x95 in directory", "dir3",
                                    5, 5, root_local_id),
                  &local_id));
    const std::string dir3_local_id = local_id;

    // drive/root/Slash \xE2\x88\x95 in directory/Slash SubDir File.txt
    EXPECT_EQ(FILE_ERROR_OK, resource_metadata_->AddEntry(
                                 GetFileEntry("Slash SubDir File.txt", "file3a",
                                              6, 6, dir3_local_id),
                                 &local_id));

    // drive/root/File 2.txt
    EXPECT_EQ(FILE_ERROR_OK,
              resource_metadata_->AddEntry(
                  GetFileEntry("File 2.txt", "file2", 7, 7, root_local_id),
                  &local_id));
    EXPECT_EQ(FILE_ERROR_OK, cache_->Store(
        local_id, temp_file_md5, temp_file, FileCache::FILE_OPERATION_COPY));

    // drive/root/Document 1 excludeDir-test
    entry =
        GetFileEntry("Document 1 excludeDir-test", "doc1", 8, 8, root_local_id);
    entry.mutable_file_specific_info()->set_is_hosted_document(true);
    entry.mutable_file_specific_info()->set_document_extension(".gdoc");
    entry.mutable_file_specific_info()->set_content_mime_type(
        drive::util::kGoogleDocumentMimeType);
    EXPECT_EQ(FILE_ERROR_OK, resource_metadata_->AddEntry(entry, &local_id));
  }

  ResourceEntry GetFileEntry(const std::string& name,
                             const std::string& resource_id,
                             int64_t last_accessed,
                             int64_t last_modified,
                             const std::string& parent_local_id) {
    ResourceEntry entry;
    entry.set_title(name);
    entry.set_resource_id(resource_id);
    entry.set_parent_local_id(parent_local_id);
    entry.mutable_file_info()->set_last_accessed(last_accessed);
    entry.mutable_file_info()->set_last_modified(last_modified);
    return entry;
  }

  ResourceEntry GetDirectoryEntry(const std::string& name,
                                  const std::string& resource_id,
                                  int64_t last_accessed,
                                  int64_t last_modified,
                                  const std::string& parent_local_id) {
    ResourceEntry entry;
    entry.set_title(name);
    entry.set_resource_id(resource_id);
    entry.set_parent_local_id(parent_local_id);
    entry.mutable_file_info()->set_last_accessed(last_accessed);
    entry.mutable_file_info()->set_last_modified(last_modified);
    entry.mutable_file_info()->set_is_directory(true);
    return entry;
  }

  content::TestBrowserThreadBundle thread_bundle_;
  base::ScopedTempDir temp_dir_;
  std::unique_ptr<FakeFreeDiskSpaceGetter> fake_free_disk_space_getter_;
  std::unique_ptr<ResourceMetadataStorage, test_util::DestroyHelperForTests>
      metadata_storage_;
  std::unique_ptr<ResourceMetadata, test_util::DestroyHelperForTests>
      resource_metadata_;
  std::unique_ptr<FileCache, test_util::DestroyHelperForTests> cache_;
};

TEST_F(SearchMetadataTest, SearchMetadata_ZeroMatches) {
  FileError error = FILE_ERROR_FAILED;
  std::unique_ptr<MetadataSearchResultVector> result;

  SearchMetadata(
      base::ThreadTaskRunnerHandle::Get(), resource_metadata_.get(),
      "NonExistent", base::Bind(&MatchesType, SEARCH_METADATA_ALL),
      kDefaultAtMostNumMatches, MetadataSearchOrder::LAST_ACCESSED,
      google_apis::test_util::CreateCopyResultCallback(&error, &result));
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(FILE_ERROR_OK, error);
  ASSERT_TRUE(result);
  ASSERT_EQ(0U, result->size());
}

TEST_F(SearchMetadataTest, SearchMetadata_RegularFile) {
  FileError error = FILE_ERROR_FAILED;
  std::unique_ptr<MetadataSearchResultVector> result;

  SearchMetadata(
      base::ThreadTaskRunnerHandle::Get(), resource_metadata_.get(),
      "SubDirectory File 1.txt", base::Bind(&MatchesType, SEARCH_METADATA_ALL),
      kDefaultAtMostNumMatches, MetadataSearchOrder::LAST_ACCESSED,
      google_apis::test_util::CreateCopyResultCallback(&error, &result));
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(FILE_ERROR_OK, error);
  ASSERT_TRUE(result);
  ASSERT_EQ(1U, result->size());
  EXPECT_EQ("drive/root/Directory-1/SubDirectory File 1.txt",
            result->at(0).path.AsUTF8Unsafe());
}

// This test checks if |FindAndHighlightWrapper| does case-insensitive search.
// Tricker test cases for |FindAndHighlightWrapper| can be found below.
TEST_F(SearchMetadataTest, SearchMetadata_CaseInsensitiveSearch) {
  FileError error = FILE_ERROR_FAILED;
  std::unique_ptr<MetadataSearchResultVector> result;

  // The query is all in lower case.
  SearchMetadata(
      base::ThreadTaskRunnerHandle::Get(), resource_metadata_.get(),
      "subdirectory file 1.txt", base::Bind(&MatchesType, SEARCH_METADATA_ALL),
      kDefaultAtMostNumMatches, MetadataSearchOrder::LAST_ACCESSED,
      google_apis::test_util::CreateCopyResultCallback(&error, &result));
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(FILE_ERROR_OK, error);
  ASSERT_TRUE(result);
  ASSERT_EQ(1U, result->size());
  EXPECT_EQ("drive/root/Directory-1/SubDirectory File 1.txt",
            result->at(0).path.AsUTF8Unsafe());
}

TEST_F(SearchMetadataTest, SearchMetadata_RegularFiles) {
  FileError error = FILE_ERROR_FAILED;
  std::unique_ptr<MetadataSearchResultVector> result;

  SearchMetadata(
      base::ThreadTaskRunnerHandle::Get(), resource_metadata_.get(), "SubDir",
      base::Bind(&MatchesType, SEARCH_METADATA_ALL), kDefaultAtMostNumMatches,
      MetadataSearchOrder::LAST_ACCESSED,
      google_apis::test_util::CreateCopyResultCallback(&error, &result));
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(FILE_ERROR_OK, error);
  ASSERT_TRUE(result);
  ASSERT_EQ(2U, result->size());

  // All base names should contain "File". The results should be sorted by the
  // last accessed time in descending order.
  EXPECT_EQ("drive/root/Slash \xE2\x88\x95 in directory/Slash SubDir File.txt",
            result->at(0).path.AsUTF8Unsafe());
  EXPECT_EQ("drive/root/Directory-1/SubDirectory File 1.txt",
            result->at(1).path.AsUTF8Unsafe());
}

TEST_F(SearchMetadataTest, SearchMetadata_AtMostOneFile_LastAccessed) {
  FileError error = FILE_ERROR_FAILED;
  std::unique_ptr<MetadataSearchResultVector> result;

  // There are two files matching "SubDir" but only one file should be
  // returned. Results are ordered by last accessed time.
  SearchMetadata(
      base::ThreadTaskRunnerHandle::Get(), resource_metadata_.get(), "SubDir",
      base::Bind(&MatchesType, SEARCH_METADATA_ALL),
      1,  // at_most_num_matches
      MetadataSearchOrder::LAST_ACCESSED,
      google_apis::test_util::CreateCopyResultCallback(&error, &result));
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(FILE_ERROR_OK, error);
  ASSERT_TRUE(result);
  ASSERT_EQ(1U, result->size());
  EXPECT_EQ("drive/root/Slash \xE2\x88\x95 in directory/Slash SubDir File.txt",
            result->at(0).path.AsUTF8Unsafe());
}

TEST_F(SearchMetadataTest, SearchMetadata_AtMostOneFile_LastModified) {
  FileError error = FILE_ERROR_FAILED;
  std::unique_ptr<MetadataSearchResultVector> result;

  // There are two files matching "SubDir" but only one file should be
  // returned. Results are ordered by last modified time.
  SearchMetadata(
      base::ThreadTaskRunnerHandle::Get(), resource_metadata_.get(), "SubDir",
      base::Bind(&MatchesType, SEARCH_METADATA_ALL),
      1,  // at_most_num_matches
      MetadataSearchOrder::LAST_MODIFIED,
      google_apis::test_util::CreateCopyResultCallback(&error, &result));
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(FILE_ERROR_OK, error);
  ASSERT_TRUE(result);
  ASSERT_EQ(1U, result->size());
  EXPECT_EQ("drive/root/Directory-1/SubDirectory File 1.txt",
            result->at(0).path.AsUTF8Unsafe());
}

TEST_F(SearchMetadataTest, SearchMetadata_Directory) {
  FileError error = FILE_ERROR_FAILED;
  std::unique_ptr<MetadataSearchResultVector> result;

  SearchMetadata(
      base::ThreadTaskRunnerHandle::Get(), resource_metadata_.get(),
      "Directory-1", base::Bind(&MatchesType, SEARCH_METADATA_ALL),
      kDefaultAtMostNumMatches, MetadataSearchOrder::LAST_ACCESSED,
      google_apis::test_util::CreateCopyResultCallback(&error, &result));
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(FILE_ERROR_OK, error);
  ASSERT_TRUE(result);
  ASSERT_EQ(1U, result->size());
  EXPECT_EQ("drive/root/Directory-1", result->at(0).path.AsUTF8Unsafe());
}

TEST_F(SearchMetadataTest, SearchMetadata_HostedDocument) {
  FileError error = FILE_ERROR_FAILED;
  std::unique_ptr<MetadataSearchResultVector> result;

  SearchMetadata(
      base::ThreadTaskRunnerHandle::Get(), resource_metadata_.get(), "Document",
      base::Bind(&MatchesType, SEARCH_METADATA_ALL), kDefaultAtMostNumMatches,
      MetadataSearchOrder::LAST_ACCESSED,
      google_apis::test_util::CreateCopyResultCallback(&error, &result));
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(FILE_ERROR_OK, error);
  ASSERT_TRUE(result);
  ASSERT_EQ(1U, result->size());

  EXPECT_EQ("drive/root/Document 1 excludeDir-test.gdoc",
            result->at(0).path.AsUTF8Unsafe());
}

TEST_F(SearchMetadataTest, SearchMetadata_ExcludeHostedDocument) {
  FileError error = FILE_ERROR_FAILED;
  std::unique_ptr<MetadataSearchResultVector> result;

  SearchMetadata(
      base::ThreadTaskRunnerHandle::Get(), resource_metadata_.get(), "Document",
      base::Bind(&MatchesType, SEARCH_METADATA_EXCLUDE_HOSTED_DOCUMENTS),
      kDefaultAtMostNumMatches, MetadataSearchOrder::LAST_ACCESSED,
      google_apis::test_util::CreateCopyResultCallback(&error, &result));
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(FILE_ERROR_OK, error);
  ASSERT_TRUE(result);
  ASSERT_EQ(0U, result->size());
}

TEST_F(SearchMetadataTest, SearchMetadata_SharedWithMe) {
  FileError error = FILE_ERROR_FAILED;
  std::unique_ptr<MetadataSearchResultVector> result;

  SearchMetadata(
      base::ThreadTaskRunnerHandle::Get(), resource_metadata_.get(), "",
      base::Bind(&MatchesType, SEARCH_METADATA_SHARED_WITH_ME),
      kDefaultAtMostNumMatches, MetadataSearchOrder::LAST_ACCESSED,
      google_apis::test_util::CreateCopyResultCallback(&error, &result));
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(FILE_ERROR_OK, error);
  ASSERT_TRUE(result);
  ASSERT_EQ(1U, result->size());
  EXPECT_EQ("drive/root/Directory-1/Shared To The Account Owner.txt",
            result->at(0).path.AsUTF8Unsafe());
}

TEST_F(SearchMetadataTest, SearchMetadata_FileAndDirectory) {
  FileError error = FILE_ERROR_FAILED;
  std::unique_ptr<MetadataSearchResultVector> result;

  SearchMetadata(
      base::ThreadTaskRunnerHandle::Get(), resource_metadata_.get(),
      "excludeDir-test", base::Bind(&MatchesType, SEARCH_METADATA_ALL),
      kDefaultAtMostNumMatches, MetadataSearchOrder::LAST_ACCESSED,
      google_apis::test_util::CreateCopyResultCallback(&error, &result));

  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(FILE_ERROR_OK, error);
  ASSERT_TRUE(result);
  ASSERT_EQ(2U, result->size());

  EXPECT_EQ("drive/root/Document 1 excludeDir-test.gdoc",
            result->at(0).path.AsUTF8Unsafe());
  EXPECT_EQ("drive/root/Directory 2 excludeDir-test",
            result->at(1).path.AsUTF8Unsafe());
}

TEST_F(SearchMetadataTest, SearchMetadata_ExcludeDirectory) {
  FileError error = FILE_ERROR_FAILED;
  std::unique_ptr<MetadataSearchResultVector> result;

  SearchMetadata(
      base::ThreadTaskRunnerHandle::Get(), resource_metadata_.get(),
      "excludeDir-test",
      base::Bind(&MatchesType, SEARCH_METADATA_EXCLUDE_DIRECTORIES),
      kDefaultAtMostNumMatches, MetadataSearchOrder::LAST_ACCESSED,
      google_apis::test_util::CreateCopyResultCallback(&error, &result));

  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(FILE_ERROR_OK, error);
  ASSERT_TRUE(result);
  ASSERT_EQ(1U, result->size());

  EXPECT_EQ("drive/root/Document 1 excludeDir-test.gdoc",
            result->at(0).path.AsUTF8Unsafe());
}

// "drive", "drive/root", "drive/other" should be excluded.
TEST_F(SearchMetadataTest, SearchMetadata_ExcludeSpecialDirectories) {
  const char* const kQueries[] = { "drive", "root", "other" };
  for (size_t i = 0; i < arraysize(kQueries); ++i) {
    FileError error = FILE_ERROR_FAILED;
    std::unique_ptr<MetadataSearchResultVector> result;

    const std::string query = kQueries[i];
    SearchMetadata(
        base::ThreadTaskRunnerHandle::Get(), resource_metadata_.get(), query,
        base::Bind(&MatchesType, SEARCH_METADATA_ALL), kDefaultAtMostNumMatches,
        MetadataSearchOrder::LAST_ACCESSED,
        google_apis::test_util::CreateCopyResultCallback(&error, &result));

    base::RunLoop().RunUntilIdle();
    EXPECT_EQ(FILE_ERROR_OK, error);
    ASSERT_TRUE(result);
    ASSERT_TRUE(result->empty()) << ": " << query << " should not match";
  }
}

TEST_F(SearchMetadataTest, SearchMetadata_Offline) {
  FileError error = FILE_ERROR_FAILED;
  std::unique_ptr<MetadataSearchResultVector> result;

  SearchMetadata(
      base::ThreadTaskRunnerHandle::Get(), resource_metadata_.get(), "",
      base::Bind(&MatchesType, SEARCH_METADATA_OFFLINE),
      kDefaultAtMostNumMatches, MetadataSearchOrder::LAST_ACCESSED,
      google_apis::test_util::CreateCopyResultCallback(&error, &result));
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(FILE_ERROR_OK, error);
  ASSERT_EQ(3U, result->size());

  // This is not included in the cache but is a hosted document.
  EXPECT_EQ("drive/root/Document 1 excludeDir-test.gdoc",
            result->at(0).path.AsUTF8Unsafe());

  EXPECT_EQ("drive/root/File 2.txt",
            result->at(1).path.AsUTF8Unsafe());
  EXPECT_EQ("drive/root/Directory-1/SubDirectory File 1.txt",
            result->at(2).path.AsUTF8Unsafe());
}

TEST_F(SearchMetadataTest, SearchMetadata_MultipleKeywords) {
  FileError error = FILE_ERROR_FAILED;
  std::unique_ptr<MetadataSearchResultVector> result;

  SearchMetadata(
      base::ThreadTaskRunnerHandle::Get(), resource_metadata_.get(),
      "Directory 1", base::Bind(&MatchesType, SEARCH_METADATA_ALL),
      kDefaultAtMostNumMatches, MetadataSearchOrder::LAST_ACCESSED,
      google_apis::test_util::CreateCopyResultCallback(&error, &result));

  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(FILE_ERROR_OK, error);
  ASSERT_TRUE(result);
  ASSERT_EQ(2U, result->size());

  EXPECT_EQ("drive/root/Directory-1/SubDirectory File 1.txt",
            result->at(0).path.AsUTF8Unsafe());
  EXPECT_EQ("drive/root/Directory-1", result->at(1).path.AsUTF8Unsafe());
}

TEST_F(SearchMetadataTest,
       SearchMetadata_KeywordsSeparatedWithIdeographicSpace) {
  FileError error = FILE_ERROR_FAILED;
  std::unique_ptr<MetadataSearchResultVector> result;

  // \xE3\x80\x80 is ideographic space.
  SearchMetadata(
      base::ThreadTaskRunnerHandle::Get(), resource_metadata_.get(),
      "Directory\xE3\x80\x80"
      "1",
      base::Bind(&MatchesType, SEARCH_METADATA_ALL), kDefaultAtMostNumMatches,
      MetadataSearchOrder::LAST_ACCESSED,
      google_apis::test_util::CreateCopyResultCallback(&error, &result));

  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(FILE_ERROR_OK, error);
  ASSERT_TRUE(result);
  ASSERT_EQ(2U, result->size());

  EXPECT_EQ("drive/root/Directory-1/SubDirectory File 1.txt",
            result->at(0).path.AsUTF8Unsafe());
  EXPECT_EQ("drive/root/Directory-1", result->at(1).path.AsUTF8Unsafe());
}

TEST(SearchMetadataSimpleTest, FindAndHighlight_ZeroMatches) {
  std::string highlighted_text;
  EXPECT_FALSE(FindAndHighlightWrapper("text", "query", &highlighted_text));
}

TEST(SearchMetadataSimpleTest, FindAndHighlight_EmptyText) {
  std::string highlighted_text;
  EXPECT_FALSE(FindAndHighlightWrapper("", "query", &highlighted_text));
}

TEST(SearchMetadataSimpleTest, FindAndHighlight_EmptyQuery) {
  std::vector<std::unique_ptr<
      base::i18n::FixedPatternStringSearchIgnoringCaseAndAccents>>
      queries;

  std::string highlighted_text;
  EXPECT_TRUE(FindAndHighlight("hello", queries, &highlighted_text));
  EXPECT_EQ("hello", highlighted_text);
}

TEST(SearchMetadataSimpleTest, FindAndHighlight_FullMatch) {
  std::string highlighted_text;
  EXPECT_TRUE(FindAndHighlightWrapper("hello", "hello", &highlighted_text));
  EXPECT_EQ("<b>hello</b>", highlighted_text);
}

TEST(SearchMetadataSimpleTest, FindAndHighlight_StartWith) {
  std::string highlighted_text;
  EXPECT_TRUE(FindAndHighlightWrapper("hello, world", "hello",
                                     &highlighted_text));
  EXPECT_EQ("<b>hello</b>, world", highlighted_text);
}

TEST(SearchMetadataSimpleTest, FindAndHighlight_EndWith) {
  std::string highlighted_text;
  EXPECT_TRUE(FindAndHighlightWrapper("hello, world", "world",
                                     &highlighted_text));
  EXPECT_EQ("hello, <b>world</b>", highlighted_text);
}

TEST(SearchMetadataSimpleTest, FindAndHighlight_InTheMiddle) {
  std::string highlighted_text;
  EXPECT_TRUE(FindAndHighlightWrapper("yo hello, world", "hello",
                                     &highlighted_text));
  EXPECT_EQ("yo <b>hello</b>, world", highlighted_text);
}

TEST(SearchMetadataSimpleTest, FindAndHighlight_MultipeMatches) {
  std::string highlighted_text;
  EXPECT_TRUE(FindAndHighlightWrapper("yoyoyoyoy", "yoy", &highlighted_text));
  // Only the first match is highlighted.
  EXPECT_EQ("<b>yoy</b>oyoyoy", highlighted_text);
}

TEST(SearchMetadataSimpleTest, FindAndHighlight_IgnoreCase) {
  std::string highlighted_text;
  EXPECT_TRUE(FindAndHighlightWrapper("HeLLo", "hello", &highlighted_text));
  EXPECT_EQ("<b>HeLLo</b>", highlighted_text);
}

TEST(SearchMetadataSimpleTest, FindAndHighlight_IgnoreCaseNonASCII) {
  std::string highlighted_text;

  // Case and accent ignorance in Greek. Find "socra" in "Socra'tes".
  EXPECT_TRUE(FindAndHighlightWrapper(
      "\xCE\xA3\xCF\x89\xCE\xBA\xCF\x81\xCE\xAC\xCF\x84\xCE\xB7\xCF\x82",
      "\xCF\x83\xCF\x89\xCE\xBA\xCF\x81\xCE\xB1", &highlighted_text));
  EXPECT_EQ(
      "<b>\xCE\xA3\xCF\x89\xCE\xBA\xCF\x81\xCE\xAC</b>\xCF\x84\xCE\xB7\xCF\x82",
      highlighted_text);

  // In Japanese characters.
  // Find Hiragana "pi" + "(small)ya" in Katakana "hi" + semi-voiced-mark + "ya"
  EXPECT_TRUE(FindAndHighlightWrapper(
      "\xE3\x81\xB2\xE3\x82\x9A\xE3\x82\x83\xE3\x83\xBC",
      "\xE3\x83\x94\xE3\x83\xA4",
      &highlighted_text));
  EXPECT_EQ(
      "<b>\xE3\x81\xB2\xE3\x82\x9A\xE3\x82\x83</b>\xE3\x83\xBC",
      highlighted_text);
}

TEST(SearchMetadataSimpleTest, MultiTextBySingleQuery) {
  std::vector<std::unique_ptr<
      base::i18n::FixedPatternStringSearchIgnoringCaseAndAccents>>
      queries;
  queries.push_back(std::make_unique<
                    base::i18n::FixedPatternStringSearchIgnoringCaseAndAccents>(
      base::UTF8ToUTF16("hello")));

  std::string highlighted_text;
  EXPECT_TRUE(FindAndHighlight("hello", queries, &highlighted_text));
  EXPECT_EQ("<b>hello</b>", highlighted_text);
  EXPECT_FALSE(FindAndHighlight("goodbye", queries, &highlighted_text));
  EXPECT_TRUE(FindAndHighlight("1hello2", queries, &highlighted_text));
  EXPECT_EQ("1<b>hello</b>2", highlighted_text);
}

TEST(SearchMetadataSimpleTest, FindAndHighlight_MetaChars) {
  std::string highlighted_text;
  EXPECT_TRUE(FindAndHighlightWrapper("<hello>", "hello", &highlighted_text));
  EXPECT_EQ("&lt;<b>hello</b>&gt;", highlighted_text);
}

TEST(SearchMetadataSimpleTest, FindAndHighlight_MoreMetaChars) {
  std::string highlighted_text;
  EXPECT_TRUE(FindAndHighlightWrapper("a&b&c&d", "b&c", &highlighted_text));
  EXPECT_EQ("a&amp;<b>b&amp;c</b>&amp;d", highlighted_text);
}

TEST(SearchMetadataSimpleTest, FindAndHighlight_SurrogatePair) {
  std::string highlighted_text;
  // \xF0\x9F\x98\x81 (U+1F601) is a surrogate pair for smile icon of emoji.
  EXPECT_TRUE(FindAndHighlightWrapper("hi\xF0\x9F\x98\x81hello",
                                      "i\xF0\x9F\x98\x81", &highlighted_text));
  EXPECT_EQ("h<b>i\xF0\x9F\x98\x81</b>hello", highlighted_text);
}

TEST(SearchMetadataSimpleTest, FindAndHighlight_MultipleQueries) {
  std::vector<std::unique_ptr<
      base::i18n::FixedPatternStringSearchIgnoringCaseAndAccents>>
      queries;
  queries.push_back(std::make_unique<
                    base::i18n::FixedPatternStringSearchIgnoringCaseAndAccents>(
      base::UTF8ToUTF16("hello")));
  queries.push_back(std::make_unique<
                    base::i18n::FixedPatternStringSearchIgnoringCaseAndAccents>(
      base::UTF8ToUTF16("good")));

  std::string highlighted_text;
  EXPECT_TRUE(
      FindAndHighlight("good morning, hello", queries, &highlighted_text));
  EXPECT_EQ("<b>good</b> morning, <b>hello</b>", highlighted_text);
}

TEST(SearchMetadataSimpleTest, FindAndHighlight_OverlappingHighlights) {
  std::vector<std::unique_ptr<
      base::i18n::FixedPatternStringSearchIgnoringCaseAndAccents>>
      queries;
  queries.push_back(std::make_unique<
                    base::i18n::FixedPatternStringSearchIgnoringCaseAndAccents>(
      base::UTF8ToUTF16("morning")));
  queries.push_back(std::make_unique<
                    base::i18n::FixedPatternStringSearchIgnoringCaseAndAccents>(
      base::UTF8ToUTF16("ing,")));

  std::string highlighted_text;
  EXPECT_TRUE(
      FindAndHighlight("good morning, hello", queries, &highlighted_text));
  EXPECT_EQ("good <b>morning,</b> hello", highlighted_text);
}

}  // namespace internal
}  // namespace drive
