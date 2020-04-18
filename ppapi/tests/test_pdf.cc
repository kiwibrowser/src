// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/tests/test_pdf.h"

#include "ppapi/c/private/ppb_pdf.h"
#include "ppapi/cpp/image_data.h"
#include "ppapi/cpp/point.h"
#include "ppapi/cpp/private/pdf.h"
#include "ppapi/cpp/var.h"
#include "ppapi/tests/testing_instance.h"

REGISTER_TEST_CASE(PDF);

TestPDF::TestPDF(TestingInstance* instance)
    : TestCase(instance) {
}

void TestPDF::RunTests(const std::string& filter) {
  RUN_TEST(GetV8ExternalSnapshotData, filter);
}

std::string TestPDF::TestGetV8ExternalSnapshotData() {
  const char* natives_data;
  const char* snapshot_data;
  int natives_size;
  int snapshot_size;

  pp::PDF::GetV8ExternalSnapshotData(instance_, &natives_data, &natives_size,
      &snapshot_data, &snapshot_size);
#ifdef V8_USE_EXTERNAL_STARTUP_DATA
  ASSERT_NE(natives_data, (char*) (NULL));
  ASSERT_NE(natives_size, 0);
  ASSERT_NE(snapshot_data, (char*) (NULL));
  ASSERT_NE(snapshot_size, 0);
#else
  ASSERT_EQ(natives_data, (char*) (NULL));
  ASSERT_EQ(natives_size, 0);
  ASSERT_EQ(snapshot_data, (char*) (NULL));
  ASSERT_EQ(snapshot_size, 0);
#endif
  PASS();
}
