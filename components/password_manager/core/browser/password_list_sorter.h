// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_PASSWORD_LIST_SORTER_H_
#define COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_PASSWORD_LIST_SORTER_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace autofill {
struct PasswordForm;
}

namespace password_manager {

enum class PasswordEntryType { SAVED, BLACKLISTED };

// Multimap from sort key to password forms.
using DuplicatesMap =
    std::multimap<std::string, std::unique_ptr<autofill::PasswordForm>>;

// Creates key for sorting password or password exception entries. The key is
// eTLD+1 followed by the reversed list of domains (e.g.
// secure.accounts.example.com => example.com.com.example.accounts.secure) and
// the scheme. If |entry_type == SAVED|, username, password and federation are
// appended to the key. For Android credentials the canocial spec is included.
std::string CreateSortKey(const autofill::PasswordForm& form,
                          PasswordEntryType entry_type);

// Sort entries of |list| based on sort key. The key is the concatenation of
// origin, entry type (non-Android credential, Android w/ affiliated web realm
// or Android w/o affiliated web realm). If |entry_type == SAVED|,
// username, password and federation are also included in sort key. If there
// are several forms with the same key, all such forms but the first one are
// stored in |duplicates| instead of |list|.
void SortEntriesAndHideDuplicates(
    std::vector<std::unique_ptr<autofill::PasswordForm>>* list,
    DuplicatesMap* duplicates,
    PasswordEntryType entry_type);

}  // namespace password_manager

#endif  // COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_PASSWORD_LIST_SORTER_H_
