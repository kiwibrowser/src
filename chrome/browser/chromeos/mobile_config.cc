// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/mobile_config.h"

#include <stddef.h>

#include <utility>

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/stl_util.h"
#include "base/task_scheduler/post_task.h"
#include "base/threading/thread_restrictions.h"
#include "base/values.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chromeos/login/startup_utils.h"

namespace {

// Config attributes names.
const char kAcceptedConfigVersion[] = "1.0";
const char kDefaultAttr[] = "default";

// Carrier config attributes.
const char kCarriersAttr[] = "carriers";
const char kCarrierIdsAttr[] = "ids";
const char kCarrierIdAttr[] = "id";
const char kTopUpURLAttr[] = "top_up_url";
const char kShowPortalButtonAttr[] = "show_portal_button";
const char kDealsAttr[] = "deals";

// Carrier deal attributes.
const char kDealIdAttr[] = "deal_id";
const char kDealLocalesAttr[] = "locales";

const char kInfoURLAttr[] = "info_url";
const char kNotificationCountAttr[] = "notification_count";
const char kDealExpireDateAttr[] = "expire_date";
const char kLocalizedContentAttr[] = "localized_content";

// Initial locale carrier config attributes.
const char kInitialLocalesAttr[] = "initial_locales";
const char kSetupURLAttr[] = "setup_url";

// Local config properties.
const char kExcludeDealsAttr[] = "exclude_deals";

// Location of the global carrier config.
const char kGlobalCarrierConfigPath[] =
    "/usr/share/chromeos-assets/mobile/carrier_config.json";

// Location of the local carrier config.
const char kLocalCarrierConfigPath[] =
    "/opt/oem/etc/carrier_config.json";

// Executes on background thread and reads config files to string.
chromeos::MobileConfig::Config ReadConfigInBackground(
    const base::FilePath& global_config_file,
    const base::FilePath& local_config_file) {
  base::AssertBlockingAllowed();

  chromeos::MobileConfig::Config config;
  if (!base::ReadFileToString(global_config_file, &config.global_config)) {
    VLOG(1) << "Failed to load global mobile config from: "
            << global_config_file.value();
  }
  if (!base::ReadFileToString(local_config_file, &config.local_config)) {
    VLOG(1) << "Failed to load local mobile config from: "
            << local_config_file.value();
  }
  return config;
}

}  // anonymous namespace

