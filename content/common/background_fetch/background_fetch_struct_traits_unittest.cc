// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/background_fetch/background_fetch_struct_traits.h"

#include <utility>

#include "content/common/background_fetch/background_fetch_types.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {

namespace {

// Creates a new IconDefinition object for the given arguments.
IconDefinition CreateIconDefinition(std::string src,
                                    std::string sizes,
                                    std::string type) {
  IconDefinition definition;
  definition.src = std::move(src);
  definition.sizes = std::move(sizes);
  definition.type = std::move(type);

  return definition;
}

// Returns whether the given IconDefinition objects are identical.
bool IconDefinitionsAreIdentical(const IconDefinition& left,
                                 const IconDefinition& right) {
  return left.src == right.src && left.sizes == right.sizes &&
         left.type == right.type;
}

}  // namespace

TEST(BackgroundFetchStructTraitsTest, BackgroundFetchOptionsRoundtrip) {
  BackgroundFetchOptions options;
  options.icons = {
      CreateIconDefinition("my_icon.png", "256x256", "image/png"),
      CreateIconDefinition("my_small_icon.jpg", "128x128", "image/jpg")};
  options.title = "My Background Fetch";
  options.download_total = 9001;

  BackgroundFetchOptions roundtrip_options;
  ASSERT_TRUE(blink::mojom::BackgroundFetchOptions::Deserialize(
      blink::mojom::BackgroundFetchOptions::Serialize(&options),
      &roundtrip_options));

  ASSERT_EQ(roundtrip_options.icons.size(), options.icons.size());
  for (size_t i = 0; i < options.icons.size(); ++i) {
    EXPECT_TRUE(IconDefinitionsAreIdentical(options.icons[i],
                                            roundtrip_options.icons[i]));
  }

  EXPECT_EQ(roundtrip_options.title, options.title);
  EXPECT_EQ(roundtrip_options.download_total, options.download_total);
}

TEST(BackgroundFetchStructTraitsTest, BackgroundFetchRegistrationRoundTrip) {
  BackgroundFetchRegistration registration;
  registration.developer_id = "my_id";
  registration.unique_id = "7e57ab1e-c0de-a150-ca75-1e75f005ba11";
  registration.download_total = 9001;

  BackgroundFetchRegistration roundtrip_registration;
  ASSERT_TRUE(blink::mojom::BackgroundFetchRegistration::Deserialize(
      blink::mojom::BackgroundFetchRegistration::Serialize(&registration),
      &roundtrip_registration));

  EXPECT_EQ(roundtrip_registration.developer_id, registration.developer_id);
  EXPECT_EQ(roundtrip_registration.unique_id, registration.unique_id);

  EXPECT_EQ(roundtrip_registration.download_total, registration.download_total);
}

TEST(BackgroundFetchStructTraitsTest, IconDefinitionRoundtrip) {
  IconDefinition definition =
      CreateIconDefinition("my_icon.png", "256x256", "image/png");

  IconDefinition roundtrip_definition;
  ASSERT_TRUE(blink::mojom::IconDefinition::Deserialize(
      blink::mojom::IconDefinition::Serialize(&definition),
      &roundtrip_definition));

  EXPECT_TRUE(IconDefinitionsAreIdentical(definition, roundtrip_definition));
}

}  // namespace content
