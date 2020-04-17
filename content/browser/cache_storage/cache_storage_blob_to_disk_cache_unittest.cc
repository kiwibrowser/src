// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/cache_storage/cache_storage_blob_to_disk_cache.h"

#include <string>
#include <utility>

#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/test/null_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "content/browser/blob_storage/chrome_blob_storage_context.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/test/test_browser_context.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "net/disk_cache/disk_cache.h"
#include "net/url_request/url_request.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_getter.h"
#include "net/url_request/url_request_job_factory_impl.h"
#include "storage/browser/blob/blob_data_builder.h"
#include "storage/browser/blob/blob_impl.h"
#include "storage/browser/blob/blob_storage_context.h"
#include "storage/browser/blob/blob_url_request_job_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {

namespace {

const char kTestData[] = "Hello World";
const char kEntryKey[] = "FooEntry";
const int kCacheEntryIndex = 1;

class NullURLRequestContextGetter : public net::URLRequestContextGetter {
 public:
  NullURLRequestContextGetter() : null_task_runner_(new base::NullTaskRunner) {}

  net::URLRequestContext* GetURLRequestContext() override { return nullptr; }

  scoped_refptr<base::SingleThreadTaskRunner> GetNetworkTaskRunner()
      const override {
    return null_task_runner_;
  }

 private:
  ~NullURLRequestContextGetter() override {}

  scoped_refptr<base::SingleThreadTaskRunner> null_task_runner_;
};

// Returns a BlobProtocolHandler that uses |blob_storage_context|. Caller owns
// the memory.
std::unique_ptr<storage::BlobProtocolHandler> CreateMockBlobProtocolHandler(
    storage::BlobStorageContext* blob_storage_context) {
  return std::make_unique<storage::BlobProtocolHandler>(blob_storage_context);
}

// A CacheStorageBlobToDiskCache that can delay reading from blobs.
class TestCacheStorageBlobToDiskCache : public CacheStorageBlobToDiskCache {
 public:
  TestCacheStorageBlobToDiskCache() {}
  ~TestCacheStorageBlobToDiskCache() override {}

  void ContinueReadFromBlob() { CacheStorageBlobToDiskCache::ReadFromBlob(); }

  void set_delay_blob_reads(bool delay) { delay_blob_reads_ = delay; }

 protected:
  void ReadFromBlob() override {
    if (delay_blob_reads_)
      return;
    ContinueReadFromBlob();
  }

 private:
  bool delay_blob_reads_ = false;

  DISALLOW_COPY_AND_ASSIGN(TestCacheStorageBlobToDiskCache);
};

class CacheStorageBlobToDiskCacheTest : public testing::Test {
 protected:
  CacheStorageBlobToDiskCacheTest()
      : browser_thread_bundle_(TestBrowserThreadBundle::IO_MAINLOOP),
        browser_context_(new TestBrowserContext()),
        url_request_context_getter_(
            BrowserContext::GetDefaultStoragePartition(browser_context_.get())->
                GetURLRequestContext()),
        cache_storage_blob_to_disk_cache_(
            new TestCacheStorageBlobToDiskCache()),
        data_(kTestData),
        callback_success_(false),
        callback_called_(false) {}

  void SetUp() override {
    InitBlobStorage();
    InitURLRequestJobFactory();
    InitBlob();
    InitCache();
  }

  void InitBlobStorage() {
    ChromeBlobStorageContext* blob_storage_context =
        ChromeBlobStorageContext::GetFor(browser_context_.get());
    base::RunLoop().RunUntilIdle();

    blob_storage_context_ = blob_storage_context->context();
  }

  void InitURLRequestJobFactory() {
    url_request_job_factory_.reset(new net::URLRequestJobFactoryImpl);
    url_request_job_factory_->SetProtocolHandler(
        "blob", CreateMockBlobProtocolHandler(blob_storage_context_));

    net::URLRequestContext* url_request_context =
        url_request_context_getter_->GetURLRequestContext();

    url_request_context->set_job_factory(url_request_job_factory_.get());
  }

  void InitBlob() {
    auto blob_data =
        std::make_unique<storage::BlobDataBuilder>("blob-id:myblob");
    blob_data->AppendData(data_);
    blob_handle_ = blob_storage_context_->AddFinishedBlob(std::move(blob_data));
  }

