// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/model/update_file_path_task.h"

#include <memory>

#include "base/files/file_path.h"
#include "components/offline_pages/core/model/model_task_test_base.h"
#include "components/offline_pages/core/model/offline_page_item_generator.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace offline_pages {

namespace {

// Callback to check results of task.
class UpdateFilePathTaskTestCallback {
 public:
  UpdateFilePathTaskTestCallback()
      : called_(false), success_(false), weak_ptr_factory_(this) {}

  bool called() const { return called_; }

  bool success() const { return success_; }

  void Run(bool success) {
    called_ = true;
    success_ = success;
  }

  base::WeakPtr<UpdateFilePathTaskTestCallback> GetWeakPtr() {
    return weak_ptr_factory_.GetWeakPtr();
  }

 private:
  bool called_;
  bool success_;
  base::WeakPtrFactory<UpdateFilePathTaskTestCallback> weak_ptr_factory_;
};
}  // namespace

class UpdateFilePathTaskTest : public ModelTaskTestBase {};

TEST_F(UpdateFilePathTaskTest, UpdateFilePath) {
  OfflinePageItem page = generator()->CreateItem();
  store_test_util()->InsertItem(page);
  base::FilePath new_file_path(FILE_PATH_LITERAL("/new/path/to/file"));
  UpdateFilePathTaskTestCallback done_callback;

  // Build and run a task to change the file path in the offline page model.
  auto task = std::make_unique<UpdateFilePathTask>(
      store(), page.offline_id, new_file_path,
      base::BindOnce(&UpdateFilePathTaskTestCallback::Run,
                     done_callback.GetWeakPtr()));
  RunTask(std::move(task));

  auto offline_page = store_test_util()->GetPageByOfflineId(page.offline_id);

  EXPECT_EQ(new_file_path, offline_page->file_path);
  EXPECT_TRUE(done_callback.called());
  EXPECT_TRUE(done_callback.success());
}

}  // namespace offline_pages
