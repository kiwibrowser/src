// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/password_manager/core/browser/form_saver_impl.h"

#include <memory>
#include <vector>

#include "base/strings/string16.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "components/password_manager/core/browser/password_store.h"
#include "google_apis/gaia/gaia_auth_util.h"
#include "google_apis/gaia/gaia_urls.h"
#include "url/gurl.h"
#include "url/origin.h"

using autofill::PasswordForm;

namespace password_manager {

FormSaverImpl::FormSaverImpl(PasswordStore* store) : store_(store) {
  DCHECK(store);
}

FormSaverImpl::~FormSaverImpl() = default;

void FormSaverImpl::PermanentlyBlacklist(PasswordForm* observed) {
  observed->preferred = false;
  observed->blacklisted_by_user = true;
  observed->username_value.clear();
  observed->password_value.clear();
  observed->other_possible_usernames.clear();
  observed->date_created = base::Time::Now();

  store_->AddLogin(*observed);
}

void FormSaverImpl::Save(
    const PasswordForm& pending,
    const std::map<base::string16, const PasswordForm*>& best_matches) {
  SaveImpl(pending, true, best_matches, nullptr, nullptr);
}

void FormSaverImpl::Update(
    const PasswordForm& pending,
    const std::map<base::string16, const PasswordForm*>& best_matches,
    const std::vector<PasswordForm>* credentials_to_update,
    const PasswordForm* old_primary_key) {
  SaveImpl(pending, false, best_matches, credentials_to_update,
           old_primary_key);
}

void FormSaverImpl::PresaveGeneratedPassword(const PasswordForm& generated) {
  if (presaved_)
    store_->UpdateLoginWithPrimaryKey(generated, *presaved_);
  else
    store_->AddLogin(generated);
  presaved_.reset(new PasswordForm(generated));
}

void FormSaverImpl::RemovePresavedPassword() {
  if (!presaved_)
    return;

  store_->RemoveLogin(*presaved_);
  presaved_ = nullptr;
}

void FormSaverImpl::WipeOutdatedCopies(
    const PasswordForm& pending,
    std::map<base::string16, const PasswordForm*>* best_matches,
    const PasswordForm** preferred_match) {
  DCHECK(preferred_match);  // Note: *preferred_match may still be null.
  if (!url::Origin::Create(GURL(pending.signon_realm))
           .IsSameOriginWith(url::Origin::Create(
               GaiaUrls::GetInstance()->gaia_url().GetOrigin()))) {
    // GAIA change password forms might be skipped since success detection may
    // fail on them.
    return;
  }

  for (auto it = best_matches->begin(); it != best_matches->end();
       /* increment inside the for loop */) {
    if ((pending.password_value != it->second->password_value) &&
        gaia::AreEmailsSame(base::UTF16ToUTF8(pending.username_value),
                            base::UTF16ToUTF8(it->second->username_value))) {
      if (it->second == *preferred_match)
        *preferred_match = nullptr;
      store_->RemoveLogin(*it->second);
      it = best_matches->erase(it);
    } else {
      ++it;
    }
  }
}

std::unique_ptr<FormSaver> FormSaverImpl::Clone() {
  auto result = std::make_unique<FormSaverImpl>(store_);
  if (presaved_)
    result->presaved_ = std::make_unique<PasswordForm>(*presaved_);
  return std::move(result);
}

void FormSaverImpl::SaveImpl(
    const PasswordForm& pending,
    bool is_new_login,
    const std::map<base::string16, const PasswordForm*>& best_matches,
    const std::vector<PasswordForm>* credentials_to_update,
    const PasswordForm* old_primary_key) {
  DCHECK(pending.preferred);
  DCHECK(!pending.blacklisted_by_user);

  UpdatePreferredLoginState(pending.username_value, best_matches);
  if (presaved_) {
    store_->UpdateLoginWithPrimaryKey(pending, *presaved_);
    presaved_ = nullptr;
  } else if (is_new_login) {
    store_->AddLogin(pending);
    if (!pending.username_value.empty())
      DeleteEmptyUsernameCredentials(pending, best_matches);
  } else {
    if (old_primary_key)
      store_->UpdateLoginWithPrimaryKey(pending, *old_primary_key);
    else
      store_->UpdateLogin(pending);
  }

  if (credentials_to_update) {
    for (const PasswordForm& credential : *credentials_to_update) {
      store_->UpdateLogin(credential);
    }
  }
}

void FormSaverImpl::UpdatePreferredLoginState(
    const base::string16& preferred_username,
    const std::map<base::string16, const PasswordForm*>& best_matches) {
  for (const auto& key_value_pair : best_matches) {
    const PasswordForm& form = *key_value_pair.second;
    if (form.preferred && !form.is_public_suffix_match &&
        form.username_value != preferred_username) {
      // This wasn't the selected login but it used to be preferred.
      PasswordForm update(form);
      update.preferred = false;
      store_->UpdateLogin(update);
    }
  }
}

void FormSaverImpl::DeleteEmptyUsernameCredentials(
    const PasswordForm& pending,
    const std::map<base::string16, const PasswordForm*>& best_matches) {
  DCHECK(!pending.username_value.empty());

  for (const auto& match : best_matches) {
    const PasswordForm* form = match.second;
    if (!form->is_public_suffix_match && form->username_value.empty() &&
        form->password_value == pending.password_value) {
      store_->RemoveLogin(*form);
    }
  }
}

}  // namespace password_manager
