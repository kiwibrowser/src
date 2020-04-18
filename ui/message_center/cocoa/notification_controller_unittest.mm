// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ui/message_center/cocoa/notification_controller.h"

#include <memory>

#include "base/mac/foundation_util.h"
#include "base/mac/scoped_nsobject.h"
#include "base/macros.h"
#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "third_party/skia/include/core/SkBitmap.h"
#import "ui/base/cocoa/hover_image_button.h"
#import "ui/base/test/cocoa_helper.h"
#include "ui/message_center/fake_message_center.h"
#include "ui/message_center/public/cpp/message_center_constants.h"
#include "ui/message_center/public/cpp/notification.h"
#include "ui/message_center/public/cpp/notification_types.h"

using base::ASCIIToUTF16;
using base::UTF8ToUTF16;

namespace {

class MockMessageCenter : public message_center::FakeMessageCenter {
 public:
  MockMessageCenter()
      : last_removed_by_user_(false),
        remove_count_(0),
        last_clicked_index_(-1) {}

  void RemoveNotification(const std::string& id, bool by_user) override {
    last_removed_id_ = id;
    last_removed_by_user_ = by_user;
    ++remove_count_;
  }

  void ClickOnNotificationButton(const std::string& id,
                                 int button_index) override {
    last_clicked_id_ = id;
    last_clicked_index_ = button_index;
  }

  const std::string& last_removed_id() const { return last_removed_id_; }
  bool last_removed_by_user() const { return last_removed_by_user_; }
  int remove_count() const { return remove_count_; }
  const std::string& last_clicked_id() const { return last_clicked_id_; }
  int last_clicked_index() const { return last_clicked_index_; }

 private:
  std::string last_removed_id_;
  bool last_removed_by_user_;
  int remove_count_;

  std::string last_clicked_id_;
  int last_clicked_index_;

  DISALLOW_COPY_AND_ASSIGN(MockMessageCenter);
};

}  // namespace

@implementation MCNotificationController (TestingInterface)
- (NSButton*)closeButton {
  return closeButton_.get();
}

- (NSImageView*)smallImageView {
  return smallImage_.get();
}

- (NSButton*)secondButton {
  // The buttons are in Cocoa-y-order, so the 2nd button is first.
  NSView* view = [[bottomView_ subviews] objectAtIndex:0];
  return base::mac::ObjCCastStrict<NSButton>(view);
}

- (NSArray*)bottomSubviews {
  return [bottomView_ subviews];
}

- (NSImageView*)iconView {
  return icon_.get();
}

- (NSTextView*)titleView {
  return title_.get();
}

- (NSTextView*)messageView {
  return message_.get();
}

- (NSTextView*)contextMessageView {
  return contextMessage_.get();
}

- (HoverImageButton*)settingsButton {
  return settingsButton_.get();
}

- (NSView*)listView {
  return listView_.get();
}
@end

namespace message_center {

class NotificationControllerTest : public ui::CocoaTest {
 public:
  NSImage* TestIcon() {
    return [NSImage imageNamed:NSImageNameUser];
  }

