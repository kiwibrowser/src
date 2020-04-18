// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/drive/service/test_util.h"

#include "base/run_loop.h"
#include "components/drive/drive_api_util.h"
#include "components/drive/service/fake_drive_service.h"
#include "google_apis/drive/drive_api_parser.h"
#include "google_apis/drive/test_util.h"

using google_apis::DRIVE_OTHER_ERROR;
using google_apis::DriveApiErrorCode;
using google_apis::FileResource;
using google_apis::HTTP_CREATED;

namespace drive {
namespace test_util {

bool SetUpTestEntries(FakeDriveService* drive_service) {
  DriveApiErrorCode error = DRIVE_OTHER_ERROR;
  std::unique_ptr<FileResource> entry;

  drive_service->AddNewFileWithResourceId(
      "2_file_resource_id",
      "audio/mpeg",
      "This is some test content.",
      drive_service->GetRootResourceId(),
      "File 1.txt",
      false,  // shared_with_me
      google_apis::test_util::CreateCopyResultCallback(&error, &entry));
  base::RunLoop().RunUntilIdle();
  if (error != HTTP_CREATED)
    return false;

  drive_service->AddNewFileWithResourceId(
      "slash_file_resource_id",
      "audio/mpeg",
      "This is some test content.",
      drive_service->GetRootResourceId(),
      "Slash / in file 1.txt",
      false,  // shared_with_me
      google_apis::test_util::CreateCopyResultCallback(&error, &entry));
  base::RunLoop().RunUntilIdle();
  if (error != HTTP_CREATED)
    return false;

  drive_service->AddNewFileWithResourceId(
      "3_file_resource_id",
      "audio/mpeg",
      "This is some test content.",
      drive_service->GetRootResourceId(),
      "Duplicate Name.txt",
      false,  // shared_with_me
      google_apis::test_util::CreateCopyResultCallback(&error, &entry));
  base::RunLoop().RunUntilIdle();
  if (error != HTTP_CREATED)
    return false;

  drive_service->AddNewFileWithResourceId(
      "4_file_resource_id",
      "audio/mpeg",
      "This is some test content.",
      drive_service->GetRootResourceId(),
      "Duplicate Name.txt",
      false,  // shared_with_me
      google_apis::test_util::CreateCopyResultCallback(&error, &entry));
  base::RunLoop().RunUntilIdle();
  if (error != HTTP_CREATED)
    return false;

  drive_service->AddNewFileWithResourceId(
      "5_document_resource_id",
      util::kGoogleDocumentMimeType,
      std::string(),
      drive_service->GetRootResourceId(),
      "Document 1 excludeDir-test",
      false,  // shared_with_me
      google_apis::test_util::CreateCopyResultCallback(&error, &entry));
  base::RunLoop().RunUntilIdle();
  if (error != HTTP_CREATED)
    return false;

  drive_service->AddNewFileWithResourceId(
      "1_folder_resource_id",
      util::kDriveFolderMimeType,
      std::string(),
      drive_service->GetRootResourceId(),
      "Directory 1",
      false,  // shared_with_me
      google_apis::test_util::CreateCopyResultCallback(&error, &entry));
  base::RunLoop().RunUntilIdle();
  if (error != HTTP_CREATED)
    return false;

  drive_service->AddNewFileWithResourceId(
      "subdirectory_file_1_id",
      "audio/mpeg",
      "This is some test content.",
      "1_folder_resource_id",
      "SubDirectory File 1.txt",
      false,  // shared_with_me
      google_apis::test_util::CreateCopyResultCallback(&error, &entry));
  base::RunLoop().RunUntilIdle();
  if (error != HTTP_CREATED)
    return false;

  drive_service->AddNewFileWithResourceId(
      "subdirectory_unowned_file_1_id",
      "audio/mpeg",
      "This is some test content.",
      "1_folder_resource_id",
      "Shared to The Account Owner.txt",
      true,  // shared_with_me
      google_apis::test_util::CreateCopyResultCallback(&error, &entry));
  base::RunLoop().RunUntilIdle();
  if (error != HTTP_CREATED)
    return false;

  drive_service->AddNewDirectoryWithResourceId(
      "sub_dir_folder_resource_id", "1_folder_resource_id",
      "Sub Directory Folder", AddNewDirectoryOptions(),
      google_apis::test_util::CreateCopyResultCallback(&error, &entry));
  base::RunLoop().RunUntilIdle();
  if (error != HTTP_CREATED)
    return false;

  drive_service->AddNewDirectoryWithResourceId(
      "sub_sub_directory_folder_id", "sub_dir_folder_resource_id",
      "Sub Sub Directory Folder", AddNewDirectoryOptions(),
      google_apis::test_util::CreateCopyResultCallback(&error, &entry));
  base::RunLoop().RunUntilIdle();
  if (error != HTTP_CREATED)
    return false;

  drive_service->AddNewDirectoryWithResourceId(
      "slash_dir_folder_resource_id", drive_service->GetRootResourceId(),
      "Slash / in directory", AddNewDirectoryOptions(),
      google_apis::test_util::CreateCopyResultCallback(&error, &entry));
  base::RunLoop().RunUntilIdle();
  if (error != HTTP_CREATED)
    return false;

  drive_service->AddNewFileWithResourceId(
      "slash_subdir_file",
      "audio/mpeg",
      "This is some test content.",
      "slash_dir_folder_resource_id",
      "Slash SubDir File.txt",
      false,  // shared_with_me
      google_apis::test_util::CreateCopyResultCallback(&error, &entry));
  base::RunLoop().RunUntilIdle();
  if (error != HTTP_CREATED)
    return false;

  drive_service->AddNewDirectoryWithResourceId(
      "sub_dir_folder_2_self_link", drive_service->GetRootResourceId(),
      "Directory 2 excludeDir-test", AddNewDirectoryOptions(),
      google_apis::test_util::CreateCopyResultCallback(&error, &entry));
  base::RunLoop().RunUntilIdle();
  if (error != HTTP_CREATED)
    return false;

  drive_service->AddNewFileWithResourceId(
      "1_orphanfile_resource_id",
      "text/plain",
      "This is some test content.",
      std::string(),
      "Orphan File 1.txt",
      true,  // shared_with_me
      google_apis::test_util::CreateCopyResultCallback(&error, &entry));
  base::RunLoop().RunUntilIdle();
  if (error != HTTP_CREATED)
    return false;

  drive_service->AddNewFileWithResourceId(
      "orphan_doc_1",
      util::kGoogleDocumentMimeType,
      std::string(),
      std::string(),
      "Orphan Document",
      true,  // shared_with_me
      google_apis::test_util::CreateCopyResultCallback(&error, &entry));
  base::RunLoop().RunUntilIdle();
  if (error != HTTP_CREATED)
    return false;

  return true;
}

}  // namespace test_util
}  // namespace drive
