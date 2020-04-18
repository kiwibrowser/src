// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_INDEXED_DB_INDEXED_DB_DATABASE_ERROR_H_
#define CONTENT_BROWSER_INDEXED_DB_INDEXED_DB_DATABASE_ERROR_H_

#include <stdint.h>

#include "base/strings/string16.h"
#include "base/strings/utf_string_conversions.h"
#include "content/common/content_export.h"

namespace content {

class CONTENT_EXPORT IndexedDBDatabaseError {
 public:
  IndexedDBDatabaseError();
  explicit IndexedDBDatabaseError(uint16_t code);
  IndexedDBDatabaseError(uint16_t code, const char* message);
  IndexedDBDatabaseError(uint16_t code, const base::string16& message);
  ~IndexedDBDatabaseError();

  IndexedDBDatabaseError& operator=(const IndexedDBDatabaseError& rhs);

  uint16_t code() const { return code_; }
  const base::string16& message() const { return message_; }

 private:
  uint16_t code_ = 0;
  base::string16 message_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_INDEXED_DB_INDEXED_DB_DATABASE_ERROR_H_
