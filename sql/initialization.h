// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SQL_INITIALIZATION_H_
#define SQL_INITIALIZATION_H_

#include "sql/sql_export.h"

namespace sql {

// Makes sure that sqlite3_initialize() is called.
//
// Users of the APIs exposed in //sql do not need to worry about SQLite
// initialization, because sql::Connection calls this function internally.
//
// The function is exposed for other components that use SQLite indirectly, such
// as Blink.
SQL_EXPORT void EnsureSqliteInitialized();

}  // namespace sql

#endif  // SQL_INITIALIZATION_H_
