// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/test/refresh_token_store.h"

#include "base/files/file_util.h"
#include "base/files/important_file_writer.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/values.h"

namespace {
const base::FilePath::CharType kTokenFileName[] =
    FILE_PATH_LITERAL("refresh_tokens.json");
const base::FilePath::CharType kRemotingFolder[] =
    FILE_PATH_LITERAL("remoting");
const base::FilePath::CharType kRefreshTokenStoreFolder[] =
    FILE_PATH_LITERAL("token_store");
}  // namespace

namespace remoting {
namespace test {

// Provides functionality to write a refresh token to a local folder on disk and
// read it back during subsequent tool runs.
class RefreshTokenStoreOnDisk : public RefreshTokenStore {
 public:
  RefreshTokenStoreOnDisk(const std::string& user_name,
                          const base::FilePath& refresh_token_file_path);
  ~RefreshTokenStoreOnDisk() override;

  // RefreshTokenStore interface.
  std::string FetchRefreshToken() override;
  bool StoreRefreshToken(const std::string& refresh_token) override;

 private:
  // Returns the path for the file used to read from or store a refresh token
  // for the user.
  base::FilePath GetPathForRefreshTokenFile();

  // Used to access the user specific token file.
  std::string user_name_;

  // Path used to retrieve the refresh token file.
  base::FilePath refresh_token_file_path_;

  DISALLOW_COPY_AND_ASSIGN(RefreshTokenStoreOnDisk);
};

RefreshTokenStoreOnDisk::RefreshTokenStoreOnDisk(
    const std::string& user_name,
    const base::FilePath& refresh_token_path)
    : user_name_(user_name),
    refresh_token_file_path_(base::MakeAbsoluteFilePath(refresh_token_path)) {
}

RefreshTokenStoreOnDisk::~RefreshTokenStoreOnDisk() = default;

std::string RefreshTokenStoreOnDisk::FetchRefreshToken() {
  base::FilePath refresh_token_file_path(GetPathForRefreshTokenFile());
  DCHECK(!refresh_token_file_path.empty());
  VLOG(1) << "Reading token from: " << refresh_token_file_path.value();

  std::string file_contents;
  if (!base::ReadFileToString(refresh_token_file_path, &file_contents)) {
    VLOG(1) << "Couldn't read token file: " << refresh_token_file_path.value();
    return std::string();
  }

  std::unique_ptr<base::Value> token_data(
      base::JSONReader::Read(file_contents));
  base::DictionaryValue* tokens = nullptr;
  if (!token_data || !token_data->GetAsDictionary(&tokens)) {
    LOG(ERROR) << "Refresh token file contents were not valid JSON, "
               << "could not retrieve token.";
    return std::string();
  }

  std::string refresh_token;
  if (!tokens->GetStringWithoutPathExpansion(user_name_, &refresh_token)) {
    // This may not be an error as the file could exist but contain refresh
    // tokens for other users.
    VLOG(1) << "Could not find token for: " << user_name_;
    return std::string();
  }

  return refresh_token;
}

bool RefreshTokenStoreOnDisk::StoreRefreshToken(
    const std::string& refresh_token) {
  DCHECK(!refresh_token.empty());

  base::FilePath file_path(GetPathForRefreshTokenFile());
  DCHECK(!file_path.empty());
  VLOG(2) << "Storing token to: " << file_path.value();

  base::FilePath refresh_token_file_dir(file_path.DirName());
  if (!base::DirectoryExists(refresh_token_file_dir) &&
      !base::CreateDirectory(refresh_token_file_dir)) {
    LOG(ERROR) << "Failed to create directory, refresh token not stored.";
    return false;
  }

  std::string file_contents("{}");
  if (base::PathExists(file_path)) {
    if (!base::ReadFileToString(file_path, &file_contents)) {
      LOG(ERROR) << "Invalid token file: " << file_path.value();
      return false;
    }
  }

  std::unique_ptr<base::Value> token_data(
      base::JSONReader::Read(file_contents));
  base::DictionaryValue* tokens = nullptr;
  if (!token_data || !token_data->GetAsDictionary(&tokens)) {
    LOG(ERROR) << "Invalid refresh token file format, could not store token.";
    return false;
  }

  std::string json_string;
  tokens->SetKey(user_name_, base::Value(refresh_token));
  if (!base::JSONWriter::Write(*token_data, &json_string)) {
    LOG(ERROR) << "Couldn't convert JSON data to string";
    return false;
  }

  if (!base::ImportantFileWriter::WriteFileAtomically(file_path, json_string)) {
    LOG(ERROR) << "Failed to save refresh token to the file on disk.";
    return false;
  }

  return true;
}

base::FilePath RefreshTokenStoreOnDisk::GetPathForRefreshTokenFile() {
  base::FilePath refresh_token_file_path(refresh_token_file_path_);

  // If we weren't given a specific file path, then use the default path.
  if (refresh_token_file_path.empty()) {
    if (!GetTempDir(&refresh_token_file_path)) {
      LOG(WARNING) << "Failed to retrieve temporary directory path.";
      return base::FilePath();
    }

    refresh_token_file_path = refresh_token_file_path.Append(kRemotingFolder);
    refresh_token_file_path =
        refresh_token_file_path.Append(kRefreshTokenStoreFolder);
  }

  // If no file has been specified, then we will use a default file name.
  if (refresh_token_file_path.Extension().empty()) {
    refresh_token_file_path = refresh_token_file_path.Append(kTokenFileName);
  }

  return refresh_token_file_path;
}

std::unique_ptr<RefreshTokenStore> RefreshTokenStore::OnDisk(
    const std::string& user_name,
    const base::FilePath& refresh_token_file_path) {
  return base::WrapUnique<RefreshTokenStore>(
      new RefreshTokenStoreOnDisk(user_name, refresh_token_file_path));
}

}  // namespace test
}  // namespace remoting
