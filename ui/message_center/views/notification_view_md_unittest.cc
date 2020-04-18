// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/message_center/views/notification_view_md.h"

#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/compositor/scoped_animation_duration_scale_mode.h"
#include "ui/events/event_processor.h"
#include "ui/events/event_utils.h"
#include "ui/events/test/event_generator.h"
#include "ui/gfx/canvas.h"
#include "ui/message_center/message_center.h"
#include "ui/message_center/public/cpp/message_center_constants.h"
#include "ui/message_center/views/bounded_label.h"
#include "ui/message_center/views/notification_control_buttons_view.h"
#include "ui/message_center/views/notification_header_view.h"
#include "ui/message_center/views/padded_button.h"
#include "ui/message_center/views/proportional_image_view.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/button/radio_button.h"
#include "ui/views/test/views_test_base.h"
#include "ui/views/test/widget_test.h"

namespace message_center {

/* Test fixture ***************************************************************/

// Used to fill bitmaps returned by CreateBitmap().
static const SkColor kBitmapColor = SK_ColorGREEN;

class NotificationTestDelegate : public NotificationDelegate {
 public:
  NotificationTestDelegate() = default;

  void Click(const base::Optional<int>& button_index,
             const base::Optional<base::string16>& reply) override {
    if (!button_index)
      return;

    if (!reply && !expecting_button_click_)
      ADD_FAILURE() << "Click should not be invoked with a button index.";
    if (reply && !expecting_reply_submission_)
      ADD_FAILURE() << "Click should not be invoked with a reply.";

    clicked_button_index_ = *button_index;
    submitted_reply_string_ = reply.value_or(base::string16());
  }

  void Reset() {
    clicked_button_index_ = -1;
    submitted_reply_string_.clear();
  }

  void DisableNotification() override { disable_notification_called_ = true; }

  int clicked_button_index() const { return clicked_button_index_; }
  const base::string16& submitted_reply_string() const {
    return submitted_reply_string_;
  }
  bool disable_notification_called() { return disable_notification_called_; }
  void set_expecting_button_click(bool expecting) {
    expecting_button_click_ = expecting;
  }
  void set_expecting_reply_submission(bool expecting) {
    expecting_reply_submission_ = expecting;
  }

 private:
  ~NotificationTestDelegate() override = default;

  int clicked_button_index_ = -1;
  base::string16 submitted_reply_string_;
  bool expecting_button_click_ = false;
  bool expecting_reply_submission_ = false;
  bool disable_notification_called_ = false;

