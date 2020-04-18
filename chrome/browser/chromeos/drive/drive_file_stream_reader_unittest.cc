// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/drive/drive_file_stream_reader.h"

#include <stddef.h>
#include <stdint.h>
#include <string>
#include <utility>

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/files/scoped_temp_dir.h"
#include "base/run_loop.h"
#include "base/threading/thread.h"
#include "components/drive/chromeos/drive_test_util.h"
#include "components/drive/chromeos/fake_file_system.h"
#include "components/drive/file_system_core_util.h"
#include "components/drive/local_file_reader.h"
#include "components/drive/service/fake_drive_service.h"
#include "components/drive/service/test_util.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "content/public/test/test_utils.h"
#include "google_apis/drive/drive_api_parser.h"
#include "google_apis/drive/test_util.h"
#include "net/base/io_buffer.h"
#include "net/base/net_errors.h"
#include "net/base/test_completion_callback.h"
#include "net/http/http_byte_range.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace drive {
namespace internal {
namespace {

// Increments the |num_called|, when this method is invoked.
void IncrementCallback(int* num_called) {
  DCHECK(num_called);
  ++*num_called;
}

}  // namespace

class LocalReaderProxyTest : public ::testing::Test {
 protected:
  LocalReaderProxyTest()
      : thread_bundle_(content::TestBrowserThreadBundle::IO_MAINLOOP) {
  }

  void SetUp() override {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    ASSERT_TRUE(google_apis::test_util::CreateFileOfSpecifiedSize(
        temp_dir_.GetPath(), 1024, &file_path_, &file_content_));

    worker_thread_.reset(new base::Thread("ReaderProxyTest"));
    ASSERT_TRUE(worker_thread_->Start());
  }

  content::TestBrowserThreadBundle thread_bundle_;

  base::ScopedTempDir temp_dir_;
  base::FilePath file_path_;
  std::string file_content_;

  std::unique_ptr<base::Thread> worker_thread_;
};

TEST_F(LocalReaderProxyTest, Read) {
  // Open the file first.
  std::unique_ptr<util::LocalFileReader> file_reader(
      new util::LocalFileReader(worker_thread_->task_runner().get()));
  net::TestCompletionCallback callback;
  file_reader->Open(file_path_, 0, callback.callback());
  ASSERT_EQ(net::OK, callback.WaitForResult());

  // Test instance.
  LocalReaderProxy proxy(std::move(file_reader), file_content_.size());

  // Make sure the read content is as same as the file.
  std::string content;
  ASSERT_EQ(net::OK, test_util::ReadAllData(&proxy, &content));
  EXPECT_EQ(file_content_, content);
}

TEST_F(LocalReaderProxyTest, ReadWithLimit) {
  // This test case, we only read first half of the file.
  const std::string expected_content =
      file_content_.substr(0, file_content_.size() / 2);

  // Open the file first.
  std::unique_ptr<util::LocalFileReader> file_reader(
      new util::LocalFileReader(worker_thread_->task_runner().get()));
  net::TestCompletionCallback callback;
  file_reader->Open(file_path_, 0, callback.callback());
  ASSERT_EQ(net::OK, callback.WaitForResult());

  // Test instance.
  LocalReaderProxy proxy(std::move(file_reader), expected_content.size());

  // Make sure the read content is as same as the file.
  std::string content;
  ASSERT_EQ(net::OK, test_util::ReadAllData(&proxy, &content));
  EXPECT_EQ(expected_content, content);
}

class NetworkReaderProxyTest : public ::testing::Test {
 protected:
  NetworkReaderProxyTest()
      : thread_bundle_(content::TestBrowserThreadBundle::IO_MAINLOOP) {
  }

