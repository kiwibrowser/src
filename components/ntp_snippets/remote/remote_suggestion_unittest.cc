// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/ntp_snippets/remote/remote_suggestion.h"

#include <utility>

#include "base/json/json_reader.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "base/values.h"
#include "components/ntp_snippets/remote/proto/ntp_snippets.pb.h"
#include "components/ntp_snippets/time_serialization.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ntp_snippets {
namespace {

using ::testing::ElementsAre;
using ::testing::Eq;
using ::testing::IsNull;
using ::testing::NotNull;

std::unique_ptr<RemoteSuggestion> SnippetFromContentSuggestionJSON(
    const std::string& json,
    const base::Time& fetch_date) {
  auto json_value = base::JSONReader::Read(json);
  base::DictionaryValue* json_dict;
  if (!json_value->GetAsDictionary(&json_dict)) {
    return nullptr;
  }
  return RemoteSuggestion::CreateFromContentSuggestionsDictionary(
      *json_dict, kArticlesRemoteId, fetch_date);
}

TEST(RemoteSuggestionTest, FromContentSuggestionsDictionary) {
  const std::string kJsonStr = R"(
    {
      "ids" : ["http://localhost/foobar"],
      "title" : "Foo Barred from Baz",
      "snippet" : "...",
      "fullPageUrl" : "http://localhost/foobar",
      "creationTime" : "2016-06-30T11:01:37.000Z",
      "expirationTime" : "2016-07-01T11:01:37.000Z",
      "attribution" : "Foo News",
      "imageUrl" : "http://localhost/foobar.jpg",
      "ampUrl" : "http://localhost/amp",
      "faviconUrl" : "http://localhost/favicon.ico",
      "score": 9001,
      "notificationInfo": {
        "shouldNotify": true,
        "deadline": "2016-06-30T13:01:37.000Z"
      },
      "imageDominantColor": 4289379276
    }
  )";
  const base::Time fetch_date = DeserializeTime(1466634774L);
  auto snippet = SnippetFromContentSuggestionJSON(kJsonStr, fetch_date);
  ASSERT_THAT(snippet, NotNull());

  EXPECT_EQ(snippet->id(), "http://localhost/foobar");
  EXPECT_EQ(snippet->title(), "Foo Barred from Baz");
  EXPECT_EQ(snippet->snippet(), "...");
  EXPECT_EQ(snippet->salient_image_url(), GURL("http://localhost/foobar.jpg"));
  ASSERT_TRUE(snippet->optional_image_dominant_color().has_value());
  EXPECT_EQ(*snippet->optional_image_dominant_color(), 4289379276u);
  EXPECT_EQ(snippet->score(), 9001);
  auto unix_publish_date = snippet->publish_date() - base::Time::UnixEpoch();
  auto expiry_duration = snippet->expiry_date() - snippet->publish_date();
  EXPECT_FLOAT_EQ(unix_publish_date.InSecondsF(), 1467284497.000000f);
  EXPECT_FLOAT_EQ(expiry_duration.InSecondsF(), 86400.000000f);

  EXPECT_EQ(snippet->publisher_name(), "Foo News");
  EXPECT_EQ(snippet->url(), GURL("http://localhost/foobar"));
  EXPECT_EQ(snippet->amp_url(), GURL("http://localhost/amp"));

  EXPECT_TRUE(snippet->should_notify());
  auto notification_duration =
      snippet->notification_deadline() - snippet->publish_date();
  EXPECT_EQ(7200.0f, notification_duration.InSecondsF());
  EXPECT_EQ(fetch_date, snippet->fetch_date());
}

