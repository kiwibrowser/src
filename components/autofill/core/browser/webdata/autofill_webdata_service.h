// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_AUTOFILL_CORE_BROWSER_WEBDATA_AUTOFILL_WEBDATA_SERVICE_H_
#define COMPONENTS_AUTOFILL_CORE_BROWSER_WEBDATA_AUTOFILL_WEBDATA_SERVICE_H_

#include <vector>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/supports_user_data.h"
#include "components/autofill/core/browser/webdata/autofill_webdata.h"
#include "components/autofill/core/common/form_field_data.h"
#include "components/sync/base/model_type.h"
#include "components/webdata/common/web_data_results.h"
#include "components/webdata/common/web_data_service_base.h"
#include "components/webdata/common/web_data_service_consumer.h"
#include "components/webdata/common/web_database.h"

class WebDatabaseService;

namespace base {
class SingleThreadTaskRunner;
}

namespace autofill {

class AutofillEntry;
class AutofillProfile;
class AutofillWebDataBackend;
class AutofillWebDataBackendImpl;
class AutofillWebDataServiceObserverOnDBSequence;
class AutofillWebDataServiceObserverOnUISequence;
class CreditCard;

// API for Autofill web data.
class AutofillWebDataService : public AutofillWebData,
                               public WebDataServiceBase {
 public:
  AutofillWebDataService(
      scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner,
      scoped_refptr<base::SingleThreadTaskRunner> db_task_runner);
  AutofillWebDataService(
      scoped_refptr<WebDatabaseService> wdbs,
      scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner,
      scoped_refptr<base::SingleThreadTaskRunner> db_task_runner,
      const ProfileErrorCallback& callback);

  // WebDataServiceBase implementation.
  void ShutdownOnUISequence() override;

  // AutofillWebData implementation.
  void AddFormFields(const std::vector<FormFieldData>& fields) override;
  WebDataServiceBase::Handle GetFormValuesForElementName(
      const base::string16& name,
      const base::string16& prefix,
      int limit,
      WebDataServiceConsumer* consumer) override;

  void RemoveFormElementsAddedBetween(const base::Time& delete_begin,
                                      const base::Time& delete_end) override;
  void RemoveFormValueForElementName(const base::string16& name,
                                     const base::string16& value) override;

  // Profiles.
  void AddAutofillProfile(const AutofillProfile& profile) override;
  void UpdateAutofillProfile(const AutofillProfile& profile) override;
  void RemoveAutofillProfile(const std::string& guid) override;
  WebDataServiceBase::Handle GetAutofillProfiles(
      WebDataServiceConsumer* consumer) override;

  // Server profiles.
  WebDataServiceBase::Handle GetServerProfiles(
      WebDataServiceConsumer* consumer) override;

  WebDataServiceBase::Handle GetCountOfValuesContainedBetween(
      const base::Time& begin,
      const base::Time& end,
      WebDataServiceConsumer* consumer) override;
  void UpdateAutofillEntries(
      const std::vector<AutofillEntry>& autofill_entries) override;

  // Credit cards.
  void AddCreditCard(const CreditCard& credit_card) override;
  void UpdateCreditCard(const CreditCard& credit_card) override;
  void RemoveCreditCard(const std::string& guid) override;
  void AddFullServerCreditCard(const CreditCard& credit_card) override;
  WebDataServiceBase::Handle GetCreditCards(
      WebDataServiceConsumer* consumer) override;

  // Server cards.
  WebDataServiceBase::Handle GetServerCreditCards(
      WebDataServiceConsumer* consumer) override;
  void UnmaskServerCreditCard(const CreditCard& card,
                              const base::string16& full_number) override;
  void MaskServerCreditCard(const std::string& id) override;

  void ClearAllServerData();

  void UpdateServerCardMetadata(const CreditCard& credit_card) override;
  void UpdateServerAddressMetadata(const AutofillProfile& profile) override;

  void RemoveAutofillDataModifiedBetween(const base::Time& delete_begin,
                                         const base::Time& delete_end) override;
  void RemoveOriginURLsModifiedBetween(const base::Time& delete_begin,
                                       const base::Time& delete_end) override;

  void RemoveOrphanAutofillTableRows() override;

  void AddObserver(AutofillWebDataServiceObserverOnDBSequence* observer);
  void RemoveObserver(AutofillWebDataServiceObserverOnDBSequence* observer);

  void AddObserver(AutofillWebDataServiceObserverOnUISequence* observer);
  void RemoveObserver(AutofillWebDataServiceObserverOnUISequence* observer);

  // Returns a SupportsUserData object that may be used to store data accessible
  // from the DB sequence. Should be called only from the DB sequence, and will
  // be destroyed on the DB sequence soon after ShutdownOnUISequence() is
  // called.
  base::SupportsUserData* GetDBUserData();

  // Takes a callback which will be called on the DB sequence with a pointer to
  // an AutofillWebdataBackend. This backend can be used to access or update the
  // WebDatabase directly on the DB sequence.
  void GetAutofillBackend(
      const base::Callback<void(AutofillWebDataBackend*)>& callback);

  // Returns a task runner that can be used to schedule tasks on the DB
  // sequence.
  base::SingleThreadTaskRunner* GetDBTaskRunner();

 protected:
  ~AutofillWebDataService() override;

  virtual void NotifyAutofillMultipleChangedOnUISequence();

  virtual void NotifySyncStartedOnUISequence(syncer::ModelType model_type);

  base::WeakPtr<AutofillWebDataService> AsWeakPtr() {
    return weak_ptr_factory_.GetWeakPtr();
  }

 private:
  base::ObserverList<AutofillWebDataServiceObserverOnUISequence>
      ui_observer_list_;

  // The task runner that this class uses for UI tasks.
  scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner_;

  // The task runner that this class uses for DB tasks.
  scoped_refptr<base::SingleThreadTaskRunner> db_task_runner_;

  scoped_refptr<AutofillWebDataBackendImpl> autofill_backend_;

  // This factory is used on the UI sequence. All vended weak pointers are
  // invalidated in ShutdownOnUISequence().
  base::WeakPtrFactory<AutofillWebDataService> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(AutofillWebDataService);
};

}  // namespace autofill

#endif  // COMPONENTS_AUTOFILL_CORE_BROWSER_WEBDATA_AUTOFILL_WEBDATA_SERVICE_H_