  void InitCache() {
    int rv = CreateCacheBackend(
        net::MEMORY_CACHE, net::CACHE_BACKEND_DEFAULT, base::FilePath(),
        (CacheStorageBlobToDiskCache::kBufferSize * 100) /* max bytes */,
        false /* force */, nullptr /* net log */, &cache_backend_,
        base::DoNothing());
    // The memory cache runs synchronously.
    EXPECT_EQ(net::OK, rv);
    EXPECT_TRUE(cache_backend_);

    std::unique_ptr<disk_cache::Entry*> entry(new disk_cache::Entry*());
    disk_cache::Entry** entry_ptr = entry.get();
    rv = cache_backend_->CreateEntry(kEntryKey, net::HIGHEST, entry_ptr,
                                     base::DoNothing());
    EXPECT_EQ(net::OK, rv);
    disk_cache_entry_.reset(*entry);
  }

  std::string ReadCacheContent() {
    int bytes_to_read = disk_cache_entry_->GetDataSize(kCacheEntryIndex);
    scoped_refptr<net::IOBufferWithSize> buffer(
        new net::IOBufferWithSize(bytes_to_read));

    int rv = disk_cache_entry_->ReadData(kCacheEntryIndex, 0 /* offset */,
                                         buffer.get(), buffer->size(),
                                         base::DoNothing());
    EXPECT_EQ(bytes_to_read, rv);
    return std::string(buffer->data(), bytes_to_read);
  }

  bool Stream() {
    blink::mojom::BlobPtr blob_ptr;
    storage::BlobImpl::Create(
        std::make_unique<storage::BlobDataHandle>(*blob_handle_),
        MakeRequest(&blob_ptr));

    cache_storage_blob_to_disk_cache_->StreamBlobToCache(
        std::move(disk_cache_entry_), kCacheEntryIndex, std::move(blob_ptr),
        base::BindOnce(&CacheStorageBlobToDiskCacheTest::StreamCallback,
                       base::Unretained(this)));

    base::RunLoop().RunUntilIdle();

    return callback_called_ && callback_success_;
  }

  void StreamCallback(disk_cache::ScopedEntryPtr entry_ptr, bool success) {
    disk_cache_entry_ = std::move(entry_ptr);
    callback_success_ = success;
    callback_called_ = true;
  }

  TestBrowserThreadBundle browser_thread_bundle_;
  std::unique_ptr<TestBrowserContext> browser_context_;
  std::unique_ptr<net::URLRequestJobFactoryImpl> url_request_job_factory_;
  storage::BlobStorageContext* blob_storage_context_;
  scoped_refptr<net::URLRequestContextGetter> url_request_context_getter_;
  std::unique_ptr<storage::BlobDataHandle> blob_handle_;
  std::unique_ptr<disk_cache::Backend> cache_backend_;
  disk_cache::ScopedEntryPtr disk_cache_entry_;
  std::unique_ptr<TestCacheStorageBlobToDiskCache>
      cache_storage_blob_to_disk_cache_;
  std::string data_;

  bool callback_success_;
  bool callback_called_;
};

TEST_F(CacheStorageBlobToDiskCacheTest, Stream) {
  EXPECT_TRUE(Stream());
  EXPECT_STREQ(data_.c_str(), ReadCacheContent().c_str());
}

TEST_F(CacheStorageBlobToDiskCacheTest, StreamLarge) {
  blob_handle_.reset();
  base::RunLoop().RunUntilIdle();

  data_ = std::string(CacheStorageBlobToDiskCache::kBufferSize * 4, '.');
  InitBlob();

  EXPECT_TRUE(Stream());
  EXPECT_STREQ(data_.c_str(), ReadCacheContent().c_str());
}

TEST_F(CacheStorageBlobToDiskCacheTest, TestDelayMidStream) {
  cache_storage_blob_to_disk_cache_->set_delay_blob_reads(true);

  // The operation should stall while reading from the blob.
  EXPECT_FALSE(Stream());
  EXPECT_FALSE(callback_called_);

  // Verify that continuing results in a completed operation.
  cache_storage_blob_to_disk_cache_->set_delay_blob_reads(false);
  cache_storage_blob_to_disk_cache_->ContinueReadFromBlob();
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called_);
  EXPECT_TRUE(callback_success_);
}

TEST_F(CacheStorageBlobToDiskCacheTest, DeleteMidStream) {
  cache_storage_blob_to_disk_cache_->set_delay_blob_reads(true);

  // The operation should stall while reading from the blob.
  EXPECT_FALSE(Stream());

  // Deleting the BlobToDiskCache mid-stream shouldn't cause a crash.
  cache_storage_blob_to_disk_cache_.reset();
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(callback_called_);
}

}  // namespace

}  // namespace content