TEST(RemoteSuggestionTest,
     ShouldSupportMultipleIdsFromContentSuggestionsServer) {
  const std::string kJsonStr = R"(
    {
      "ids" : ["http://localhost/foobar", "012345"],
      "title" : "Foo Barred from Baz",
      "snippet" : "...",
      "fullPageUrl" : "http://localhost/foobar",
      "creationTime" : "2016-06-30T11:01:37.000Z",
      "expirationTime" : "2016-07-01T11:01:37.000Z",
      "attribution" : "Foo News",
      "imageUrl" : "http://localhost/foobar.jpg",
      "ampUrl" : "http://localhost/amp",
      "faviconUrl" : "http://localhost/favicon.ico"
    }
  )";
  auto snippet = SnippetFromContentSuggestionJSON(kJsonStr, base::Time());
  ASSERT_THAT(snippet, NotNull());

  EXPECT_EQ(snippet->id(), "http://localhost/foobar");
  EXPECT_THAT(snippet->GetAllIDs(),
              ElementsAre("http://localhost/foobar", "012345"));
}

TEST(RemoteSuggestionTest, CreateFromProtoToProtoRoundtrip) {
  SnippetProto proto;
  proto.add_ids("foo");
  proto.add_ids("bar");
  proto.set_title("a suggestion title");
  proto.set_snippet("the snippet describing the suggestion.");
  proto.set_salient_image_url("http://google.com/logo/");
  proto.set_image_dominant_color(4289379276);
  proto.set_publish_date(1476095492);
  proto.set_expiry_date(1476354691);
  proto.set_score(0.1f);
  proto.set_dismissed(false);
  proto.set_remote_category_id(1);
  proto.set_fetch_date(1476364691);
  proto.set_content_type(SnippetProto_ContentType_VIDEO);
  auto* source = proto.mutable_source();
  source->set_url("http://cool-suggestions.com/");
  source->set_publisher_name("Great Suggestions Inc.");
  source->set_amp_url("http://cdn.ampproject.org/c/foo/");
  proto.set_rank(7);

  std::unique_ptr<RemoteSuggestion> snippet =
      RemoteSuggestion::CreateFromProto(proto);
  ASSERT_THAT(snippet, NotNull());
  // The snippet database relies on the fact that the first id in the protocol
  // buffer is considered the unique id.
  EXPECT_EQ(snippet->id(), "foo");
  // Unfortunately, we only have MessageLite protocol buffers in Chrome, so
  // comparing via DebugString() or MessageDifferencer is not working.
  // So we either need to compare field-by-field (maintenance heavy) or
  // compare the binary version (unusable diagnostic). Deciding for the latter.
  std::string proto_serialized, round_tripped_serialized;
  proto.SerializeToString(&proto_serialized);
  snippet->ToProto().SerializeToString(&round_tripped_serialized);
  EXPECT_EQ(proto_serialized, round_tripped_serialized);
}

TEST(RemoteSuggestionTest, CreateFromProtoIgnoreMissingFetchDate) {
  SnippetProto proto;
  proto.add_ids("foo");
  proto.add_ids("bar");
  proto.set_title("a suggestion title");
  proto.set_snippet("the snippet describing the suggestion.");
  proto.set_salient_image_url("http://google.com/logo/");
  proto.set_publish_date(1476095492);
  proto.set_expiry_date(1476354691);
  proto.set_score(0.1f);
  proto.set_dismissed(false);
  proto.set_remote_category_id(1);
  auto* source = proto.mutable_source();
  source->set_url("http://cool-suggestions.com/");
  source->set_publisher_name("Great Suggestions Inc.");
  source->set_amp_url("http://cdn.ampproject.org/c/foo/");

  std::unique_ptr<RemoteSuggestion> snippet =
      RemoteSuggestion::CreateFromProto(proto);
  ASSERT_THAT(snippet, NotNull());
  EXPECT_EQ(snippet->fetch_date(), base::Time());
}

