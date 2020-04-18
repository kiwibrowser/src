// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/media/cdm_storage_impl.h"

#include "base/bind.h"
#include "base/callback.h"
#include "base/files/file.h"
#include "base/logging.h"
#include "base/run_loop.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/navigation_simulator.h"
#include "content/public/test/test_renderer_host.h"
#include "media/mojo/interfaces/cdm_storage.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"
#include "url/origin.h"

using media::mojom::CdmFile;
using media::mojom::CdmFileAssociatedPtr;
using media::mojom::CdmFileAssociatedPtrInfo;
using media::mojom::CdmStorage;
using media::mojom::CdmStoragePtr;

namespace content {

namespace {

const char kTestFileSystemId[] = "test_file_system";
const char kTestOrigin[] = "http://www.test.com";

// Helper functions to manipulate RenderFrameHosts.

void SimulateNavigation(RenderFrameHost** rfh, const GURL& url) {
  auto navigation_simulator =
      NavigationSimulator::CreateRendererInitiated(url, *rfh);
  navigation_simulator->Commit();
  *rfh = navigation_simulator->GetFinalRenderFrameHost();
}

}  // namespace

class CdmStorageTest : public RenderViewHostTestHarness {
 protected:
  void SetUp() final {
    RenderViewHostTestHarness::SetUp();
    rfh_ = web_contents()->GetMainFrame();
    RenderFrameHostTester::For(rfh_)->InitializeRenderFrameIfNeeded();
    SimulateNavigation(&rfh_, GURL(kTestOrigin));
  }

  // Creates and initializes the CdmStorage object using |file_system_id|.
  // Returns true if successful, false otherwise.
  void Initialize(const std::string& file_system_id) {
    DVLOG(3) << __func__;

    // Most tests don't do any I/O, but Open() returns a base::File which needs
    // to be closed.
    base::ThreadRestrictions::SetIOAllowed(true);

    // Create the CdmStorageImpl object. |cdm_storage_| will own the resulting
    // object.
    CdmStorageImpl::Create(rfh_, file_system_id,
                           mojo::MakeRequest(&cdm_storage_));
  }

  // Open the file |name|. Returns true if the file returned is valid, false
  // otherwise. Updates |status|, |file|, and |cdm_file| with the values
  // returned by CdmStorage. If |status| = kSuccess, |file| should be valid to
  // access, and |cdm_file| should be reset when the file is closed.
  bool Open(const std::string& name,
            CdmStorage::Status* status,
            base::File* file,
            CdmFileAssociatedPtr* cdm_file) {
    DVLOG(3) << __func__;

    cdm_storage_->Open(
        name, base::BindOnce(&CdmStorageTest::OpenDone, base::Unretained(this),
                             status, file, cdm_file));
    RunAndWaitForResult();
    return file->IsValid();
  }

  bool Write(CdmFile* cdm_file,
             const std::vector<uint8_t>& data,
             base::File* file) {
    bool status;
    cdm_file->OpenFileForWriting(
        base::BindOnce(&CdmStorageTest::FileOpenedForWrite,
                       base::Unretained(this), cdm_file, data, file, &status));
    RunAndWaitForResult();
    return status;
  }

 private:
  void OpenDone(CdmStorage::Status* status,
                base::File* file,
                CdmFileAssociatedPtr* cdm_file,
                CdmStorage::Status actual_status,
                base::File actual_file,
                CdmFileAssociatedPtrInfo actual_cdm_file) {
    DVLOG(3) << __func__;
    *status = actual_status;
    *file = std::move(actual_file);

    // Open() returns a CdmFileAssociatedPtrInfo, so bind it to the
    // CdmFileAssociatedPtr provided.
    CdmFileAssociatedPtr cdm_file_ptr;
    cdm_file_ptr.Bind(std::move(actual_cdm_file));
    *cdm_file = std::move(cdm_file_ptr);
    run_loop_->Quit();
  }

  void FileOpenedForWrite(CdmFile* cdm_file,
                          const std::vector<uint8_t>& data,
                          base::File* file,
                          bool* status,
                          base::File file_to_write) {
    int bytes_to_write = base::checked_cast<int>(data.size());
    int bytes_written = file_to_write.Write(
        0, reinterpret_cast<const char*>(data.data()), bytes_to_write);
    *status = bytes_to_write == bytes_written;
    cdm_file->CommitWrite(base::BindOnce(&CdmStorageTest::WriteDone,
                                         base::Unretained(this), file));
  }

