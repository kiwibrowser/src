// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/printing/external_printers.h"

#include <set>

#include "base/feature_list.h"
#include "base/json/json_reader.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/sequence_checker.h"
#include "base/sequenced_task_runner.h"
#include "base/stl_util.h"
#include "base/task_runner_util.h"
#include "base/task_scheduler/post_task.h"
#include "base/threading/thread_restrictions.h"
#include "base/values.h"
#include "chrome/common/chrome_features.h"
#include "chromeos/printing/printer_translator.h"

namespace chromeos {

namespace {

constexpr int kMaxRecords = 20000;

using PrinterCache = std::vector<std::unique_ptr<Printer>>;
using PrinterView = std::map<const std::string, const Printer>;

// Parses |data|, a JSON blob, into a vector of Printers.  If |data| cannot be
// parsed, returns nullptr.  This is run off the UI thread as it could be very
// slow.
std::unique_ptr<PrinterCache> ParsePrinters(std::unique_ptr<std::string> data) {
  int error_code = 0;
  int error_line = 0;

  // This could be really slow.
  base::AssertBlockingAllowed();
  std::unique_ptr<base::Value> json_blob = base::JSONReader::ReadAndReturnError(
      *data, base::JSONParserOptions::JSON_PARSE_RFC, &error_code,
      nullptr /* error_msg_out */, &error_line);
  // It's not valid JSON.  Give up.
  if (!json_blob || !json_blob->is_list()) {
    LOG(WARNING) << "Failed to parse printers policy (" << error_code
                 << ") on line " << error_line;
    return nullptr;
  }

  const base::Value::ListStorage& printer_list = json_blob->GetList();
  if (printer_list.size() > kMaxRecords) {
    LOG(WARNING) << "Too many records in printers policy: "
                 << printer_list.size();
    return nullptr;
  }

  auto parsed_printers = std::make_unique<PrinterCache>();
  parsed_printers->reserve(printer_list.size());
  for (const base::Value& val : printer_list) {
    // TODO(skau): Convert to the new Value APIs.
    const base::DictionaryValue* printer_dict;
    if (!val.GetAsDictionary(&printer_dict)) {
      LOG(WARNING) << "Entry in printers policy skipped.  Not a dictionary.";
      continue;
    }

    auto printer = RecommendedPrinterToPrinter(*printer_dict);
    if (!printer) {
      LOG(WARNING) << "Failed to parse printer configuration.  Skipped.";
      continue;
    }
    parsed_printers->push_back(std::move(printer));
  }

  return parsed_printers;
}

// Computes the effective printer list using the access mode and
// blacklist/whitelist.  Methods are required to be sequenced.  This object is
// the owner of all the policy data.
class Restrictions {
 public:
  Restrictions() : printers_cache_(nullptr) {
    DETACH_FROM_SEQUENCE(sequence_checker_);
  }
  ~Restrictions() { DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_); }

  // Sets the printer cache using the policy blob |data|.  If the policy can be
  // computed, returns the computed list.  Otherwise, nullptr.
  std::unique_ptr<PrinterView> SetData(std::unique_ptr<std::string> data) {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    base::AssertBlockingAllowed();
    printers_cache_ = ParsePrinters(std::move(data));
    return ComputePrinters();
  }

  // Clear the printer cache.  Computed lists will be invalid until we receive
  // new data.
  void ClearData() {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    printers_cache_.reset();
  }

  // Sets the access mode to |mode|.  If the policy can be computed, returns the
  // computed list.  Otherwise, nullptr.
  std::unique_ptr<PrinterView> UpdateAccessMode(
      ExternalPrinters::AccessMode mode) {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    mode_ = mode;
    return ComputePrinters();
  }

  // Sets the blacklist to |blacklist|.  If the policy can be computed, returns
  // the computed list. Otherwise, nullptr.
  std::unique_ptr<PrinterView> UpdateBlacklist(
      const std::vector<std::string>& blacklist) {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    has_blacklist_ = true;
    blacklist_ = std::set<std::string>(blacklist.begin(), blacklist.end());
    return ComputePrinters();
  }

