// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/app_list/pagination_model.h"

#include <string>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "base/strings/string_number_conversions.h"
#include "base/test/scoped_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/app_list/pagination_model_observer.h"

namespace app_list {
namespace test {

class TestPaginationModelObserver : public PaginationModelObserver {
 public:
  TestPaginationModelObserver()
      : model_(NULL),
        expected_page_selection_(0),
        expected_transition_start_(0),
        expected_transition_end_(0),
        transition_page_(-1) {
    Reset();
  }
  ~TestPaginationModelObserver() override {}

  void Reset() {
    selection_count_ = 0;
    transition_start_count_ = 0;
    transition_end_count_ = 0;
    selected_pages_.clear();
  }

  void set_model(PaginationModel* model) { model_ = model; }

  void set_expected_page_selection(int expected_page_selection) {
    expected_page_selection_ = expected_page_selection;
  }
  void set_expected_transition_start(int expected_transition_start) {
    expected_transition_start_ = expected_transition_start;
  }
  void set_expected_transition_end(int expected_transition_end) {
    expected_transition_end_ = expected_transition_end;
  }
  void set_transition_page(int page) { transition_page_ = page; }

  const std::string& selected_pages() const { return selected_pages_; }
  int selection_count() const { return selection_count_; }
  int transition_start_count() const { return transition_start_count_; }
  int transition_end_count() const { return transition_end_count_; }

 private:
  void AppendSelectedPage(int page) {
    if (selected_pages_.length())
      selected_pages_.append(std::string(" "));
    selected_pages_.append(base::IntToString(page));
  }

  // PaginationModelObserver overrides:
  void TotalPagesChanged() override {}
  void SelectedPageChanged(int old_selected, int new_selected) override {
    AppendSelectedPage(new_selected);
    ++selection_count_;
    if (expected_page_selection_ &&
        selection_count_ == expected_page_selection_) {
      base::RunLoop::QuitCurrentWhenIdleDeprecated();
    }
  }

  void TransitionStarted() override {}

  void TransitionChanged() override {
    if (transition_page_ == -1 ||
        model_->transition().target_page == transition_page_) {
      if (model_->transition().progress == 0)
        ++transition_start_count_;
      if (model_->transition().progress == 1)
        ++transition_end_count_;
    }

    if ((expected_transition_start_ &&
         transition_start_count_ == expected_transition_start_) ||
        (expected_transition_end_ &&
         transition_end_count_ == expected_transition_end_)) {
      base::RunLoop::QuitCurrentWhenIdleDeprecated();
    }
  }

  void TransitionEnded() override {}

  PaginationModel* model_;

  int expected_page_selection_;
  int expected_transition_start_;
  int expected_transition_end_;

  int selection_count_;
  int transition_start_count_;
  int transition_end_count_;

  // Indicate which page index should be counted for |transition_start_count_|
  // and |transition_end_count_|. -1 means all the pages should be counted.
  int transition_page_;

  std::string selected_pages_;

  DISALLOW_COPY_AND_ASSIGN(TestPaginationModelObserver);
};

class PaginationModelTest : public testing::Test {
 public:
  PaginationModelTest()
      : scoped_task_environment_(
            base::test::ScopedTaskEnvironment::MainThreadType::UI) {}
  ~PaginationModelTest() override {}

  // testing::Test overrides:
  void SetUp() override {
    pagination_.SetTotalPages(5);
    pagination_.SetTransitionDurations(1, 1);
    observer_.set_model(&pagination_);
    pagination_.AddObserver(&observer_);
  }
  void TearDown() override {
    pagination_.RemoveObserver(&observer_);
    observer_.set_model(NULL);
  }

 protected:
  void SetStartPageAndExpects(int start_page,
                              int expected_selection,
                              int expected_transition_start,
                              int expected_transition_end) {
    observer_.set_expected_page_selection(0);
    pagination_.SelectPage(start_page, false /* animate */);
    observer_.Reset();
    observer_.set_expected_page_selection(expected_selection);
    observer_.set_expected_transition_start(expected_transition_start);
    observer_.set_expected_transition_end(expected_transition_end);
  }

  PaginationModel pagination_;
  TestPaginationModelObserver observer_;

 private:
  base::test::ScopedTaskEnvironment scoped_task_environment_;

