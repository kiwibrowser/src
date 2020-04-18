// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/chromeos/ime/candidate_view.h"

#include <stddef.h>

#include "base/logging.h"
#include "base/macros.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/aura/window.h"
#include "ui/events/test/event_generator.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/test/views_test_base.h"
#include "ui/views/widget/widget_delegate.h"

namespace ui {
namespace ime {
namespace {

const char* const kDummyCandidates[] = {
  "candidate1",
  "candidate2",
  "candidate3",
};

}  // namespace

class CandidateViewTest : public views::ViewsTestBase,
                          public views::ButtonListener {
 public:
  CandidateViewTest() : widget_(NULL), last_pressed_(NULL) {}
  ~CandidateViewTest() override {}

  void SetUp() override {
    views::ViewsTestBase::SetUp();

    views::Widget::InitParams init_params(CreateParams(
        views::Widget::InitParams::TYPE_WINDOW));

    init_params.delegate = new views::WidgetDelegateView();

    container_ = init_params.delegate->GetContentsView();
    container_->SetLayoutManager(
        std::make_unique<views::BoxLayout>(views::BoxLayout::kVertical));
    for (size_t i = 0; i < arraysize(kDummyCandidates); ++i) {
      CandidateView* candidate = new CandidateView(
          this, ui::CandidateWindow::VERTICAL);
      ui::CandidateWindow::Entry entry;
      entry.value = base::UTF8ToUTF16(kDummyCandidates[i]);
      candidate->SetEntry(entry);
      container_->AddChildView(candidate);
    }

    widget_ = new views::Widget();
    widget_->Init(init_params);
    widget_->Show();

    aura::Window* native_window = widget_->GetNativeWindow();
    event_generator_.reset(new ui::test::EventGenerator(
        native_window->GetRootWindow(), native_window));
  }

  void TearDown() override {
    widget_->Close();

    views::ViewsTestBase::TearDown();
  }

 protected:
  CandidateView* GetCandidateAt(int index) {
    return static_cast<CandidateView*>(container_->child_at(index));
  }

  int GetHighlightedIndex(int* highlighted_count) const {
    *highlighted_count = 0;
    int last_highlighted = -1;
    for (int i = 0; i < container_->child_count(); ++i) {
      if (container_->child_at(i)->background() != NULL) {
        (*highlighted_count)++;
        last_highlighted = i;
      }
    }
    return last_highlighted;
  }

  int GetLastPressedIndexAndReset() {
    for (int i = 0; i < container_->child_count(); ++i) {
      if (last_pressed_ == container_->child_at(i)) {
        last_pressed_ = NULL;
        return i;
      }
    }

    DCHECK(last_pressed_ == NULL);
    last_pressed_ = NULL;
    return -1;
  }

  ui::test::EventGenerator* event_generator() { return event_generator_.get(); }

 private:
  void ButtonPressed(views::Button* sender, const ui::Event& event) override {
    last_pressed_ = sender;
  }

  views::Widget* widget_;
  views::View* container_;
  std::unique_ptr<ui::test::EventGenerator> event_generator_;
  views::View* last_pressed_;

  DISALLOW_COPY_AND_ASSIGN(CandidateViewTest);
};

TEST_F(CandidateViewTest, MouseHovers) {
  GetCandidateAt(0)->SetHighlighted(true);

  int highlighted_count = 0;
  EXPECT_EQ(0, GetHighlightedIndex(&highlighted_count));
  EXPECT_EQ(1, highlighted_count);

  // Mouse hover shouldn't change the background.
  event_generator()->MoveMouseTo(
      GetCandidateAt(0)->GetBoundsInScreen().CenterPoint());
  EXPECT_EQ(0, GetHighlightedIndex(&highlighted_count));
  EXPECT_EQ(1, highlighted_count);

  // Mouse hover shouldn't change the background.
  event_generator()->MoveMouseTo(
      GetCandidateAt(1)->GetBoundsInScreen().CenterPoint());
  EXPECT_EQ(0, GetHighlightedIndex(&highlighted_count));
  EXPECT_EQ(1, highlighted_count);

  // Mouse hover shouldn't change the background.
  event_generator()->MoveMouseTo(
      GetCandidateAt(2)->GetBoundsInScreen().CenterPoint());
  EXPECT_EQ(0, GetHighlightedIndex(&highlighted_count));
  EXPECT_EQ(1, highlighted_count);
}

TEST_F(CandidateViewTest, MouseClick) {
  event_generator()->MoveMouseTo(
      GetCandidateAt(1)->GetBoundsInScreen().CenterPoint());
  event_generator()->ClickLeftButton();
  EXPECT_EQ(1, GetLastPressedIndexAndReset());
}

TEST_F(CandidateViewTest, ClickAndMove) {
  GetCandidateAt(0)->SetHighlighted(true);

  int highlighted_count = 0;
  EXPECT_EQ(0, GetHighlightedIndex(&highlighted_count));
  EXPECT_EQ(1, highlighted_count);

  event_generator()->MoveMouseTo(
      GetCandidateAt(2)->GetBoundsInScreen().CenterPoint());
  event_generator()->PressLeftButton();
  EXPECT_EQ(2, GetHighlightedIndex(&highlighted_count));
  EXPECT_EQ(1, highlighted_count);

  // Highlight follows the drag.
  event_generator()->MoveMouseTo(
      GetCandidateAt(1)->GetBoundsInScreen().CenterPoint());
  EXPECT_EQ(1, GetHighlightedIndex(&highlighted_count));
  EXPECT_EQ(1, highlighted_count);

  event_generator()->MoveMouseTo(
      GetCandidateAt(0)->GetBoundsInScreen().CenterPoint());
  EXPECT_EQ(0, GetHighlightedIndex(&highlighted_count));
  EXPECT_EQ(1, highlighted_count);

  event_generator()->MoveMouseTo(
      GetCandidateAt(1)->GetBoundsInScreen().CenterPoint());
  EXPECT_EQ(1, GetHighlightedIndex(&highlighted_count));
  EXPECT_EQ(1, highlighted_count);

  event_generator()->ReleaseLeftButton();
  EXPECT_EQ(1, GetLastPressedIndexAndReset());
}

}  // namespace ime
}  // namespace ui