  // Sets the whitelist to |whitelist|.  If the policy can be computed, returns
  // the computed list.  Otherwise, nullptr.
  std::unique_ptr<PrinterView> UpdateWhitelist(
      const std::vector<std::string>& whitelist) {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    has_whitelist_ = true;
    whitelist_ = std::set<std::string>(whitelist.begin(), whitelist.end());
    return ComputePrinters();
  }

 private:
  // Returns true if we have enough data to compute the effective printer list.
  bool IsReady() const {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    if (!printers_cache_) {
      return false;
    }

    switch (mode_) {
      case ExternalPrinters::AccessMode::ALL_ACCESS:
        return true;
      case ExternalPrinters::AccessMode::BLACKLIST_ONLY:
        return has_blacklist_;
      case ExternalPrinters::AccessMode::WHITELIST_ONLY:
        return has_whitelist_;
      case ExternalPrinters::AccessMode::UNSET:
        return false;
    }
    NOTREACHED();
    return false;
  }

  // Returns the effective printer list based on |mode_| from the entries in
  // |printers_cache_|.
  std::unique_ptr<PrinterView> ComputePrinters() {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

    if (!IsReady()) {
      return nullptr;
    }

    auto view = std::make_unique<PrinterView>();
    switch (mode_) {
      case ExternalPrinters::UNSET:
        NOTREACHED();
        break;
      case ExternalPrinters::WHITELIST_ONLY:
        for (const auto& printer : *printers_cache_) {
          if (base::ContainsKey(whitelist_, printer->id())) {
            view->insert({printer->id(), *printer});
          }
        }
        break;
      case ExternalPrinters::BLACKLIST_ONLY:
        for (const auto& printer : *printers_cache_) {
          if (!base::ContainsKey(blacklist_, printer->id())) {
            view->insert({printer->id(), *printer});
          }
        }
        break;
      case ExternalPrinters::ALL_ACCESS:
        for (const auto& printer : *printers_cache_) {
          view->insert({printer->id(), *printer});
        }
        break;
    }

    return view;
  }

  // Cache of the parsed printer configuration file.
  std::unique_ptr<PrinterCache> printers_cache_;

  // The type of restriction which is enforced.
  ExternalPrinters::AccessMode mode_ = ExternalPrinters::UNSET;
  // The list of ids which should not appear in the final list.
  bool has_blacklist_ = false;
  std::set<std::string> blacklist_;
  // The list of the only ids which should appear in the final list.
  bool has_whitelist_ = false;
  std::set<std::string> whitelist_;

  SEQUENCE_CHECKER(sequence_checker_);
  DISALLOW_COPY_AND_ASSIGN(Restrictions);
};

class ExternalPrintersImpl : public ExternalPrinters {
 public:
  ExternalPrintersImpl()
      : restrictions_(std::make_unique<Restrictions>()),
        restrictions_runner_(base::CreateSequencedTaskRunnerWithTraits(
            {base::TaskPriority::BACKGROUND, base::MayBlock(),
             base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN})),
        weak_ptr_factory_(this) {}
  ~ExternalPrintersImpl() override {
    bool success =
        restrictions_runner_->DeleteSoon(FROM_HERE, std::move(restrictions_));
    if (!success) {
      LOG(WARNING) << "Unable to schedule deletion of policy object.";
    }
  }

  void AddObserver(Observer* observer) override {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    observers_.AddObserver(observer);
  }

  void RemoveObserver(Observer* observer) override {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    observers_.RemoveObserver(observer);
  }

  // Resets the printer state fields.
  void ClearData() override {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    if (!base::FeatureList::IsEnabled(features::kBulkPrinters)) {
      return;
    }

    // Update restrictions then clear our local cache on return so we don't get
    // out of sequence.
    restrictions_runner_->PostTaskAndReply(
        FROM_HERE,
        base::BindOnce(&Restrictions::ClearData,
                       base::Unretained(restrictions_.get())),
        base::BindOnce(&ExternalPrintersImpl::OnComputationComplete,
                       weak_ptr_factory_.GetWeakPtr(), nullptr));
  }

