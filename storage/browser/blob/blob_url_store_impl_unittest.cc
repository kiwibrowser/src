// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "storage/browser/blob/blob_url_store_impl.h"

#include "base/test/scoped_task_environment.h"
#include "mojo/edk/embedder/embedder.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "storage/browser/blob/blob_data_builder.h"
#include "storage/browser/blob/blob_impl.h"
#include "storage/browser/blob/blob_storage_context.h"
#include "storage/browser/test/mock_blob_registry_delegate.h"
#include "testing/gtest/include/gtest/gtest.h"

using blink::mojom::BlobPtr;
using blink::mojom::BlobURLStore;
using blink::mojom::BlobURLStorePtr;

namespace storage {

class BlobURLStoreImplTest : public testing::Test {
 public:
  void SetUp() override {
    context_ = std::make_unique<BlobStorageContext>();

    mojo::edk::SetDefaultProcessErrorCallback(base::BindRepeating(
        &BlobURLStoreImplTest::OnBadMessage, base::Unretained(this)));
  }

  void TearDown() override {
    mojo::edk::SetDefaultProcessErrorCallback(
        mojo::edk::ProcessErrorCallback());
  }

  void OnBadMessage(const std::string& error) {
    bad_messages_.push_back(error);
  }

  BlobPtr CreateBlobFromString(const std::string& uuid,
                               const std::string& contents) {
    auto builder = std::make_unique<BlobDataBuilder>(uuid);
    builder->set_content_type("text/plain");
    builder->AppendData(contents);
    BlobPtr blob;
    BlobImpl::Create(context_->AddFinishedBlob(std::move(builder)),
                     MakeRequest(&blob));
    return blob;
  }

  std::string UUIDFromBlob(blink::mojom::Blob* blob) {
    base::RunLoop loop;
    std::string received_uuid;
    blob->GetInternalUUID(base::BindOnce(
        [](base::OnceClosure quit_closure, std::string* uuid_out,
           const std::string& uuid) {
          *uuid_out = uuid;
          std::move(quit_closure).Run();
        },
        loop.QuitClosure(), &received_uuid));
    loop.Run();
    return received_uuid;
  }

  BlobURLStorePtr CreateURLStore() {
    BlobURLStorePtr result;
    mojo::MakeStrongBinding(
        std::make_unique<BlobURLStoreImpl>(context_->AsWeakPtr(), &delegate_),
        MakeRequest(&result));
    return result;
  }

  void RegisterURL(BlobURLStore* store, BlobPtr blob, const GURL& url) {
    base::RunLoop loop;
    store->Register(std::move(blob), url, loop.QuitClosure());
    loop.Run();
  }

  BlobPtr ResolveURL(BlobURLStore* store, const GURL& url) {
    BlobPtr result;
    base::RunLoop loop;
    store->Resolve(kValidUrl, base::BindOnce(
                                  [](base::OnceClosure done, BlobPtr* blob_out,
                                     BlobPtr blob) {
                                    *blob_out = std::move(blob);
                                    std::move(done).Run();
                                  },
                                  loop.QuitClosure(), &result));
    loop.Run();
    return result;
  }

  const std::string kId = "id";
  const GURL kValidUrl = GURL("blob:id");
  const GURL kInvalidUrl = GURL("bolb:id");
  const GURL kFragmentUrl = GURL("blob:id#fragment");