TEST(RemoteSuggestionTest, CreateFromProtoIgnoreMissingImageDominantColor) {
  SnippetProto proto;
  proto.add_ids("foo");
  proto.add_ids("bar");
  proto.set_title("a suggestion title");
  proto.set_snippet("the snippet describing the suggestion.");
  proto.set_salient_image_url("http://google.com/logo/");
  proto.set_publish_date(1476095492);
  proto.set_expiry_date(1476354691);
  proto.set_score(0.1f);
  proto.set_dismissed(false);
  proto.set_remote_category_id(1);
  auto* source = proto.mutable_source();
  source->set_url("http://cool-suggestions.com/");
  source->set_publisher_name("Great Suggestions Inc.");
  source->set_amp_url("http://cdn.ampproject.org/c/foo/");

  std::unique_ptr<RemoteSuggestion> snippet =
      RemoteSuggestion::CreateFromProto(proto);
  ASSERT_THAT(snippet, NotNull());
  // The snippet database relies on the fact that the first id in the protocol
  // buffer is considered the unique id.
  EXPECT_EQ(snippet->id(), "foo");
  EXPECT_FALSE(snippet->optional_image_dominant_color().has_value());
}

std::unique_ptr<base::DictionaryValue> ContentSuggestionSnippet() {
  const std::string kJsonStr = R"(
    {
      "ids" : ["http://localhost/foobar"],
      "title" : "Foo Barred from Baz",
      "snippet" : "...",
      "fullPageUrl" : "http://localhost/foobar",
      "creationTime" : "2016-06-30T11:01:37.000Z",
      "expirationTime" : "2016-07-01T11:01:37.000Z",
      "attribution" : "Foo News",
      "imageUrl" : "http://localhost/foobar.jpg",
      "ampUrl" : "http://localhost/amp",
      "faviconUrl" : "http://localhost/favicon.ico",
      "score": 9001
    }
  )";
  auto json_value = base::JSONReader::Read(kJsonStr);
  base::DictionaryValue* json_dict;
  CHECK(json_value->GetAsDictionary(&json_dict));
  return json_dict->CreateDeepCopy();
}

TEST(RemoteSuggestionTest, NotifcationInfoAllSpecified) {
  auto json = ContentSuggestionSnippet();
  json->SetBoolean("notificationInfo.shouldNotify", true);
  json->SetString("notificationInfo.deadline", "2016-06-30T13:01:37.000Z");
  auto snippet = RemoteSuggestion::CreateFromContentSuggestionsDictionary(
      *json, 0, base::Time());
  EXPECT_TRUE(snippet->should_notify());
  EXPECT_EQ(7200.0f,
            (snippet->notification_deadline() - snippet->publish_date())
                .InSecondsF());
}

TEST(RemoteSuggestionTest, NotificationInfoDeadlineInvalid) {
  auto json = ContentSuggestionSnippet();
  json->SetBoolean("notificationInfo.shouldNotify", true);
  json->SetInteger("notificationInfo.notificationDeadline", 0);
  auto snippet = RemoteSuggestion::CreateFromContentSuggestionsDictionary(
      *json, 0, base::Time());
  EXPECT_TRUE(snippet->should_notify());
  EXPECT_EQ(base::Time::Max(), snippet->notification_deadline());
}

TEST(RemoteSuggestionTest, NotificationInfoDeadlineAbsent) {
  auto json = ContentSuggestionSnippet();
  json->SetBoolean("notificationInfo.shouldNotify", true);
  auto snippet = RemoteSuggestion::CreateFromContentSuggestionsDictionary(
      *json, 0, base::Time());
  EXPECT_TRUE(snippet->should_notify());
  EXPECT_EQ(base::Time::Max(), snippet->notification_deadline());
}

TEST(RemoteSuggestionTest, NotificationInfoShouldNotifyInvalid) {
  auto json = ContentSuggestionSnippet();
  json->SetString("notificationInfo.shouldNotify", "non-bool");
  auto snippet = RemoteSuggestion::CreateFromContentSuggestionsDictionary(
      *json, 0, base::Time());
  EXPECT_FALSE(snippet->should_notify());
}

