// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>

#include <memory>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/optional.h"
#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
#include "chrome/browser/ui/autofill/autofill_popup_controller_impl.h"
#include "chrome/browser/ui/autofill/autofill_popup_view.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "chrome/test/base/testing_profile.h"
#include "components/autofill/content/browser/content_autofill_driver.h"
#include "components/autofill/content/browser/content_autofill_driver_factory.h"
#include "components/autofill/core/browser/autofill_external_delegate.h"
#include "components/autofill/core/browser/autofill_manager.h"
#include "components/autofill/core/browser/autofill_test_utils.h"
#include "components/autofill/core/browser/popup_item_ids.h"
#include "components/autofill/core/browser/test_autofill_client.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_contents.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/text_utils.h"

using ::testing::_;
using ::testing::AtLeast;
using ::testing::Mock;
using ::testing::NiceMock;
using ::testing::StrictMock;
using base::ASCIIToUTF16;
using base::WeakPtr;

namespace autofill {
namespace {

class MockAutofillExternalDelegate : public AutofillExternalDelegate {
 public:
  MockAutofillExternalDelegate(AutofillManager* autofill_manager,
                               AutofillDriver* autofill_driver)
      : AutofillExternalDelegate(autofill_manager, autofill_driver) {}
  ~MockAutofillExternalDelegate() override {}

  void DidSelectSuggestion(const base::string16& value,
                           int identifier) override {}
  bool RemoveSuggestion(const base::string16& value, int identifier) override {
    return true;
  }
  base::WeakPtr<AutofillExternalDelegate> GetWeakPtr() {
    return AutofillExternalDelegate::GetWeakPtr();
  }

  MOCK_METHOD0(ClearPreviewedForm, void());
};

class MockAutofillClient : public autofill::TestAutofillClient {
 public:
  MockAutofillClient() : prefs_(autofill::test::PrefServiceForTesting()) {}
  ~MockAutofillClient() override {}

  PrefService* GetPrefs() override { return prefs_.get(); }

 private:
  std::unique_ptr<PrefService> prefs_;

  DISALLOW_COPY_AND_ASSIGN(MockAutofillClient);
};

class MockAutofillPopupView : public AutofillPopupView {
 public:
  MockAutofillPopupView() {}

  MOCK_METHOD0(Show, void());
  MOCK_METHOD0(Hide, void());
  MOCK_METHOD2(OnSelectedRowChanged,
               void(base::Optional<int> previous_row_selection,
                    base::Optional<int> current_row_selection));
  MOCK_METHOD0(OnSuggestionsChanged, void());

 private:
  DISALLOW_COPY_AND_ASSIGN(MockAutofillPopupView);
};

class TestAutofillPopupController : public AutofillPopupControllerImpl {
 public:
  TestAutofillPopupController(
      base::WeakPtr<AutofillExternalDelegate> external_delegate,
      const gfx::RectF& element_bounds)
      : AutofillPopupControllerImpl(external_delegate,
                                    NULL,
                                    NULL,
                                    element_bounds,
                                    base::i18n::UNKNOWN_DIRECTION) {}
  ~TestAutofillPopupController() override {}

  // Making protected functions public for testing
  using AutofillPopupControllerImpl::GetLineCount;
  using AutofillPopupControllerImpl::GetSuggestionAt;
  using AutofillPopupControllerImpl::GetElidedValueAt;
  using AutofillPopupControllerImpl::GetElidedLabelAt;
  using AutofillPopupControllerImpl::selected_line;
  using AutofillPopupControllerImpl::SetSelectedLine;
  using AutofillPopupControllerImpl::SelectNextLine;
  using AutofillPopupControllerImpl::SelectPreviousLine;
  using AutofillPopupControllerImpl::RemoveSelectedLine;
  using AutofillPopupControllerImpl::popup_bounds;
  using AutofillPopupControllerImpl::element_bounds;
  using AutofillPopupControllerImpl::SetValues;
  using AutofillPopupControllerImpl::GetWeakPtr;
  MOCK_METHOD0(OnSuggestionsChanged, void());
  MOCK_METHOD0(Hide, void());

