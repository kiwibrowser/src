// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SQL_TEST_SQL_TEST_SUITE_H_
#define SQL_TEST_SQL_TEST_SUITE_H_

#include "base/macros.h"
#include "base/test/test_suite.h"

namespace sql {

class SQLTestSuite : public base::TestSuite {
 public:
  SQLTestSuite(int argc, char** argv);
  ~SQLTestSuite() override;

 protected:
  // Overridden from base::TestSuite:
  void Initialize() override;
  void Shutdown() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(SQLTestSuite);
};

}  // namespace sql

#endif  // SQL_TEST_SQL_TEST_SUITE_H_
