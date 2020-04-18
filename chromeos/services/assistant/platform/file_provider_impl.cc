// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/services/assistant/platform/file_provider_impl.h"

#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/path_service.h"

namespace chromeos {
namespace assistant {
namespace {

constexpr int kReadFileSizeLimitInBytes = 10 * 1024 * 1024;

// Get the root path for assistant files.
base::FilePath GetRootPath() {
  base::FilePath home_dir;
  CHECK(base::PathService::Get(base::DIR_HOME, &home_dir));
  // Ensures DIR_HOME is overridden after primary user sign-in.
  CHECK_NE(base::GetHomeDir(), home_dir);
  return home_dir;
}

}  // namespace

FileProviderImpl::FileProviderImpl()
    : root_path_(
          GetRootPath().Append(FILE_PATH_LITERAL("google-assistant-library"))) {
}

FileProviderImpl::~FileProviderImpl() = default;

std::string FileProviderImpl::ReadFile(const std::string& path) {
  std::string data;
  base::ReadFileToStringWithMaxSize(root_path_.Append(path), &data,
                                    kReadFileSizeLimitInBytes);
  return data;
}

bool FileProviderImpl::WriteFile(const std::string& path,
                                 const std::string& data) {
  base::FilePath full_path = root_path_.Append(path);
  if (!base::PathExists(full_path.DirName()) &&
      !base::CreateDirectory(full_path.DirName()))
    return false;

  // Create a temp file.
  base::FilePath temp_file;
  if (!base::CreateTemporaryFileInDir(full_path.DirName(), &temp_file)) {
    return false;
  }

  // Write to the tmp file.
  const int size = data.size();
  int written_size = base::WriteFile(temp_file, data.data(), size);
  if (written_size != size) {
    return false;
  }

  // Replace the current file with the temp file.
  if (!base::ReplaceFile(temp_file, full_path, nullptr)) {
    return false;
  }

  return true;
}

std::string FileProviderImpl::ReadSecureFile(const std::string& path) {
  return ReadFile(path);
}

bool FileProviderImpl::WriteSecureFile(const std::string& path,
                                       const std::string& data) {
  // No need to encrypt since |root_path_| should be inside the primary user's
  // cryptohome and is already encrypted.
  return WriteFile(path, data);
}

void FileProviderImpl::CleanAssistantData() {
  base::DeleteFile(root_path_, true);
}

}  // namespace assistant
}  // namespace chromeos
