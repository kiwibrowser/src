// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_AUTOFILL_VALIDATION_RULES_STORAGE_FACTORY_H_
#define IOS_CHROME_BROWSER_AUTOFILL_VALIDATION_RULES_STORAGE_FACTORY_H_

#include <memory>

#include "base/lazy_instance.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"

namespace i18n {
namespace addressinput {
class Storage;
}
}

class JsonPrefStore;

namespace autofill {

// Creates Storage objects, all of which are backed by a common pref store.
// Adapted for iOS from
// chrome/browser/autofill/validation_rules_storage_factory.{cc,h}, to use
// storage paths specific to iOS.
class ValidationRulesStorageFactory {
 public:
  static std::unique_ptr<::i18n::addressinput::Storage> CreateStorage();

 private:
  friend struct base::LazyInstanceTraitsBase<ValidationRulesStorageFactory>;

  ValidationRulesStorageFactory();
  ~ValidationRulesStorageFactory();

  scoped_refptr<JsonPrefStore> json_pref_store_;

  DISALLOW_COPY_AND_ASSIGN(ValidationRulesStorageFactory);
};

}  // namespace autofill

#endif  // IOS_CHROME_BROWSER_AUTOFILL_VALIDATION_RULES_STORAGE_FACTORY_H_
