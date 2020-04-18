// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/password_manager/password_store_win.h"

#include <stddef.h>

#include <map>
#include <memory>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "components/os_crypt/ie7_password_win.h"
#include "components/password_manager/core/browser/password_manager.h"
#include "components/password_manager/core/browser/password_manager_metrics_util.h"
#include "components/password_manager/core/browser/webdata/password_web_data_service_win.h"

using autofill::PasswordForm;
using password_manager::PasswordStore;
using password_manager::PasswordStoreDefault;

// Handles requests to PasswordWebDataService.
class PasswordStoreWin::DBHandler : public WebDataServiceConsumer {
 public:
  typedef base::Callback<void(std::vector<std::unique_ptr<PasswordForm>>)>
      ResultCallback;

  DBHandler(const scoped_refptr<PasswordWebDataService>& web_data_service,
            PasswordStoreWin* password_store)
      : web_data_service_(web_data_service), password_store_(password_store) {}

  ~DBHandler() override;

  // Requests the IE7 login for |form|. This is async. |result_callback| will be
  // run when complete.
  void GetIE7Login(const PasswordStore::FormDigest& form,
                   const ResultCallback& result_callback);

 private:
  struct RequestInfo {
    RequestInfo() {}

    RequestInfo(std::unique_ptr<PasswordStore::FormDigest> request_form,
                const ResultCallback& result_callback)
        : form(std::move(request_form)), result_callback(result_callback) {}

    std::unique_ptr<PasswordStore::FormDigest> form;
    ResultCallback result_callback;
  };

  // Holds info associated with in-flight GetIE7Login requests.
  typedef std::map<PasswordWebDataService::Handle, RequestInfo>
      PendingRequestMap;

  // Gets logins from IE7 if no others are found. Also copies them into
  // Chrome's WebDatabase so we don't need to look next time.
  std::vector<std::unique_ptr<PasswordForm>> GetIE7Results(
      const WDTypedResult* result,
      const PasswordStore::FormDigest& form);

  // WebDataServiceConsumer implementation.
  void OnWebDataServiceRequestDone(
      PasswordWebDataService::Handle handle,
      std::unique_ptr<WDTypedResult> result) override;

  scoped_refptr<PasswordWebDataService> web_data_service_;

  // This creates a cycle between us and PasswordStore. The cycle is broken
  // from PasswordStoreWin::ShutdownOnUIThread(), which deletes us.
  scoped_refptr<PasswordStoreWin> password_store_;

  PendingRequestMap pending_requests_;

  DISALLOW_COPY_AND_ASSIGN(DBHandler);
};

PasswordStoreWin::DBHandler::~DBHandler() {
  DCHECK(
      password_store_->background_task_runner()->RunsTasksInCurrentSequence());
  for (PendingRequestMap::const_iterator i = pending_requests_.begin();
       i != pending_requests_.end();
       ++i) {
    web_data_service_->CancelRequest(i->first);
  }
}

void PasswordStoreWin::DBHandler::GetIE7Login(
    const PasswordStore::FormDigest& form,
    const ResultCallback& result_callback) {
  DCHECK(
      password_store_->background_task_runner()->RunsTasksInCurrentSequence());
  IE7PasswordInfo info;
  info.url_hash =
      ie7_password::GetUrlHash(base::UTF8ToWide(form.origin.spec()));
  PasswordWebDataService::Handle handle =
      web_data_service_->GetIE7Login(info, this);
  pending_requests_[handle] = {
      base::WrapUnique(new PasswordStore::FormDigest(form)), result_callback};
}