  void DoHide() {
    AutofillPopupControllerImpl::Hide();
  }
};

static constexpr base::Optional<int> kNoSelection;

}  // namespace

class AutofillPopupControllerUnitTest : public ChromeRenderViewHostTestHarness {
 public:
  AutofillPopupControllerUnitTest()
      : autofill_client_(new MockAutofillClient()),
        autofill_popup_controller_(NULL) {}
  ~AutofillPopupControllerUnitTest() override {}

  void SetUp() override {
    ChromeRenderViewHostTestHarness::SetUp();

    ContentAutofillDriverFactory::CreateForWebContentsAndDelegate(
        web_contents(), autofill_client_.get(), "en-US",
        AutofillManager::ENABLE_AUTOFILL_DOWNLOAD_MANAGER);
    // Make sure RenderFrame is created.
    NavigateAndCommit(GURL("about:blank"));
    ContentAutofillDriverFactory* factory =
        ContentAutofillDriverFactory::FromWebContents(web_contents());
    ContentAutofillDriver* driver =
        factory->DriverForFrame(web_contents()->GetMainFrame());
    external_delegate_.reset(
        new NiceMock<MockAutofillExternalDelegate>(
            driver->autofill_manager(),
            driver));
    autofill_popup_view_.reset(new NiceMock<MockAutofillPopupView>());
    autofill_popup_controller_ = new NiceMock<TestAutofillPopupController>(
        external_delegate_->GetWeakPtr(), gfx::RectF());
    autofill_popup_controller_->SetViewForTesting(autofill_popup_view());
  }

  void TearDown() override {
    // This will make sure the controller and the view (if any) are both
    // cleaned up.
    if (autofill_popup_controller_)
      autofill_popup_controller_->DoHide();

    external_delegate_.reset();
    ChromeRenderViewHostTestHarness::TearDown();
  }

  TestAutofillPopupController* popup_controller() {
    return autofill_popup_controller_;
  }

  MockAutofillExternalDelegate* delegate() {
    return external_delegate_.get();
  }

  MockAutofillPopupView* autofill_popup_view() {
    return autofill_popup_view_.get();
  }

