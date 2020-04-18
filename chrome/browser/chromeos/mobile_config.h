// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_MOBILE_CONFIG_H_
#define CHROME_BROWSER_CHROMEOS_MOBILE_CONFIG_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/memory/singleton.h"
#include "base/time/time.h"
#include "chrome/browser/chromeos/customization/customization_document.h"

namespace base {
class DictionaryValue;
}

namespace chromeos {

// Class that processes mobile (carrier) configuration.
// Configuration is defined as a JSON file - global and local one.
// First global configuration is loaded then local one if it exists.
// Notes on global/local configuration:
// 1. All global config data is inherited unless some carrier properties
//    are overridden or carrier deals are explicitly marked as excluded.
// 2. Local config could mark that all carrier deals should be excluded or
//    only specific carrier deals are excluded.
// 3. New ID mappings in local config are not supported.
// 4. If local config exists, at least trivial global config should exist too.
// 5. If any error occurs while parsing global/local config,
//    MobileConfig::IsReady() will return false.
class MobileConfig : public CustomizationDocument  {
 public:
  // Carrier deal.
  class CarrierDeal {
   public:
    explicit CarrierDeal(const base::DictionaryValue* deal_dict);
    ~CarrierDeal();

    // Returns string with the specified |locale| and |id|.
    // If there's no version for |locale|, default one is returned.
    // If there's no string with specified |id|, empty string is returned.
    std::string GetLocalizedString(const std::string& locale,
                                   const std::string& id) const;

    const std::string& deal_id() const { return deal_id_; }
    const std::vector<std::string>& locales() const { return locales_; }
    const std::string& info_url() const { return info_url_; }
    int notification_count() const { return notification_count_; }
    base::Time expire_date() const { return expire_date_; }

   private:
    std::string deal_id_;
    std::vector<std::string> locales_;
    std::string info_url_;
    int notification_count_;
    base::Time expire_date_;
    const base::DictionaryValue* localized_strings_;

    DISALLOW_COPY_AND_ASSIGN(CarrierDeal);
  };

  // Carrier config.
  class Carrier {
   public:
    Carrier(const base::DictionaryValue* carrier_dict,
            const std::string& initial_locale);
    ~Carrier();

    const std::vector<std::string>& external_ids() { return external_ids_; }
    const std::string& top_up_url() const { return top_up_url_; }
    bool show_portal_button() const { return show_portal_button_; }

    // Returns "default" carrier deal i.e. first deal defined or NULL
    // if there're no deals defined.
    const CarrierDeal* GetDefaultDeal() const;

    // Returns carrier deal by ID.
    const CarrierDeal* GetDeal(const std::string& deal_id) const;

    // Initializes carrier from supplied dictionary.
    // Multiple calls supported (i.e. second call for local config).
    void InitFromDictionary(const base::DictionaryValue* carrier_dict,
                            const std::string& initial_locale);

    // Removes all carrier deals. Might be executed when local config is loaded.
    void RemoveDeals();

   private:
    // List of external IDs that should map to this carrier.
    std::vector<std::string> external_ids_;

    // Top-up URL. Used in network menu ("View account" link) +
    // carrier name in network details (in settings) is a link.
    std::string top_up_url_;

    // If true, show a separate "View account" button on network details page
    // even if device is activated and doesn't need new data plan.
    // It's not shown when one of the "Buy plan" / "Activate" is shown.
    // All "Buy plan" / "Activate" / "View account" buttons launch
    // carrier portal (chrome://mobilesetup/ extension).
    bool show_portal_button_;

    // Maps deal id to deal instance.
    std::map<std::string, std::unique_ptr<CarrierDeal>> deals_;

    DISALLOW_COPY_AND_ASSIGN(Carrier);
  };

  // Carrier config for a specific initial locale.
  class LocaleConfig {
   public:
    explicit LocaleConfig(base::DictionaryValue* locale_dict);
    ~LocaleConfig();

    const std::string& setup_url() const { return setup_url_; }

    // Initializes local config carrier from supplied dictionary.
    // Multiple calls supported (i.e. second call for local config).
    void InitFromDictionary(base::DictionaryValue* locale_dict);

   private:
    // Carrier setup URL. Used in network menu ("Set-up Mobile Data" link).
    // Displayed when SIM card is not installed on the device with a
    // particular initial locale.
    std::string setup_url_;

    DISALLOW_COPY_AND_ASSIGN(LocaleConfig);
  };

  // A struct to hold global and local config content read from config files.
  struct Config {
    std::string global_config;
    std::string local_config;
  };

  // External carrier ID (ex. "Verizon (us)") mapping to internal carrier ID.
  using CarrierIdMap = std::map<std::string, std::string>;

  // Internal carrier ID mapping to Carrier config.
  using Carriers = std::map<std::string, std::unique_ptr<Carrier>>;

  static MobileConfig* GetInstance();

  // Returns carrier by external ID or NULL if there's no such carrier.
  const MobileConfig::Carrier* GetCarrier(const std::string& carrier_id) const;

  // Returns locale specific config by initial locale or NULL
  // if there's no such config defined.
  const MobileConfig::LocaleConfig* GetLocaleConfig() const;

 protected:
  bool LoadManifestFromString(const std::string& manifest) override;

 private:
  FRIEND_TEST_ALL_PREFIXES(MobileConfigTest, Basic);
  FRIEND_TEST_ALL_PREFIXES(MobileConfigTest, BadManifest);
  FRIEND_TEST_ALL_PREFIXES(MobileConfigTest, DealOtherLocale);
  FRIEND_TEST_ALL_PREFIXES(MobileConfigTest, OldDeal);
  FRIEND_TEST_ALL_PREFIXES(MobileConfigTest, LocalConfigNoDeals);
  FRIEND_TEST_ALL_PREFIXES(MobileConfigTest, LocalConfig);
  friend struct base::DefaultSingletonTraits<MobileConfig>;

  // C-tor for singleton construction.
  MobileConfig();

  // C-tor for test construction.
  MobileConfig(const std::string& config,
               const std::string& initial_locale);

  ~MobileConfig() override;

  // Loads carrier configuration.
  void LoadConfig();

  // Processes global/local config.
  void ProcessConfig(const Config& config);

  // Maps external carrier ID to internal carrier ID.
  CarrierIdMap carrier_id_map_;

  // Carrier configuration (including carrier deals).
  Carriers carriers_;

  // Initial locale specific config if defined.
  std::unique_ptr<LocaleConfig> locale_config_;

  // Initial locale value.
  std::string initial_locale_;

  // Root value of the local config (if it exists).
  // Global config is stored in root_ of the base class.
  std::unique_ptr<base::DictionaryValue> local_config_root_;

  DISALLOW_COPY_AND_ASSIGN(MobileConfig);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_MOBILE_CONFIG_H_
