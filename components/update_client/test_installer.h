// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_UPDATE_CLIENT_TEST_INSTALLER_H_
#define COMPONENTS_UPDATE_CLIENT_TEST_INSTALLER_H_

#include <memory>
#include <string>
#include <utility>

#include "base/files/file_path.h"
#include "components/update_client/update_client.h"

namespace update_client {

// TODO(sorin): consider reducing the number of the installer mocks.
// A TestInstaller is an installer that does nothing for installation except
// increment a counter.
class TestInstaller : public CrxInstaller {
 public:
  TestInstaller();

  void OnUpdateError(int error) override;

  void Install(const base::FilePath& unpack_path,
               const std::string& public_key,
               Callback callback) override;

  bool GetInstalledFile(const std::string& file,
                        base::FilePath* installed_file) override;

  bool Uninstall() override;

  int error() const { return error_; }

  int install_count() const { return install_count_; }

 protected:
  ~TestInstaller() override;

  void InstallComplete(Callback callback, const Result& result) const;

  int error_;
  int install_count_;

 private:
  // Contains the |unpack_path| argument of the Install call.
  base::FilePath unpack_path_;
};

// A ReadOnlyTestInstaller is an installer that knows about files in an existing
// directory. It will not write to the directory.
class ReadOnlyTestInstaller : public TestInstaller {
 public:
  explicit ReadOnlyTestInstaller(const base::FilePath& installed_path);

  bool GetInstalledFile(const std::string& file,
                        base::FilePath* installed_file) override;

 private:
  ~ReadOnlyTestInstaller() override;

  base::FilePath install_directory_;
};

// A VersionedTestInstaller is an installer that installs files into versioned
// directories (e.g. somedir/25.23.89.141/<files>).
class VersionedTestInstaller : public TestInstaller {
 public:
  VersionedTestInstaller();

  void Install(const base::FilePath& unpack_path,
               const std::string& public_key,
               Callback callback) override;

  bool GetInstalledFile(const std::string& file,
                        base::FilePath* installed_file) override;

 private:
  ~VersionedTestInstaller() override;

  base::FilePath install_directory_;
  base::Version current_version_;
};

}  // namespace update_client

#endif  // COMPONENTS_UPDATE_CLIENT_TEST_INSTALLER_H_
