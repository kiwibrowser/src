// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_HISTORY_CORE_BROWSER_IN_MEMORY_DATABASE_H_
#define COMPONENTS_HISTORY_CORE_BROWSER_IN_MEMORY_DATABASE_H_

#include "base/macros.h"
#include "components/history/core/browser/url_database.h"
#include "sql/connection.h"

namespace base {
class FilePath;
}

namespace history {

// Class used for a fast in-memory cache of typed URLs. Used for inline
// autocomplete since it is fast enough to be called synchronously as the user
// is typing.
class InMemoryDatabase : public URLDatabase {
 public:
  InMemoryDatabase();
  ~InMemoryDatabase() override;

  // Creates an empty in-memory database.
  bool InitFromScratch();

  // Initializes the database by directly slurping the data from the given
  // file. Conceptually, the InMemoryHistoryBackend should do the populating
  // after this object does some common initialization, but that would be
  // much slower.
  bool InitFromDisk(const base::FilePath& history_name);

 protected:
  // Implemented for URLDatabase.
  sql::Connection& GetDB() override;

 private:
  // Initializes the database connection, this is the shared code between
  // InitFromScratch() and InitFromDisk() above. Returns true on success.
  bool InitDB();

  sql::Connection db_;

  DISALLOW_COPY_AND_ASSIGN(InMemoryDatabase);
};

}  // namespace history

#endif  // COMPONENTS_HISTORY_CORE_BROWSER_IN_MEMORY_DATABASE_H_
