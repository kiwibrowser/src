// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PERMISSIONS_PERMISSION_DECISION_AUTO_BLOCKER_H_
#define CHROME_BROWSER_PERMISSIONS_PERMISSION_DECISION_AUTO_BLOCKER_H_

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/singleton.h"
#include "base/time/default_clock.h"
#include "chrome/browser/permissions/permission_result.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"
#include "components/keyed_service/core/keyed_service.h"
#include "url/gurl.h"

class GURL;
class Profile;

// The PermissionDecisionAutoBlocker decides whether or not a given origin
// should be automatically blocked from requesting a permission. When an origin
// is blocked, it is placed under an "embargo". Until the embargo expires, any
// requests made by the origin are automatically blocked. Once the embargo is
// lifted, the origin will be permitted to request a permission again, which may
// result in it being placed under embargo again. Currently, an origin can be
// placed under embargo if it has a number of prior dismissals greater than a
// threshold.
class PermissionDecisionAutoBlocker : public KeyedService {
 public:
  class Factory : public BrowserContextKeyedServiceFactory {
   public:
    static PermissionDecisionAutoBlocker* GetForProfile(Profile* profile);
    static PermissionDecisionAutoBlocker::Factory* GetInstance();

   private:
    friend struct base::DefaultSingletonTraits<Factory>;

    Factory();
    ~Factory() override;

    // BrowserContextKeyedServiceFactory
    KeyedService* BuildServiceInstanceFor(
        content::BrowserContext* context) const override;

    content::BrowserContext* GetBrowserContextToUse(
        content::BrowserContext* context) const override;
  };

  static PermissionDecisionAutoBlocker* GetForProfile(Profile* profile);

  // Checks the status of the content setting to determine if |request_origin|
  // is under embargo for |permission|. This checks all types of embargo.
  // Prefer to use PermissionManager::GetPermissionStatus when possible. This
  // method is only exposed to facilitate permission checks from threads other
  // than the UI thread. See https://crbug.com/658020.
  static PermissionResult GetEmbargoResult(HostContentSettingsMap* settings_map,
                                           const GURL& request_origin,
                                           ContentSettingsType permission,
                                           base::Time current_time);

  // Updates the threshold to start blocking prompts from the field trial.
  static void UpdateFromVariations();

  // Checks the status of the content setting to determine if |request_origin|
  // is under embargo for |permission|. This checks all types of embargo.
  PermissionResult GetEmbargoResult(const GURL& request_origin,
                                    ContentSettingsType permission);

  // Returns the current number of dismisses recorded for |permission| type at
  // |url|.
  int GetDismissCount(const GURL& url, ContentSettingsType permission);

  // Returns the current number of ignores recorded for |permission|
  // type at |url|.
  int GetIgnoreCount(const GURL& url, ContentSettingsType permission);

  // Records that a dismissal of a prompt for |permission| was made. If the
  // total number of dismissals exceeds a threshhold and
  // features::kBlockPromptsIfDismissedOften is enabled, it will place |url|
  // under embargo for |permission|.
  bool RecordDismissAndEmbargo(const GURL& url, ContentSettingsType permission);

  // Records that an ignore of a prompt for |permission| was made. If the total
  // number of ignores exceeds a threshold and
  // features::kBlockPromptsIfIgnoredOften is enabled, it will place |url| under
  // embargo for |permission|.
  bool RecordIgnoreAndEmbargo(const GURL& url, ContentSettingsType permission);

  // Clears any existing embargo status for |url|, |permission|. For permissions
  // embargoed under repeated dismissals, this means a prompt will be shown to
  // the user on next permission request. This is a NO-OP for non-embargoed
  // |url|, |permission| pairs.
  void RemoveEmbargoByUrl(const GURL& url, ContentSettingsType permission);

  // Removes any recorded counts for urls which match |filter|.
  void RemoveCountsByUrl(base::Callback<bool(const GURL& url)> filter);

 private:
  friend class PermissionContextBaseTests;
  friend class PermissionDecisionAutoBlockerUnitTest;

  explicit PermissionDecisionAutoBlocker(Profile* profile);
  ~PermissionDecisionAutoBlocker() override;

  void PlaceUnderEmbargo(const GURL& request_origin,
                         ContentSettingsType permission,
                         const char* key);

  void SetClockForTesting(base::Clock* clock);

  // Keys used for storing count data in a website setting.
  static const char kPromptDismissCountKey[];
  static const char kPromptIgnoreCountKey[];
  static const char kPermissionDismissalEmbargoKey[];
  static const char kPermissionIgnoreEmbargoKey[];

  Profile* profile_;

  base::Clock* clock_;

  DISALLOW_IMPLICIT_CONSTRUCTORS(PermissionDecisionAutoBlocker);
};
#endif  // CHROME_BROWSER_PERMISSIONS_PERMISSION_DECISION_AUTO_BLOCKER_H_
