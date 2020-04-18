// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/dom_distiller/core/dom_distiller_service.h"

#include <utility>

#include "base/bind.h"
#include "base/callback.h"
#include "base/containers/hash_tables.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/strings/string_number_conversions.h"
#include "components/dom_distiller/core/article_entry.h"
#include "components/dom_distiller/core/distilled_page_prefs.h"
#include "components/dom_distiller/core/dom_distiller_model.h"
#include "components/dom_distiller/core/dom_distiller_store.h"
#include "components/dom_distiller/core/dom_distiller_test_util.h"
#include "components/dom_distiller/core/fake_distiller.h"
#include "components/dom_distiller/core/fake_distiller_page.h"
#include "components/dom_distiller/core/task_tracker.h"
#include "components/leveldb_proto/testing/fake_db.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using leveldb_proto::test::FakeDB;
using testing::Invoke;
using testing::Return;
using testing::_;

namespace dom_distiller {
namespace test {

namespace {

class FakeViewRequestDelegate : public ViewRequestDelegate {
 public:
  ~FakeViewRequestDelegate() override {}
  MOCK_METHOD1(OnArticleReady, void(const DistilledArticleProto* proto));
  MOCK_METHOD1(OnArticleUpdated,
               void(ArticleDistillationUpdate article_update));
};

class MockDistillerObserver : public DomDistillerObserver {
 public:
  MOCK_METHOD1(ArticleEntriesUpdated, void(const std::vector<ArticleUpdate>&));
  ~MockDistillerObserver() override {}
};

class MockArticleAvailableCallback {
 public:
  MOCK_METHOD1(DistillationCompleted, void(bool));
};

DomDistillerService::ArticleAvailableCallback ArticleCallback(
    MockArticleAvailableCallback* callback) {
  return base::Bind(&MockArticleAvailableCallback::DistillationCompleted,
                    base::Unretained(callback));
}

void RunDistillerCallback(FakeDistiller* distiller,
                          std::unique_ptr<DistilledArticleProto> proto) {
  distiller->RunDistillerCallback(std::move(proto));
  base::RunLoop().RunUntilIdle();
}

std::unique_ptr<DistilledArticleProto> CreateArticleWithURL(
    const std::string& url) {
  std::unique_ptr<DistilledArticleProto> proto(new DistilledArticleProto);
  DistilledPageProto* page = proto->add_pages();
  page->set_url(url);
  return proto;
}

std::unique_ptr<DistilledArticleProto> CreateDefaultArticle() {
  return CreateArticleWithURL("http://www.example.com/default_article_page1");
}

}  // namespace

class DomDistillerServiceTest : public testing::Test {
 public:
  void SetUp() override {
    main_loop_.reset(new base::MessageLoop());
    FakeDB<ArticleEntry>* fake_db = new FakeDB<ArticleEntry>(&db_model_);
    FakeDB<ArticleEntry>::EntryMap store_model;
    store_ =
        test::util::CreateStoreWithFakeDB(fake_db, store_model);
    distiller_factory_ = new MockDistillerFactory();
    distiller_page_factory_ = new MockDistillerPageFactory();
    service_.reset(new DomDistillerService(
        std::unique_ptr<DomDistillerStoreInterface>(store_),
        std::unique_ptr<DistillerFactory>(distiller_factory_),
        std::unique_ptr<DistillerPageFactory>(distiller_page_factory_),
        std::unique_ptr<DistilledPagePrefs>()));
    fake_db->InitCallback(true);
    fake_db->LoadCallback(true);
  }

  void TearDown() override {
    base::RunLoop().RunUntilIdle();
    store_ = nullptr;
    distiller_factory_ = nullptr;
    service_.reset();
  }

