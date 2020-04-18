// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/loader/temporary_file_stream.h"

#include <string.h>
#include <string>
#include <utility>

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "content/public/test/test_utils.h"
#include "net/base/file_stream.h"
#include "net/base/io_buffer.h"
#include "net/base/net_errors.h"
#include "net/base/test_completion_callback.h"
#include "storage/browser/blob/shareable_file_reference.h"
#include "testing/gtest/include/gtest/gtest.h"

using storage::ShareableFileReference;

namespace content {

namespace {

const char kTestData[] = "0123456789";
const int kTestDataSize = arraysize(kTestData) - 1;

class WaitForFileStream {
 public:
  base::File::Error error() const { return error_; }
  net::FileStream* file_stream() const { return file_stream_.get(); }
  ShareableFileReference* deletable_file() const {
    return deletable_file_.get();
  }

  void OnFileStreamCreated(base::File::Error error,
                           std::unique_ptr<net::FileStream> file_stream,
                           ShareableFileReference* deletable_file) {
    error_ = error;
    file_stream_ = std::move(file_stream);
    deletable_file_ = deletable_file;
    loop_.Quit();
  }

  void Wait() {
    loop_.Run();
  }

  void Release() {
    file_stream_.reset(nullptr);
    deletable_file_ = nullptr;
  }
 private:
  base::RunLoop loop_;
  base::File::Error error_;
  std::unique_ptr<net::FileStream> file_stream_;
  scoped_refptr<ShareableFileReference> deletable_file_;
};

}  // namespace

TEST(TemporaryFileStreamTest, Basic) {
  TestBrowserThreadBundle thread_bundle(TestBrowserThreadBundle::IO_MAINLOOP);

  // Create a temporary.
  WaitForFileStream file_stream_waiter;
  CreateTemporaryFileStream(base::Bind(&WaitForFileStream::OnFileStreamCreated,
                                       base::Unretained(&file_stream_waiter)));
  file_stream_waiter.Wait();

  // The temporary should exist.
  EXPECT_EQ(base::File::FILE_OK, file_stream_waiter.error());
  base::FilePath file_path = file_stream_waiter.deletable_file()->path();
  EXPECT_TRUE(base::PathExists(file_path));

  // Write some data to the temporary.
  int bytes_written = 0;
  scoped_refptr<net::IOBufferWithSize> buf =
      new net::IOBufferWithSize(kTestDataSize);
  memcpy(buf->data(), kTestData, kTestDataSize);
  scoped_refptr<net::DrainableIOBuffer> drainable =
      new net::DrainableIOBuffer(buf.get(), buf->size());
  while (bytes_written != kTestDataSize) {
    net::TestCompletionCallback write_callback;
    int rv = file_stream_waiter.file_stream()->Write(
        drainable.get(), drainable->BytesRemaining(),
        write_callback.callback());
    if (rv == net::ERR_IO_PENDING)
      rv = write_callback.WaitForResult();
    ASSERT_LT(0, rv);
    drainable->DidConsume(rv);
    bytes_written += rv;
  }

  // Verify the data matches.
  std::string contents;
  ASSERT_TRUE(base::ReadFileToString(file_path, &contents));
  EXPECT_EQ(kTestData, contents);

  // Close the file.
  net::TestCompletionCallback close_callback;
  int rv = file_stream_waiter.file_stream()->Close(close_callback.callback());
  if (rv == net::ERR_IO_PENDING)
    rv = close_callback.WaitForResult();
  EXPECT_EQ(net::OK, rv);

  // Release everything. The file should be gone now.
  file_stream_waiter.Release();
  content::RunAllTasksUntilIdle();

  // The temporary should be gone now.
  EXPECT_FALSE(base::PathExists(file_path));
}

}  // content
