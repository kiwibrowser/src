// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/mojo/services/mojo_cdm_file_io.h"

#include "base/files/file.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/run_loop.h"
#include "base/test/scoped_task_environment.h"
#include "media/cdm/api/content_decryption_module.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::_;
using Status = cdm::FileIOClient::Status;

namespace media {

namespace {

class MockFileIOClient : public cdm::FileIOClient {
 public:
  MockFileIOClient() = default;
  ~MockFileIOClient() override = default;

  MOCK_METHOD1(OnOpenComplete, void(Status));
  MOCK_METHOD3(OnReadComplete, void(Status, const uint8_t*, uint32_t));
  MOCK_METHOD1(OnWriteComplete, void(Status));
};

class MockCdmStorage : public mojom::CdmStorage {
 public:
  MockCdmStorage() = default;
  ~MockCdmStorage() override = default;

  bool SetUp() { return temp_directory_.CreateUniqueTempDir(); }

  // MojoCdmFileIO calls CdmStorage::Open() to actually open the file.
  // Simulate this by creating a file in the temp directory and returning it.
  void Open(const std::string& file_name, OpenCallback callback) override {
    base::FilePath temp_file = temp_directory_.GetPath().AppendASCII(file_name);
    DVLOG(1) << __func__ << " " << temp_file;
    base::File file(temp_file, base::File::FLAG_CREATE_ALWAYS |
                                   base::File::FLAG_READ |
                                   base::File::FLAG_WRITE);
    std::move(callback).Run(mojom::CdmStorage::Status::kSuccess,
                            std::move(file), nullptr);
  }

 private:
  base::ScopedTempDir temp_directory_;
};

}  // namespace

// Currently MockCdmStorage::Open() returns NULL for the
// CdmFileAssociatedPtrInfo, so it is not possible to connect to a CdmFile
// object when writing data. This will require setting up a mojo connection
// between MojoCdmFileIOTest and CdmStorage, rather than using the object
// directly.
//
// Note that the current browser_test ECKEncryptedMediaTest.FileIOTest
// does test writing (and reading) files using mojo. However, additional
// unittests would be good.
// TODO(crbug.com/777550): Implement tests that write to files.

class MojoCdmFileIOTest : public testing::Test, public MojoCdmFileIO::Delegate {
 protected:
  MojoCdmFileIOTest() = default;
  ~MojoCdmFileIOTest() override = default;

  // testing::Test implementation.
  void SetUp() override {
    client_ = std::make_unique<MockFileIOClient>();
    cdm_storage_ = std::make_unique<MockCdmStorage>();
    ASSERT_TRUE(cdm_storage_->SetUp());
    file_io_ = std::make_unique<MojoCdmFileIO>(this, client_.get(),
                                               cdm_storage_.get());
  }

  // MojoCdmFileIO::Delegate implementation.
  void CloseCdmFileIO(MojoCdmFileIO* cdm_file_io) override {
    DCHECK_EQ(file_io_.get(), cdm_file_io);
    file_io_.reset();
  }

  void ReportFileReadSize(int file_size_bytes) override {}

  base::test::ScopedTaskEnvironment scoped_task_environment_;
  std::unique_ptr<MojoCdmFileIO> file_io_;
  std::unique_ptr<MockCdmStorage> cdm_storage_;
  std::unique_ptr<MockFileIOClient> client_;
};

TEST_F(MojoCdmFileIOTest, OpenFile) {
  const std::string kFileName = "openfile";
  EXPECT_CALL(*client_.get(), OnOpenComplete(Status::kSuccess));
  file_io_->Open(kFileName.data(), kFileName.length());

  base::RunLoop().RunUntilIdle();
}

TEST_F(MojoCdmFileIOTest, OpenFileTwice) {
  const std::string kFileName = "openfile";
  EXPECT_CALL(*client_.get(), OnOpenComplete(Status::kSuccess));
  file_io_->Open(kFileName.data(), kFileName.length());

  EXPECT_CALL(*client_.get(), OnOpenComplete(Status::kError));
  file_io_->Open(kFileName.data(), kFileName.length());

  base::RunLoop().RunUntilIdle();
}

TEST_F(MojoCdmFileIOTest, OpenFileAfterOpen) {
  const std::string kFileName = "openfile";
  EXPECT_CALL(*client_.get(), OnOpenComplete(Status::kSuccess));
  file_io_->Open(kFileName.data(), kFileName.length());

  // Run now so that the file is opened.
  base::RunLoop().RunUntilIdle();

  EXPECT_CALL(*client_.get(), OnOpenComplete(Status::kError));
  file_io_->Open(kFileName.data(), kFileName.length());

  // Run a second time so Open() tries after the file is already open.
  base::RunLoop().RunUntilIdle();
}

TEST_F(MojoCdmFileIOTest, OpenDifferentFiles) {
  const std::string kFileName1 = "openfile1";
  EXPECT_CALL(*client_.get(), OnOpenComplete(Status::kSuccess));
  file_io_->Open(kFileName1.data(), kFileName1.length());

  const std::string kFileName2 = "openfile2";
  EXPECT_CALL(*client_.get(), OnOpenComplete(Status::kError));
  file_io_->Open(kFileName2.data(), kFileName2.length());

  base::RunLoop().RunUntilIdle();
}

TEST_F(MojoCdmFileIOTest, OpenBadFileName) {
  // Anything other than ASCII letter, digits, and -._ will fail. Add a
  // Unicode character to the name.
  const std::string kFileName = "openfile\u1234";
  EXPECT_CALL(*client_.get(), OnOpenComplete(Status::kError));
  file_io_->Open(kFileName.data(), kFileName.length());
  base::RunLoop().RunUntilIdle();
}

TEST_F(MojoCdmFileIOTest, OpenTooLongFileName) {
  // Limit is 256 characters, so try a file name with 257.
  const std::string kFileName(257, 'a');
  EXPECT_CALL(*client_.get(), OnOpenComplete(Status::kError));
  file_io_->Open(kFileName.data(), kFileName.length());
  base::RunLoop().RunUntilIdle();
}

TEST_F(MojoCdmFileIOTest, Read) {
  const std::string kFileName = "readfile";
  EXPECT_CALL(*client_.get(), OnOpenComplete(Status::kSuccess));
  file_io_->Open(kFileName.data(), kFileName.length());
  base::RunLoop().RunUntilIdle();

  // File doesn't exist, so reading it should return 0 length buffer.
  EXPECT_CALL(*client_.get(), OnReadComplete(Status::kSuccess, _, 0));
  file_io_->Read();
  base::RunLoop().RunUntilIdle();
}

TEST_F(MojoCdmFileIOTest, ReadBeforeOpen) {
  // File not open, so reading should fail.
  EXPECT_CALL(*client_.get(), OnReadComplete(Status::kError, _, _));
  file_io_->Read();
  base::RunLoop().RunUntilIdle();
}

TEST_F(MojoCdmFileIOTest, TwoReads) {
  const std::string kFileName = "readfile";
  EXPECT_CALL(*client_.get(), OnOpenComplete(Status::kSuccess));
  file_io_->Open(kFileName.data(), kFileName.length());
  base::RunLoop().RunUntilIdle();

  EXPECT_CALL(*client_.get(), OnReadComplete(Status::kSuccess, _, 0));
  EXPECT_CALL(*client_.get(), OnReadComplete(Status::kInUse, _, 0));
  file_io_->Read();
  file_io_->Read();
  base::RunLoop().RunUntilIdle();
}

}  // namespace media