 protected:
  message_center::NotifierId DummyNotifierId() {
    return message_center::NotifierId();
  }
};

TEST_F(NotificationControllerTest, BasicLayout) {
  std::unique_ptr<message_center::Notification> notification(
      new message_center::Notification(
          message_center::NOTIFICATION_TYPE_SIMPLE, "",
          ASCIIToUTF16("Added to circles"),
          ASCIIToUTF16("Jonathan and 5 others"), gfx::Image(), base::string16(),
          GURL(), DummyNotifierId(), message_center::RichNotificationData(),
          NULL));
  gfx::Image testIcon([TestIcon() retain]);
  notification->set_icon(testIcon);
  notification->set_small_image(testIcon);

  base::scoped_nsobject<MCNotificationController> controller(
      [[MCNotificationController alloc] initWithNotification:notification.get()
                                               messageCenter:NULL]);
  [controller view];

  EXPECT_EQ(TestIcon(), [[controller iconView] image]);
  EXPECT_EQ(TestIcon(), [[controller smallImageView] image]);
  EXPECT_EQ(base::SysNSStringToUTF16([[controller titleView] string]),
            notification->title());
  EXPECT_EQ(base::SysNSStringToUTF16([[controller messageView] string]),
            notification->message());
  EXPECT_EQ(controller.get(), [[controller closeButton] target]);
}

TEST_F(NotificationControllerTest, NotificationSetttingsButtonLayout) {
  message_center::RichNotificationData data;
  data.settings_button_handler = SettingsButtonHandler::INLINE;
  std::unique_ptr<message_center::Notification> notification(
      new message_center::Notification(
          message_center::NOTIFICATION_TYPE_SIMPLE, "",
          ASCIIToUTF16("Added to circles"),
          ASCIIToUTF16("Jonathan and 5 others"), gfx::Image(), base::string16(),
          GURL("https://plus.com"), DummyNotifierId(), data, NULL));

  base::scoped_nsobject<MCNotificationController> controller(
      [[MCNotificationController alloc] initWithNotification:notification.get()
                                               messageCenter:nullptr]);
  [controller view];
  EXPECT_EQ(controller.get(), [[controller settingsButton] target]);
}

TEST_F(NotificationControllerTest, ContextMessageAsDomainNotificationLayout) {
  std::unique_ptr<message_center::Notification> notification(
      new message_center::Notification(
          message_center::NOTIFICATION_TYPE_SIMPLE, "",
          ASCIIToUTF16("Added to circles"),
          ASCIIToUTF16("Jonathan and 5 others"), gfx::Image(), base::string16(),
          GURL("https://plus.com"), DummyNotifierId(),
          message_center::RichNotificationData(), new NotificationDelegate()));
  base::scoped_nsobject<MCNotificationController> controller(
      [[MCNotificationController alloc] initWithNotification:notification.get()
                                               messageCenter:nullptr]);
  [controller view];

  EXPECT_EQ(base::SysNSStringToUTF8([[controller contextMessageView] string]),
            "plus.com");
}

TEST_F(NotificationControllerTest, OverflowText) {
  std::unique_ptr<message_center::Notification> notification(
      new message_center::Notification(
          message_center::NOTIFICATION_TYPE_SIMPLE, "",
          ASCIIToUTF16("This is a much longer title that should wrap "
                       "multiple lines."),
          ASCIIToUTF16("And even the message is long. This sure is a wordy "
                       "notification. Are you really going to read this "
                       "entire thing?"),
          gfx::Image(), base::string16(), GURL(), DummyNotifierId(),
          message_center::RichNotificationData(), NULL));
  base::scoped_nsobject<MCNotificationController> controller(
      [[MCNotificationController alloc] initWithNotification:notification.get()
                                               messageCenter:NULL]);
  [controller view];

  EXPECT_GT(NSHeight([[controller view] frame]),
            message_center::kNotificationIconSize);
}

TEST_F(NotificationControllerTest, Close) {
  std::unique_ptr<message_center::Notification> notification(
      new message_center::Notification(
          message_center::NOTIFICATION_TYPE_SIMPLE, "an_id", base::string16(),
          base::string16(), gfx::Image(), base::string16(), GURL(),
          DummyNotifierId(), message_center::RichNotificationData(), NULL));
  MockMessageCenter message_center;

  base::scoped_nsobject<MCNotificationController> controller(
      [[MCNotificationController alloc] initWithNotification:notification.get()
                                               messageCenter:&message_center]);
  [controller view];

  [[controller closeButton] performClick:nil];

  EXPECT_EQ(1, message_center.remove_count());
  EXPECT_EQ("an_id", message_center.last_removed_id());
  EXPECT_TRUE(message_center.last_removed_by_user());
}

TEST_F(NotificationControllerTest, Update) {
  std::unique_ptr<message_center::Notification> notification(
      new message_center::Notification(
          message_center::NOTIFICATION_TYPE_SIMPLE, "",
          ASCIIToUTF16("A simple title"),
          ASCIIToUTF16("This message isn't too long and should fit in the"
                       "default bounds."),
          gfx::Image(), base::string16(), GURL(), DummyNotifierId(),
          message_center::RichNotificationData(), NULL));
  base::scoped_nsobject<MCNotificationController> controller(
      [[MCNotificationController alloc] initWithNotification:notification.get()
                                               messageCenter:NULL]);

  // Set up the default layout.
  [controller view];
  EXPECT_EQ(NSHeight([[controller view] frame]),
            message_center::kNotificationIconSize);
  EXPECT_FALSE([[controller iconView] image]);
  EXPECT_FALSE([[controller smallImageView] image]);

  // Update the icon.
  gfx::Image testIcon([TestIcon() retain]);
  notification->set_icon(testIcon);
  notification->set_small_image(testIcon);
  [controller updateNotification:notification.get()];
  EXPECT_EQ(TestIcon(), [[controller iconView] image]);
  EXPECT_EQ(TestIcon(), [[controller smallImageView] image]);
  EXPECT_EQ(NSHeight([[controller view] frame]),
            message_center::kNotificationIconSize);
}

TEST_F(NotificationControllerTest, Buttons) {
  message_center::RichNotificationData optional;
  message_center::ButtonInfo button1(UTF8ToUTF16("button1"));
  optional.buttons.push_back(button1);
  message_center::ButtonInfo button2(UTF8ToUTF16("button2"));
  optional.buttons.push_back(button2);

  std::unique_ptr<message_center::Notification> notification(
      new message_center::Notification(
          message_center::NOTIFICATION_TYPE_BASE_FORMAT, "an_id",
          base::string16(), base::string16(), gfx::Image(), base::string16(),
          GURL(), DummyNotifierId(), optional, NULL));
  MockMessageCenter message_center;

  base::scoped_nsobject<MCNotificationController> controller(
      [[MCNotificationController alloc] initWithNotification:notification.get()
                                               messageCenter:&message_center]);
  [controller view];

  [[controller secondButton] performClick:nil];

  EXPECT_EQ("an_id", message_center.last_clicked_id());
  EXPECT_EQ(1, message_center.last_clicked_index());
}

TEST_F(NotificationControllerTest, Image) {
  std::unique_ptr<message_center::Notification> notification(
      new message_center::Notification(
          message_center::NOTIFICATION_TYPE_BASE_FORMAT, "an_id",
          base::string16(), base::string16(), gfx::Image(), base::string16(),
          GURL(), DummyNotifierId(), message_center::RichNotificationData(),
          NULL));
  NSImage* image = [NSImage imageNamed:NSImageNameFolder];
  notification->set_image(gfx::Image([image retain]));

  MockMessageCenter message_center;

  base::scoped_nsobject<MCNotificationController> controller(
      [[MCNotificationController alloc] initWithNotification:notification.get()
                                               messageCenter:&message_center]);
  [controller view];

  ASSERT_EQ(1u, [[controller bottomSubviews] count]);
  ASSERT_TRUE([[[[controller bottomSubviews] lastObject] contentView]
      isKindOfClass:[NSImageView class]]);
  EXPECT_EQ(image,
      [[[[controller bottomSubviews] lastObject] contentView] image]);
}

TEST_F(NotificationControllerTest, List) {
  message_center::RichNotificationData optional;
  message_center::NotificationItem item1{UTF8ToUTF16("First title"),
                                         UTF8ToUTF16("first message")};
  optional.items.push_back(item1);
  message_center::NotificationItem item2{
      UTF8ToUTF16("Second title"),
      UTF8ToUTF16("second slightly longer message")};
  optional.items.push_back(item2);
  message_center::NotificationItem item3{
      UTF8ToUTF16(""),    // Test for empty string.
      UTF8ToUTF16(" ")};  // Test for string containing only spaces.
  optional.items.push_back(item3);
  optional.context_message = UTF8ToUTF16("Context Message");

  std::unique_ptr<message_center::Notification> notification(
      new message_center::Notification(
          message_center::NOTIFICATION_TYPE_BASE_FORMAT, "an_id",
          UTF8ToUTF16("Notification Title"),
          UTF8ToUTF16("Notification Message - should be hidden"), gfx::Image(),
          base::string16(), GURL(), DummyNotifierId(), optional, NULL));

  MockMessageCenter message_center;
  base::scoped_nsobject<MCNotificationController> controller(
      [[MCNotificationController alloc] initWithNotification:notification.get()
                                               messageCenter:&message_center]);
  [controller view];

  EXPECT_FALSE([[controller titleView] isHidden]);
  EXPECT_TRUE([[controller messageView] isHidden]);
  EXPECT_FALSE([[controller contextMessageView] isHidden]);

  EXPECT_EQ(3u, [[[controller listView] subviews] count]);
  EXPECT_LT(NSMaxY([[controller listView] frame]),
            NSMinY([[controller titleView] frame]));
}

TEST_F(NotificationControllerTest, NoMessage) {
  message_center::RichNotificationData optional;
  optional.context_message = UTF8ToUTF16("Context Message");

  std::unique_ptr<message_center::Notification> notification(
      new message_center::Notification(
          message_center::NOTIFICATION_TYPE_BASE_FORMAT, "an_id",
          UTF8ToUTF16("Notification Title"), UTF8ToUTF16(""), gfx::Image(),
          base::string16(), GURL(), DummyNotifierId(), optional, NULL));

  MockMessageCenter message_center;
  base::scoped_nsobject<MCNotificationController> controller(
      [[MCNotificationController alloc] initWithNotification:notification.get()
                                               messageCenter:&message_center]);
  [controller view];

  EXPECT_FALSE([[controller titleView] isHidden]);
  EXPECT_TRUE([[controller messageView] isHidden]);
  EXPECT_FALSE([[controller contextMessageView] isHidden]);
}

TEST_F(NotificationControllerTest, MessageSize) {
  message_center::RichNotificationData data;
  std::string id("id");
  NotifierId notifier_id(NotifierId::APPLICATION, "notifier");
  std::unique_ptr<Notification> notification(new Notification(
      NOTIFICATION_TYPE_BASE_FORMAT, id, base::UTF8ToUTF16(""),
      ASCIIToUTF16("And\neven\nthe\nmessage is long.\nThis sure is wordy"),
      gfx::Image(), base::string16() /* display_source */, GURL(), notifier_id,
      data, NULL /* delegate */));

  base::scoped_nsobject<MCNotificationController> controller(
      [[MCNotificationController alloc] initWithNotification:notification.get()
                                               messageCenter:NULL]);

  // Set up the default layout.
  [controller view];

  auto compute_message_lines = ^{
      NSString* string = [[[controller messageView] textStorage] string];
      unsigned numberOfLines, index, stringLength = [string length];
      for (index = 0, numberOfLines = 0; index < stringLength; numberOfLines++)
        index = NSMaxRange([string lineRangeForRange:NSMakeRange(index, 0)]);

      return numberOfLines;
  };

  // Message and no title: 5 lines.
  EXPECT_EQ(5u, compute_message_lines());

  // Message and one line title: 5 lines.
  notification->set_title(ASCIIToUTF16("one line"));
  [controller updateNotification:notification.get()];
  EXPECT_EQ(5u, compute_message_lines());

  // Message and two line title: 3 lines.
  notification->set_title(ASCIIToUTF16("two\nlines"));
  [controller updateNotification:notification.get()];
  EXPECT_EQ(3u, compute_message_lines());

  // Message, image and no title: 2 lines.
  SkBitmap bitmap;
  bitmap.allocN32Pixels(2, 2);
  bitmap.eraseColor(SK_ColorGREEN);
  notification->set_title(ASCIIToUTF16(""));
  notification->set_image(gfx::Image::CreateFrom1xBitmap(bitmap));
  [controller updateNotification:notification.get()];
  EXPECT_EQ(2u, compute_message_lines());

  // Message, image and one line title: 2 lines.
  notification->set_title(ASCIIToUTF16("one line"));
  [controller updateNotification:notification.get()];
  EXPECT_EQ(2u, compute_message_lines());

  // Message, image and two line title: 1 lines.
  notification->set_title(ASCIIToUTF16("two\nlines"));
  [controller updateNotification:notification.get()];
  EXPECT_EQ(1u, compute_message_lines());

  // Same as above, but context message takes away from message lines.
  notification->set_context_message(UTF8ToUTF16("foo"));
  notification->set_title(ASCIIToUTF16(""));
  [controller updateNotification:notification.get()];
  EXPECT_EQ(1u, compute_message_lines());

  notification->set_title(ASCIIToUTF16("one line"));
  [controller updateNotification:notification.get()];
  EXPECT_EQ(1u, compute_message_lines());

  notification->set_title(ASCIIToUTF16("two\nlines"));
  [controller updateNotification:notification.get()];
  EXPECT_EQ(0u, compute_message_lines());
}

}  // namespace message_center