  content::TestBrowserThreadBundle thread_bundle_;
};

TEST_F(NetworkReaderProxyTest, EmptyFile) {
  NetworkReaderProxy proxy(0, 0, 0, base::DoNothing());

  net::TestCompletionCallback callback;
  const int kBufferSize = 10;
  scoped_refptr<net::IOBuffer> buffer(new net::IOBuffer(kBufferSize));
  int result = proxy.Read(buffer.get(), kBufferSize, callback.callback());

  // For empty file, Read() should return 0 immediately.
  EXPECT_EQ(0, result);
}

TEST_F(NetworkReaderProxyTest, Read) {
  int cancel_called = 0;
  {
    NetworkReaderProxy proxy(0, 10, 10,
                             base::Bind(&IncrementCallback, &cancel_called));

    net::TestCompletionCallback callback;
    const int kBufferSize = 3;
    scoped_refptr<net::IOBuffer> buffer(new net::IOBuffer(kBufferSize));

    // If no data is available yet, ERR_IO_PENDING should be returned.
    int result = proxy.Read(buffer.get(), kBufferSize, callback.callback());
    EXPECT_EQ(net::ERR_IO_PENDING, result);

    // And when the data is supplied, the callback will be called.
    std::unique_ptr<std::string> data(new std::string("abcde"));
    proxy.OnGetContent(std::move(data));

    // The returned data should be fit to the buffer size.
    result = callback.GetResult(result);
    EXPECT_EQ(3, result);
    EXPECT_EQ("abc", std::string(buffer->data(), result));

    // The next Read should return immediately because there is pending data
    result = proxy.Read(buffer.get(), kBufferSize, callback.callback());
    EXPECT_EQ(2, result);
    EXPECT_EQ("de", std::string(buffer->data(), result));

    // Supply the data before calling Read operation.
    data.reset(new std::string("fg"));
    proxy.OnGetContent(std::move(data));
    data.reset(new std::string("hij"));
    proxy.OnGetContent(std::move(data));  // Now 10 bytes are supplied.

    // The data should be concatenated if possible.
    result = proxy.Read(buffer.get(), kBufferSize, callback.callback());
    EXPECT_EQ(3, result);
    EXPECT_EQ("fgh", std::string(buffer->data(), result));

    result = proxy.Read(buffer.get(), kBufferSize, callback.callback());
    EXPECT_EQ(2, result);
    EXPECT_EQ("ij", std::string(buffer->data(), result));

    // The whole data is read, so Read() should return 0 immediately by then.
    result = proxy.Read(buffer.get(), kBufferSize, callback.callback());
    EXPECT_EQ(0, result);
  }

  // Proxy is deleted without any called to OnCompleted(). Even in the case,
  // cancel callback should not be invoked.
  EXPECT_EQ(0, cancel_called);
}

TEST_F(NetworkReaderProxyTest, ReadWithLimit) {
  NetworkReaderProxy proxy(10, 10, 10, base::DoNothing());

  net::TestCompletionCallback callback;
  const int kBufferSize = 3;
  scoped_refptr<net::IOBuffer> buffer(new net::IOBuffer(kBufferSize));

  // If no data is available yet, ERR_IO_PENDING should be returned.
  int result = proxy.Read(buffer.get(), kBufferSize, callback.callback());
  EXPECT_EQ(net::ERR_IO_PENDING, result);

  // And when the data is supplied, the callback will be called.
  std::unique_ptr<std::string> data(new std::string("abcde"));
  proxy.OnGetContent(std::move(data));
  data.reset(new std::string("fgh"));
  proxy.OnGetContent(std::move(data));
  data.reset(new std::string("ijklmno"));
  proxy.OnGetContent(std::move(data));

  // The returned data should be fit to the buffer size.
  result = callback.GetResult(result);
  EXPECT_EQ(3, result);
  EXPECT_EQ("klm", std::string(buffer->data(), result));

  // The next Read should return immediately because there is pending data
  result = proxy.Read(buffer.get(), kBufferSize, callback.callback());
  EXPECT_EQ(2, result);
  EXPECT_EQ("no", std::string(buffer->data(), result));

  // Supply the data before calling Read operation.
  data.reset(new std::string("pqrs"));
  proxy.OnGetContent(std::move(data));
  data.reset(new std::string("tuvwxyz"));
  proxy.OnGetContent(std::move(data));  // 't' is the 20-th byte.

  // The data should be concatenated if possible.
  result = proxy.Read(buffer.get(), kBufferSize, callback.callback());
  EXPECT_EQ(3, result);
  EXPECT_EQ("pqr", std::string(buffer->data(), result));

  result = proxy.Read(buffer.get(), kBufferSize, callback.callback());
  EXPECT_EQ(2, result);
  EXPECT_EQ("st", std::string(buffer->data(), result));

  // The whole data is read, so Read() should return 0 immediately by then.
  result = proxy.Read(buffer.get(), kBufferSize, callback.callback());
  EXPECT_EQ(0, result);
}

TEST_F(NetworkReaderProxyTest, ErrorWithPendingCallback) {
  NetworkReaderProxy proxy(0, 10, 10, base::DoNothing());

  net::TestCompletionCallback callback;
  const int kBufferSize = 3;
  scoped_refptr<net::IOBuffer> buffer(new net::IOBuffer(kBufferSize));

  // Set pending callback.
  int result = proxy.Read(buffer.get(), kBufferSize, callback.callback());
  EXPECT_EQ(net::ERR_IO_PENDING, result);

  // Emulate that an error is found. The callback should be called internally.
  proxy.OnCompleted(FILE_ERROR_FAILED);
  result = callback.GetResult(result);
  EXPECT_EQ(net::ERR_FAILED, result);

  // The next Read call should also return the same error code.
  EXPECT_EQ(net::ERR_FAILED,
            proxy.Read(buffer.get(), kBufferSize, callback.callback()));
}

TEST_F(NetworkReaderProxyTest, ErrorWithPendingData) {
  NetworkReaderProxy proxy(0, 10, 10, base::DoNothing());

  net::TestCompletionCallback callback;
  const int kBufferSize = 3;
  scoped_refptr<net::IOBuffer> buffer(new net::IOBuffer(kBufferSize));

  // Supply the data before an error.
  std::unique_ptr<std::string> data(new std::string("abcde"));
  proxy.OnGetContent(std::move(data));

  // Emulate that an error is found.
  proxy.OnCompleted(FILE_ERROR_FAILED);

  // The next Read call should return the error code, even if there is
  // pending data (the pending data should be released in OnCompleted.
  EXPECT_EQ(net::ERR_FAILED,
            proxy.Read(buffer.get(), kBufferSize, callback.callback()));
}

TEST_F(NetworkReaderProxyTest, CancelJob) {
  int num_called = 0;
  {
    NetworkReaderProxy proxy(
        0, 0, 0, base::Bind(&IncrementCallback, &num_called));
    proxy.OnCompleted(FILE_ERROR_OK);
    // Destroy the instance after the network operation is completed.
    // The cancelling callback shouldn't be called.
  }
  EXPECT_EQ(0, num_called);

  num_called = 0;
  {
    NetworkReaderProxy proxy(
        0, 0, 0, base::Bind(&IncrementCallback, &num_called));
    // Destroy the instance before the network operation is completed.
    // The cancelling callback should be called.
  }
  EXPECT_EQ(1, num_called);
}

}  // namespace internal

