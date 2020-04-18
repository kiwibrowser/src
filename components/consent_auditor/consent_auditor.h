// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CONSENT_AUDITOR_CONSENT_AUDITOR_H_
#define COMPONENTS_CONSENT_AUDITOR_CONSENT_AUDITOR_H_

#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "components/keyed_service/core/keyed_service.h"

namespace syncer {
class UserEventService;
}

namespace sync_pb {
class UserEventSpecifics;
}

class PrefService;
class PrefRegistrySimple;

namespace consent_auditor {

// Feature for which a consent moment is to be recorded.
//
// This enum is used in histograms. Entries should not be renumbered and
// numeric values should never be reused.
//
// A Java counterpart will be generated for this enum.
// GENERATED_JAVA_ENUM_PACKAGE: org.chromium.chrome.browser.consent_auditor
// GENERATED_JAVA_CLASS_NAME_OVERRIDE: ConsentAuditorFeature
enum class Feature {
  CHROME_SYNC = 0,
  PLAY_STORE = 1,
  BACKUP_AND_RESTORE = 2,
  GOOGLE_LOCATION_SERVICE = 3,

  FEATURE_LAST = GOOGLE_LOCATION_SERVICE
};

// Whether a consent is given or not given.
//
// A Java counterpart will be generated for this enum.
// GENERATED_JAVA_ENUM_PACKAGE: org.chromium.chrome.browser.consent_auditor
enum class ConsentStatus { NOT_GIVEN, GIVEN };

class ConsentAuditor : public KeyedService {
 public:
  ConsentAuditor(PrefService* pref_service,
                 syncer::UserEventService* user_event_service,
                 const std::string& app_version,
                 const std::string& app_locale);
  ~ConsentAuditor() override;

  // KeyedService:
  void Shutdown() override;

  // Registers the preferences needed by this service.
  static void RegisterProfilePrefs(PrefRegistrySimple* registry);

  // Records a consent for |feature| for the signed-in GAIA account with
  // the ID |account_id| (as defined in AccountInfo).
  // Consent text consisted of strings with |consent_grd_ids|, and the UI
  // element the user clicked had the ID |confirmation_grd_id|.
  // Whether the consent was GIVEN or NOT_GIVEN is passed as |status|.
  virtual void RecordGaiaConsent(const std::string& account_id,
                                 Feature feature,
                                 const std::vector<int>& description_grd_ids,
                                 int confirmation_grd_id,
                                 ConsentStatus status);

  // Records that the user consented to a |feature|. The user was presented with
  // |description_text| and accepted it by interacting |confirmation_text|
  // (e.g. clicking on a button; empty if not applicable).
  // Returns true if successful.
  void RecordLocalConsent(const std::string& feature,
                          const std::string& description_text,
                          const std::string& confirmation_text);

 private:
  std::unique_ptr<sync_pb::UserEventSpecifics> ConstructUserConsent(
      const std::string& account_id,
      Feature feature,
      const std::vector<int>& description_grd_ids,
      int confirmation_grd_id,
      ConsentStatus status);

  PrefService* pref_service_;
  syncer::UserEventService* user_event_service_;
  std::string app_version_;
  std::string app_locale_;

  DISALLOW_COPY_AND_ASSIGN(ConsentAuditor);
};

}  // namespace consent_auditor

#endif  // COMPONENTS_CONSENT_AUDITOR_CONSENT_AUDITOR_H_
