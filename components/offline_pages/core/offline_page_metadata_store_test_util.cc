// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/offline_page_metadata_store_test_util.h"

#include "base/bind.h"
#include "base/callback_forward.h"
#include "base/memory/ptr_util.h"
#include "components/offline_pages/core/model/add_page_task.h"
#include "components/offline_pages/core/model/get_pages_task.h"
#include "components/offline_pages/core/offline_page_types.h"
#include "sql/connection.h"
#include "sql/statement.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace offline_pages {

namespace {

int64_t GetPageCountSync(sql::Connection* db) {
  const char kSql[] = "SELECT count(*) FROM offlinepages_v1";
  sql::Statement statement(db->GetCachedStatement(SQL_FROM_HERE, kSql));
  if (statement.Step()) {
    return statement.ColumnInt64(0);
  }
  return 0UL;
}

}  // namespace

OfflinePageMetadataStoreTestUtil::OfflinePageMetadataStoreTestUtil(
    scoped_refptr<base::TestMockTimeTaskRunner> task_runner)
    : task_runner_(task_runner), store_ptr_(nullptr) {}

OfflinePageMetadataStoreTestUtil::~OfflinePageMetadataStoreTestUtil() {}

void OfflinePageMetadataStoreTestUtil::BuildStore() {
  if (!temp_directory_.IsValid() && !temp_directory_.CreateUniqueTempDir()) {
    DVLOG(1) << "temp_directory_ not created";
    return;
  }

  store_.reset(
      new OfflinePageMetadataStoreSQL(task_runner_, temp_directory_.GetPath()));
  store_ptr_ = store_.get();
}

void OfflinePageMetadataStoreTestUtil::BuildStoreInMemory() {
  store_.reset(new OfflinePageMetadataStoreSQL(task_runner_));
  store_ptr_ = store_.get();
}

void OfflinePageMetadataStoreTestUtil::DeleteStore() {
  store_.reset();
  store_ptr_ = nullptr;
  task_runner_->FastForwardUntilNoTasksRemain();
}

std::unique_ptr<OfflinePageMetadataStoreSQL>
OfflinePageMetadataStoreTestUtil::ReleaseStore() {
  return std::move(store_);
}

void OfflinePageMetadataStoreTestUtil::InsertItem(const OfflinePageItem& page) {
  AddPageResult result;
  auto task = std::make_unique<AddPageTask>(
      store(), page,
      base::Bind([](AddPageResult* out_result,
                    AddPageResult cb_result) { *out_result = cb_result; },
                 &result));
  task->Run();
  task_runner_->RunUntilIdle();
  EXPECT_EQ(AddPageResult::SUCCESS, result);
}

int64_t OfflinePageMetadataStoreTestUtil::GetPageCount() {
  int64_t count = 0;
  store()->Execute(
      base::BindOnce(&GetPageCountSync),
      base::BindOnce(
          [](int64_t* out_count, int64_t cb_count) { *out_count = cb_count; },
          &count));
  task_runner_->RunUntilIdle();
  return count;
}

std::unique_ptr<OfflinePageItem>
OfflinePageMetadataStoreTestUtil::GetPageByOfflineId(int64_t offline_id) {
  OfflinePageItem* page = nullptr;
  auto task = GetPagesTask::CreateTaskMatchingOfflineId(
      store(),
      base::Bind(
          [](OfflinePageItem** out_page, const OfflinePageItem* cb_page) {
            if (cb_page)
              *out_page = new OfflinePageItem(*cb_page);
          },
          &page),
      offline_id);
  task->Run();
  task_runner_->RunUntilIdle();
  return base::WrapUnique<OfflinePageItem>(page);
}

}  // namespace offline_pages
