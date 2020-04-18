/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/modules/webdatabase/sqlite/sqlite_file_system.h"

#include "sql/initialization.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/heap/safe_point.h"
#include "third_party/blink/renderer/platform/wtf/text/cstring.h"
#include "third_party/sqlite/sqlite3.h"

// SQLiteFileSystem::registerSQLiteVFS() is implemented in the
// platform-specific files SQLiteFileSystemChromium{Win|Posix}.cpp
namespace blink {

#if DCHECK_IS_ON()
// static
bool SQLiteFileSystem::initialize_sqlite_called_ = false;
#endif  // DCHECK_IS_ON

// static
void SQLiteFileSystem::InitializeSQLite() {
#if DCHECK_IS_ON()
  DCHECK(!initialize_sqlite_called_) << __func__ << " already called";
  initialize_sqlite_called_ = true;
#endif  // DCHECK_IS_ON()

  sql::EnsureSqliteInitialized();
  RegisterSQLiteVFS();
}

// static
int SQLiteFileSystem::OpenDatabase(const String& filename, sqlite3** database) {
#if DCHECK_IS_ON()
  DCHECK(initialize_sqlite_called_)
      << "InitializeSQLite() must be called before " << __func__;
#endif  // DCHECK_IS_ON()

  return sqlite3_open_v2(filename.Utf8().data(), database,
                         SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
                         "chromium_vfs");
}

}  // namespace blink
