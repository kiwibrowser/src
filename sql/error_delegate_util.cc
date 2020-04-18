// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "sql/error_delegate_util.h"

#include "third_party/sqlite/sqlite3.h"

namespace sql {

bool IsErrorCatastrophic(int error) {
  switch (error) {
    case SQLITE_DONE:
    case SQLITE_OK:
      // Theoretically, the wrapped delegate might have resolved the error, and
      // we would end up here.
      return false;

    case SQLITE_CORRUPT:
    case SQLITE_NOTADB:
      // Highly unlikely we would ever recover from these.
      return true;

    case SQLITE_CANTOPEN:
      // TODO(erikwright): Figure out what this means.
      return false;

    case SQLITE_IOERR:
      // This could be broken blocks, in which case deleting the DB would be a
      // good idea. But it might also be transient.
      // TODO(erikwright): Figure out if we can distinguish between the two,
      // or determine through metrics analysis to what extent these failures are
      // transient.
      return false;

    case SQLITE_BUSY:
      // Presumably transient.
      return false;

    case SQLITE_TOOBIG:
    case SQLITE_FULL:
    case SQLITE_NOMEM:
      // Not a problem with the database.
      return false;

    case SQLITE_READONLY:
      // Presumably either transient or we don't have the privileges to
      // move/delete the file anyway.
      return false;

    case SQLITE_CONSTRAINT:
    case SQLITE_ERROR:
      // These probgably indicate a programming error or a migration failure
      // that we prefer not to mask.
      return false;

    case SQLITE_LOCKED:
    case SQLITE_INTERNAL:
    case SQLITE_PERM:
    case SQLITE_ABORT:
    case SQLITE_INTERRUPT:
    case SQLITE_NOTFOUND:
    case SQLITE_PROTOCOL:
    case SQLITE_EMPTY:
    case SQLITE_SCHEMA:
    case SQLITE_MISMATCH:
    case SQLITE_MISUSE:
    case SQLITE_NOLFS:
    case SQLITE_AUTH:
    case SQLITE_FORMAT:
    case SQLITE_RANGE:
    case SQLITE_ROW:
      // None of these appear in error reports, so for now let's not try to
      // guess at how to handle them.
      return false;
  }
  return false;
}

std::string GetCorruptFileDiagnosticsInfo(
    const base::FilePath& corrupted_file_path) {
  std::string corrupted_file_info("Corrupted file: ");
  corrupted_file_info +=
      corrupted_file_path.DirName().BaseName().AsUTF8Unsafe() + "/" +
      corrupted_file_path.BaseName().AsUTF8Unsafe() + "\n";
  return corrupted_file_info;
}

}  // namespace sql