namespace chromeos {

// MobileConfig::CarrierDeal implementation. -----------------------------------

MobileConfig::CarrierDeal::CarrierDeal(const base::DictionaryValue* deal_dict)
    : notification_count_(0),
      localized_strings_(NULL) {
  deal_dict->GetString(kDealIdAttr, &deal_id_);

  // Extract list of deal locales.
  const base::ListValue* locale_list = NULL;
  if (deal_dict->GetList(kDealLocalesAttr, &locale_list)) {
    for (size_t i = 0; i < locale_list->GetSize(); ++i) {
      std::string locale;
      if (locale_list->GetString(i, &locale))
        locales_.push_back(locale);
    }
  }

  deal_dict->GetString(kInfoURLAttr, &info_url_);
  deal_dict->GetInteger(kNotificationCountAttr, &notification_count_);
  std::string date_string;
  if (deal_dict->GetString(kDealExpireDateAttr, &date_string)) {
    if (!base::Time::FromString(date_string.c_str(), &expire_date_))
      LOG(ERROR) << "Error parsing deal_expire_date: " << date_string;
  }
  deal_dict->GetDictionary(kLocalizedContentAttr, &localized_strings_);
}

MobileConfig::CarrierDeal::~CarrierDeal() {
}

std::string MobileConfig::CarrierDeal::GetLocalizedString(
    const std::string& locale, const std::string& id) const {
  std::string result;
  if (localized_strings_) {
    const base::DictionaryValue* locale_dict = NULL;
    if (localized_strings_->GetDictionary(locale, &locale_dict) &&
        locale_dict->GetString(id, &result)) {
      return result;
    } else if (localized_strings_->GetDictionary(kDefaultAttr, &locale_dict) &&
               locale_dict->GetString(id, &result)) {
      return result;
    }
  }
  return result;
}

// MobileConfig::Carrier implementation. ---------------------------------------

MobileConfig::Carrier::Carrier(const base::DictionaryValue* carrier_dict,
                               const std::string& initial_locale)
    : show_portal_button_(false) {
  InitFromDictionary(carrier_dict, initial_locale);
}

MobileConfig::Carrier::~Carrier() {
  RemoveDeals();
}

const MobileConfig::CarrierDeal* MobileConfig::Carrier::GetDefaultDeal() const {
  // TODO(nkostylev): Use carrier "default_deal_id" attribute.
  auto iter = deals_.begin();
  if (iter != deals_.end())
    return GetDeal((*iter).first);
  else
    return NULL;
}

const MobileConfig::CarrierDeal* MobileConfig::Carrier::GetDeal(
    const std::string& deal_id) const {
  auto iter = deals_.find(deal_id);
  if (iter != deals_.end()) {
    CarrierDeal* deal = iter->second.get();
    // Make sure that deal is still active,
    // i.e. if deal expire date is defined, check it.
    if (!deal->expire_date().is_null() &&
        deal->expire_date() <= base::Time::Now()) {
      return NULL;
    }
    return deal;
  } else {
    return NULL;
  }
}

void MobileConfig::Carrier::InitFromDictionary(
    const base::DictionaryValue* carrier_dict,
    const std::string& initial_locale) {
  carrier_dict->GetString(kTopUpURLAttr, &top_up_url_);
  carrier_dict->GetBoolean(kShowPortalButtonAttr, &show_portal_button_);

  bool exclude_deals = false;
  if (carrier_dict->GetBoolean(kExcludeDealsAttr, &exclude_deals) &&
      exclude_deals) {
    RemoveDeals();
  }

  // Extract list of external IDs for this carrier.
  const base::ListValue* id_list = NULL;
  if (carrier_dict->GetList(kCarrierIdsAttr, &id_list)) {
    for (size_t i = 0; i < id_list->GetSize(); ++i) {
      const base::DictionaryValue* id_dict = NULL;
      std::string external_id;
      if (id_list->GetDictionary(i, &id_dict) &&
          id_dict->GetString(kCarrierIdAttr, &external_id)) {
        external_ids_.push_back(external_id);
      }
    }
  }

  // Extract list of deals for this carrier.
  const base::ListValue* deals_list = NULL;
  if (carrier_dict->GetList(kDealsAttr, &deals_list)) {
    for (size_t i = 0; i < deals_list->GetSize(); ++i) {
      const base::DictionaryValue* deal_dict = NULL;
      if (deals_list->GetDictionary(i, &deal_dict)) {
        std::unique_ptr<CarrierDeal> deal(new CarrierDeal(deal_dict));
        // Filter out deals by initial_locale right away.
        if (base::ContainsValue(deal->locales(), initial_locale)) {
          const std::string& deal_id = deal->deal_id();
          deals_[deal_id] = std::move(deal);
        }
      }
    }
  }
}

void MobileConfig::Carrier::RemoveDeals() {
  deals_.clear();
}

// MobileConfig::LocaleConfig implementation. ----------------------------------

MobileConfig::LocaleConfig::LocaleConfig(base::DictionaryValue* locale_dict) {
  InitFromDictionary(locale_dict);
}

MobileConfig::LocaleConfig::~LocaleConfig() {
}

void MobileConfig::LocaleConfig::InitFromDictionary(
    base::DictionaryValue* locale_dict) {
  locale_dict->GetString(kSetupURLAttr, &setup_url_);
}

// MobileConfig implementation, public -----------------------------------------

// static
MobileConfig* MobileConfig::GetInstance() {
  return base::Singleton<MobileConfig,
                         base::DefaultSingletonTraits<MobileConfig>>::get();
}

const MobileConfig::Carrier* MobileConfig::GetCarrier(
    const std::string& carrier_id) const {
  auto id_iter = carrier_id_map_.find(carrier_id);
  std::string internal_id;
  if (id_iter != carrier_id_map_.end())
    internal_id = id_iter->second;
  else
    return NULL;
  auto iter = carriers_.find(internal_id);
  if (iter != carriers_.end())
    return iter->second.get();
  else
    return NULL;
}

const MobileConfig::LocaleConfig* MobileConfig::GetLocaleConfig() const {
  return locale_config_.get();
}

// MobileConfig implementation, protected --------------------------------------

bool MobileConfig::LoadManifestFromString(const std::string& manifest) {
  if (!CustomizationDocument::LoadManifestFromString(manifest))
    return false;

  // Local config specific attribute.
  bool exclude_deals = false;
  if (root_.get() &&
      root_->GetBoolean(kExcludeDealsAttr, &exclude_deals) &&
      exclude_deals) {
    for (auto iter = carriers_.begin(); iter != carriers_.end(); ++iter) {
      iter->second->RemoveDeals();
    }
  }

  // Other parts are optional and are the same among global/local config.
  base::DictionaryValue* carriers = NULL;
  if (root_.get() && root_->GetDictionary(kCarriersAttr, &carriers)) {
    for (base::DictionaryValue::Iterator iter(*carriers); !iter.IsAtEnd();
         iter.Advance()) {
      const base::DictionaryValue* carrier_dict = NULL;
      if (iter.value().GetAsDictionary(&carrier_dict)) {
        const std::string& internal_id = iter.key();
        auto inner_iter = carriers_.find(internal_id);
        if (inner_iter != carriers_.end()) {
          // Carrier already defined i.e. loading from the local config.
          // New ID mappings in local config is not supported.
          inner_iter->second->InitFromDictionary(carrier_dict, initial_locale_);
        } else {
          std::unique_ptr<Carrier> carrier =
              std::make_unique<Carrier>(carrier_dict, initial_locale_);
          if (!carrier->external_ids().empty()) {
            // Map all external IDs to a single internal one.
            for (auto i = carrier->external_ids().begin();
                 i != carrier->external_ids().end(); ++i) {
              carrier_id_map_[*i] = internal_id;
            }
          } else {
            // Trivial case - using same ID for external/internal one.
            carrier_id_map_[internal_id] = internal_id;
          }
          carriers_[internal_id] = std::move(carrier);
        }
      }
    }
  }

  base::DictionaryValue* initial_locales = NULL;
  if (root_.get() && root_->GetDictionary(kInitialLocalesAttr,
                                          &initial_locales)) {
    base::DictionaryValue* locale_config_dict = NULL;
    // Search for a config based on current initial locale.
    if (initial_locales->GetDictionary(initial_locale_,
                                       &locale_config_dict)) {
      locale_config_.reset(new LocaleConfig(locale_config_dict));
    }
  }

  return true;
}

// MobileConfig implementation, private ----------------------------------------

MobileConfig::MobileConfig()
    : CustomizationDocument(kAcceptedConfigVersion),
      initial_locale_(StartupUtils::GetInitialLocale()) {
  LoadConfig();
}

MobileConfig::MobileConfig(const std::string& config,
                           const std::string& initial_locale)
    : CustomizationDocument(kAcceptedConfigVersion),
      initial_locale_(initial_locale) {
  LoadManifestFromString(config);
}

MobileConfig::~MobileConfig() {
}

void MobileConfig::LoadConfig() {
  base::PostTaskWithTraitsAndReplyWithResult(
      FROM_HERE, {base::TaskPriority::BACKGROUND, base::MayBlock()},
      base::BindOnce(&ReadConfigInBackground,
                     base::FilePath(kGlobalCarrierConfigPath),
                     base::FilePath(kLocalCarrierConfigPath)),
      base::BindOnce(&MobileConfig::ProcessConfig,
                     base::Unretained(this)));  // singleton.
}

void MobileConfig::ProcessConfig(const Config& config) {
  // Global config is mandatory, local config is optional.
  bool global_initialized = false;
  bool local_initialized = true;
  std::unique_ptr<base::DictionaryValue> global_config_root;

  if (!config.global_config.empty()) {
    global_initialized = LoadManifestFromString(config.global_config);
    // Backup global config root as it might be
    // owerwritten while loading local config.
    global_config_root = std::move(root_);
  }
  if (!config.local_config.empty())
    local_initialized = LoadManifestFromString(config.local_config);

  // Treat any parser errors as fatal.
  if (!global_initialized || !local_initialized) {
    root_.reset();
    local_config_root_.reset();
  } else {
    local_config_root_ = std::move(root_);
    root_ = std::move(global_config_root);
  }
}

}  // namespace chromeos