 protected:
  // store is owned by service_.
  DomDistillerStoreInterface* store_;
  MockDistillerFactory* distiller_factory_;
  MockDistillerPageFactory* distiller_page_factory_;
  std::unique_ptr<DomDistillerService> service_;
  std::unique_ptr<base::MessageLoop> main_loop_;
  FakeDB<ArticleEntry>::EntryMap db_model_;
};

TEST_F(DomDistillerServiceTest, TestViewEntry) {
  FakeDistiller* distiller = new FakeDistiller(false);
  EXPECT_CALL(*distiller_factory_, CreateDistillerImpl())
      .WillOnce(Return(distiller));

  GURL url("http://www.example.com/p1");
  std::string entry_id("id0");
  ArticleEntry entry;
  entry.set_entry_id(entry_id);
  entry.add_pages()->set_url(url.spec());

  store_->AddEntry(entry);

  FakeViewRequestDelegate viewer_delegate;
  std::unique_ptr<ViewerHandle> handle = service_->ViewEntry(
      &viewer_delegate, service_->CreateDefaultDistillerPage(gfx::Size()),
      entry_id);

  ASSERT_FALSE(distiller->GetArticleCallback().is_null());

  std::unique_ptr<DistilledArticleProto> proto = CreateDefaultArticle();
  EXPECT_CALL(viewer_delegate, OnArticleReady(proto.get()));

  RunDistillerCallback(distiller, std::move(proto));
}

TEST_F(DomDistillerServiceTest, TestViewUrl) {
  FakeDistiller* distiller = new FakeDistiller(false);
  EXPECT_CALL(*distiller_factory_, CreateDistillerImpl())
      .WillOnce(Return(distiller));

  FakeViewRequestDelegate viewer_delegate;
  GURL url("http://www.example.com/p1");
  std::unique_ptr<ViewerHandle> handle = service_->ViewUrl(
      &viewer_delegate, service_->CreateDefaultDistillerPage(gfx::Size()), url);

  ASSERT_FALSE(distiller->GetArticleCallback().is_null());
  EXPECT_EQ(url, distiller->GetUrl());

  std::unique_ptr<DistilledArticleProto> proto = CreateDefaultArticle();
  EXPECT_CALL(viewer_delegate, OnArticleReady(proto.get()));

  RunDistillerCallback(distiller, std::move(proto));
}

TEST_F(DomDistillerServiceTest, TestMultipleViewUrl) {
  FakeDistiller* distiller = new FakeDistiller(false);
  FakeDistiller* distiller2 = new FakeDistiller(false);
  EXPECT_CALL(*distiller_factory_, CreateDistillerImpl())
      .WillOnce(Return(distiller))
      .WillOnce(Return(distiller2));

  FakeViewRequestDelegate viewer_delegate;
  FakeViewRequestDelegate viewer_delegate2;

  GURL url("http://www.example.com/p1");
  GURL url2("http://www.example.com/a/p1");

  std::unique_ptr<ViewerHandle> handle = service_->ViewUrl(
      &viewer_delegate, service_->CreateDefaultDistillerPage(gfx::Size()), url);
  std::unique_ptr<ViewerHandle> handle2 = service_->ViewUrl(
      &viewer_delegate2, service_->CreateDefaultDistillerPage(gfx::Size()),
      url2);

  ASSERT_FALSE(distiller->GetArticleCallback().is_null());
  EXPECT_EQ(url, distiller->GetUrl());

  std::unique_ptr<DistilledArticleProto> proto = CreateDefaultArticle();
  EXPECT_CALL(viewer_delegate, OnArticleReady(proto.get()));

  RunDistillerCallback(distiller, std::move(proto));

  ASSERT_FALSE(distiller2->GetArticleCallback().is_null());
  EXPECT_EQ(url2, distiller2->GetUrl());

  std::unique_ptr<DistilledArticleProto> proto2 = CreateDefaultArticle();
  EXPECT_CALL(viewer_delegate2, OnArticleReady(proto2.get()));

  RunDistillerCallback(distiller2, std::move(proto2));
}

TEST_F(DomDistillerServiceTest, TestViewUrlCancelled) {
  FakeDistiller* distiller = new FakeDistiller(false);
  EXPECT_CALL(*distiller_factory_, CreateDistillerImpl())
      .WillOnce(Return(distiller));

  bool distiller_destroyed = false;
  EXPECT_CALL(*distiller, Die())
      .WillOnce(testing::Assign(&distiller_destroyed, true));

  FakeViewRequestDelegate viewer_delegate;
  GURL url("http://www.example.com/p1");
  std::unique_ptr<ViewerHandle> handle = service_->ViewUrl(
      &viewer_delegate, service_->CreateDefaultDistillerPage(gfx::Size()), url);

  ASSERT_FALSE(distiller->GetArticleCallback().is_null());
  EXPECT_EQ(url, distiller->GetUrl());

  EXPECT_CALL(viewer_delegate, OnArticleReady(_)).Times(0);

  EXPECT_FALSE(distiller_destroyed);

  handle.reset();
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(distiller_destroyed);
}

TEST_F(DomDistillerServiceTest, TestViewUrlDoesNotAddEntry) {
  FakeDistiller* distiller = new FakeDistiller(false);
  EXPECT_CALL(*distiller_factory_, CreateDistillerImpl())
      .WillOnce(Return(distiller));

  FakeViewRequestDelegate viewer_delegate;
  GURL url("http://www.example.com/p1");
  std::unique_ptr<ViewerHandle> handle = service_->ViewUrl(
      &viewer_delegate, service_->CreateDefaultDistillerPage(gfx::Size()), url);

  std::unique_ptr<DistilledArticleProto> proto =
      CreateArticleWithURL(url.spec());
  EXPECT_CALL(viewer_delegate, OnArticleReady(proto.get()));

  RunDistillerCallback(distiller, std::move(proto));
  base::RunLoop().RunUntilIdle();
  // The entry should not be added to the store.
  EXPECT_EQ(0u, store_->GetEntries().size());
}

TEST_F(DomDistillerServiceTest, TestAddAndRemoveEntry) {
  FakeDistiller* distiller = new FakeDistiller(false);
  EXPECT_CALL(*distiller_factory_, CreateDistillerImpl())
      .WillOnce(Return(distiller));

  GURL url("http://www.example.com/p1");

  MockArticleAvailableCallback article_cb;
  EXPECT_CALL(article_cb, DistillationCompleted(true));

  std::string entry_id = service_->AddToList(
      url, service_->CreateDefaultDistillerPage(gfx::Size()),
      ArticleCallback(&article_cb));

  ASSERT_FALSE(distiller->GetArticleCallback().is_null());
  EXPECT_EQ(url, distiller->GetUrl());

  std::unique_ptr<DistilledArticleProto> proto =
      CreateArticleWithURL(url.spec());
  RunDistillerCallback(distiller, std::move(proto));

  ArticleEntry entry;
  EXPECT_TRUE(store_->GetEntryByUrl(url, &entry));
  EXPECT_EQ(entry.entry_id(), entry_id);
  EXPECT_EQ(1u, store_->GetEntries().size());
  service_->RemoveEntry(entry_id);
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(0u, store_->GetEntries().size());
}

TEST_F(DomDistillerServiceTest, TestCancellation) {
  FakeDistiller* distiller = new FakeDistiller(false);
  MockDistillerObserver observer;
  service_->AddObserver(&observer);

  EXPECT_CALL(*distiller_factory_, CreateDistillerImpl())
      .WillOnce(Return(distiller));

  MockArticleAvailableCallback article_cb;
  EXPECT_CALL(article_cb, DistillationCompleted(false));

  GURL url("http://www.example.com/p1");
  std::string entry_id = service_->AddToList(
      url, service_->CreateDefaultDistillerPage(gfx::Size()),
      ArticleCallback(&article_cb));

  // Remove entry will cause the |article_cb| to be called with false value.
  service_->RemoveEntry(entry_id);
  base::RunLoop().RunUntilIdle();
}

TEST_F(DomDistillerServiceTest, TestMultipleObservers) {
  FakeDistiller* distiller = new FakeDistiller(false);
  EXPECT_CALL(*distiller_factory_, CreateDistillerImpl())
      .WillOnce(Return(distiller));

  const int kObserverCount = 5;
  MockDistillerObserver observers[kObserverCount];
  for (int i = 0; i < kObserverCount; ++i) {
    service_->AddObserver(&observers[i]);
  }

  DomDistillerService::ArticleAvailableCallback article_cb;
  GURL url("http://www.example.com/p1");
  std::string entry_id = service_->AddToList(
      url, service_->CreateDefaultDistillerPage(gfx::Size()), article_cb);

  // Distillation should notify all observers that article is added.
  std::vector<DomDistillerObserver::ArticleUpdate> expected_updates;
  DomDistillerObserver::ArticleUpdate update;
  update.entry_id = entry_id;
  update.update_type = DomDistillerObserver::ArticleUpdate::ADD;
  expected_updates.push_back(update);

  for (int i = 0; i < kObserverCount; ++i) {
    EXPECT_CALL(observers[i], ArticleEntriesUpdated(
                                  util::HasExpectedUpdates(expected_updates)));
  }

  std::unique_ptr<DistilledArticleProto> proto = CreateDefaultArticle();
  RunDistillerCallback(distiller, std::move(proto));

  // Remove should notify all observers that article is removed.
  update.update_type = DomDistillerObserver::ArticleUpdate::REMOVE;
  expected_updates.clear();
  expected_updates.push_back(update);
  for (int i = 0; i < kObserverCount; ++i) {
    EXPECT_CALL(observers[i], ArticleEntriesUpdated(
                                  util::HasExpectedUpdates(expected_updates)));
  }

  service_->RemoveEntry(entry_id);
  base::RunLoop().RunUntilIdle();
}

TEST_F(DomDistillerServiceTest, TestMultipleCallbacks) {
  FakeDistiller* distiller = new FakeDistiller(false);
  EXPECT_CALL(*distiller_factory_, CreateDistillerImpl())
      .WillOnce(Return(distiller));

  const int kClientsCount = 5;
  MockArticleAvailableCallback article_cb[kClientsCount];
  // Adding a URL and then distilling calls all clients.
  GURL url("http://www.example.com/p1");
  const std::string entry_id = service_->AddToList(
      url, service_->CreateDefaultDistillerPage(gfx::Size()),
      ArticleCallback(&article_cb[0]));
  EXPECT_CALL(article_cb[0], DistillationCompleted(true));

  for (int i = 1; i < kClientsCount; ++i) {
    EXPECT_EQ(entry_id,
              service_->AddToList(
                  url, service_->CreateDefaultDistillerPage(gfx::Size()),
                  ArticleCallback(&article_cb[i])));
    EXPECT_CALL(article_cb[i], DistillationCompleted(true));
  }

  std::unique_ptr<DistilledArticleProto> proto =
      CreateArticleWithURL(url.spec());
  RunDistillerCallback(distiller, std::move(proto));

  // Add the same url again, all callbacks should be called with true.
  for (int i = 0; i < kClientsCount; ++i) {
    EXPECT_CALL(article_cb[i], DistillationCompleted(true));
    EXPECT_EQ(entry_id,
              service_->AddToList(
                  url, service_->CreateDefaultDistillerPage(gfx::Size()),
                  ArticleCallback(&article_cb[i])));
  }

  base::RunLoop().RunUntilIdle();
}

TEST_F(DomDistillerServiceTest, TestMultipleCallbacksOnRemove) {
  FakeDistiller* distiller = new FakeDistiller(false);
  EXPECT_CALL(*distiller_factory_, CreateDistillerImpl())
      .WillOnce(Return(distiller));

  const int kClientsCount = 5;
  MockArticleAvailableCallback article_cb[kClientsCount];
  // Adding a URL and remove the entry before distillation. Callback should be
  // called with false.
  GURL url("http://www.example.com/p1");
  const std::string entry_id = service_->AddToList(
      url, service_->CreateDefaultDistillerPage(gfx::Size()),
      ArticleCallback(&article_cb[0]));

  EXPECT_CALL(article_cb[0], DistillationCompleted(false));
  for (int i = 1; i < kClientsCount; ++i) {
    EXPECT_EQ(entry_id,
              service_->AddToList(
                  url, service_->CreateDefaultDistillerPage(gfx::Size()),
                  ArticleCallback(&article_cb[i])));
    EXPECT_CALL(article_cb[i], DistillationCompleted(false));
  }

  service_->RemoveEntry(entry_id);
  base::RunLoop().RunUntilIdle();
}

TEST_F(DomDistillerServiceTest, TestMultiplePageArticle) {
  FakeDistiller* distiller = new FakeDistiller(false);
  EXPECT_CALL(*distiller_factory_, CreateDistillerImpl())
      .WillOnce(Return(distiller));

  const int kPageCount = 8;

  std::string base_url("http://www.example.com/p");
  GURL pages_url[kPageCount];
  for (int page_num = 0; page_num < kPageCount; ++page_num) {
    pages_url[page_num] = GURL(base_url + base::IntToString(page_num));
  }

  MockArticleAvailableCallback article_cb;
  EXPECT_CALL(article_cb, DistillationCompleted(true));

  std::string entry_id = service_->AddToList(
      pages_url[0], service_->CreateDefaultDistillerPage(gfx::Size()),
      ArticleCallback(&article_cb));

  ArticleEntry entry;
  ASSERT_FALSE(distiller->GetArticleCallback().is_null());
  EXPECT_EQ(pages_url[0], distiller->GetUrl());

  // Create the article with pages to pass to the distiller.
  std::unique_ptr<DistilledArticleProto> proto =
      CreateArticleWithURL(pages_url[0].spec());
  for (int page_num = 1; page_num < kPageCount; ++page_num) {
    DistilledPageProto* distilled_page = proto->add_pages();
    distilled_page->set_url(pages_url[page_num].spec());
  }

  RunDistillerCallback(distiller, std::move(proto));
  EXPECT_TRUE(store_->GetEntryByUrl(pages_url[0], &entry));

  EXPECT_EQ(kPageCount, entry.pages_size());
  // An article should have just one entry.
  EXPECT_EQ(1u, store_->GetEntries().size());

  // All pages should have correct urls.
  for (int page_num = 0; page_num < kPageCount; ++page_num) {
    EXPECT_EQ(pages_url[page_num].spec(), entry.pages(page_num).url());
  }

  // Should be able to query article using any of the pages url.
  for (int page_num = 0; page_num < kPageCount; ++page_num) {
    EXPECT_TRUE(store_->GetEntryByUrl(pages_url[page_num], &entry));
  }

  service_->RemoveEntry(entry_id);
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(0u, store_->GetEntries().size());
}

TEST_F(DomDistillerServiceTest, TestHasEntry) {
  FakeDistiller* distiller = new FakeDistiller(false);
  EXPECT_CALL(*distiller_factory_, CreateDistillerImpl())
      .WillOnce(Return(distiller));

  GURL url("http://www.example.com/p1");

  MockArticleAvailableCallback article_cb;
  EXPECT_CALL(article_cb, DistillationCompleted(true));

  std::string entry_id = service_->AddToList(
      url, service_->CreateDefaultDistillerPage(gfx::Size()),
      ArticleCallback(&article_cb));

  ASSERT_FALSE(distiller->GetArticleCallback().is_null());
  EXPECT_EQ(url, distiller->GetUrl());

  std::unique_ptr<DistilledArticleProto> proto =
      CreateArticleWithURL(url.spec());
  RunDistillerCallback(distiller, std::move(proto));

  // Check that HasEntry returns true for the article just added.
  EXPECT_TRUE(service_->HasEntry(entry_id));

  // Remove article and check that there is no longer an entry for the given
  // entry id.
  service_->RemoveEntry(entry_id);
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(0u, store_->GetEntries().size());
  EXPECT_FALSE(service_->HasEntry(entry_id));
}

TEST_F(DomDistillerServiceTest, TestGetUrlForOnePageEntry) {
  FakeDistiller* distiller = new FakeDistiller(false);
  EXPECT_CALL(*distiller_factory_, CreateDistillerImpl())
      .WillOnce(Return(distiller));

  GURL url("http://www.example.com/p1");

  MockArticleAvailableCallback article_cb;
  EXPECT_CALL(article_cb, DistillationCompleted(true));

  std::string entry_id = service_->AddToList(
      url, service_->CreateDefaultDistillerPage(gfx::Size()),
      ArticleCallback(&article_cb));

  ASSERT_FALSE(distiller->GetArticleCallback().is_null());
  EXPECT_EQ(url, distiller->GetUrl());

  std::unique_ptr<DistilledArticleProto> proto =
      CreateArticleWithURL(url.spec());
  RunDistillerCallback(distiller, std::move(proto));

  // Check if retrieved URL is same as given URL.
  GURL retrieved_url(service_->GetUrlForEntry(entry_id));
  EXPECT_EQ(url, retrieved_url);

  // Remove article and check that there is no longer an entry for the given
  // entry id.
  service_->RemoveEntry(entry_id);
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(0u, store_->GetEntries().size());
  EXPECT_EQ("", service_->GetUrlForEntry(entry_id));
}

TEST_F(DomDistillerServiceTest, TestGetUrlForMultiPageEntry) {
  FakeDistiller* distiller = new FakeDistiller(false);
  EXPECT_CALL(*distiller_factory_, CreateDistillerImpl())
      .WillOnce(Return(distiller));

  const int kPageCount = 8;

  std::string base_url("http://www.example.com/p");
  GURL pages_url[kPageCount];
  for (int page_num = 0; page_num < kPageCount; ++page_num) {
    pages_url[page_num] = GURL(base_url + base::IntToString(page_num));
  }

  MockArticleAvailableCallback article_cb;
  EXPECT_CALL(article_cb, DistillationCompleted(true));

  std::string entry_id = service_->AddToList(
      pages_url[0], service_->CreateDefaultDistillerPage(gfx::Size()),
      ArticleCallback(&article_cb));

  ArticleEntry entry;
  ASSERT_FALSE(distiller->GetArticleCallback().is_null());
  EXPECT_EQ(pages_url[0], distiller->GetUrl());

  // Create the article with pages to pass to the distiller.
  std::unique_ptr<DistilledArticleProto> proto =
      CreateArticleWithURL(pages_url[0].spec());
  for (int page_num = 1; page_num < kPageCount; ++page_num) {
    DistilledPageProto* distilled_page = proto->add_pages();
    distilled_page->set_url(pages_url[page_num].spec());
  }

  RunDistillerCallback(distiller, std::move(proto));
  EXPECT_TRUE(store_->GetEntryByUrl(pages_url[0], &entry));

  // Check if retrieved URL is same as given URL for the first page.
  GURL retrieved_url(service_->GetUrlForEntry(entry_id));
  EXPECT_EQ(pages_url[0], retrieved_url);

  // Remove the article and check that no URL can be retrieved for the entry.
  service_->RemoveEntry(entry_id);
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(0u, store_->GetEntries().size());
  EXPECT_EQ("", service_->GetUrlForEntry(entry_id));
}

}  // namespace test
}  // namespace dom_distiller
