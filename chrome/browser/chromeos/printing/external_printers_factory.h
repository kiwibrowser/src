// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_PRINTING_EXTERNAL_PRINTERS_FACTORY_H_
#define CHROME_BROWSER_CHROMEOS_PRINTING_EXTERNAL_PRINTERS_FACTORY_H_

#include <map>
#include <memory>

#include "base/lazy_instance.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/sequence_checker.h"
#include "chrome/browser/chromeos/printing/external_printers.h"
#include "components/account_id/account_id.h"

class Profile;

namespace chromeos {

// Dispenses ExternalPrinters objects based on account id.  Access to this
// object should be sequenced.
class ExternalPrintersFactory {
 public:
  static ExternalPrintersFactory* Get();

  // Returns a WeakPtr to the ExternalPrinters registered for |account_id|. If
  // an ExternalPrinters does not exist, one will be created for |account_id|.
  // The returned object remains valid until RemoveForUserId or Shutdown is
  // called.
  base::WeakPtr<ExternalPrinters> GetForAccountId(const AccountId& account_id);

  // Returns a WeakPtr to the ExternalPrinters registered for |profile| which
  // could be null if |profile| does not map to a valid AccountId. The returned
  // object remains valid until RemoveForUserId or Shutdown is called.
  base::WeakPtr<ExternalPrinters> GetForProfile(Profile* profile);

  // Deletes the ExternalPrinters registered for |account_id|.
  void RemoveForUserId(const AccountId& account_id);

  // Tear down all ExternalPrinters.
  void Shutdown();

 private:
  friend struct base::LazyInstanceTraitsBase<ExternalPrintersFactory>;

  ExternalPrintersFactory();
  ~ExternalPrintersFactory();

  std::map<AccountId, std::unique_ptr<ExternalPrinters>> printers_by_user_;
  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(ExternalPrintersFactory);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_PRINTING_EXTERNAL_PRINTERS_FACTORY_H_