 protected:
  std::unique_ptr<MockAutofillClient> autofill_client_;
  std::unique_ptr<NiceMock<MockAutofillExternalDelegate>> external_delegate_;
  std::unique_ptr<NiceMock<MockAutofillPopupView>> autofill_popup_view_;
  NiceMock<TestAutofillPopupController>* autofill_popup_controller_;
};

TEST_F(AutofillPopupControllerUnitTest, ChangeSelectedLine) {
  // Set up the popup.
  std::vector<Suggestion> suggestions;
  suggestions.push_back(Suggestion("", "", "", 0));
  suggestions.push_back(Suggestion("", "", "", 0));
  autofill_popup_controller_->Show(suggestions);

  EXPECT_FALSE(autofill_popup_controller_->selected_line());
  // Check that there are at least 2 values so that the first and last selection
  // are different.
  EXPECT_GE(2,
      static_cast<int>(autofill_popup_controller_->GetLineCount()));

  // Test wrapping before the front.
  autofill_popup_controller_->SelectPreviousLine();
  EXPECT_EQ(autofill_popup_controller_->GetLineCount() - 1,
            autofill_popup_controller_->selected_line().value());

  // Test wrapping after the end.
  autofill_popup_controller_->SelectNextLine();
  EXPECT_EQ(0, *autofill_popup_controller_->selected_line());
}

TEST_F(AutofillPopupControllerUnitTest, RedrawSelectedLine) {
  // Set up the popup.
  std::vector<Suggestion> suggestions;
  suggestions.push_back(Suggestion("", "", "", 0));
  suggestions.push_back(Suggestion("", "", "", 0));
  autofill_popup_controller_->Show(suggestions);

  // Make sure that when a new line is selected, it is invalidated so it can
  // be updated to show it is selected.
  base::Optional<int> selected_line = 0;
  EXPECT_CALL(*autofill_popup_view_,
              OnSelectedRowChanged(kNoSelection, selected_line));

  autofill_popup_controller_->SetSelectedLine(selected_line);

  // Ensure that the row isn't invalidated if it didn't change.
  EXPECT_CALL(*autofill_popup_view_, OnSelectedRowChanged(_, _)).Times(0);
  autofill_popup_controller_->SetSelectedLine(selected_line);

  // Change back to no selection.
  EXPECT_CALL(*autofill_popup_view_,
              OnSelectedRowChanged(selected_line, kNoSelection));

  autofill_popup_controller_->SetSelectedLine(kNoSelection);
}

TEST_F(AutofillPopupControllerUnitTest, RemoveLine) {
  // Set up the popup.
  std::vector<Suggestion> suggestions;
  suggestions.push_back(Suggestion("", "", "", 1));
  suggestions.push_back(Suggestion("", "", "", 1));
  suggestions.push_back(Suggestion("", "", "", POPUP_ITEM_ID_AUTOFILL_OPTIONS));
  autofill_popup_controller_->Show(suggestions);

  // Generate a popup, so it can be hidden later. It doesn't matter what the
  // external_delegate thinks is being shown in the process, since we are just
  // testing the popup here.
  test::GenerateTestAutofillPopup(external_delegate_.get());

  // No line is selected so the removal should fail.
  EXPECT_FALSE(autofill_popup_controller_->RemoveSelectedLine());

  // Select the first entry.
  base::Optional<int> selected_line(0);
  EXPECT_CALL(*autofill_popup_view_,
              OnSelectedRowChanged(kNoSelection, selected_line));
  autofill_popup_controller_->SetSelectedLine(selected_line);
  Mock::VerifyAndClearExpectations(autofill_popup_view());

  // Remove the first entry. The popup should be redrawn since its size has
  // changed.
  EXPECT_CALL(*autofill_popup_view_, OnSelectedRowChanged(_, _)).Times(0);
  EXPECT_CALL(*autofill_popup_controller_, OnSuggestionsChanged());
  EXPECT_TRUE(autofill_popup_controller_->RemoveSelectedLine());
  Mock::VerifyAndClearExpectations(autofill_popup_view());

  // Select the last entry.
  EXPECT_CALL(*autofill_popup_view_,
              OnSelectedRowChanged(kNoSelection, selected_line));
  autofill_popup_controller_->SetSelectedLine(selected_line);
  Mock::VerifyAndClearExpectations(autofill_popup_view());

  // Remove the last entry. The popup should then be hidden since there are
  // no Autofill entries left.
  EXPECT_CALL(*autofill_popup_view_, OnSelectedRowChanged(_, _)).Times(0);
  EXPECT_CALL(*autofill_popup_controller_, Hide());
  EXPECT_TRUE(autofill_popup_controller_->RemoveSelectedLine());
}

TEST_F(AutofillPopupControllerUnitTest, RemoveOnlyLine) {
  // Set up the popup.
  std::vector<Suggestion> suggestions;
  suggestions.push_back(Suggestion("", "", "", 1));
  autofill_popup_controller_->Show(suggestions);

  // Generate a popup.
  test::GenerateTestAutofillPopup(external_delegate_.get());

  // No selection immediately after drawing popup.
  EXPECT_FALSE(autofill_popup_controller_->selected_line());

  // Select the only line.
  base::Optional<int> selected_line(0);
  EXPECT_CALL(*autofill_popup_view_,
              OnSelectedRowChanged(kNoSelection, selected_line));
  autofill_popup_controller_->SetSelectedLine(selected_line);
  Mock::VerifyAndClearExpectations(autofill_popup_view());

  // Remove the only line. The popup should then be hidden since there are no
  // Autofill entries left.
  EXPECT_CALL(*autofill_popup_controller_, Hide());
  EXPECT_CALL(*autofill_popup_view_, OnSelectedRowChanged(_, _)).Times(0);
  EXPECT_TRUE(autofill_popup_controller_->RemoveSelectedLine());
}

TEST_F(AutofillPopupControllerUnitTest, SkipSeparator) {
  // Set up the popup.
  std::vector<Suggestion> suggestions;
  suggestions.push_back(Suggestion("", "", "", 1));
  suggestions.push_back(Suggestion("", "", "", POPUP_ITEM_ID_SEPARATOR));
  suggestions.push_back(Suggestion("", "", "", POPUP_ITEM_ID_AUTOFILL_OPTIONS));
  autofill_popup_controller_->Show(suggestions);

  autofill_popup_controller_->SetSelectedLine(0);

  // Make sure next skips the unselectable separator.
  autofill_popup_controller_->SelectNextLine();
  EXPECT_EQ(2, *autofill_popup_controller_->selected_line());

  // Make sure previous skips the unselectable separator.
  autofill_popup_controller_->SelectPreviousLine();
  EXPECT_EQ(0, *autofill_popup_controller_->selected_line());
}

TEST_F(AutofillPopupControllerUnitTest, SkipInsecureFormWarning) {
  std::vector<Suggestion> suggestions;
  suggestions.push_back(Suggestion("", "", "", 1));
  suggestions.push_back(Suggestion("", "", "", POPUP_ITEM_ID_SEPARATOR));
  suggestions.push_back(Suggestion(
      "", "", "", POPUP_ITEM_ID_INSECURE_CONTEXT_PAYMENT_DISABLED_MESSAGE));
  autofill_popup_controller_->Show(suggestions);

  // Make sure previous skips the unselectable form warning when there is no
  // selection.
  autofill_popup_controller_->SelectPreviousLine();
  EXPECT_FALSE(autofill_popup_controller_->selected_line());

  autofill_popup_controller_->SetSelectedLine(0);
  EXPECT_EQ(0, *autofill_popup_controller_->selected_line());

  // Make sure previous skips the unselectable form warning when there is a
  // selection.
  autofill_popup_controller_->SelectPreviousLine();
  EXPECT_FALSE(autofill_popup_controller_->selected_line());
}

TEST_F(AutofillPopupControllerUnitTest, UpdateDataListValues) {
  std::vector<Suggestion> suggestions;
  suggestions.push_back(Suggestion("", "", "", 1));
  autofill_popup_controller_->Show(suggestions);

  // Add one data list entry.
  base::string16 value1 = ASCIIToUTF16("data list value 1");
  std::vector<base::string16> data_list_values;
  data_list_values.push_back(value1);

  autofill_popup_controller_->UpdateDataListValues(data_list_values,
                                                   data_list_values);

  ASSERT_EQ(3, autofill_popup_controller_->GetLineCount());

  Suggestion result0 = autofill_popup_controller_->GetSuggestionAt(0);
  EXPECT_EQ(value1, result0.value);
  EXPECT_EQ(value1, autofill_popup_controller_->GetElidedValueAt(0));
  EXPECT_EQ(value1, result0.label);
  EXPECT_EQ(value1, autofill_popup_controller_->GetElidedLabelAt(0));
  EXPECT_EQ(POPUP_ITEM_ID_DATALIST_ENTRY, result0.frontend_id);

  Suggestion result1 = autofill_popup_controller_->GetSuggestionAt(1);
  EXPECT_EQ(base::string16(), result1.value);
  EXPECT_EQ(base::string16(), result1.label);
  EXPECT_EQ(POPUP_ITEM_ID_SEPARATOR, result1.frontend_id);

  Suggestion result2 = autofill_popup_controller_->GetSuggestionAt(2);
  EXPECT_EQ(base::string16(), result2.value);
  EXPECT_EQ(base::string16(), result2.label);
  EXPECT_EQ(1, result2.frontend_id);

  // Add two data list entries (which should replace the current one).
  base::string16 value2 = ASCIIToUTF16("data list value 2");
  data_list_values.push_back(value2);

  autofill_popup_controller_->UpdateDataListValues(data_list_values,
                                                   data_list_values);
  ASSERT_EQ(4, autofill_popup_controller_->GetLineCount());

  // Original one first, followed by new one, then separator.
  EXPECT_EQ(value1, autofill_popup_controller_->GetSuggestionAt(0).value);
  EXPECT_EQ(value1, autofill_popup_controller_->GetElidedValueAt(0));
  EXPECT_EQ(value2, autofill_popup_controller_->GetSuggestionAt(1).value);
  EXPECT_EQ(value2, autofill_popup_controller_->GetElidedValueAt(1));
  EXPECT_EQ(POPUP_ITEM_ID_SEPARATOR,
            autofill_popup_controller_->GetSuggestionAt(2).frontend_id);

  // Clear all data list values.
  data_list_values.clear();
  autofill_popup_controller_->UpdateDataListValues(data_list_values,
                                                   data_list_values);

  ASSERT_EQ(1, autofill_popup_controller_->GetLineCount());
  EXPECT_EQ(1, autofill_popup_controller_->GetSuggestionAt(0).frontend_id);
}

TEST_F(AutofillPopupControllerUnitTest, PopupsWithOnlyDataLists) {
  // Create the popup with a single datalist element.
  std::vector<Suggestion> suggestions;
  suggestions.push_back(Suggestion("", "", "", POPUP_ITEM_ID_DATALIST_ENTRY));
  autofill_popup_controller_->Show(suggestions);

  // Replace the datalist element with a new one.
  base::string16 value1 = ASCIIToUTF16("data list value 1");
  std::vector<base::string16> data_list_values;
  data_list_values.push_back(value1);

  autofill_popup_controller_->UpdateDataListValues(data_list_values,
                                                   data_list_values);

  ASSERT_EQ(1, autofill_popup_controller_->GetLineCount());
  EXPECT_EQ(value1, autofill_popup_controller_->GetSuggestionAt(0).value);
  EXPECT_EQ(POPUP_ITEM_ID_DATALIST_ENTRY,
            autofill_popup_controller_->GetSuggestionAt(0).frontend_id);

  // Clear datalist values and check that the popup becomes hidden.
  EXPECT_CALL(*autofill_popup_controller_, Hide());
  data_list_values.clear();
  autofill_popup_controller_->UpdateDataListValues(data_list_values,
                                                   data_list_values);
}

TEST_F(AutofillPopupControllerUnitTest, GetOrCreate) {
  ContentAutofillDriverFactory* factory =
      ContentAutofillDriverFactory::FromWebContents(web_contents());
  ContentAutofillDriver* driver =
      factory->DriverForFrame(web_contents()->GetMainFrame());
  MockAutofillExternalDelegate delegate(driver->autofill_manager(), driver);

  WeakPtr<AutofillPopupControllerImpl> controller =
      AutofillPopupControllerImpl::GetOrCreate(
          WeakPtr<AutofillPopupControllerImpl>(), delegate.GetWeakPtr(), NULL,
          NULL, gfx::RectF(), base::i18n::UNKNOWN_DIRECTION);
  EXPECT_TRUE(controller.get());

  controller->Hide();

  controller = AutofillPopupControllerImpl::GetOrCreate(
      WeakPtr<AutofillPopupControllerImpl>(), delegate.GetWeakPtr(), NULL, NULL,
      gfx::RectF(), base::i18n::UNKNOWN_DIRECTION);
  EXPECT_TRUE(controller.get());

  WeakPtr<AutofillPopupControllerImpl> controller2 =
      AutofillPopupControllerImpl::GetOrCreate(
          controller, delegate.GetWeakPtr(), NULL, NULL, gfx::RectF(),
          base::i18n::UNKNOWN_DIRECTION);
  EXPECT_EQ(controller.get(), controller2.get());
  controller->Hide();

  NiceMock<TestAutofillPopupController>* test_controller =
      new NiceMock<TestAutofillPopupController>(delegate.GetWeakPtr(),
                                                gfx::RectF());
  EXPECT_CALL(*test_controller, Hide());

  gfx::RectF bounds(0.f, 0.f, 1.f, 2.f);
  base::WeakPtr<AutofillPopupControllerImpl> controller3 =
      AutofillPopupControllerImpl::GetOrCreate(
          test_controller->GetWeakPtr(),
          delegate.GetWeakPtr(),
          NULL,
          NULL,
          bounds,
          base::i18n::UNKNOWN_DIRECTION);
  EXPECT_EQ(
      bounds,
      static_cast<AutofillPopupController*>(controller3.get())->
          element_bounds());
  controller3->Hide();

  // Hide the test_controller to delete it.
  test_controller->DoHide();

  test_controller = new NiceMock<TestAutofillPopupController>(
      delegate.GetWeakPtr(), gfx::RectF());
  EXPECT_CALL(*test_controller, Hide()).Times(0);

  const base::WeakPtr<AutofillPopupControllerImpl> controller4 =
      AutofillPopupControllerImpl::GetOrCreate(
          test_controller->GetWeakPtr(), delegate.GetWeakPtr(), nullptr,
          nullptr, bounds, base::i18n::UNKNOWN_DIRECTION);
  EXPECT_EQ(bounds,
            static_cast<const AutofillPopupController*>(controller4.get())
                ->element_bounds());
  delete test_controller;
}

TEST_F(AutofillPopupControllerUnitTest, ProperlyResetController) {
  std::vector<Suggestion> suggestions;
  suggestions.push_back(Suggestion("", "", "", 0));
  suggestions.push_back(Suggestion("", "", "", 0));
  popup_controller()->Show(suggestions);
  popup_controller()->SetSelectedLine(0);

  // Now show a new popup with the same controller, but with fewer items.
  WeakPtr<AutofillPopupControllerImpl> controller =
      AutofillPopupControllerImpl::GetOrCreate(
          popup_controller()->GetWeakPtr(), delegate()->GetWeakPtr(), NULL,
          NULL, gfx::RectF(), base::i18n::UNKNOWN_DIRECTION);
  EXPECT_FALSE(controller->selected_line());
  EXPECT_EQ(0, controller->GetLineCount());
}

TEST_F(AutofillPopupControllerUnitTest, HidingClearsPreview) {
  // Create a new controller, because hiding destroys it and we can't destroy it
  // twice.
  ContentAutofillDriverFactory* factory =
      ContentAutofillDriverFactory::FromWebContents(web_contents());
  ContentAutofillDriver* driver =
      factory->DriverForFrame(web_contents()->GetMainFrame());
  StrictMock<MockAutofillExternalDelegate> delegate(driver->autofill_manager(),
                                                    driver);
  StrictMock<TestAutofillPopupController>* test_controller =
      new StrictMock<TestAutofillPopupController>(delegate.GetWeakPtr(),
                                                  gfx::RectF());

  EXPECT_CALL(delegate, ClearPreviewedForm());
  // Hide() also deletes the object itself.
  test_controller->DoHide();
}

#if !defined(OS_ANDROID)
TEST_F(AutofillPopupControllerUnitTest, ElideText) {
  std::vector<Suggestion> suggestions;
  suggestions.push_back(
      Suggestion("Text that will need to be trimmed",
      "Label that will be trimmed", "genericCC", 0));
  suggestions.push_back(
      Suggestion("untrimmed", "Untrimmed", "genericCC", 0));

  autofill_popup_controller_->SetValues(suggestions);

  // Ensure the popup will be too small to display all of the first row.
  int popup_max_width =
      gfx::GetStringWidth(
          suggestions[0].value,
          autofill_popup_controller_->layout_model().GetValueFontListForRow(
              0)) +
      gfx::GetStringWidth(
          suggestions[0].label,
          autofill_popup_controller_->layout_model().GetLabelFontListForRow(
              0)) -
      25;

  autofill_popup_controller_->ElideValueAndLabelForRow(0, popup_max_width);

  // The first element was long so it should have been trimmed.
  EXPECT_NE(autofill_popup_controller_->GetSuggestionAt(0).value,
            autofill_popup_controller_->GetElidedValueAt(0));
  EXPECT_NE(autofill_popup_controller_->GetSuggestionAt(0).label,
            autofill_popup_controller_->GetElidedLabelAt(0));

  autofill_popup_controller_->ElideValueAndLabelForRow(1, popup_max_width);

  // The second element was shorter so it should be unchanged.
  EXPECT_EQ(autofill_popup_controller_->GetSuggestionAt(1).value,
            autofill_popup_controller_->GetElidedValueAt(1));
  EXPECT_EQ(autofill_popup_controller_->GetSuggestionAt(1).label,
            autofill_popup_controller_->GetElidedLabelAt(1));
}
#endif

}  // namespace autofill
