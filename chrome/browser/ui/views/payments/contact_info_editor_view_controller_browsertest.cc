// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "base/run_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/ui/views/payments/payment_request_browsertest_base.h"
#include "chrome/browser/ui/views/payments/payment_request_dialog_view_ids.h"
#include "components/autofill/core/browser/personal_data_manager.h"

namespace payments {

namespace {

const char kNameFull[] = "Kirby Puckett";
const char kPhoneNumber[] = "6515558946";
const char kPhoneNumberInvalid[] = "123";
const char kEmailAddress[] = "kirby@example.com";
const char kEmailAddressInvalid[] = "kirby";

std::string GetLocale() {
  return g_browser_process->GetApplicationLocale();
}

}  // namespace

class PaymentRequestContactInfoEditorTest
    : public PaymentRequestBrowserTestBase {
 protected:
  PaymentRequestContactInfoEditorTest() {}

  PersonalDataLoadedObserverMock personal_data_observer_;
};

IN_PROC_BROWSER_TEST_F(PaymentRequestContactInfoEditorTest, HappyPath) {
  NavigateTo("/payment_request_contact_details_test.html");
  InvokePaymentRequestUI();
  OpenContactInfoEditorScreen();

  SetEditorTextfieldValue(base::ASCIIToUTF16(kNameFull), autofill::NAME_FULL);
  SetEditorTextfieldValue(base::ASCIIToUTF16(kPhoneNumber),
                          autofill::PHONE_HOME_WHOLE_NUMBER);
  SetEditorTextfieldValue(base::ASCIIToUTF16(kEmailAddress),
                          autofill::EMAIL_ADDRESS);

  autofill::PersonalDataManager* personal_data_manager = GetDataManager();
  personal_data_manager->AddObserver(&personal_data_observer_);

  // Wait until the web database has been updated and the notification sent.
  base::RunLoop data_loop;
  EXPECT_CALL(personal_data_observer_, OnPersonalDataChanged())
      .WillOnce(QuitMessageLoop(&data_loop));
  ClickOnDialogViewAndWait(DialogViewID::EDITOR_SAVE_BUTTON);
  data_loop.Run();

  ASSERT_EQ(1UL, personal_data_manager->GetProfiles().size());
  autofill::AutofillProfile* profile = personal_data_manager->GetProfiles()[0];
  DCHECK(profile);

  EXPECT_EQ(base::ASCIIToUTF16(kNameFull),
            profile->GetInfo(autofill::AutofillType(autofill::NAME_FULL),
                             GetLocale()));
  EXPECT_EQ(base::ASCIIToUTF16("16515558946"),
            profile->GetInfo(
                autofill::AutofillType(autofill::PHONE_HOME_WHOLE_NUMBER),
                GetLocale()));
  EXPECT_EQ(base::ASCIIToUTF16(kEmailAddress),
            profile->GetInfo(autofill::AutofillType(autofill::EMAIL_ADDRESS),
                             GetLocale()));

  PaymentRequest* request = GetPaymentRequests(GetActiveWebContents()).front();
  EXPECT_EQ(1U, request->state()->contact_profiles().size());
  EXPECT_EQ(request->state()->contact_profiles().back(),
            request->state()->selected_contact_profile());
}

IN_PROC_BROWSER_TEST_F(PaymentRequestContactInfoEditorTest,
                       EnterAcceleratorHappyPath) {
  NavigateTo("/payment_request_contact_details_test.html");
  InvokePaymentRequestUI();
  OpenContactInfoEditorScreen();

  SetEditorTextfieldValue(base::ASCIIToUTF16(kNameFull), autofill::NAME_FULL);
  SetEditorTextfieldValue(base::ASCIIToUTF16(kPhoneNumber),
                          autofill::PHONE_HOME_WHOLE_NUMBER);
  SetEditorTextfieldValue(base::ASCIIToUTF16(kEmailAddress),
                          autofill::EMAIL_ADDRESS);

  autofill::PersonalDataManager* personal_data_manager = GetDataManager();
  personal_data_manager->AddObserver(&personal_data_observer_);

  // Wait until the web database has been updated and the notification sent.
  base::RunLoop data_loop;
  EXPECT_CALL(personal_data_observer_, OnPersonalDataChanged())
      .WillOnce(QuitMessageLoop(&data_loop));
  views::View* editor_sheet = dialog_view()->GetViewByID(
      static_cast<int>(DialogViewID::CONTACT_INFO_EDITOR_SHEET));
  editor_sheet->AcceleratorPressed(
      ui::Accelerator(ui::VKEY_RETURN, ui::EF_NONE));
  data_loop.Run();

  ASSERT_EQ(1UL, personal_data_manager->GetProfiles().size());
  autofill::AutofillProfile* profile = personal_data_manager->GetProfiles()[0];
  DCHECK(profile);

  EXPECT_EQ(base::ASCIIToUTF16(kNameFull),
            profile->GetInfo(autofill::AutofillType(autofill::NAME_FULL),
                             GetLocale()));
  EXPECT_EQ(base::ASCIIToUTF16("16515558946"),
            profile->GetInfo(
                autofill::AutofillType(autofill::PHONE_HOME_WHOLE_NUMBER),
                GetLocale()));
  EXPECT_EQ(base::ASCIIToUTF16(kEmailAddress),
            profile->GetInfo(autofill::AutofillType(autofill::EMAIL_ADDRESS),
                             GetLocale()));
}

IN_PROC_BROWSER_TEST_F(PaymentRequestContactInfoEditorTest, Validation) {
  NavigateTo("/payment_request_contact_details_test.html");
  InvokePaymentRequestUI();
  OpenContactInfoEditorScreen();

  // Insert invalid values into fields which have rules more complex than
  // just emptiness, and an empty string into simple required fields.
  SetEditorTextfieldValue(base::string16(), autofill::NAME_FULL);
  SetEditorTextfieldValue(base::ASCIIToUTF16(kPhoneNumberInvalid),
                          autofill::PHONE_HOME_WHOLE_NUMBER);
  SetEditorTextfieldValue(base::ASCIIToUTF16(kEmailAddressInvalid),
                          autofill::EMAIL_ADDRESS);

  EXPECT_TRUE(IsEditorTextfieldInvalid(autofill::NAME_FULL));
  EXPECT_TRUE(IsEditorTextfieldInvalid(autofill::PHONE_HOME_WHOLE_NUMBER));
  EXPECT_TRUE(IsEditorTextfieldInvalid(autofill::EMAIL_ADDRESS));

  // Correct the problems.
  SetEditorTextfieldValue(base::ASCIIToUTF16(kNameFull), autofill::NAME_FULL);
  SetEditorTextfieldValue(base::ASCIIToUTF16(kPhoneNumber),
                          autofill::PHONE_HOME_WHOLE_NUMBER);
  SetEditorTextfieldValue(base::ASCIIToUTF16(kEmailAddress),
                          autofill::EMAIL_ADDRESS);

  EXPECT_FALSE(IsEditorTextfieldInvalid(autofill::NAME_FULL));
  EXPECT_FALSE(IsEditorTextfieldInvalid(autofill::PHONE_HOME_WHOLE_NUMBER));
  EXPECT_FALSE(IsEditorTextfieldInvalid(autofill::EMAIL_ADDRESS));

  autofill::PersonalDataManager* personal_data_manager = GetDataManager();
  personal_data_manager->AddObserver(&personal_data_observer_);

  // Wait until the web database has been updated and the notification sent.
  base::RunLoop data_loop;
  EXPECT_CALL(personal_data_observer_, OnPersonalDataChanged())
      .WillOnce(QuitMessageLoop(&data_loop));
  ClickOnDialogViewAndWait(DialogViewID::EDITOR_SAVE_BUTTON);
  data_loop.Run();

  ASSERT_EQ(1UL, personal_data_manager->GetProfiles().size());
  autofill::AutofillProfile* profile = personal_data_manager->GetProfiles()[0];
  DCHECK(profile);

  EXPECT_EQ(base::ASCIIToUTF16(kNameFull),
            profile->GetInfo(autofill::AutofillType(autofill::NAME_FULL),
                             GetLocale()));
  EXPECT_EQ(base::ASCIIToUTF16("16515558946"),
            profile->GetInfo(
                autofill::AutofillType(autofill::PHONE_HOME_WHOLE_NUMBER),
                GetLocale()));
  EXPECT_EQ(base::ASCIIToUTF16(kEmailAddress),
            profile->GetInfo(autofill::AutofillType(autofill::EMAIL_ADDRESS),
                             GetLocale()));
}

IN_PROC_BROWSER_TEST_F(PaymentRequestContactInfoEditorTest, ModifyExisting) {
  NavigateTo("/payment_request_contact_details_test.html");
  autofill::PersonalDataManager* personal_data_manager = GetDataManager();
  personal_data_manager->AddObserver(&personal_data_observer_);

  autofill::AutofillProfile incomplete_profile;
  incomplete_profile.SetInfo(autofill::NAME_FULL, base::ASCIIToUTF16(kNameFull),
                             GetLocale());
  AddAutofillProfile(incomplete_profile);

  InvokePaymentRequestUI();
  OpenContactInfoScreen();

  views::View* list_view = dialog_view()->GetViewByID(
      static_cast<int>(DialogViewID::CONTACT_INFO_SHEET_LIST_VIEW));
  DCHECK(list_view);
  ClickOnDialogViewAndWait(list_view->child_at(0));

  // Do not set name: This should have been populated when opening the screen.
  EXPECT_EQ(base::ASCIIToUTF16(kNameFull),
            GetEditorTextfieldValue(autofill::NAME_FULL));
  SetEditorTextfieldValue(base::ASCIIToUTF16(kPhoneNumber),
                          autofill::PHONE_HOME_WHOLE_NUMBER);
  SetEditorTextfieldValue(base::ASCIIToUTF16(kEmailAddress),
                          autofill::EMAIL_ADDRESS);

  // Wait until the web database has been updated and the notification sent.
  base::RunLoop save_data_loop;
  EXPECT_CALL(personal_data_observer_, OnPersonalDataChanged())
      .WillOnce(QuitMessageLoop(&save_data_loop));
  ClickOnDialogViewAndWait(DialogViewID::EDITOR_SAVE_BUTTON);
  save_data_loop.Run();

  ASSERT_EQ(1UL, personal_data_manager->GetProfiles().size());
  autofill::AutofillProfile* profile = personal_data_manager->GetProfiles()[0];
  DCHECK(profile);

  EXPECT_EQ(base::ASCIIToUTF16(kNameFull),
            profile->GetInfo(autofill::AutofillType(autofill::NAME_FULL),
                             GetLocale()));
  EXPECT_EQ(base::ASCIIToUTF16("16515558946"),
            profile->GetInfo(
                autofill::AutofillType(autofill::PHONE_HOME_WHOLE_NUMBER),
                GetLocale()));
  EXPECT_EQ(base::ASCIIToUTF16(kEmailAddress),
            profile->GetInfo(autofill::AutofillType(autofill::EMAIL_ADDRESS),
                             GetLocale()));
}

IN_PROC_BROWSER_TEST_F(PaymentRequestContactInfoEditorTest,
                       ModifyExistingSelectsIt) {
  NavigateTo("/payment_request_contact_details_test.html");
  autofill::PersonalDataManager* personal_data_manager = GetDataManager();
  personal_data_manager->AddObserver(&personal_data_observer_);

  autofill::AutofillProfile incomplete_profile;
  incomplete_profile.SetInfo(autofill::NAME_FULL, base::ASCIIToUTF16(kNameFull),
                             GetLocale());
  AddAutofillProfile(incomplete_profile);

  autofill::AutofillProfile other_incomplete_profile;
  other_incomplete_profile.SetInfo(autofill::NAME_FULL,
                                   base::ASCIIToUTF16("other"), GetLocale());
  AddAutofillProfile(other_incomplete_profile);

  InvokePaymentRequestUI();
  OpenContactInfoScreen();

  PaymentRequest* request = GetPaymentRequests(GetActiveWebContents()).front();

  // No contact profiles are selected because both are incomplete.
  EXPECT_EQ(nullptr, request->state()->selected_contact_profile());

  views::View* list_view = dialog_view()->GetViewByID(
      static_cast<int>(DialogViewID::CONTACT_INFO_SHEET_LIST_VIEW));
  DCHECK(list_view);
  ClickOnDialogViewAndWait(list_view->child_at(1));

  SetEditorTextfieldValue(base::ASCIIToUTF16(kPhoneNumber),
                          autofill::PHONE_HOME_WHOLE_NUMBER);
  SetEditorTextfieldValue(base::ASCIIToUTF16(kEmailAddress),
                          autofill::EMAIL_ADDRESS);

  // Wait until the web database has been updated and the notification sent.
  base::RunLoop save_data_loop;
  EXPECT_CALL(personal_data_observer_, OnPersonalDataChanged())
      .WillOnce(QuitMessageLoop(&save_data_loop));
  ClickOnDialogViewAndWait(DialogViewID::EDITOR_SAVE_BUTTON);
  save_data_loop.Run();

  autofill::AutofillProfile* profile =
      request->state()->selected_contact_profile();
  DCHECK(profile);

  EXPECT_EQ(base::ASCIIToUTF16("16515558946"),
            profile->GetInfo(
                autofill::AutofillType(autofill::PHONE_HOME_WHOLE_NUMBER),
                GetLocale()));
  EXPECT_EQ(base::ASCIIToUTF16(kEmailAddress),
            profile->GetInfo(autofill::AutofillType(autofill::EMAIL_ADDRESS),
                             GetLocale()));

  // Expect the newly-completed profile to be selected.
  EXPECT_EQ(2U, request->state()->contact_profiles().size());
  EXPECT_EQ(request->state()->contact_profiles().back(), profile);
}

IN_PROC_BROWSER_TEST_F(PaymentRequestContactInfoEditorTest,
                       HappyPathInIncognito) {
  SetIncognito();
  NavigateTo("/payment_request_contact_details_test.html");
  InvokePaymentRequestUI();
  OpenContactInfoEditorScreen();

  SetEditorTextfieldValue(base::ASCIIToUTF16(kNameFull), autofill::NAME_FULL);
  SetEditorTextfieldValue(base::ASCIIToUTF16(kPhoneNumber),
                          autofill::PHONE_HOME_WHOLE_NUMBER);
  SetEditorTextfieldValue(base::ASCIIToUTF16(kEmailAddress),
                          autofill::EMAIL_ADDRESS);

  autofill::PersonalDataManager* personal_data_manager = GetDataManager();
  personal_data_manager->AddObserver(&personal_data_observer_);

  EXPECT_CALL(personal_data_observer_, OnPersonalDataChanged()).Times(0);
  ClickOnDialogViewAndWait(DialogViewID::EDITOR_SAVE_BUTTON);

  // In incognito, the profile should be available in contact_profiles but it
  // shouldn't be saved to the PersonalDataManager.
  ASSERT_EQ(0UL, personal_data_manager->GetProfiles().size());
  PaymentRequest* request = GetPaymentRequests(GetActiveWebContents()).front();
  EXPECT_EQ(1U, request->state()->contact_profiles().size());
  EXPECT_EQ(request->state()->contact_profiles().back(),
            request->state()->selected_contact_profile());

  autofill::AutofillProfile* profile =
      request->state()->contact_profiles().back();
  DCHECK(profile);

  EXPECT_EQ(base::ASCIIToUTF16(kNameFull),
            profile->GetInfo(autofill::AutofillType(autofill::NAME_FULL),
                             GetLocale()));
  EXPECT_EQ(base::ASCIIToUTF16("16515558946"),
            profile->GetInfo(
                autofill::AutofillType(autofill::PHONE_HOME_WHOLE_NUMBER),
                GetLocale()));
  EXPECT_EQ(base::ASCIIToUTF16(kEmailAddress),
            profile->GetInfo(autofill::AutofillType(autofill::EMAIL_ADDRESS),
                             GetLocale()));
}

}  // namespace payments