 protected:
  base::test::ScopedTaskEnvironment scoped_task_environment_;
  std::unique_ptr<BlobStorageContext> context_;
  MockBlobRegistryDelegate delegate_;
  std::vector<std::string> bad_messages_;
};

TEST_F(BlobURLStoreImplTest, BasicRegisterRevoke) {
  BlobPtr blob = CreateBlobFromString(kId, "hello world");

  // Register a URL and make sure the URL keeps the blob alive.
  BlobURLStoreImpl url_store(context_->AsWeakPtr(), &delegate_);
  RegisterURL(&url_store, std::move(blob), kValidUrl);

  std::unique_ptr<BlobDataHandle> blob_data_handle =
      context_->GetBlobDataFromPublicURL(kValidUrl);
  ASSERT_TRUE(blob_data_handle);
  EXPECT_EQ(kId, blob_data_handle->uuid());
  blob_data_handle = nullptr;

  // Revoke the URL.
  url_store.Revoke(kValidUrl);
  blob_data_handle = context_->GetBlobDataFromPublicURL(kValidUrl);
  EXPECT_FALSE(blob_data_handle);

  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(context_->registry().HasEntry(kId));
}

TEST_F(BlobURLStoreImplTest, RegisterInvalidScheme) {
  BlobPtr blob = CreateBlobFromString(kId, "hello world");

  BlobURLStorePtr url_store = CreateURLStore();
  RegisterURL(url_store.get(), std::move(blob), kInvalidUrl);
  EXPECT_FALSE(context_->GetBlobDataFromPublicURL(kInvalidUrl));
  EXPECT_EQ(1u, bad_messages_.size());
}

TEST_F(BlobURLStoreImplTest, RegisterCantCommit) {
  BlobPtr blob = CreateBlobFromString(kId, "hello world");

  delegate_.can_commit_url_result = false;

  BlobURLStorePtr url_store = CreateURLStore();
  RegisterURL(url_store.get(), std::move(blob), kValidUrl);
  EXPECT_FALSE(context_->GetBlobDataFromPublicURL(kValidUrl));
  EXPECT_EQ(1u, bad_messages_.size());
}

TEST_F(BlobURLStoreImplTest, RegisterUrlFragment) {
  BlobPtr blob = CreateBlobFromString(kId, "hello world");

  BlobURLStorePtr url_store = CreateURLStore();
  RegisterURL(url_store.get(), std::move(blob), kFragmentUrl);
  EXPECT_FALSE(context_->GetBlobDataFromPublicURL(kFragmentUrl));
  EXPECT_EQ(1u, bad_messages_.size());
}

TEST_F(BlobURLStoreImplTest, ImplicitRevoke) {
  const GURL kValidUrl2("blob:id2");
  BlobPtr blob = CreateBlobFromString(kId, "hello world");
  BlobPtr blob2;
  blob->Clone(MakeRequest(&blob2));

  auto url_store =
      std::make_unique<BlobURLStoreImpl>(context_->AsWeakPtr(), &delegate_);
  RegisterURL(url_store.get(), std::move(blob), kValidUrl);
  EXPECT_TRUE(context_->GetBlobDataFromPublicURL(kValidUrl));
  RegisterURL(url_store.get(), std::move(blob2), kValidUrl2);
  EXPECT_TRUE(context_->GetBlobDataFromPublicURL(kValidUrl2));

  // Destroy URL Store, should revoke URLs.
  url_store = nullptr;
  EXPECT_FALSE(context_->GetBlobDataFromPublicURL(kValidUrl));
  EXPECT_FALSE(context_->GetBlobDataFromPublicURL(kValidUrl2));
}

TEST_F(BlobURLStoreImplTest, RevokeThroughDifferentURLStore) {
  BlobPtr blob = CreateBlobFromString(kId, "hello world");

  BlobURLStoreImpl url_store1(context_->AsWeakPtr(), &delegate_);
  BlobURLStoreImpl url_store2(context_->AsWeakPtr(), &delegate_);

  RegisterURL(&url_store1, std::move(blob), kValidUrl);
  EXPECT_TRUE(context_->GetBlobDataFromPublicURL(kValidUrl));

  url_store2.Revoke(kValidUrl);
  EXPECT_FALSE(context_->GetBlobDataFromPublicURL(kValidUrl));
}

TEST_F(BlobURLStoreImplTest, RevokeInvalidScheme) {
  BlobURLStorePtr url_store = CreateURLStore();
  url_store->Revoke(kInvalidUrl);
  url_store.FlushForTesting();
  EXPECT_EQ(1u, bad_messages_.size());
}

TEST_F(BlobURLStoreImplTest, RevokeCantCommit) {
  delegate_.can_commit_url_result = false;

  BlobURLStorePtr url_store = CreateURLStore();
  url_store->Revoke(kValidUrl);
  url_store.FlushForTesting();
  EXPECT_EQ(1u, bad_messages_.size());
}

TEST_F(BlobURLStoreImplTest, RevokeURLWithFragment) {
  BlobURLStorePtr url_store = CreateURLStore();
  url_store->Revoke(kFragmentUrl);
  url_store.FlushForTesting();
  EXPECT_EQ(1u, bad_messages_.size());
}

TEST_F(BlobURLStoreImplTest, Resolve) {
  BlobPtr blob = CreateBlobFromString(kId, "hello world");

  BlobURLStoreImpl url_store(context_->AsWeakPtr(), &delegate_);
  RegisterURL(&url_store, std::move(blob), kValidUrl);

  blob = ResolveURL(&url_store, kValidUrl);
  ASSERT_TRUE(blob);
  EXPECT_EQ(kId, UUIDFromBlob(blob.get()));
  blob = ResolveURL(&url_store, kFragmentUrl);
  ASSERT_TRUE(blob);
  EXPECT_EQ(kId, UUIDFromBlob(blob.get()));
}

TEST_F(BlobURLStoreImplTest, ResolveNonExistentURL) {
  BlobURLStoreImpl url_store(context_->AsWeakPtr(), &delegate_);

  BlobPtr blob = ResolveURL(&url_store, kValidUrl);
  EXPECT_FALSE(blob);
  blob = ResolveURL(&url_store, kFragmentUrl);
  EXPECT_FALSE(blob);
}

TEST_F(BlobURLStoreImplTest, ResolveInvalidURL) {
  BlobURLStoreImpl url_store(context_->AsWeakPtr(), &delegate_);

  BlobPtr blob = ResolveURL(&url_store, kInvalidUrl);
  EXPECT_FALSE(blob);
}

}  // namespace storage