  DISALLOW_COPY_AND_ASSIGN(PaginationModelTest);
};

TEST_F(PaginationModelTest, SelectPage) {
  pagination_.SelectPage(2, false /* animate */);
  pagination_.SelectPage(4, false /* animate */);
  pagination_.SelectPage(3, false /* animate */);
  pagination_.SelectPage(1, false /* animate */);

  EXPECT_EQ(0, observer_.transition_start_count());
  EXPECT_EQ(0, observer_.transition_end_count());
  EXPECT_EQ(4, observer_.selection_count());
  EXPECT_EQ(std::string("2 4 3 1"), observer_.selected_pages());

  // Nothing happens if select the same page.
  pagination_.SelectPage(1, false /* animate */);
  EXPECT_EQ(4, observer_.selection_count());
  EXPECT_EQ(std::string("2 4 3 1"), observer_.selected_pages());
}

TEST_F(PaginationModelTest, IsValidPageRelative) {
  pagination_.SelectPage(0, false /*animate*/);
  ASSERT_FALSE(pagination_.IsValidPageRelative(-1));
  ASSERT_FALSE(pagination_.IsValidPageRelative(5));
  ASSERT_TRUE(pagination_.IsValidPageRelative(1));
  ASSERT_TRUE(pagination_.IsValidPageRelative(4));
}

TEST_F(PaginationModelTest, SelectPageAnimated) {
  const int kStartPage = 0;

  // One transition.
  SetStartPageAndExpects(kStartPage, 1, 0, 0);
  pagination_.SelectPage(1, true /* animate */);
  base::RunLoop().Run();
  EXPECT_EQ(1, observer_.transition_start_count());
  EXPECT_EQ(1, observer_.transition_end_count());
  EXPECT_EQ(1, observer_.selection_count());
  EXPECT_EQ(std::string("1"), observer_.selected_pages());

  // Two transitions in a row.
  SetStartPageAndExpects(kStartPage, 2, 0, 0);
  pagination_.SelectPage(1, true /* animate */);
  pagination_.SelectPage(3, true /* animate */);
  base::RunLoop().Run();
  EXPECT_EQ(2, observer_.transition_start_count());
  EXPECT_EQ(2, observer_.transition_end_count());
  EXPECT_EQ(2, observer_.selection_count());
  EXPECT_EQ(std::string("1 3"), observer_.selected_pages());

  // Transition to same page twice and only one should happen.
  SetStartPageAndExpects(kStartPage, 1, 0, 0);
  pagination_.SelectPage(1, true /* animate */);
  pagination_.SelectPage(1, true /* animate */);  // Ignored.
  base::RunLoop().Run();
  EXPECT_EQ(1, observer_.transition_start_count());
  EXPECT_EQ(1, observer_.transition_end_count());
  EXPECT_EQ(1, observer_.selection_count());
  EXPECT_EQ(std::string("1"), observer_.selected_pages());

  // More than two transitions and only the first and last would happen.
  SetStartPageAndExpects(kStartPage, 2, 0, 0);
  pagination_.SelectPage(1, true /* animate */);
  pagination_.SelectPage(3, true /* animate */);  // Ignored
  pagination_.SelectPage(4, true /* animate */);  // Ignored
  pagination_.SelectPage(2, true /* animate */);
  base::RunLoop().Run();
  EXPECT_EQ(2, observer_.transition_start_count());
  EXPECT_EQ(2, observer_.transition_end_count());
  EXPECT_EQ(2, observer_.selection_count());
  EXPECT_EQ(std::string("1 2"), observer_.selected_pages());

  // Multiple transitions with one transition that goes back to the original
  // and followed by a new transition. Two transitions would happen. The first
  // one will be reversed by the kStart transition and the second one will be
  // finished.
  SetStartPageAndExpects(kStartPage, 1, 0, 0);
  pagination_.SelectPage(1, true /* animate */);
  pagination_.SelectPage(2, true /* animate */);  // Ignored
  pagination_.SelectPage(kStartPage, true /* animate */);
  pagination_.SelectPage(3, true /* animate */);
  base::RunLoop().Run();
  EXPECT_EQ(std::string("3"), observer_.selected_pages());
}

TEST_F(PaginationModelTest, SimpleScroll) {
  const int kStartPage = 2;

  // Scroll to the next page (negative delta) and finish it.
  SetStartPageAndExpects(kStartPage, 1, 0, 0);
  pagination_.StartScroll();
  pagination_.UpdateScroll(-0.1);
  EXPECT_EQ(kStartPage + 1, pagination_.transition().target_page);
  pagination_.EndScroll(false);  // Finish transition
  base::RunLoop().Run();
  EXPECT_EQ(1, observer_.selection_count());

  // Scroll to the previous page (positive delta) and finish it.
  SetStartPageAndExpects(kStartPage, 1, 0, 0);
  pagination_.StartScroll();
  pagination_.UpdateScroll(0.1);
  EXPECT_EQ(kStartPage - 1, pagination_.transition().target_page);
  pagination_.EndScroll(false);  // Finish transition
  base::RunLoop().Run();
  EXPECT_EQ(1, observer_.selection_count());

  // Scroll to the next page (negative delta) and cancel it.
  SetStartPageAndExpects(kStartPage, 0, 1, 0);
  pagination_.StartScroll();
  pagination_.UpdateScroll(-0.1);
  EXPECT_EQ(kStartPage + 1, pagination_.transition().target_page);
  pagination_.EndScroll(true);  // Cancel transition
  base::RunLoop().Run();
  EXPECT_EQ(0, observer_.selection_count());

  // Scroll to the previous page (position delta) and cancel it.
  SetStartPageAndExpects(kStartPage, 0, 1, 0);
  pagination_.StartScroll();
  pagination_.UpdateScroll(0.1);
  EXPECT_EQ(kStartPage - 1, pagination_.transition().target_page);
  pagination_.EndScroll(true);  // Cancel transition
  base::RunLoop().Run();
  EXPECT_EQ(0, observer_.selection_count());
}

TEST_F(PaginationModelTest, ScrollWithTransition) {
  const int kStartPage = 2;

  // Scroll to the next page (negative delta) with a transition in the same
  // direction.
  SetStartPageAndExpects(kStartPage, 1, 0, 0);
  pagination_.SetTransition(PaginationModel::Transition(kStartPage + 1, 0.5));
  pagination_.StartScroll();
  pagination_.UpdateScroll(-0.1);
  EXPECT_EQ(kStartPage + 1, pagination_.transition().target_page);
  EXPECT_EQ(0.6, pagination_.transition().progress);
  pagination_.EndScroll(false);
  base::RunLoop().Run();
  EXPECT_EQ(1, observer_.selection_count());

  // Scroll to the next page (negative delta) with a transition in a different
  // direction.
  SetStartPageAndExpects(kStartPage, 0, 1, 0);
  pagination_.SetTransition(PaginationModel::Transition(kStartPage - 1, 0.5));
  pagination_.StartScroll();
  pagination_.UpdateScroll(-0.1);
  EXPECT_EQ(kStartPage - 1, pagination_.transition().target_page);
  EXPECT_EQ(0.4, pagination_.transition().progress);
  pagination_.EndScroll(true);

  // Scroll to the previous page (positive delta) with a transition in the same
  // direction.
  SetStartPageAndExpects(kStartPage, 1, 0, 0);
  pagination_.SetTransition(PaginationModel::Transition(kStartPage - 1, 0.5));
  pagination_.StartScroll();
  pagination_.UpdateScroll(0.1);
  EXPECT_EQ(kStartPage - 1, pagination_.transition().target_page);
  EXPECT_EQ(0.6, pagination_.transition().progress);
  pagination_.EndScroll(false);
  base::RunLoop().Run();
  EXPECT_EQ(1, observer_.selection_count());

  // Scroll to the previous page (positive delta) with a transition in a
  // different direction.
  SetStartPageAndExpects(kStartPage, 0, 1, 0);
  pagination_.SetTransition(PaginationModel::Transition(kStartPage + 1, 0.5));
  pagination_.StartScroll();
  pagination_.UpdateScroll(0.1);
  EXPECT_EQ(kStartPage + 1, pagination_.transition().target_page);
  EXPECT_EQ(0.4, pagination_.transition().progress);
  pagination_.EndScroll(true);
}

TEST_F(PaginationModelTest, LongScroll) {
  const int kStartPage = 2;

  // Scroll to the next page (negative delta) with a transition in the same
  // direction. And scroll enough to change page twice.
  SetStartPageAndExpects(kStartPage, 2, 0, 0);
  pagination_.SetTransition(PaginationModel::Transition(kStartPage + 1, 0.5));
  pagination_.StartScroll();
  pagination_.UpdateScroll(-0.1);
  EXPECT_EQ(kStartPage + 1, pagination_.transition().target_page);
  EXPECT_EQ(0.6, pagination_.transition().progress);
  pagination_.UpdateScroll(-0.5);
  EXPECT_EQ(1, observer_.selection_count());
  pagination_.UpdateScroll(-0.5);
  EXPECT_EQ(kStartPage + 2, pagination_.transition().target_page);
  pagination_.EndScroll(false);
  base::RunLoop().Run();
  EXPECT_EQ(2, observer_.selection_count());

  // Scroll to the next page (negative delta) with a transition in a different
  // direction. And scroll enough to revert it and switch page once.
  SetStartPageAndExpects(kStartPage, 1, 0, 0);
  pagination_.SetTransition(PaginationModel::Transition(kStartPage - 1, 0.5));
  pagination_.StartScroll();
  pagination_.UpdateScroll(-0.1);
  EXPECT_EQ(kStartPage - 1, pagination_.transition().target_page);
  EXPECT_EQ(0.4, pagination_.transition().progress);
  pagination_.UpdateScroll(-0.5);  // This clears the transition.
  pagination_.UpdateScroll(-0.5);  // This starts a new transition.
  EXPECT_EQ(kStartPage + 1, pagination_.transition().target_page);
  pagination_.EndScroll(false);
  base::RunLoop().Run();
  EXPECT_EQ(1, observer_.selection_count());

  // Similar cases as above but in the opposite direction.
  // Scroll to the previous page (positive delta) with a transition in the same
  // direction. And scroll enough to change page twice.
  SetStartPageAndExpects(kStartPage, 2, 0, 0);
  pagination_.SetTransition(PaginationModel::Transition(kStartPage - 1, 0.5));
  pagination_.StartScroll();
  pagination_.UpdateScroll(0.1);
  EXPECT_EQ(kStartPage - 1, pagination_.transition().target_page);
  EXPECT_EQ(0.6, pagination_.transition().progress);
  pagination_.UpdateScroll(0.5);
  EXPECT_EQ(1, observer_.selection_count());
  pagination_.UpdateScroll(0.5);
  EXPECT_EQ(kStartPage - 2, pagination_.transition().target_page);
  pagination_.EndScroll(false);
  base::RunLoop().Run();
  EXPECT_EQ(2, observer_.selection_count());

  // Scroll to the previous page (positive delta) with a transition in a
  // different direction. And scroll enough to revert it and switch page once.
  SetStartPageAndExpects(kStartPage, 1, 0, 0);
  pagination_.SetTransition(PaginationModel::Transition(kStartPage + 1, 0.5));
  pagination_.StartScroll();
  pagination_.UpdateScroll(0.1);
  EXPECT_EQ(kStartPage + 1, pagination_.transition().target_page);
  EXPECT_EQ(0.4, pagination_.transition().progress);
  pagination_.UpdateScroll(0.5);  // This clears the transition.
  pagination_.UpdateScroll(0.5);  // This starts a new transition.
  EXPECT_EQ(kStartPage - 1, pagination_.transition().target_page);
  pagination_.EndScroll(false);
  base::RunLoop().Run();
  EXPECT_EQ(1, observer_.selection_count());
}

TEST_F(PaginationModelTest, FireTransitionZero) {
  const int kStartPage = 2;

  // Scroll to next page then revert the scroll and make sure transition
  // progress 0 is fired when previous scroll is cleared.
  SetStartPageAndExpects(kStartPage, 0, 0, 0);
  pagination_.StartScroll();
  pagination_.UpdateScroll(-0.1);

  int target_page = kStartPage + 1;
  EXPECT_EQ(target_page, pagination_.transition().target_page);
  observer_.set_transition_page(target_page);

  pagination_.UpdateScroll(0.2);  // This clears the transition.
  EXPECT_EQ(1, observer_.transition_start_count());
  pagination_.EndScroll(true);  // Cancel transition.

  // Similar to above but in the other direction.
  SetStartPageAndExpects(kStartPage, 0, 0, 0);
  pagination_.StartScroll();
  pagination_.UpdateScroll(0.1);

  target_page = kStartPage - 1;
  EXPECT_EQ(target_page, pagination_.transition().target_page);
  observer_.set_transition_page(target_page);

  pagination_.UpdateScroll(-0.2);  // This clears the transition.
  EXPECT_EQ(1, observer_.transition_start_count());
  pagination_.EndScroll(true);  // Cancel transition.
}

TEST_F(PaginationModelTest, SelectedPageIsLost) {
  pagination_.SetTotalPages(2);
  // The selected page is set to 0 once the total number of pages are set.
  EXPECT_EQ(0, pagination_.selected_page());

  pagination_.SelectPage(1, false /* animate */);
  EXPECT_EQ(1, pagination_.selected_page());

  // The selected page is unchanged if it's still valid.
  pagination_.SetTotalPages(3);
  EXPECT_EQ(1, pagination_.selected_page());
  pagination_.SetTotalPages(2);
  EXPECT_EQ(1, pagination_.selected_page());

  // But if the currently selected_page exceeds the total number of pages,
  // it automatically switches to the last page.
  pagination_.SetTotalPages(1);
  EXPECT_EQ(0, pagination_.selected_page());
}

TEST_F(PaginationModelTest, SelectPageRelativeBeginning) {
  // Test starts with 5 pages. Select Page 1.
  pagination_.SelectPage(1, false);

  pagination_.SelectPageRelative(-1, false);
  EXPECT_EQ(0, pagination_.selected_page());
}

TEST_F(PaginationModelTest, SelectPageRelativeMiddle) {
  // Test starts with 5 pages. Select page 2.
  pagination_.SelectPage(2, false);

  pagination_.SelectPageRelative(-1, false);
  EXPECT_EQ(1, pagination_.selected_page());

  pagination_.SelectPageRelative(1, false);
  EXPECT_EQ(2, pagination_.selected_page());
}

}  // namespace test
}  // namespace app_list
