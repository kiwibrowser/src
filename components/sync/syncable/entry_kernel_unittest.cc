// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/syncable/entry_kernel.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace syncer {

namespace syncable {

class EntryKernelTest : public testing::Test {};

TEST_F(EntryKernelTest, ToValue) {
  EntryKernel kernel;
  std::unique_ptr<base::DictionaryValue> value(kernel.ToValue(nullptr));
  if (value) {
    // Not much to check without repeating the ToValue() code.
    EXPECT_TRUE(value->HasKey("isDirty"));
    // The extra +2 is for "isDirty" and "modelType".
    EXPECT_EQ(BIT_TEMPS_END - BEGIN_FIELDS + 2,
              static_cast<int>(value->size()));
  } else {
    ADD_FAILURE();
  }
}

bool ProtoFieldValuesEqual(const sync_pb::EntitySpecifics& v1,
                           const sync_pb::EntitySpecifics& v2) {
  return v1.SerializeAsString() == v2.SerializeAsString();
}

bool ProtoFieldValuesAreSame(const sync_pb::EntitySpecifics& v1,
                             const sync_pb::EntitySpecifics& v2) {
  return &v1 == &v2;
}

bool ProtoFieldValueIsDefault(const sync_pb::EntitySpecifics& v) {
  return ProtoFieldValuesAreSame(v,
                                 sync_pb::EntitySpecifics::default_instance());
}

// Tests default value, assignment, and sharing of proto fields.
TEST_F(EntryKernelTest, ProtoFieldTest) {
  EntryKernel kernel;

  // Check default values.
  EXPECT_TRUE(ProtoFieldValueIsDefault(kernel.ref(SPECIFICS)));
  EXPECT_TRUE(ProtoFieldValueIsDefault(kernel.ref(SERVER_SPECIFICS)));
  EXPECT_TRUE(ProtoFieldValueIsDefault(kernel.ref(BASE_SERVER_SPECIFICS)));

  sync_pb::EntitySpecifics specifics;

  // Assign empty value and verify that the field still returns the
  // default value.
  kernel.put(SPECIFICS, specifics);
  EXPECT_TRUE(ProtoFieldValueIsDefault(kernel.ref(SPECIFICS)));
  EXPECT_TRUE(ProtoFieldValuesEqual(kernel.ref(SPECIFICS), specifics));

  // Verifies that the kernel holds the copy of the value assigned to it.
  specifics.mutable_bookmark()->set_url("http://demo/");
  EXPECT_FALSE(ProtoFieldValuesEqual(kernel.ref(SPECIFICS), specifics));

  // Put the new value and verify the equality.
  kernel.put(SPECIFICS, specifics);
  EXPECT_TRUE(ProtoFieldValuesEqual(kernel.ref(SPECIFICS), specifics));
  EXPECT_FALSE(ProtoFieldValueIsDefault(kernel.ref(SPECIFICS)));

  // Copy the value between the fields and verify that exactly the same
  // underlying value is shared.
  kernel.copy(SPECIFICS, SERVER_SPECIFICS);
  EXPECT_TRUE(ProtoFieldValuesEqual(kernel.ref(SERVER_SPECIFICS), specifics));
  EXPECT_TRUE(ProtoFieldValuesAreSame(kernel.ref(SPECIFICS),
                                      kernel.ref(SERVER_SPECIFICS)));

  // Put the new value into SPECIFICS and verify that that stops sharing even
  // though the values are still equal.
  kernel.put(SPECIFICS, specifics);
  EXPECT_FALSE(ProtoFieldValuesAreSame(kernel.ref(SPECIFICS),
                                       kernel.ref(SERVER_SPECIFICS)));
}

}  // namespace syncable

}  // namespace syncer
