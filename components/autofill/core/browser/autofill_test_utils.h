// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_AUTOFILL_CORE_BROWSER_AUTOFILL_TEST_UTILS_H_
#define COMPONENTS_AUTOFILL_CORE_BROWSER_AUTOFILL_TEST_UTILS_H_

#include <memory>
#include <string>
#include <vector>

#include "components/autofill/core/browser/credit_card.h"
#include "components/autofill/core/browser/field_types.h"
#include "components/autofill/core/browser/proto/server.pb.h"

class PrefService;

namespace user_prefs {
class PrefRegistrySyncable;
}  // namespace user_prefs

namespace autofill {

class AutofillExternalDelegate;
class AutofillProfile;
class AutofillTable;
struct FormData;
struct FormFieldData;

// Common utilities shared amongst Autofill tests.
namespace test {

// The following methods return a PrefService that can be used for
// Autofill-related testing in contexts where the PrefService would otherwise
// have to be constructed manually (e.g., in unit tests within Autofill core
// code). The returned PrefService has had Autofill preferences registered on
// its associated registry.
std::unique_ptr<PrefService> PrefServiceForTesting();
std::unique_ptr<PrefService> PrefServiceForTesting(
    user_prefs::PrefRegistrySyncable* registry);

// Provides a quick way to populate a FormField with c-strings.
void CreateTestFormField(const char* label,
                         const char* name,
                         const char* value,
                         const char* type,
                         FormFieldData* field);

// Provides a quick way to populate a select field.
void CreateTestSelectField(const char* label,
                           const char* name,
                           const char* value,
                           const std::vector<const char*>& values,
                           const std::vector<const char*>& contents,
                           size_t select_size,
                           FormFieldData* field);

void CreateTestSelectField(const std::vector<const char*>& values,
                           FormFieldData* field);

// Populates |form| with data corresponding to a simple address form.
// Note that this actually appends fields to the form data, which can be useful
// for building up more complex test forms. Another version of the function is
// provided in case the caller wants the vector of expected field |types|.
void CreateTestAddressFormData(FormData* form);
void CreateTestAddressFormData(FormData* form,
                               std::vector<ServerFieldTypeSet>* types);

// Populates |form| with data corresponding to a simple personal information
// form, including name and email, but no address-related fields.
void CreateTestPersonalInformationFormData(FormData* form);

// Returns a full profile with valid info according to rules for Canada.
AutofillProfile GetFullValidProfileForCanada();

// Returns a full profile with valid info according to rules for China.
AutofillProfile GetFullValidProfileForChina();

// Returns a profile full of dummy info.
AutofillProfile GetFullProfile();

// Returns a profile full of dummy info, different to the above.
AutofillProfile GetFullProfile2();

// Returns a profile full of dummy info, different to the above.
AutofillProfile GetFullCanadianProfile();

// Returns an incomplete profile of dummy info.
AutofillProfile GetIncompleteProfile1();

// Returns an incomplete profile of dummy info, different to the above.
AutofillProfile GetIncompleteProfile2();

// Returns a verified profile full of dummy info.
AutofillProfile GetVerifiedProfile();

// Returns a verified profile full of dummy info, different to the above.
AutofillProfile GetVerifiedProfile2();

// Returns a credit card full of dummy info.
CreditCard GetCreditCard();

// Returns a credit card full of dummy info, different to the above.
CreditCard GetCreditCard2();

// Returns a verified credit card full of dummy info.
CreditCard GetVerifiedCreditCard();

// Returns a verified credit card full of dummy info, different to the above.
CreditCard GetVerifiedCreditCard2();

// Returns a masked server card full of dummy info.
CreditCard GetMaskedServerCard();
CreditCard GetMaskedServerCardAmex();

// Returns a randomly generated credit card of |record_type|. Note that the
// card is not guaranteed to be valid/sane from a card validation standpoint.
CreditCard GetRandomCreditCard(CreditCard::RecordType record_Type);

// A unit testing utility that is common to a number of the Autofill unit
// tests.  |SetProfileInfo| provides a quick way to populate a profile with
// c-strings.
void SetProfileInfo(AutofillProfile* profile,
                    const char* first_name,
                    const char* middle_name,
                    const char* last_name,
                    const char* email,
                    const char* company,
                    const char* address1,
                    const char* address2,
                    const char* dependent_locality,
                    const char* city,
                    const char* state,
                    const char* zipcode,
                    const char* country,
                    const char* phone);

// This one doesn't require the |dependent_locality|.
void SetProfileInfo(AutofillProfile* profile,
                    const char* first_name,
                    const char* middle_name,
                    const char* last_name,
                    const char* email,
                    const char* company,
                    const char* address1,
                    const char* address2,
                    const char* city,
                    const char* state,
                    const char* zipcode,
                    const char* country,
                    const char* phone);

void SetProfileInfoWithGuid(AutofillProfile* profile,
    const char* guid, const char* first_name, const char* middle_name,
    const char* last_name, const char* email, const char* company,
    const char* address1, const char* address2, const char* city,
    const char* state, const char* zipcode, const char* country,
    const char* phone);

// A unit testing utility that is common to a number of the Autofill unit
// tests.  |SetCreditCardInfo| provides a quick way to populate a credit card
// with c-strings.
void SetCreditCardInfo(CreditCard* credit_card,
                       const char* name_on_card,
                       const char* card_number,
                       const char* expiration_month,
                       const char* expiration_year,
                       const std::string& billing_address_id);

// TODO(isherman): We should do this automatically for all tests, not manually
// on a per-test basis: http://crbug.com/57221
// Disables or mocks out code that would otherwise reach out to system services.
// Revert this configuration with |ReenableSystemServices|.
void DisableSystemServices(PrefService* prefs);

// Undoes the mocking set up by |DisableSystemServices|
void ReenableSystemServices();

// Sets |cards| for |table|. |cards| may contain full, unmasked server cards,
// whereas AutofillTable::SetServerCreditCards can only contain masked cards.
void SetServerCreditCards(AutofillTable* table,
                          const std::vector<CreditCard>& cards);

// Fills the upload |field| with the information passed by parameter. If the
// value of a const char* parameter is NULL, the corresponding attribute won't
// be set at all, as opposed to being set to empty string.
void FillUploadField(AutofillUploadContents::Field* field,
                     unsigned signature,
                     const char* name,
                     const char* control_type,
                     const char* autocomplete,
                     unsigned autofill_type);

// Fills the query form |field| with the information passed by parameter. If the
// value of a const char* parameter is NULL, the corresponding attribute won't
// be set at all, as opposed to being set to empty string.
void FillQueryField(AutofillQueryContents::Form::Field* field,
                    unsigned signature,
                    const char* name,
                    const char* control_type);

// Calls the required functions on the given external delegate to cause the
// delegate to display a popup.
void GenerateTestAutofillPopup(
    AutofillExternalDelegate* autofill_external_delegate);

std::string ObfuscatedCardDigitsAsUTF8(const std::string& str);

}  // namespace test
}  // namespace autofill

#endif  // COMPONENTS_AUTOFILL_CORE_BROWSER_AUTOFILL_TEST_UTILS_H_