std::vector<std::unique_ptr<PasswordForm>>
PasswordStoreWin::DBHandler::GetIE7Results(
    const WDTypedResult* result,
    const PasswordStore::FormDigest& form) {
  DCHECK(
      password_store_->background_task_runner()->RunsTasksInCurrentSequence());
  std::vector<std::unique_ptr<PasswordForm>> matched_forms;
  const WDResult<IE7PasswordInfo>* r =
      static_cast<const WDResult<IE7PasswordInfo>*>(result);
  IE7PasswordInfo info = r->GetValue();

  if (!info.encrypted_data.empty()) {
    // We got a result.
    // Delete the entry. If it's good we will add it to the real saved password
    // table.
    web_data_service_->RemoveIE7Login(info);
    std::vector<ie7_password::DecryptedCredentials> credentials;
    base::string16 url = base::ASCIIToUTF16(form.origin.spec());
    if (ie7_password::DecryptPasswords(url,
                                       info.encrypted_data,
                                       &credentials)) {
      for (size_t i = 0; i < credentials.size(); ++i) {
        auto matched_form = std::make_unique<PasswordForm>();
        matched_form->username_value = credentials[i].username;
        matched_form->password_value = credentials[i].password;
        matched_form->signon_realm = form.signon_realm;
        matched_form->origin = form.origin;
        matched_form->preferred = true;
        matched_form->date_created = info.date_created;

        // Add this PasswordForm to the saved password table. We're on the
        // background sequence already, so we use AddLoginImpl.
        password_store_->AddLoginImpl(*matched_form);
        matched_forms.push_back(std::move(matched_form));
      }
    }
  }
  return matched_forms;
}

void PasswordStoreWin::DBHandler::OnWebDataServiceRequestDone(
    PasswordWebDataService::Handle handle,
    std::unique_ptr<WDTypedResult> result) {
  DCHECK(
      password_store_->background_task_runner()->RunsTasksInCurrentSequence());

  PendingRequestMap::iterator i = pending_requests_.find(handle);
  DCHECK(i != pending_requests_.end());

  ResultCallback result_callback(i->second.result_callback);
  std::unique_ptr<PasswordStore::FormDigest> form = std::move(i->second.form);
  pending_requests_.erase(i);

  if (!result) {
    // The WDS returns NULL if it is shutting down. Run callback with empty
    // result.
    result_callback.Run(std::vector<std::unique_ptr<PasswordForm>>());
    return;
  }

  DCHECK_EQ(PASSWORD_IE7_RESULT, result->GetType());
  std::vector<std::unique_ptr<PasswordForm>> ie7_results =
      GetIE7Results(result.get(), *form);
  UMA_HISTOGRAM_ENUMERATION(
      "PasswordManager.IE7LookupResult",
      ie7_results.empty() ? password_manager::metrics_util::IE7_RESULTS_ABSENT
                          : password_manager::metrics_util::IE7_RESULTS_PRESENT,
      password_manager::metrics_util::IE7_RESULTS_COUNT);
  result_callback.Run(std::move(ie7_results));
}

PasswordStoreWin::PasswordStoreWin(
    std::unique_ptr<password_manager::LoginDatabase> login_db,
    const scoped_refptr<PasswordWebDataService>& web_data_service)
    : PasswordStoreDefault(std::move(login_db)) {
  db_handler_.reset(new DBHandler(web_data_service, this));
}

PasswordStoreWin::~PasswordStoreWin() {
}

void PasswordStoreWin::ShutdownOnBackgroundSequence() {
  DCHECK(background_task_runner()->RunsTasksInCurrentSequence());
  db_handler_.reset();
}

void PasswordStoreWin::ShutdownOnUIThread() {
  background_task_runner()->PostTask(
      FROM_HERE,
      base::Bind(&PasswordStoreWin::ShutdownOnBackgroundSequence, this));
  PasswordStoreDefault::ShutdownOnUIThread();
}

void PasswordStoreWin::GetLoginsImpl(
    const PasswordStore::FormDigest& form,
    std::unique_ptr<GetLoginsRequest> request) {
  // When importing from IE7, the credentials are first stored into a temporary
  // Web SQL database. Then, after each GetLogins() request that does not yield
  // any matches from the LoginDatabase, the matching credentials in the Web SQL
  // database, if any, are returned as results instead, and simultaneously get
  // moved to the LoginDatabase, so next time they will be found immediately.
  // TODO(engedy): Make the IE7-specific code synchronous, so FillMatchingLogins
  // can be overridden instead. See: https://crbug.com/78830.
  // TODO(engedy): Credentials should be imported into the LoginDatabase in the
  // first place. See: https://crbug.com/456119.
  std::vector<std::unique_ptr<PasswordForm>> matched_forms(
      FillMatchingLogins(form));
  if (matched_forms.empty() && db_handler_) {
    db_handler_->GetIE7Login(
        form, base::Bind(&GetLoginsRequest::NotifyConsumerWithResults,
                         base::Owned(request.release())));
  } else {
    request->NotifyConsumerWithResults(std::move(matched_forms));
  }
}
