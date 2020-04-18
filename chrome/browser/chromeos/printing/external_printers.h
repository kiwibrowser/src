// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_PRINTING_EXTERNAL_PRINTERS_H_
#define CHROME_BROWSER_CHROMEOS_PRINTING_EXTERNAL_PRINTERS_H_

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "chromeos/chromeos_export.h"
#include "chromeos/printing/printer_configuration.h"

namespace chromeos {

// Manages download and parsing of the external policy printer configuration and
// enforces restrictions.
class CHROMEOS_EXPORT ExternalPrinters
    : public base::SupportsWeakPtr<ExternalPrinters> {
 public:
  // Choose the policy for printer access.
  enum AccessMode {
    UNSET = -1,
    // Printers in the blacklist are disallowed.  Others are allowed.
    BLACKLIST_ONLY = 0,
    // Only printers in the whitelist are allowed.
    WHITELIST_ONLY = 1,
    // All printers in the policy are allowed.
    ALL_ACCESS = 2
  };

  // Observer is notified when the computed set of printers change.  It is
  // assumed that the observer is on the same sequence as the object it is
  // observing.
  class Observer {
   public:
    // Called when the printers have changed and should be queried.  |valid| is
    // true if |printers| is based on a valid policy.  |printers| are the
    // printers that should be available to the user.
    virtual void OnPrintersChanged(
        bool valid,
        const std::map<const std::string, const Printer>& printers) = 0;
  };

  // Creates a handler for the external printer policies.
  static std::unique_ptr<ExternalPrinters> Create();

  virtual ~ExternalPrinters() = default;

  virtual void AddObserver(Observer* observer) = 0;
  virtual void RemoveObserver(Observer* observer) = 0;

  // Parses |data| which is the contents of the bulk printes file and extracts
  // printer information.  The file format is assumed to be JSON.
  virtual void SetData(std::unique_ptr<std::string> data) = 0;
  // Removes all printer data and invalidates the configuration.
  virtual void ClearData() = 0;

  // Set the access mode which chooses the type of filtering that is performed.
  virtual void SetAccessMode(AccessMode mode) = 0;
  // Set the |blacklist| which excludes printers with the given id if access
  // mode is BLACKLIST_ONLY.
  virtual void SetBlacklist(const std::vector<std::string>& blacklist) = 0;
  // Set the |whitelist| which is the complete list of printers that are show to
  // the user.  This is in effect if access mode is WHITELIST_ONLY.
  virtual void SetWhitelist(const std::vector<std::string>& whitelist) = 0;

  // Returns true if the printer map has been computed from a valid policy.
  // Returns false otherwise.  This is computed asynchronously.  Observe
  // OnPrintersChanged() to be notified when it is updated.  This may never
  // become true if a user does not have the appropriate printer policies.
  virtual bool IsPolicySet() const = 0;

  // Returns a refernce to a map of the computed set of printers.  The map is
  // empty if either the policy was empty or we are yet to receive the full
  // policy. Use IsPolicySet() to differentiate betweeen the two.
  virtual const std::map<const std::string, const Printer>& GetPrinters()
      const = 0;
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_PRINTING_EXTERNAL_PRINTERS_H_