  void SetData(std::unique_ptr<std::string> data) override {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    if (!base::FeatureList::IsEnabled(features::kBulkPrinters)) {
      return;
    }

    if (!data) {
      LOG(WARNING) << "Received null data";
      return;
    }

    // Forward data to Restrictions for computation.
    base::PostTaskAndReplyWithResult(
        restrictions_runner_.get(), FROM_HERE,
        base::BindOnce(&Restrictions::SetData,
                       base::Unretained(restrictions_.get()), std::move(data)),
        base::BindOnce(&ExternalPrintersImpl::OnComputationComplete,
                       weak_ptr_factory_.GetWeakPtr()));
  }

  void SetAccessMode(AccessMode mode) override {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    base::PostTaskAndReplyWithResult(
        restrictions_runner_.get(), FROM_HERE,
        base::BindOnce(&Restrictions::UpdateAccessMode,
                       base::Unretained(restrictions_.get()), mode),
        base::BindOnce(&ExternalPrintersImpl::OnComputationComplete,
                       weak_ptr_factory_.GetWeakPtr()));
  }

  void SetBlacklist(const std::vector<std::string>& blacklist) override {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    base::PostTaskAndReplyWithResult(
        restrictions_runner_.get(), FROM_HERE,
        base::BindOnce(&Restrictions::UpdateBlacklist,
                       base::Unretained(restrictions_.get()), blacklist),
        base::BindOnce(&ExternalPrintersImpl::OnComputationComplete,
                       weak_ptr_factory_.GetWeakPtr()));
  }

  void SetWhitelist(const std::vector<std::string>& whitelist) override {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    base::PostTaskAndReplyWithResult(
        restrictions_runner_.get(), FROM_HERE,
        base::BindOnce(&Restrictions::UpdateWhitelist,
                       base::Unretained(restrictions_.get()), whitelist),
        base::BindOnce(&ExternalPrintersImpl::OnComputationComplete,
                       weak_ptr_factory_.GetWeakPtr()));
  }

  bool IsPolicySet() const override {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    return received_data_;
  }

  const std::map<const std::string, const Printer>& GetPrinters()
      const override {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    return printers_;
  }

 private:
  // Called on computation completion.  |view| is the computed printers which a
  // user should be able to see.  If |view| is nullptr, it's taken to mean that
  // the list is now invalid and will be cleared.
  void OnComputationComplete(std::unique_ptr<PrinterView> view) {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    bool valid;
    if (!view) {
      // Printers are dropped if parsing failed.  We can no longer determine
      // what the domain owner wanted.
      printers_.clear();
      valid = false;
    } else {
      printers_.swap(*view);
      valid = true;
    }

    // Maybe notify that the computed list has changed.
    // Do not notify for invalid->invalid transitions
    if (!valid && !received_data_) {
      return;
    }

    received_data_ = valid;
    for (auto& observer : observers_) {
      // We rely on the assumption that this is sequenced with the rest of our
      // code to guarantee that printers_ remains valid.
      observer.OnPrintersChanged(received_data_, printers_);
    }
  }

  // Holds the blacklist and whitelist.  Computes the effective printer list.
  std::unique_ptr<Restrictions> restrictions_;
  // Off UI sequence for computing the printer view.
  scoped_refptr<base::SequencedTaskRunner> restrictions_runner_;

  // True if printers_ is based on a current policy.
  bool received_data_ = false;
  // The computed set of printers.
  PrinterView printers_;

  base::ObserverList<ExternalPrinters::Observer> observers_;

  SEQUENCE_CHECKER(sequence_checker_);
  base::WeakPtrFactory<ExternalPrintersImpl> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(ExternalPrintersImpl);
};

}  // namespace

// static
std::unique_ptr<ExternalPrinters> ExternalPrinters::Create() {
  return std::make_unique<ExternalPrintersImpl>();
}

}  // namespace chromeos
