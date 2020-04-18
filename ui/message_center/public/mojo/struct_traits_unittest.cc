// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <utility>

#include "base/message_loop/message_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "testing/gmock/include/gmock/gmock-matchers.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/image/image_unittest_util.h"
#include "ui/message_center/public/cpp/notification.h"
#include "ui/message_center/public/cpp/notifier_id.h"
#include "ui/message_center/public/mojo/traits_test_service.mojom.h"

namespace message_center {
namespace {

class StructTraitsTest : public testing::Test, public mojom::TraitsTestService {
 public:
  StructTraitsTest() {}

 protected:
  mojom::TraitsTestServicePtr GetTraitsTestProxy() {
    mojom::TraitsTestServicePtr proxy;
    traits_test_bindings_.AddBinding(this, mojo::MakeRequest(&proxy));
    return proxy;
  }

  void Compare(const Notification& input, const Notification& output) {
    EXPECT_EQ(input.type(), output.type());
    EXPECT_EQ(input.id(), output.id());
    EXPECT_EQ(input.title(), output.title());
    EXPECT_EQ(input.message(), output.message());
    EXPECT_EQ(input.icon().Width(), output.icon().Width());
    EXPECT_EQ(input.icon().Height(), output.icon().Height());
    EXPECT_EQ(input.display_source(), output.display_source());
    EXPECT_EQ(input.origin_url(), output.origin_url());
    EXPECT_EQ(input.notifier_id(), output.notifier_id());
    EXPECT_EQ(input.priority(), output.priority());
    EXPECT_TRUE(gfx::test::AreImagesEqual(input.image(), output.image()));
    EXPECT_TRUE(
        gfx::test::AreImagesEqual(input.small_image(), output.small_image()));
    EXPECT_EQ(input.timestamp(), output.timestamp());
    EXPECT_EQ(input.progress(), output.progress());
    EXPECT_EQ(input.progress_status(), output.progress_status());
    EXPECT_EQ(input.rich_notification_data()
                  .should_make_spoken_feedback_for_popup_updates,
              output.rich_notification_data()
                  .should_make_spoken_feedback_for_popup_updates);
    EXPECT_EQ(input.pinned(), output.pinned());
    EXPECT_EQ(input.renotify(), output.renotify());
    EXPECT_EQ(input.accessible_name(), output.accessible_name());
    EXPECT_EQ(input.accent_color(), output.accent_color());
    EXPECT_EQ(input.should_show_settings_button(),
              output.should_show_settings_button());
    EXPECT_EQ(input.fullscreen_visibility(), output.fullscreen_visibility());
  }

 private:
  // TraitsTestService:
  void EchoNotification(const Notification& n,
                        EchoNotificationCallback callback) override {
    std::move(callback).Run(n);
  }

  base::MessageLoop loop_;
  mojo::BindingSet<TraitsTestService> traits_test_bindings_;

  DISALLOW_COPY_AND_ASSIGN(StructTraitsTest);
};

}  // namespace

TEST_F(StructTraitsTest, Notification) {
  NotificationType type = NotificationType::NOTIFICATION_TYPE_SIMPLE;
  std::string id("notification_id");
  base::string16 title(base::ASCIIToUTF16("Notification"));
  base::string16 message(base::ASCIIToUTF16("I'm a notification!"));
  gfx::Image icon(gfx::test::CreateImage(64, 64));
  base::string16 display_source(base::ASCIIToUTF16("display_source"));
  GURL origin_url("www.example.com");
  NotifierId notifier_id(NotifierId::NotifierType::APPLICATION, id);
  notifier_id.profile_id = "profile_id";
  RichNotificationData optional_fields;
  optional_fields.settings_button_handler = SettingsButtonHandler::INLINE;
  Notification input(type, id, title, message, icon, display_source, origin_url,
                     notifier_id, optional_fields, nullptr);

  mojom::TraitsTestServicePtr proxy = GetTraitsTestProxy();
  Notification output;
  proxy->EchoNotification(input, &output);
  Compare(input, output);

  // Set some optional fields to non-default values and test again.
  input.set_never_timeout(true);
  input.set_type(NotificationType::NOTIFICATION_TYPE_PROGRESS);
  input.set_progress(50);
  input.set_progress_status(base::ASCIIToUTF16("progress text"));
  input.set_image(gfx::test::CreateImage(48, 48));
  input.set_small_image(gfx::test::CreateImage(16, 16));
  input.set_accent_color(SK_ColorMAGENTA);
  input.set_fullscreen_visibility(FullscreenVisibility::OVER_USER);
  proxy->EchoNotification(input, &output);
  Compare(input, output);
}

TEST_F(StructTraitsTest, EmptyIconNotification) {
  NotificationType type = NotificationType::NOTIFICATION_TYPE_SIMPLE;
  std::string id("notification_id");
  base::string16 title(base::ASCIIToUTF16("Notification"));
  base::string16 message(
      base::ASCIIToUTF16("I'm a notification with no icon!"));
  base::string16 display_source(base::ASCIIToUTF16("display_source"));
  GURL origin_url("www.example.com");
  NotifierId notifier_id(NotifierId::NotifierType::APPLICATION, id);
  Notification input(type, id, title, message, gfx::Image(), display_source,
                     origin_url, notifier_id, RichNotificationData(), nullptr);
  mojom::TraitsTestServicePtr proxy = GetTraitsTestProxy();
  Notification output;
  proxy->EchoNotification(input, &output);
  Compare(input, output);
}

}  // namespace message_center
