// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/fileapi/fileapi_message_filter.h"

#include <stddef.h>

#include <string>
#include <vector>

#include "base/memory/ref_counted.h"
#include "base/memory/shared_memory.h"
#include "base/process/process_handle.h"
#include "base/run_loop.h"
#include "content/browser/blob_storage/chrome_blob_storage_context.h"
#include "content/browser/child_process_security_policy_impl.h"
#include "content/common/fileapi/file_system_messages.h"
#include "content/common/fileapi/webblob_messages.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/common/common_param_traits.h"
#include "content/public/test/mock_render_process_host.h"
#include "content/public/test/test_browser_context.h"
#include "content/public/test/test_browser_thread.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "net/base/io_buffer.h"
#include "storage/browser/blob/blob_storage_context.h"
#include "storage/browser/fileapi/file_system_context.h"
#include "storage/browser/test/test_file_system_context.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {

class FileAPIMessageFilterTest : public testing::Test {
 public:
  FileAPIMessageFilterTest()
      : browser_thread_bundle_(TestBrowserThreadBundle::IO_MAINLOOP) {
  }

 protected:
  void SetUp() override {
    file_system_context_ =
        CreateFileSystemContextForTesting(nullptr, base::FilePath());

    for (const storage::FileSystemType& type :
         file_system_context_->GetFileSystemTypes()) {
      ChildProcessSecurityPolicyImpl::GetInstance()
          ->RegisterFileSystemPermissionPolicy(
              type, storage::FileSystemContext::GetPermissionPolicy(type));
    }

    blob_storage_context_ = ChromeBlobStorageContext::GetFor(&browser_context_);

    filter_ = new FileAPIMessageFilter(
        0 /* process_id */,
        BrowserContext::GetDefaultStoragePartition(&browser_context_)
            ->GetURLRequestContext(),
        file_system_context_.get(), blob_storage_context_);

    // Complete initialization.
    base::RunLoop().RunUntilIdle();
  }

  TestBrowserThreadBundle browser_thread_bundle_;
  TestBrowserContext browser_context_;
  scoped_refptr<storage::FileSystemContext> file_system_context_;
  ChromeBlobStorageContext* blob_storage_context_;

  scoped_refptr<FileAPIMessageFilter> filter_;
};

TEST_F(FileAPIMessageFilterTest, CloseChannelWithInflightRequest) {
  scoped_refptr<FileAPIMessageFilter> filter(new FileAPIMessageFilter(
      0 /* process_id */,
      BrowserContext::GetDefaultStoragePartition(&browser_context_)
          ->GetURLRequestContext(),
      file_system_context_.get(),
      ChromeBlobStorageContext::GetFor(&browser_context_)));
  filter->OnChannelConnected(0);

  // Complete initialization.
  base::RunLoop().RunUntilIdle();

  int request_id = 0;
  const GURL kUrl("filesystem:http://example.com/temporary/foo");
  FileSystemHostMsg_ReadMetadata read_metadata(request_id++, kUrl);
  EXPECT_TRUE(filter->OnMessageReceived(read_metadata));

  // Close the filter while it has inflight request.
  filter->OnChannelClosing();

  // This shouldn't cause DCHECK failure.
  base::RunLoop().RunUntilIdle();
}

TEST_F(FileAPIMessageFilterTest, MultipleFilters) {
  scoped_refptr<FileAPIMessageFilter> filter1(new FileAPIMessageFilter(
      0 /* process_id */,
      BrowserContext::GetDefaultStoragePartition(&browser_context_)
          ->GetURLRequestContext(),
      file_system_context_.get(),
      ChromeBlobStorageContext::GetFor(&browser_context_)));
  scoped_refptr<FileAPIMessageFilter> filter2(new FileAPIMessageFilter(
      1 /* process_id */,
      BrowserContext::GetDefaultStoragePartition(&browser_context_)
          ->GetURLRequestContext(),
      file_system_context_.get(),
      ChromeBlobStorageContext::GetFor(&browser_context_)));
  filter1->OnChannelConnected(0);
  filter2->OnChannelConnected(1);

  // Complete initialization.
  base::RunLoop().RunUntilIdle();

  int request_id = 0;
  const GURL kUrl("filesystem:http://example.com/temporary/foo");
  FileSystemHostMsg_ReadMetadata read_metadata(request_id++, kUrl);
  EXPECT_TRUE(filter1->OnMessageReceived(read_metadata));

  // Close the other filter before the request for filter1 is processed.
  filter2->OnChannelClosing();

  // This shouldn't cause DCHECK failure.
  base::RunLoop().RunUntilIdle();
}

}  // namespace content
