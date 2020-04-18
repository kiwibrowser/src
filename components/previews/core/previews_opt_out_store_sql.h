// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PREVIEWS_PREVIEWS_OPT_OUT_STORE_SQL_H_
#define COMPONENTS_PREVIEWS_PREVIEWS_OPT_OUT_STORE_SQL_H_

#include <stdint.h>

#include <memory>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/threading/thread_checker.h"
#include "base/time/time.h"
#include "components/previews/core/previews_experiments.h"
#include "components/previews/core/previews_opt_out_store.h"

namespace base {
class SequencedTaskRunner;
class SingleThreadTaskRunner;
}

namespace sql {
class Connection;
}

namespace previews {

// PreviewsOptOutStoreSQL is an instance of PreviewsOptOutStore
// which is implemented using a SQLite database.
class PreviewsOptOutStoreSQL : public PreviewsOptOutStore {
 public:
  PreviewsOptOutStoreSQL(
      scoped_refptr<base::SingleThreadTaskRunner> io_task_runner,
      scoped_refptr<base::SequencedTaskRunner> background_task_runner,
      const base::FilePath& database_dir,
      std::unique_ptr<PreviewsTypeList> enabled_previews);
  ~PreviewsOptOutStoreSQL() override;

  // PreviewsOptOutStore implementation:
  void AddPreviewNavigation(bool opt_out,
                            const std::string& host_name,
                            PreviewsType type,
                            base::Time now) override;
  void ClearBlackList(base::Time begin_time, base::Time end_time) override;
  void LoadBlackList(LoadBlackListCallback callback) override;

 private:
  // Thread this object is accessed on.
  scoped_refptr<base::SingleThreadTaskRunner> io_task_runner_;

  // Background thread where all SQL access should be run.
  scoped_refptr<base::SequencedTaskRunner> background_task_runner_;

  // Path to the database on disk.
  const base::FilePath db_file_path_;

  // SQL connection to the SQLite database.
  std::unique_ptr<sql::Connection> db_;

  // All enabled previews and versions.
  const std::unique_ptr<PreviewsTypeList> enabled_previews_;

  DISALLOW_COPY_AND_ASSIGN(PreviewsOptOutStoreSQL);
};

}  // namespace previews

#endif  // COMPONENTS_PREVIEWS_PREVIEWS_OPT_OUT_STORE_SQL_H_