class DriveFileStreamReaderTest : public ::testing::Test {
 protected:
  DriveFileStreamReaderTest()
      : thread_bundle_(content::TestBrowserThreadBundle::IO_MAINLOOP) {
  }

  void SetUp() override {
    worker_thread_.reset(new base::Thread("DriveFileStreamReaderTest"));
    ASSERT_TRUE(worker_thread_->Start());

    // Initialize FakeDriveService.
    fake_drive_service_.reset(new FakeDriveService);
    ASSERT_TRUE(test_util::SetUpTestEntries(fake_drive_service_.get()));

    // Create a testee instance.
    fake_file_system_.reset(
        new test_util::FakeFileSystem(fake_drive_service_.get()));
  }

  FileSystemInterface* GetFileSystem() {
    return fake_file_system_.get();
  }

  DriveFileStreamReader::FileSystemGetter GetFileSystemGetter() {
    return base::Bind(&DriveFileStreamReaderTest::GetFileSystem,
                      base::Unretained(this));
  }

  content::TestBrowserThreadBundle thread_bundle_;

  std::unique_ptr<base::Thread> worker_thread_;

  std::unique_ptr<FakeDriveService> fake_drive_service_;
  std::unique_ptr<test_util::FakeFileSystem> fake_file_system_;
};

