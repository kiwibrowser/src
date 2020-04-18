// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_AUTOFILL_CORE_BROWSER_WEBDATA_AUTOFILL_WEBDATA_H_
#define COMPONENTS_AUTOFILL_CORE_BROWSER_WEBDATA_AUTOFILL_WEBDATA_H_

#include <string>
#include <vector>

#include "base/strings/string16.h"
#include "components/webdata/common/web_data_service_base.h"

namespace base {

class Time;

}  // namespace base

class WebDataServiceConsumer;

namespace autofill {

class AutofillEntry;
class AutofillProfile;
class CreditCard;
struct FormFieldData;

// Pure virtual interface for retrieving Autofill data.  API users
// should use AutofillWebDataService.
class AutofillWebData {
 public:
  virtual ~AutofillWebData() {}

  // Schedules a task to add form fields to the web database.
  virtual void AddFormFields(
      const std::vector<FormFieldData>& fields) = 0;

  // Initiates the request for a vector of values which have been entered in
  // form input fields named |name|.  The method OnWebDataServiceRequestDone of
  // |consumer| gets called back when the request is finished, with the vector
  // included in the argument |result|.
  virtual WebDataServiceBase::Handle GetFormValuesForElementName(
      const base::string16& name,
      const base::string16& prefix,
      int limit,
      WebDataServiceConsumer* consumer) = 0;

  // Removes form elements recorded for Autocomplete from the database.
  virtual void RemoveFormElementsAddedBetween(
      const base::Time& delete_begin, const base::Time& delete_end) = 0;

  virtual void RemoveFormValueForElementName(const base::string16& name,
                                             const base::string16& value) = 0;

  // Schedules a task to add an Autofill profile to the web database.
  virtual void AddAutofillProfile(const AutofillProfile& profile) = 0;

  // Schedules a task to update an Autofill profile in the web database.
  virtual void UpdateAutofillProfile(const AutofillProfile& profile) = 0;

  // Schedules a task to remove an Autofill profile from the web database.
  // |guid| is the identifier of the profile to remove.
  virtual void RemoveAutofillProfile(const std::string& guid) = 0;

  // Initiates the request for local/server Autofill profiles.  The method
  // OnWebDataServiceRequestDone of |consumer| gets called when the request is
  // finished, with the profiles included in the argument |result|.  The
  // consumer owns the profiles.
  virtual WebDataServiceBase::Handle GetAutofillProfiles(
      WebDataServiceConsumer* consumer) = 0;
  virtual WebDataServiceBase::Handle GetServerProfiles(
      WebDataServiceConsumer* consumer) = 0;

  // Schedules a task to count the number of unique autofill values contained
  // in the time interval [|begin|, |end|). |begin| and |end| can be null
  // to indicate no time limitation.
  virtual WebDataServiceBase::Handle GetCountOfValuesContainedBetween(
      const base::Time& begin,
      const base::Time& end,
      WebDataServiceConsumer* consumer) = 0;

  // Schedules a task to update autofill entries in the web database.
  virtual void UpdateAutofillEntries(
      const std::vector<AutofillEntry>& autofill_entries) = 0;

  // Schedules a task to add credit card to the web database.
  virtual void AddCreditCard(const CreditCard& credit_card) = 0;

  // Schedules a task to update credit card in the web database.
  virtual void UpdateCreditCard(const CreditCard& credit_card) = 0;

  // Schedules a task to remove a credit card from the web database.
  // |guid| is identifier of the credit card to remove.
  virtual void RemoveCreditCard(const std::string& guid) = 0;

  // Schedules a task to add a full server credit card to the web database.
  virtual void AddFullServerCreditCard(const CreditCard& credit_card) = 0;

  // Initiates the request for local/server credit cards.  The method
  // OnWebDataServiceRequestDone of |consumer| gets called when the request is
  // finished, with the credit cards included in the argument |result|.  The
  // consumer owns the credit cards.
  virtual WebDataServiceBase::Handle GetCreditCards(
      WebDataServiceConsumer* consumer) = 0;
  virtual WebDataServiceBase::Handle GetServerCreditCards(
      WebDataServiceConsumer* consumer) = 0;

  // Toggles the record for a server credit card between masked (only last 4
  // digits) and full (all digits).
  virtual void UnmaskServerCreditCard(const CreditCard& credit_card,
                                      const base::string16& full_number) = 0;
  virtual void MaskServerCreditCard(const std::string& id) = 0;

  // Updates the metadata for a server card (masked or not).
  virtual void UpdateServerCardMetadata(const CreditCard& credit_card) = 0;

  // Updates the metadata for a server address.
  virtual void UpdateServerAddressMetadata(const AutofillProfile& profile) = 0;

  // Removes Autofill records from the database.
  virtual void RemoveAutofillDataModifiedBetween(
      const base::Time& delete_begin, const base::Time& delete_end) = 0;

  // Removes origin URLs associated with Autofill profiles and credit cards from
  // the database.
  virtual void RemoveOriginURLsModifiedBetween(
      const base::Time& delete_begin, const base::Time& delete_end) = 0;

  // Removes the orphan rows in the autofill_profile_names,
  // autofill_profile_emails and autofill_profile_phones tables.
  virtual void RemoveOrphanAutofillTableRows() = 0;
};

}  // namespace autofill

#endif  // COMPONENTS_AUTOFILL_CORE_BROWSER_WEBDATA_AUTOFILL_WEBDATA_H_
