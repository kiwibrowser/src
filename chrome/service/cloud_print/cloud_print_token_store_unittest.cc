// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/service/cloud_print/cloud_print_token_store.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace cloud_print {

TEST(CloudPrintTokenStoreTest, Basic) {
  EXPECT_EQ(NULL, CloudPrintTokenStore::current());
  CloudPrintTokenStore* store = new CloudPrintTokenStore;
  EXPECT_EQ(store, CloudPrintTokenStore::current());
  CloudPrintTokenStore::current()->SetToken("myclientlogintoken");
  EXPECT_EQ(CloudPrintTokenStore::current()->token(), "myclientlogintoken");
  delete store;
  EXPECT_EQ(NULL, CloudPrintTokenStore::current());
}

}  // namespace cloud_print