TEST_F(DriveFileStreamReaderTest, Read) {
  const base::FilePath kDriveFile =
      util::GetDriveMyDriveRootPath().AppendASCII("File 1.txt");
  // Create the reader, and initialize it.
  // In this case, the file is not yet locally cached.
  std::unique_ptr<DriveFileStreamReader> reader(new DriveFileStreamReader(
      GetFileSystemGetter(), worker_thread_->task_runner().get()));
  EXPECT_FALSE(reader->IsInitialized());

  int error = net::ERR_FAILED;
  std::unique_ptr<ResourceEntry> entry;
  {
    base::RunLoop run_loop;
    reader->Initialize(
        kDriveFile,
        net::HttpByteRange(),
        google_apis::test_util::CreateQuitCallback(
            &run_loop,
            google_apis::test_util::CreateCopyResultCallback(&error, &entry)));
    run_loop.Run();
  }
  EXPECT_EQ(net::OK, error);
  ASSERT_TRUE(entry);
  EXPECT_TRUE(reader->IsInitialized());
  size_t content_size = entry->file_info().size();

  // Read data from the reader.
  std::string first_content;
  ASSERT_EQ(net::OK, test_util::ReadAllData(reader.get(), &first_content));
  EXPECT_EQ(content_size, first_content.size());

  // Create second instance and initialize it.
  // In this case, the file should be cached one.
  reader.reset(new DriveFileStreamReader(GetFileSystemGetter(),
                                         worker_thread_->task_runner().get()));
  EXPECT_FALSE(reader->IsInitialized());

  error = net::ERR_FAILED;
  entry.reset();
  {
    base::RunLoop run_loop;
    reader->Initialize(
        kDriveFile,
        net::HttpByteRange(),
        google_apis::test_util::CreateQuitCallback(
            &run_loop,
            google_apis::test_util::CreateCopyResultCallback(&error, &entry)));
    run_loop.Run();
  }
  EXPECT_EQ(net::OK, error);
  ASSERT_TRUE(entry);
  EXPECT_TRUE(reader->IsInitialized());

  // The size should be same.
  EXPECT_EQ(content_size, static_cast<size_t>(entry->file_info().size()));

  // Read data from the reader, again.
  std::string second_content;
  ASSERT_EQ(net::OK, test_util::ReadAllData(reader.get(), &second_content));

  // The same content is expected.
  EXPECT_EQ(first_content, second_content);
}

TEST_F(DriveFileStreamReaderTest, ReadRange) {
  // In this test case, we just confirm that the part of file is read.
  const int64_t kRangeOffset = 3;
  const int64_t kRangeLength = 4;

  const base::FilePath kDriveFile =
      util::GetDriveMyDriveRootPath().AppendASCII("File 1.txt");
  // Create the reader, and initialize it.
  // In this case, the file is not yet locally cached.
  std::unique_ptr<DriveFileStreamReader> reader(new DriveFileStreamReader(
      GetFileSystemGetter(), worker_thread_->task_runner().get()));
  EXPECT_FALSE(reader->IsInitialized());

  int error = net::ERR_FAILED;
  std::unique_ptr<ResourceEntry> entry;
  net::HttpByteRange byte_range;
  byte_range.set_first_byte_position(kRangeOffset);
  // Last byte position is inclusive.
  byte_range.set_last_byte_position(kRangeOffset + kRangeLength - 1);
  {
    base::RunLoop run_loop;
    reader->Initialize(
        kDriveFile,
        byte_range,
        google_apis::test_util::CreateQuitCallback(
            &run_loop,
            google_apis::test_util::CreateCopyResultCallback(&error, &entry)));
    run_loop.Run();
  }
  EXPECT_EQ(net::OK, error);
  ASSERT_TRUE(entry);
  EXPECT_TRUE(reader->IsInitialized());

  // Read data from the reader.
  std::string first_content;
  ASSERT_EQ(net::OK, test_util::ReadAllData(reader.get(), &first_content));

  // The length should be equal to range length.
  EXPECT_EQ(kRangeLength, static_cast<int64_t>(first_content.size()));

  // Create second instance and initialize it.
  // In this case, the file should be cached one.
  reader.reset(new DriveFileStreamReader(GetFileSystemGetter(),
                                         worker_thread_->task_runner().get()));
  EXPECT_FALSE(reader->IsInitialized());

  error = net::ERR_FAILED;
  entry.reset();
  {
    base::RunLoop run_loop;
    reader->Initialize(
        kDriveFile,
        byte_range,
        google_apis::test_util::CreateQuitCallback(
            &run_loop,
            google_apis::test_util::CreateCopyResultCallback(&error, &entry)));
    run_loop.Run();
  }
  EXPECT_EQ(net::OK, error);
  ASSERT_TRUE(entry);
  EXPECT_TRUE(reader->IsInitialized());

  // Read data from the reader, again.
  std::string second_content;
  ASSERT_EQ(net::OK, test_util::ReadAllData(reader.get(), &second_content));

  // The same content is expected.
  EXPECT_EQ(first_content, second_content);
}