TEST(RemoteSuggestionTest, NotificationInfoAbsent) {
  auto json = ContentSuggestionSnippet();
  auto snippet = RemoteSuggestion::CreateFromContentSuggestionsDictionary(
      *json, 0, base::Time());
  EXPECT_FALSE(snippet->should_notify());
}

TEST(RemoteSuggestionTest, ToContentSuggestion) {
  auto json = ContentSuggestionSnippet();
  const base::Time fetch_date = DeserializeTime(1466634774L);
  auto snippet = RemoteSuggestion::CreateFromContentSuggestionsDictionary(
      *json, 0, fetch_date);
  ASSERT_THAT(snippet, NotNull());
  ContentSuggestion sugg = snippet->ToContentSuggestion(
      Category::FromKnownCategory(KnownCategories::ARTICLES));

  EXPECT_THAT(sugg.id().category(),
              Eq(Category::FromKnownCategory(KnownCategories::ARTICLES)));
  EXPECT_THAT(sugg.id().id_within_category(), Eq("http://localhost/foobar"));
  EXPECT_THAT(sugg.url(), Eq(GURL("http://localhost/amp")));
  EXPECT_THAT(sugg.title(), Eq(base::UTF8ToUTF16("Foo Barred from Baz")));
  EXPECT_THAT(sugg.snippet_text(), Eq(base::UTF8ToUTF16("...")));
  EXPECT_THAT(sugg.publish_date().ToJavaTime(), Eq(1467284497000));
  EXPECT_THAT(sugg.publisher_name(), Eq(base::UTF8ToUTF16("Foo News")));
  EXPECT_THAT(sugg.score(), Eq(9001));
  EXPECT_THAT(sugg.download_suggestion_extra(), IsNull());
  EXPECT_THAT(sugg.notification_extra(), IsNull());
  EXPECT_THAT(sugg.fetch_date(), Eq(fetch_date));
}

TEST(RemoteSuggestionTest, ToContentSuggestionWithNotificationInfo) {
  auto json = ContentSuggestionSnippet();
  json->SetBoolean("notificationInfo.shouldNotify", true);
  json->SetString("notificationInfo.deadline", "2016-06-30T13:01:37.000Z");
  auto snippet = RemoteSuggestion::CreateFromContentSuggestionsDictionary(
      *json, 0, base::Time());
  ASSERT_THAT(snippet, NotNull());
  ContentSuggestion sugg = snippet->ToContentSuggestion(
      Category::FromKnownCategory(KnownCategories::ARTICLES));

  EXPECT_THAT(sugg.id().category(),
              Eq(Category::FromKnownCategory(KnownCategories::ARTICLES)));
  EXPECT_THAT(sugg.id().id_within_category(), Eq("http://localhost/foobar"));
  EXPECT_THAT(sugg.url(), Eq(GURL("http://localhost/amp")));
  EXPECT_THAT(sugg.title(), Eq(base::UTF8ToUTF16("Foo Barred from Baz")));
  EXPECT_THAT(sugg.snippet_text(), Eq(base::UTF8ToUTF16("...")));
  EXPECT_THAT(sugg.publish_date().ToJavaTime(), Eq(1467284497000));
  EXPECT_THAT(sugg.publisher_name(), Eq(base::UTF8ToUTF16("Foo News")));
  EXPECT_THAT(sugg.score(), Eq(9001));
  EXPECT_THAT(sugg.download_suggestion_extra(), IsNull());
  ASSERT_THAT(sugg.notification_extra(), NotNull());
  EXPECT_THAT(sugg.notification_extra()->deadline.ToJavaTime(),
              Eq(1467291697000));
}