  DISALLOW_COPY_AND_ASSIGN(NotificationTestDelegate);
};

class NotificationViewMDTest : public views::ViewsTestBase,
                               public views::ViewObserver {
 public:
  NotificationViewMDTest();
  ~NotificationViewMDTest() override;

  // Overridden from ViewsTestBase:
  void SetUp() override;
  void TearDown() override;

  // Overridden from views::ViewObserver:
  void OnViewPreferredSizeChanged(views::View* observed_view) override;

  NotificationViewMD* notification_view() const {
    return notification_view_.get();
  }
  Notification* notification() const { return notification_.get(); }
  views::Widget* widget() const {
    DCHECK_EQ(widget_, notification_view()->GetWidget());
    return widget_;
  }

 protected:
  const gfx::Image CreateTestImage(int width, int height);
  const SkBitmap CreateBitmap(int width, int height);
  std::vector<ButtonInfo> CreateButtons(int number);

  // Paints |view| and returns the size that the original image (which must have
  // been created by CreateBitmap()) was scaled to.
  gfx::Size GetImagePaintSize(ProportionalImageView* view);

  void UpdateNotificationViews();
  float GetNotificationSlideAmount() const;
  bool IsRemoved(const std::string& notification_id) const;
  void DispatchGesture(const ui::GestureEventDetails& details);
  void BeginScroll();
  void EndScroll();
  void ScrollBy(int dx);
  views::View* GetCloseButton();

  std::set<std::string> removed_ids_;
  scoped_refptr<NotificationTestDelegate> delegate_;
  std::unique_ptr<RichNotificationData> data_;
  std::unique_ptr<Notification> notification_;
  std::unique_ptr<NotificationViewMD> notification_view_;
  views::Widget* widget_;

 private:
  DISALLOW_COPY_AND_ASSIGN(NotificationViewMDTest);
};

NotificationViewMDTest::NotificationViewMDTest() = default;
NotificationViewMDTest::~NotificationViewMDTest() = default;

void NotificationViewMDTest::SetUp() {
  views::ViewsTestBase::SetUp();

  MessageCenter::Initialize();

  // Create a dummy notification.
  delegate_ = new NotificationTestDelegate();
  data_.reset(new RichNotificationData());
  data_->settings_button_handler = SettingsButtonHandler::INLINE;
  notification_.reset(new Notification(
      NOTIFICATION_TYPE_BASE_FORMAT, std::string("notification id"),
      base::UTF8ToUTF16("title"), base::UTF8ToUTF16("message"),
      CreateTestImage(80, 80), base::UTF8ToUTF16("display source"), GURL(),
      NotifierId(NotifierId::APPLICATION, "extension_id"), *data_, delegate_));
  notification_->set_small_image(CreateTestImage(16, 16));
  notification_->set_image(CreateTestImage(320, 240));

  // Then create a new NotificationView with that single notification.
  // In the actual code path, this is instantiated by
  // MessageViewFactory::Create.
  // TODO(tetsui): Confirm that NotificationViewMD options are same as one
  // created by the method.
  notification_view_.reset(new NotificationViewMD(*notification_));
  notification_view_->AddObserver(this);
  notification_view_->SetIsNested();
  notification_view_->set_owned_by_client();

  views::Widget::InitParams init_params(
      CreateParams(views::Widget::InitParams::TYPE_POPUP));
  widget_ = new views::Widget();
  widget_->Init(init_params);
  widget_->SetContentsView(notification_view_.get());
  widget_->SetSize(notification_view_->GetPreferredSize());
  widget_->Show();
  widget_->widget_delegate()->set_can_activate(true);
  widget_->Activate();
}

void NotificationViewMDTest::TearDown() {
  notification_view_->SetInkDropMode(MessageView::InkDropMode::OFF);
  notification_view_->RemoveObserver(this);
  widget()->Close();
  notification_view_.reset();
  MessageCenter::Shutdown();
  views::ViewsTestBase::TearDown();
}

void NotificationViewMDTest::OnViewPreferredSizeChanged(
    views::View* observed_view) {
  EXPECT_EQ(observed_view, notification_view());
  widget()->SetSize(notification_view()->GetPreferredSize());
}

const gfx::Image NotificationViewMDTest::CreateTestImage(int width,
                                                         int height) {
  return gfx::Image::CreateFrom1xBitmap(CreateBitmap(width, height));
}

const SkBitmap NotificationViewMDTest::CreateBitmap(int width, int height) {
  SkBitmap bitmap;
  bitmap.allocN32Pixels(width, height);
  bitmap.eraseColor(kBitmapColor);
  return bitmap;
}

std::vector<ButtonInfo> NotificationViewMDTest::CreateButtons(int number) {
  ButtonInfo info(base::ASCIIToUTF16("Test button."));
  info.icon = CreateTestImage(4, 4);
  return std::vector<ButtonInfo>(number, info);
}

gfx::Size NotificationViewMDTest::GetImagePaintSize(
    ProportionalImageView* view) {
  CHECK(view);
  if (view->bounds().IsEmpty())
    return gfx::Size();

  gfx::Size canvas_size = view->bounds().size();
  gfx::Canvas canvas(canvas_size, 1.0 /* image_scale */, true /* is_opaque */);
  static_assert(kBitmapColor != SK_ColorBLACK,
                "The bitmap color must match the background color");
  canvas.DrawColor(SK_ColorBLACK);
  view->OnPaint(&canvas);

  SkBitmap bitmap = canvas.GetBitmap();
  // Incrementally inset each edge at its midpoint to find the bounds of the
  // rect containing the image's color. This assumes that the image is
  // centered in the canvas.
  const int kHalfWidth = canvas_size.width() / 2;
  const int kHalfHeight = canvas_size.height() / 2;
  gfx::Rect rect(canvas_size);
  while (rect.width() > 0 &&
         bitmap.getColor(rect.x(), kHalfHeight) != kBitmapColor)
    rect.Inset(1, 0, 0, 0);
  while (rect.height() > 0 &&
         bitmap.getColor(kHalfWidth, rect.y()) != kBitmapColor)
    rect.Inset(0, 1, 0, 0);
  while (rect.width() > 0 &&
         bitmap.getColor(rect.right() - 1, kHalfHeight) != kBitmapColor)
    rect.Inset(0, 0, 1, 0);
  while (rect.height() > 0 &&
         bitmap.getColor(kHalfWidth, rect.bottom() - 1) != kBitmapColor)
    rect.Inset(0, 0, 0, 1);

  return rect.size();
}

void NotificationViewMDTest::UpdateNotificationViews() {
  MessageCenter::Get()->AddNotification(
      std::make_unique<Notification>(*notification()));
  notification_view()->UpdateWithNotification(*notification());
}

float NotificationViewMDTest::GetNotificationSlideAmount() const {
  return notification_view_->GetSlideOutLayer()
      ->transform()
      .To2dTranslation()
      .x();
}

bool NotificationViewMDTest::IsRemoved(
    const std::string& notification_id) const {
  return !MessageCenter::Get()->FindVisibleNotificationById(notification_id);
}

void NotificationViewMDTest::DispatchGesture(
    const ui::GestureEventDetails& details) {
  ui::test::EventGenerator generator(
      notification_view()->GetWidget()->GetNativeWindow());
  ui::GestureEvent event(0, 0, 0, ui::EventTimeForNow(), details);
  generator.Dispatch(&event);
}

void NotificationViewMDTest::BeginScroll() {
  DispatchGesture(ui::GestureEventDetails(ui::ET_GESTURE_SCROLL_BEGIN));
}

void NotificationViewMDTest::EndScroll() {
  DispatchGesture(ui::GestureEventDetails(ui::ET_GESTURE_SCROLL_END));
}

void NotificationViewMDTest::ScrollBy(int dx) {
  DispatchGesture(ui::GestureEventDetails(ui::ET_GESTURE_SCROLL_UPDATE, dx, 0));
}

views::View* NotificationViewMDTest::GetCloseButton() {
  return notification_view()->GetControlButtonsView()->close_button();
}

/* Unit tests *****************************************************************/

// TODO(tetsui): Following tests are not yet ported from NotificationViewTest.
// * CreateOrUpdateTestSettingsButton
// * TestLineLimits
// * TestImageSizing
// * SettingsButtonTest
// * ViewOrderingTest
// * FormatContextMessageTest

TEST_F(NotificationViewMDTest, CreateOrUpdateTest) {
  EXPECT_NE(nullptr, notification_view()->title_view_);
  EXPECT_NE(nullptr, notification_view()->message_view_);
  EXPECT_NE(nullptr, notification_view()->icon_view_);
  EXPECT_NE(nullptr, notification_view()->image_container_view_);

  notification()->set_image(gfx::Image());
  notification()->set_title(base::string16());
  notification()->set_message(base::string16());
  notification()->set_icon(gfx::Image());

  notification_view()->CreateOrUpdateViews(*notification());

  EXPECT_EQ(nullptr, notification_view()->title_view_);
  EXPECT_EQ(nullptr, notification_view()->message_view_);
  EXPECT_EQ(nullptr, notification_view()->image_container_view_);
  EXPECT_EQ(nullptr, notification_view()->icon_view_);
}

TEST_F(NotificationViewMDTest, TestIconSizing) {
  // TODO(tetsui): Remove duplicated integer literal in CreateOrUpdateIconView.
  const int kNotificationIconSize = 36;

  notification()->set_type(NOTIFICATION_TYPE_SIMPLE);
  ProportionalImageView* view = notification_view()->icon_view_;

  // Icons smaller than the maximum size should remain unscaled.
  notification()->set_icon(
      CreateTestImage(kNotificationIconSize / 2, kNotificationIconSize / 4));
  UpdateNotificationViews();
  EXPECT_EQ(gfx::Size(kNotificationIconSize / 2, kNotificationIconSize / 4)
                .ToString(),
            GetImagePaintSize(view).ToString());

  // Icons of exactly the intended icon size should remain unscaled.
  notification()->set_icon(
      CreateTestImage(kNotificationIconSize, kNotificationIconSize));
  UpdateNotificationViews();
  EXPECT_EQ(gfx::Size(kNotificationIconSize, kNotificationIconSize).ToString(),
            GetImagePaintSize(view).ToString());

  // Icons over the maximum size should be scaled down, maintaining proportions.
  notification()->set_icon(
      CreateTestImage(2 * kNotificationIconSize, 2 * kNotificationIconSize));
  UpdateNotificationViews();
  EXPECT_EQ(gfx::Size(kNotificationIconSize, kNotificationIconSize).ToString(),
            GetImagePaintSize(view).ToString());

  notification()->set_icon(
      CreateTestImage(4 * kNotificationIconSize, 2 * kNotificationIconSize));
  UpdateNotificationViews();
  EXPECT_EQ(
      gfx::Size(kNotificationIconSize, kNotificationIconSize / 2).ToString(),
      GetImagePaintSize(view).ToString());
}

TEST_F(NotificationViewMDTest, UpdateButtonsStateTest) {
  notification()->set_buttons(CreateButtons(2));
  notification_view()->CreateOrUpdateViews(*notification());
  widget()->Show();

  // Action buttons are hidden by collapsed state.
  if (!notification_view()->expanded_)
    notification_view()->ToggleExpanded();
  EXPECT_TRUE(notification_view()->actions_row_->visible());

  EXPECT_EQ(views::Button::STATE_NORMAL,
            notification_view()->action_buttons_[0]->state());

  // Now construct a mouse move event 1 pixel inside the boundary of the action
  // button.
  gfx::Point cursor_location(1, 1);
  views::View::ConvertPointToWidget(notification_view()->action_buttons_[0],
                                    &cursor_location);
  ui::MouseEvent move(ui::ET_MOUSE_MOVED, cursor_location, cursor_location,
                      ui::EventTimeForNow(), ui::EF_NONE, ui::EF_NONE);
  widget()->OnMouseEvent(&move);

  EXPECT_EQ(views::Button::STATE_HOVERED,
            notification_view()->action_buttons_[0]->state());

  notification_view()->CreateOrUpdateViews(*notification());

  EXPECT_EQ(views::Button::STATE_HOVERED,
            notification_view()->action_buttons_[0]->state());

  // Now construct a mouse move event 1 pixel outside the boundary of the
  // widget.
  cursor_location = gfx::Point(-1, -1);
  move = ui::MouseEvent(ui::ET_MOUSE_MOVED, cursor_location, cursor_location,
                        ui::EventTimeForNow(), ui::EF_NONE, ui::EF_NONE);
  widget()->OnMouseEvent(&move);

  EXPECT_EQ(views::Button::STATE_NORMAL,
            notification_view()->action_buttons_[0]->state());
}

TEST_F(NotificationViewMDTest, UpdateButtonCountTest) {
  notification()->set_buttons(CreateButtons(2));
  UpdateNotificationViews();
  widget()->Show();

  // Action buttons are hidden by collapsed state.
  if (!notification_view()->expanded_)
    notification_view()->ToggleExpanded();
  EXPECT_TRUE(notification_view()->actions_row_->visible());

  EXPECT_EQ(views::Button::STATE_NORMAL,
            notification_view()->action_buttons_[0]->state());
  EXPECT_EQ(views::Button::STATE_NORMAL,
            notification_view()->action_buttons_[1]->state());

  // Now construct a mouse move event 1 pixel inside the boundary of the action
  // button.
  gfx::Point cursor_location(1, 1);
  views::View::ConvertPointToScreen(notification_view()->action_buttons_[0],
                                    &cursor_location);
  ui::MouseEvent move(ui::ET_MOUSE_MOVED, cursor_location, cursor_location,
                      ui::EventTimeForNow(), ui::EF_NONE, ui::EF_NONE);
  ui::EventDispatchDetails details =
      views::test::WidgetTest::GetEventSink(widget())->OnEventFromSource(&move);
  EXPECT_FALSE(details.dispatcher_destroyed);

  EXPECT_EQ(views::Button::STATE_HOVERED,
            notification_view()->action_buttons_[0]->state());
  EXPECT_EQ(views::Button::STATE_NORMAL,
            notification_view()->action_buttons_[1]->state());

  notification()->set_buttons(CreateButtons(1));
  UpdateNotificationViews();

  EXPECT_EQ(views::Button::STATE_HOVERED,
            notification_view()->action_buttons_[0]->state());
  EXPECT_EQ(1u, notification_view()->action_buttons_.size());

  // Now construct a mouse move event 1 pixel outside the boundary of the
  // widget.
  cursor_location = gfx::Point(-1, -1);
  move = ui::MouseEvent(ui::ET_MOUSE_MOVED, cursor_location, cursor_location,
                        ui::EventTimeForNow(), ui::EF_NONE, ui::EF_NONE);
  widget()->OnMouseEvent(&move);

  EXPECT_EQ(views::Button::STATE_NORMAL,
            notification_view()->action_buttons_[0]->state());
}

TEST_F(NotificationViewMDTest, TestActionButtonClick) {
  delegate_->set_expecting_button_click(true);

  notification()->set_buttons(CreateButtons(2));
  UpdateNotificationViews();
  widget()->Show();

  ui::test::EventGenerator generator(widget()->GetNativeWindow());

  // Action buttons are hidden by collapsed state.
  if (!notification_view()->expanded_)
    notification_view()->ToggleExpanded();
  EXPECT_TRUE(notification_view()->actions_row_->visible());

  // Now construct a mouse click event 1 pixel inside the boundary of the action
  // button.
  gfx::Point cursor_location(1, 1);
  views::View::ConvertPointToScreen(notification_view()->action_buttons_[1],
                                    &cursor_location);
  generator.MoveMouseTo(cursor_location);
  generator.ClickLeftButton();

  EXPECT_EQ(1, delegate_->clicked_button_index());
}

TEST_F(NotificationViewMDTest, TestInlineReply) {
  delegate_->set_expecting_reply_submission(true);

  std::vector<ButtonInfo> buttons = CreateButtons(2);
  buttons[1].placeholder = base::string16();
  notification()->set_buttons(buttons);
  UpdateNotificationViews();
  widget()->Show();

  ui::test::EventGenerator generator(widget()->GetNativeWindow());

  // Action buttons are hidden by collapsed state.
  if (!notification_view()->expanded_)
    notification_view()->ToggleExpanded();
  EXPECT_TRUE(notification_view()->actions_row_->visible());

  // Now construct a mouse click event 1 pixel inside the boundary of the action
  // button.
  gfx::Point cursor_location(1, 1);
  views::View::ConvertPointToScreen(notification_view()->action_buttons_[1],
                                    &cursor_location);
  generator.MoveMouseTo(cursor_location);
  generator.ClickLeftButton();

  // Nothing should be submitted at this point.
  EXPECT_EQ(-1, delegate_->clicked_button_index());

  // Toggling should hide the inline textfield.
  EXPECT_TRUE(notification_view()->inline_reply_->visible());
  notification_view()->ToggleExpanded();
  notification_view()->ToggleExpanded();
  EXPECT_FALSE(notification_view()->inline_reply_->visible());

  // Click the button again and the inline textfield should be focused.
  generator.ClickLeftButton();
  EXPECT_TRUE(notification_view()->inline_reply_->visible());
  EXPECT_TRUE(notification_view()->inline_reply_->textfield()->visible());
  EXPECT_TRUE(notification_view()->inline_reply_->textfield()->HasFocus());

  // Type the text.
  ui::KeyboardCode keycodes[] = {ui::VKEY_T, ui::VKEY_E, ui::VKEY_S,
                                 ui::VKEY_T};
  for (ui::KeyboardCode keycode : keycodes) {
    generator.PressKey(keycode, ui::EF_NONE);
    generator.ReleaseKey(keycode, ui::EF_NONE);
  }

  // Submit by typing RETURN key.
  generator.PressKey(ui::VKEY_RETURN, ui::EF_NONE);
  generator.ReleaseKey(ui::VKEY_RETURN, ui::EF_NONE);
  EXPECT_EQ(1, delegate_->clicked_button_index());
  EXPECT_EQ(base::ASCIIToUTF16("test"), delegate_->submitted_reply_string());

  // Reset values.
  delegate_->Reset();

  // Now construct a mouse click event 1 pixel inside the boundary of the action
  // button.
  cursor_location = gfx::Point(1, 1);
  views::View::ConvertPointToScreen(notification_view()->action_buttons_[1],
                                    &cursor_location);
  generator.MoveMouseTo(cursor_location);
  generator.ClickLeftButton();

  // Nothing should be submitted at this point.
  EXPECT_EQ(-1, delegate_->clicked_button_index());
  EXPECT_EQ(base::EmptyString16(), delegate_->submitted_reply_string());

  // Click the button again and focus on the inline textfield.
  generator.ClickLeftButton();

  // Type the text.
  for (ui::KeyboardCode keycode : keycodes) {
    generator.PressKey(keycode, ui::EF_NONE);
    generator.ReleaseKey(keycode, ui::EF_NONE);
  }

  // Submit by clicking the reply button.
  cursor_location = gfx::Point(1, 1);
  views::View::ConvertPointToScreen(
      notification_view()->inline_reply_->button(), &cursor_location);
  generator.MoveMouseTo(cursor_location);
  generator.ClickLeftButton();
  EXPECT_EQ(1, delegate_->clicked_button_index());
  EXPECT_EQ(base::ASCIIToUTF16("test"), delegate_->submitted_reply_string());
}

TEST_F(NotificationViewMDTest, SlideOut) {
  ui::ScopedAnimationDurationScaleMode zero_duration_scope(
      ui::ScopedAnimationDurationScaleMode::ZERO_DURATION);

  UpdateNotificationViews();
  std::string notification_id = notification()->id();

  BeginScroll();
  ScrollBy(-10);
  EXPECT_FALSE(IsRemoved(notification_id));
  EXPECT_EQ(-10.f, GetNotificationSlideAmount());
  EndScroll();
  EXPECT_FALSE(IsRemoved(notification_id));
  EXPECT_EQ(0.f, GetNotificationSlideAmount());

  BeginScroll();
  ScrollBy(-200);
  EXPECT_FALSE(IsRemoved(notification_id));
  EXPECT_EQ(-200.f, GetNotificationSlideAmount());
  EndScroll();
  EXPECT_TRUE(IsRemoved(notification_id));
}

TEST_F(NotificationViewMDTest, SlideOutNested) {
  ui::ScopedAnimationDurationScaleMode zero_duration_scope(
      ui::ScopedAnimationDurationScaleMode::ZERO_DURATION);

  UpdateNotificationViews();
  notification_view()->SetIsNested();
  std::string notification_id = notification()->id();

  BeginScroll();
  ScrollBy(-10);
  EXPECT_FALSE(IsRemoved(notification_id));
  EXPECT_EQ(-10.f, GetNotificationSlideAmount());
  EndScroll();
  EXPECT_FALSE(IsRemoved(notification_id));
  EXPECT_EQ(0.f, GetNotificationSlideAmount());

  BeginScroll();
  ScrollBy(-200);
  EXPECT_FALSE(IsRemoved(notification_id));
  EXPECT_EQ(-200.f, GetNotificationSlideAmount());
  EndScroll();
  EXPECT_TRUE(IsRemoved(notification_id));
}

// Pinning notification is ChromeOS only feature.
#if defined(OS_CHROMEOS)

TEST_F(NotificationViewMDTest, SlideOutPinned) {
  ui::ScopedAnimationDurationScaleMode zero_duration_scope(
      ui::ScopedAnimationDurationScaleMode::ZERO_DURATION);

  notification()->set_pinned(true);
  UpdateNotificationViews();
  std::string notification_id = notification()->id();

  BeginScroll();
  ScrollBy(-200);
  EXPECT_FALSE(IsRemoved(notification_id));
  EXPECT_LT(-200.f, GetNotificationSlideAmount());
  EndScroll();
  EXPECT_FALSE(IsRemoved(notification_id));
}

TEST_F(NotificationViewMDTest, Pinned) {
  // Visible at the initial state.
  EXPECT_TRUE(GetCloseButton());
  EXPECT_TRUE(GetCloseButton()->visible());

  // Pin.
  notification()->set_pinned(true);
  UpdateNotificationViews();
  EXPECT_FALSE(GetCloseButton());

  // Unpin.
  notification()->set_pinned(false);
  UpdateNotificationViews();
  EXPECT_TRUE(GetCloseButton());
  EXPECT_TRUE(GetCloseButton()->visible());

  // Pin again.
  notification()->set_pinned(true);
  UpdateNotificationViews();
  EXPECT_FALSE(GetCloseButton());
}

#endif  // defined(OS_CHROMEOS)

TEST_F(NotificationViewMDTest, ExpandLongMessage) {
  notification()->set_type(NotificationType::NOTIFICATION_TYPE_SIMPLE);
  // Test in a case where left_content_ does not have views other than
  // message_view_.
  // Without doing this, inappropriate fix such as
  // message_view_->GetPreferredSize() returning gfx::Size() can pass.
  notification()->set_title(base::string16());
  notification()->set_message(base::ASCIIToUTF16(
      "consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore "
      "et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud "
      "exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat."));

  UpdateNotificationViews();
  EXPECT_FALSE(notification_view()->expanded_);
  const int collapsed_height = notification_view()->message_view_->height();
  const int collapsed_preferred_height =
      notification_view()->GetPreferredSize().height();
  EXPECT_LT(0, collapsed_height);
  EXPECT_LT(0, collapsed_preferred_height);

  notification_view()->ToggleExpanded();
  EXPECT_TRUE(notification_view()->expanded_);
  EXPECT_LT(collapsed_height, notification_view()->message_view_->height());
  EXPECT_LT(collapsed_preferred_height,
            notification_view()->GetPreferredSize().height());

  notification_view()->ToggleExpanded();
  EXPECT_FALSE(notification_view()->expanded_);
  EXPECT_EQ(collapsed_height, notification_view()->message_view_->height());
  EXPECT_EQ(collapsed_preferred_height,
            notification_view()->GetPreferredSize().height());

  // Test |manually_expanded_or_collapsed| being set when the toggle is done by
  // user interaction.
  EXPECT_FALSE(notification_view()->IsManuallyExpandedOrCollapsed());

  // Construct a mouse click event 1 pixel inside the header.
  gfx::Point done_cursor_location(1, 1);
  views::View::ConvertPointToScreen(notification_view()->header_row_,
                                    &done_cursor_location);
  ui::test::EventGenerator generator(widget()->GetNativeWindow());
  generator.MoveMouseTo(done_cursor_location);
  generator.ClickLeftButton();

  EXPECT_TRUE(notification_view()->IsManuallyExpandedOrCollapsed());
}

TEST_F(NotificationViewMDTest, TestAccentColor) {
  constexpr SkColor kActionButtonTextColor = gfx::kGoogleBlue700;
  constexpr SkColor kCustomAccentColor = gfx::kGoogleYellow900;

  notification()->set_buttons(CreateButtons(2));
  UpdateNotificationViews();
  widget()->Show();

  // Action buttons are hidden by collapsed state.
  if (!notification_view()->expanded_)
    notification_view()->ToggleExpanded();
  EXPECT_TRUE(notification_view()->actions_row_->visible());

  // By default, header does not have accent color (default grey), and
  // buttons have default accent color.
  EXPECT_EQ(kNotificationDefaultAccentColor,
            notification_view()->header_row_->accent_color_for_testing());
  EXPECT_EQ(
      kActionButtonTextColor,
      notification_view()->action_buttons_[0]->enabled_color_for_testing());
  EXPECT_EQ(
      kActionButtonTextColor,
      notification_view()->action_buttons_[1]->enabled_color_for_testing());

  // If custom accent color is set, the header and the buttons should have the
  // same accent color.
  notification()->set_accent_color(kCustomAccentColor);
  UpdateNotificationViews();
  EXPECT_EQ(kCustomAccentColor,
            notification_view()->header_row_->accent_color_for_testing());
  EXPECT_EQ(
      kCustomAccentColor,
      notification_view()->action_buttons_[0]->enabled_color_for_testing());
  EXPECT_EQ(
      kCustomAccentColor,
      notification_view()->action_buttons_[1]->enabled_color_for_testing());
}

TEST_F(NotificationViewMDTest, UseImageAsIcon) {
  // TODO(tetsui): Remove duplicated integer literal in CreateOrUpdateIconView.
  const int kNotificationIconSize = 30;

  notification()->set_type(NotificationType::NOTIFICATION_TYPE_IMAGE);
  notification()->set_icon(
      CreateTestImage(kNotificationIconSize, kNotificationIconSize));

  // Test normal notification.
  UpdateNotificationViews();
  EXPECT_FALSE(notification_view()->expanded_);
  EXPECT_TRUE(notification_view()->icon_view_->visible());
  EXPECT_TRUE(notification_view()->right_content_->visible());

  // Icon on the right side is still visible when expanded.
  notification_view()->ToggleExpanded();
  EXPECT_TRUE(notification_view()->expanded_);
  EXPECT_TRUE(notification_view()->icon_view_->visible());
  EXPECT_TRUE(notification_view()->right_content_->visible());

  notification_view()->ToggleExpanded();
  EXPECT_FALSE(notification_view()->expanded_);

  // Test notification with |use_image_for_icon| e.g. screenshot preview.
  notification()->set_icon(gfx::Image());
  UpdateNotificationViews();
  EXPECT_TRUE(notification_view()->icon_view_->visible());
  EXPECT_TRUE(notification_view()->right_content_->visible());

  // Icon on the right side is not visible when expanded.
  notification_view()->ToggleExpanded();
  EXPECT_TRUE(notification_view()->expanded_);
  EXPECT_TRUE(notification_view()->icon_view_->visible());
  EXPECT_FALSE(notification_view()->right_content_->visible());
}

TEST_F(NotificationViewMDTest, NotificationWithoutIcon) {
  notification()->set_icon(gfx::Image());
  notification()->set_image(gfx::Image());
  UpdateNotificationViews();

  // If the notification has no icon, |icon_view_| shouldn't be created.
  EXPECT_FALSE(notification_view()->icon_view_);
  EXPECT_FALSE(notification_view()->right_content_->visible());

  // Toggling should not affect the icon.
  notification_view()->ToggleExpanded();
  EXPECT_FALSE(notification_view()->icon_view_);
  EXPECT_FALSE(notification_view()->right_content_->visible());
}

TEST_F(NotificationViewMDTest, InlineSettings) {
  notification()->set_type(NOTIFICATION_TYPE_SIMPLE);
  UpdateNotificationViews();

  // Inline settings will be shown by clicking settings button.
  EXPECT_FALSE(notification_view()->settings_row_->visible());
  gfx::Point settings_cursor_location(1, 1);
  views::View::ConvertPointToScreen(
      notification_view()->control_buttons_view_->settings_button(),
      &settings_cursor_location);
  ui::test::EventGenerator generator(widget()->GetNativeWindow());
  generator.MoveMouseTo(settings_cursor_location);
  generator.ClickLeftButton();
  EXPECT_TRUE(notification_view()->settings_row_->visible());

  // By clicking settings button again, it will toggle.
  generator.ClickLeftButton();
  EXPECT_FALSE(notification_view()->settings_row_->visible());

  // Show inline settings again.
  generator.ClickLeftButton();
  EXPECT_TRUE(notification_view()->settings_row_->visible());

  // Construct a mouse click event 1 pixel inside the done button.
  gfx::Point done_cursor_location(1, 1);
  views::View::ConvertPointToScreen(notification_view()->settings_done_button_,
                                    &done_cursor_location);
  generator.MoveMouseTo(done_cursor_location);
  generator.ClickLeftButton();

  // Just clicking Done button should not change the setting.
  EXPECT_FALSE(notification_view()->settings_row_->visible());
  EXPECT_FALSE(delegate_->disable_notification_called());

  generator.MoveMouseTo(settings_cursor_location);
  generator.ClickLeftButton();
  EXPECT_TRUE(notification_view()->settings_row_->visible());

  // Construct a mouse click event 1 pixel inside the block all button.
  gfx::Point block_cursor_location(1, 1);
  views::View::ConvertPointToScreen(notification_view()->block_all_button_,
                                    &block_cursor_location);
  generator.MoveMouseTo(block_cursor_location);
  generator.ClickLeftButton();
  generator.MoveMouseTo(done_cursor_location);
  generator.ClickLeftButton();

  EXPECT_FALSE(notification_view()->settings_row_->visible());
  EXPECT_TRUE(delegate_->disable_notification_called());
}

}  // namespace message_center