TEST_F(DriveFileStreamReaderTest, OutOfRangeError) {
  const int64_t kRangeOffset = 1000000;  // Out of range.
  const int64_t kRangeLength = 4;

  const base::FilePath kDriveFile =
      util::GetDriveMyDriveRootPath().AppendASCII("File 1.txt");
  // Create the reader, and initialize it.
  // In this case, the file is not yet locally cached.
  std::unique_ptr<DriveFileStreamReader> reader(new DriveFileStreamReader(
      GetFileSystemGetter(), worker_thread_->task_runner().get()));
  EXPECT_FALSE(reader->IsInitialized());

  int error = net::ERR_FAILED;
  std::unique_ptr<ResourceEntry> entry;
  net::HttpByteRange byte_range;
  byte_range.set_first_byte_position(kRangeOffset);
  // Last byte position is inclusive.
  byte_range.set_last_byte_position(kRangeOffset + kRangeLength - 1);
  {
    base::RunLoop run_loop;
    reader->Initialize(
        kDriveFile,
        byte_range,
        google_apis::test_util::CreateQuitCallback(
            &run_loop,
            google_apis::test_util::CreateCopyResultCallback(&error, &entry)));
    run_loop.Run();
  }
  EXPECT_EQ(net::ERR_REQUEST_RANGE_NOT_SATISFIABLE, error);
  EXPECT_FALSE(entry);
}

TEST_F(DriveFileStreamReaderTest, ZeroByteFileRead) {
  // Prepare an empty file
  {
    google_apis::DriveApiErrorCode error = google_apis::DRIVE_OTHER_ERROR;
    std::unique_ptr<google_apis::FileResource> entry;
    fake_drive_service_->AddNewFile(
        "text/plain",
        "",  // empty
        fake_drive_service_->GetRootResourceId(),
        "EmptyFile.txt",
        false,  // shared_with_me
        google_apis::test_util::CreateCopyResultCallback(&error, &entry));
    content::RunAllTasksUntilIdle();
    ASSERT_EQ(google_apis::HTTP_CREATED, error);
    ASSERT_TRUE(entry);
    ASSERT_EQ(0, entry->file_size());
  }

  const base::FilePath kDriveFile =
      util::GetDriveMyDriveRootPath().AppendASCII("EmptyFile.txt");
  // Create the reader, and initialize it.
  // In this case, the file is not yet locally cached.
  std::unique_ptr<DriveFileStreamReader> reader(new DriveFileStreamReader(
      GetFileSystemGetter(), worker_thread_->task_runner().get()));
  EXPECT_FALSE(reader->IsInitialized());

  int error = net::ERR_FAILED;
  std::unique_ptr<ResourceEntry> entry;
  {
    base::RunLoop run_loop;
    reader->Initialize(
        kDriveFile,
        net::HttpByteRange(),
        google_apis::test_util::CreateQuitCallback(
            &run_loop,
            google_apis::test_util::CreateCopyResultCallback(&error, &entry)));
    run_loop.Run();
  }
  EXPECT_EQ(net::OK, error);
  ASSERT_TRUE(entry);
  ASSERT_EQ(0u, entry->file_info().size());  // It's a zero-byte file.
  EXPECT_TRUE(reader->IsInitialized());

  // Read data from the reader. Check that it successfuly reads empty data.
  std::string first_content;
  ASSERT_EQ(net::OK, test_util::ReadAllData(reader.get(), &first_content));
  EXPECT_EQ(0u, first_content.size());

  // Create second instance and initialize it.
  // In this case, the file should be cached one.
  reader.reset(new DriveFileStreamReader(GetFileSystemGetter(),
                                         worker_thread_->task_runner().get()));
  EXPECT_FALSE(reader->IsInitialized());

  error = net::ERR_FAILED;
  entry.reset();
  {
    base::RunLoop run_loop;
    reader->Initialize(
        kDriveFile,
        net::HttpByteRange(),
        google_apis::test_util::CreateQuitCallback(
            &run_loop,
            google_apis::test_util::CreateCopyResultCallback(&error, &entry)));
    run_loop.Run();
  }
  EXPECT_EQ(net::OK, error);
  ASSERT_TRUE(entry);
  EXPECT_TRUE(reader->IsInitialized());

  // Read data from the reader, again.
  std::string second_content;
  ASSERT_EQ(net::OK, test_util::ReadAllData(reader.get(), &second_content));
  EXPECT_EQ(0u, second_content.size());
}

}  // namespace drive