TEST(RemoteSuggestionTest, ToContentSuggestionWithContentTypeVideo) {
  auto json = ContentSuggestionSnippet();
  json->SetString("contentType", "VIDEO");
  auto snippet = RemoteSuggestion::CreateFromContentSuggestionsDictionary(
      *json, 0, base::Time());
  ASSERT_THAT(snippet, NotNull());
  ContentSuggestion content_suggestion = snippet->ToContentSuggestion(
      Category::FromKnownCategory(KnownCategories::ARTICLES));

  EXPECT_THAT(content_suggestion.is_video_suggestion(), Eq(true));
}

TEST(RemoteSuggestionTest, ToContentSuggestionWithContentTypeUnknown) {
  auto json = ContentSuggestionSnippet();
  json->SetString("contentType", "UNKNOWN");
  auto snippet = RemoteSuggestion::CreateFromContentSuggestionsDictionary(
      *json, 0, base::Time());
  ASSERT_THAT(snippet, NotNull());
  ContentSuggestion content_suggestion = snippet->ToContentSuggestion(
      Category::FromKnownCategory(KnownCategories::ARTICLES));

  EXPECT_THAT(content_suggestion.is_video_suggestion(), Eq(false));
}

TEST(RemoteSuggestionTest, ToContentSuggestionWithMissingContentType) {
  auto json = ContentSuggestionSnippet();
  auto snippet = RemoteSuggestion::CreateFromContentSuggestionsDictionary(
      *json, 0, base::Time());
  ASSERT_THAT(snippet, NotNull());
  ContentSuggestion content_suggestion = snippet->ToContentSuggestion(
      Category::FromKnownCategory(KnownCategories::ARTICLES));

  EXPECT_THAT(content_suggestion.is_video_suggestion(), Eq(false));
}

TEST(RemoteSuggestionTest, ToContentSuggestionWithLargeImageDominantColor) {
  auto json = ContentSuggestionSnippet();
  // JSON does not support unsigned types. As a result the value is parsed as
  // int if it fits and as double otherwise.
  json->SetDouble("imageDominantColor", 4289379276.);
  auto snippet = RemoteSuggestion::CreateFromContentSuggestionsDictionary(
      *json, 0, base::Time());
  ASSERT_THAT(snippet, NotNull());
  ContentSuggestion content_suggestion = snippet->ToContentSuggestion(
      Category::FromKnownCategory(KnownCategories::ARTICLES));

  ASSERT_TRUE(content_suggestion.optional_image_dominant_color().has_value());
  EXPECT_THAT(*content_suggestion.optional_image_dominant_color(),
              Eq(4289379276u));
}

TEST(RemoteSuggestionTest, ToContentSuggestionWithSmallImageDominantColor) {
  auto json = ContentSuggestionSnippet();
  // JSON does not support unsigned types. As a result the value is parsed as
  // int if it fits and as double otherwise.
  json->SetInteger("imageDominantColor", 16777216 /*=0x1000000*/);
  auto snippet = RemoteSuggestion::CreateFromContentSuggestionsDictionary(
      *json, 0, base::Time());
  ASSERT_THAT(snippet, NotNull());
  ContentSuggestion content_suggestion = snippet->ToContentSuggestion(
      Category::FromKnownCategory(KnownCategories::ARTICLES));

  ASSERT_TRUE(content_suggestion.optional_image_dominant_color().has_value());
  EXPECT_THAT(*content_suggestion.optional_image_dominant_color(),
              Eq(16777216u));
}

TEST(RemoteSuggestionTest, ToContentSuggestionWithoutImageDominantColor) {
  auto json = ContentSuggestionSnippet();
  auto snippet = RemoteSuggestion::CreateFromContentSuggestionsDictionary(
      *json, 0, base::Time());
  ASSERT_THAT(snippet, NotNull());
  ContentSuggestion content_suggestion = snippet->ToContentSuggestion(
      Category::FromKnownCategory(KnownCategories::ARTICLES));

  EXPECT_FALSE(content_suggestion.optional_image_dominant_color().has_value());
}

}  // namespace
}  // namespace ntp_snippets