  void WriteDone(base::File* file, base::File new_file_for_reading) {
    *file = std::move(new_file_for_reading);
    run_loop_->Quit();
  }

  // Start running and allow the asynchronous IO operations to complete.
  void RunAndWaitForResult() {
    run_loop_.reset(new base::RunLoop());
    run_loop_->Run();
  }

  RenderFrameHost* rfh_ = nullptr;
  CdmStoragePtr cdm_storage_;
  std::unique_ptr<base::RunLoop> run_loop_;
};

TEST_F(CdmStorageTest, InvalidFileSystemIdWithSlash) {
  Initialize("name/");

  const char kFileName[] = "valid_file_name";
  CdmStorage::Status status;
  base::File file;
  CdmFileAssociatedPtr cdm_file;
  EXPECT_FALSE(Open(kFileName, &status, &file, &cdm_file));
  EXPECT_EQ(status, CdmStorage::Status::kFailure);
  EXPECT_FALSE(file.IsValid());
  EXPECT_FALSE(cdm_file.is_bound());
}

TEST_F(CdmStorageTest, InvalidFileSystemIdWithBackSlash) {
  Initialize("name\\");

  const char kFileName[] = "valid_file_name";
  CdmStorage::Status status;
  base::File file;
  CdmFileAssociatedPtr cdm_file;
  EXPECT_FALSE(Open(kFileName, &status, &file, &cdm_file));
  EXPECT_EQ(status, CdmStorage::Status::kFailure);
  EXPECT_FALSE(file.IsValid());
  EXPECT_FALSE(cdm_file.is_bound());
}

TEST_F(CdmStorageTest, InvalidFileSystemIdEmpty) {
  Initialize("");

  const char kFileName[] = "valid_file_name";
  CdmStorage::Status status;
  base::File file;
  CdmFileAssociatedPtr cdm_file;
  EXPECT_FALSE(Open(kFileName, &status, &file, &cdm_file));
  EXPECT_EQ(status, CdmStorage::Status::kFailure);
  EXPECT_FALSE(file.IsValid());
  EXPECT_FALSE(cdm_file.is_bound());
}

TEST_F(CdmStorageTest, InvalidFileNameEmpty) {
  Initialize(kTestFileSystemId);

  const char kFileName[] = "";
  CdmStorage::Status status;
  base::File file;
  CdmFileAssociatedPtr cdm_file;
  EXPECT_FALSE(Open(kFileName, &status, &file, &cdm_file));
  EXPECT_EQ(status, CdmStorage::Status::kFailure);
  EXPECT_FALSE(file.IsValid());
  EXPECT_FALSE(cdm_file.is_bound());
}

TEST_F(CdmStorageTest, OpenFile) {
  Initialize(kTestFileSystemId);

  const char kFileName[] = "test_file_name";
  CdmStorage::Status status;
  base::File file;
  CdmFileAssociatedPtr cdm_file;
  EXPECT_TRUE(Open(kFileName, &status, &file, &cdm_file));
  EXPECT_EQ(status, CdmStorage::Status::kSuccess);
  EXPECT_TRUE(file.IsValid());
  EXPECT_TRUE(cdm_file.is_bound());
}

TEST_F(CdmStorageTest, OpenFileLocked) {
  Initialize(kTestFileSystemId);

  const char kFileName[] = "test_file_name";
  CdmStorage::Status status;
  base::File file1;
  CdmFileAssociatedPtr cdm_file1;
  EXPECT_TRUE(Open(kFileName, &status, &file1, &cdm_file1));
  EXPECT_EQ(status, CdmStorage::Status::kSuccess);
  EXPECT_TRUE(file1.IsValid());
  EXPECT_TRUE(cdm_file1.is_bound());

  // Second attempt on the same file should fail as the file is locked.
  base::File file2;
  CdmFileAssociatedPtr cdm_file2;
  EXPECT_FALSE(Open(kFileName, &status, &file2, &cdm_file2));
  EXPECT_EQ(status, CdmStorage::Status::kInUse);
  EXPECT_FALSE(file2.IsValid());
  EXPECT_FALSE(cdm_file2.is_bound());

  // Now close the first file and try again. It should be free now.
  file1.Close();
  cdm_file1.reset();

  base::File file3;
  CdmFileAssociatedPtr cdm_file3;
  EXPECT_TRUE(Open(kFileName, &status, &file3, &cdm_file3));
  EXPECT_EQ(status, CdmStorage::Status::kSuccess);
  EXPECT_TRUE(file3.IsValid());
  EXPECT_TRUE(cdm_file3.is_bound());
}

TEST_F(CdmStorageTest, MultipleFiles) {
  Initialize(kTestFileSystemId);

  const char kFileName1[] = "file1";
  CdmStorage::Status status;
  base::File file1;
  CdmFileAssociatedPtr cdm_file1;
  EXPECT_TRUE(Open(kFileName1, &status, &file1, &cdm_file1));
  EXPECT_EQ(status, CdmStorage::Status::kSuccess);
  EXPECT_TRUE(file1.IsValid());
  EXPECT_TRUE(cdm_file1.is_bound());

  const char kFileName2[] = "file2";
  base::File file2;
  CdmFileAssociatedPtr cdm_file2;
  EXPECT_TRUE(Open(kFileName2, &status, &file2, &cdm_file2));
  EXPECT_EQ(status, CdmStorage::Status::kSuccess);
  EXPECT_TRUE(file2.IsValid());
  EXPECT_TRUE(cdm_file2.is_bound());

  const char kFileName3[] = "file3";
  base::File file3;
  CdmFileAssociatedPtr cdm_file3;
  EXPECT_TRUE(Open(kFileName3, &status, &file3, &cdm_file3));
  EXPECT_EQ(status, CdmStorage::Status::kSuccess);
  EXPECT_TRUE(file3.IsValid());
  EXPECT_TRUE(cdm_file3.is_bound());
}

TEST_F(CdmStorageTest, WriteThenReadFile) {
  Initialize(kTestFileSystemId);

  const char kFileName[] = "test_file_name";
  CdmStorage::Status status;
  base::File file;
  CdmFileAssociatedPtr cdm_file;
  EXPECT_TRUE(Open(kFileName, &status, &file, &cdm_file));
  EXPECT_EQ(status, CdmStorage::Status::kSuccess);
  EXPECT_TRUE(file.IsValid());
  EXPECT_TRUE(cdm_file.is_bound());

  // Write several bytes and read them back.
  const uint8_t kTestData[] = "random string";
  const int kTestDataSize = sizeof(kTestData);
  EXPECT_TRUE(Write(cdm_file.get(),
                    std::vector<uint8_t>(kTestData, kTestData + kTestDataSize),
                    &file));
  EXPECT_TRUE(file.IsValid());

  uint8_t data_read[32];
  const int kTestDataReadSize = sizeof(data_read);
  EXPECT_GT(kTestDataReadSize, kTestDataSize);
  EXPECT_EQ(kTestDataSize, file.Read(0, reinterpret_cast<char*>(data_read),
                                     kTestDataReadSize));
  for (size_t i = 0; i < kTestDataSize; i++)
    EXPECT_EQ(kTestData[i], data_read[i]);
}

TEST_F(CdmStorageTest, ReadThenWriteEmptyFile) {
  Initialize(kTestFileSystemId);

  const char kFileName[] = "empty_file_name";
  CdmStorage::Status status;
  base::File file;
  CdmFileAssociatedPtr cdm_file;
  EXPECT_TRUE(Open(kFileName, &status, &file, &cdm_file));
  EXPECT_EQ(status, CdmStorage::Status::kSuccess);
  EXPECT_TRUE(file.IsValid());
  EXPECT_TRUE(cdm_file.is_bound());

  // New file should be empty.
  uint8_t data_read[32];
  const int kTestDataReadSize = sizeof(data_read);
  EXPECT_EQ(
      0, file.Read(0, reinterpret_cast<char*>(data_read), kTestDataReadSize));

  // Write nothing.
  EXPECT_TRUE(Write(cdm_file.get(), std::vector<uint8_t>(), &file));
  EXPECT_TRUE(file.IsValid());

  // Should still be empty.
  EXPECT_EQ(
      0, file.Read(0, reinterpret_cast<char*>(data_read), kTestDataReadSize));
}

}  // namespace content
