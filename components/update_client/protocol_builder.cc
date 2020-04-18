// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/update_client/protocol_builder.h"

#include <stdint.h>

#include "base/guid.h"
#include "base/logging.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/sys_info.h"
#include "build/build_config.h"
#include "components/update_client/activity_data_service.h"
#include "components/update_client/component.h"
#include "components/update_client/configurator.h"
#include "components/update_client/persisted_data.h"
#include "components/update_client/protocol_parser.h"
#include "components/update_client/update_query_params.h"
#include "components/update_client/updater_state.h"
#include "components/update_client/utils.h"

#if defined(OS_WIN)
#include "base/win/windows_version.h"
#endif

namespace update_client {

namespace {

// Returns a sanitized version of the brand or an empty string otherwise.
std::string SanitizeBrand(const std::string& brand) {
  return IsValidBrand(brand) ? brand : std::string("");
}

// Filters invalid attributes from |installer_attributes|.
InstallerAttributes SanitizeInstallerAttributes(
    const InstallerAttributes& installer_attributes) {
  InstallerAttributes sanitized_attrs;
  for (const auto& attr : installer_attributes) {
    if (IsValidInstallerAttribute(attr))
      sanitized_attrs.insert(attr);
  }
  return sanitized_attrs;
}

// Returns the amount of physical memory in GB, rounded to the nearest GB.
int GetPhysicalMemoryGB() {
  const double kOneGB = 1024 * 1024 * 1024;
  const int64_t phys_mem = base::SysInfo::AmountOfPhysicalMemory();
  return static_cast<int>(std::floor(0.5 + phys_mem / kOneGB));
}

std::string GetOSVersion() {
#if defined(OS_WIN)
  const auto ver = base::win::OSInfo::GetInstance()->version_number();
  return base::StringPrintf("%d.%d.%d.%d", ver.major, ver.minor, ver.build,
                            ver.patch);
#else
  return base::SysInfo().OperatingSystemVersion();
#endif
}

std::string GetServicePack() {
#if defined(OS_WIN)
  return base::win::OSInfo::GetInstance()->service_pack_str();
#else
  return std::string();
#endif
}

// Returns a string literal corresponding to the value of the downloader |d|.
const char* DownloaderToString(CrxDownloader::DownloadMetrics::Downloader d) {
  switch (d) {
    case CrxDownloader::DownloadMetrics::kUrlFetcher:
      return "direct";
    case CrxDownloader::DownloadMetrics::kBits:
      return "bits";
    default:
      return "unknown";
  }
}

// Returns a formatted string of previousversion and nextversion in an event.
std::string EventVersions(const Component& component) {
  std::string event_versions;
  base::StringAppendF(&event_versions, " previousversion=\"%s\"",
                      component.previous_version().GetString().c_str());
  const base::Version& next_version = component.next_version();
  if (next_version.IsValid()) {
    base::StringAppendF(&event_versions, " nextversion=\"%s\"",
                        next_version.GetString().c_str());
  }
  return event_versions;
}

}  // namespace

std::string BuildDownloadCompleteEventElement(
    const Component& component,
    const CrxDownloader::DownloadMetrics& metrics) {
  using base::StringAppendF;

  std::string event("<event eventtype=\"14\"");
  StringAppendF(&event, " eventresult=\"%d\"", metrics.error == 0);
  StringAppendF(&event, " downloader=\"%s\"",
                DownloaderToString(metrics.downloader));
  if (metrics.error) {
    StringAppendF(&event, " errorcode=\"%d\"", metrics.error);
  }
  StringAppendF(&event, " url=\"%s\"", metrics.url.spec().c_str());

  // -1 means that the  byte counts are not known.
  if (metrics.downloaded_bytes != -1) {
    StringAppendF(&event, " downloaded=\"%s\"",
                  base::Int64ToString(metrics.downloaded_bytes).c_str());
  }
  if (metrics.total_bytes != -1) {
    StringAppendF(&event, " total=\"%s\"",
                  base::Int64ToString(metrics.total_bytes).c_str());
  }

  if (metrics.download_time_ms) {
    StringAppendF(&event, " download_time_ms=\"%s\"",
                  base::NumberToString(metrics.download_time_ms).c_str());
  }
  base::StrAppend(&event, {EventVersions(component)});
  StringAppendF(&event, "/>");
  return event;
}

std::string BuildUpdateCompleteEventElement(const Component& component) {
  DCHECK(component.state() == ComponentState::kUpdateError ||
         component.state() == ComponentState::kUpdated);

  using base::StringAppendF;

  std::string event("<event eventtype=\"3\"");
  const int event_result = component.state() == ComponentState::kUpdated;
  StringAppendF(&event, " eventresult=\"%d\"", event_result);
  if (component.error_category() != ErrorCategory::kNone)
    StringAppendF(&event, " errorcat=\"%d\"",
                  static_cast<int>(component.error_category()));
  if (component.error_code())
    StringAppendF(&event, " errorcode=\"%d\"", component.error_code());
  if (component.extra_code1())
    StringAppendF(&event, " extracode1=\"%d\"", component.extra_code1());
  if (HasDiffUpdate(component))
    StringAppendF(&event, " diffresult=\"%d\"",
                  !component.diff_update_failed());
  if (component.diff_error_category() != ErrorCategory::kNone) {
    StringAppendF(&event, " differrorcat=\"%d\"",
                  static_cast<int>(component.diff_error_category()));
  }
  if (component.diff_error_code())
    StringAppendF(&event, " differrorcode=\"%d\"", component.diff_error_code());
  if (component.diff_extra_code1()) {
    StringAppendF(&event, " diffextracode1=\"%d\"",
                  component.diff_extra_code1());
  }
  if (!component.previous_fp().empty())
    StringAppendF(&event, " previousfp=\"%s\"",
                  component.previous_fp().c_str());
  if (!component.next_fp().empty())
    StringAppendF(&event, " nextfp=\"%s\"", component.next_fp().c_str());
  base::StrAppend(&event, {EventVersions(component)});
  StringAppendF(&event, "/>");
  return event;
}

std::string BuildUninstalledEventElement(const Component& component) {
  DCHECK(component.state() == ComponentState::kUninstalled);

  using base::StringAppendF;

  std::string event;
  StringAppendF(&event, "<event eventtype=\"4\" eventresult=\"1\"");
  if (component.extra_code1()) {
    StringAppendF(&event, " extracode1=\"%d\"", component.extra_code1());
  }
  base::StrAppend(&event, {EventVersions(component)});
  StringAppendF(&event, "/>");
  return event;
}

std::string BuildActionRunEventElement(bool succeeded,
                                       int error_code,
                                       int extra_code1) {
  using base::StringAppendF;

  std::string event;
  StringAppendF(&event, "<event eventtype=\"42\" eventresult=\"%d\"",
                succeeded);
  if (error_code)
    StringAppendF(&event, " errorcode=\"%d\"", error_code);
  if (extra_code1)
    StringAppendF(&event, " extracode1=\"%d\"", extra_code1);
  StringAppendF(&event, "/>");
  return event;
}

std::string BuildProtocolRequest(
    const std::string& session_id,
    const std::string& prod_id,
    const std::string& browser_version,
    const std::string& channel,
    const std::string& lang,
    const std::string& os_long_name,
    const std::string& download_preference,
    const std::string& request_body,
    const std::string& additional_attributes,
    const std::unique_ptr<UpdaterState::Attributes>& updater_state_attributes) {
  std::string request = base::StringPrintf(
      "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
      "<request protocol=\"%s\" ",
      kProtocolVersion);

  if (!additional_attributes.empty())
    base::StringAppendF(&request, "%s ", additional_attributes.c_str());

  // Constant information for this updater.
  base::StringAppendF(&request, "dedup=\"cr\" acceptformat=\"crx2,crx3\" ");

  // Sesssion id and request id
  DCHECK(!session_id.empty());
  DCHECK(!base::StartsWith(session_id, "{", base::CompareCase::SENSITIVE));
  DCHECK(!base::EndsWith(session_id, "}", base::CompareCase::SENSITIVE));
  base::StringAppendF(&request, "sessionid=\"{%s}\" requestid=\"{%s}\" ",
                      session_id.c_str(), base::GenerateGUID().c_str());

  // Chrome version and platform information.
  base::StringAppendF(&request,
                      "updater=\"%s\" updaterversion=\"%s\" prodversion=\"%s\" "
                      "lang=\"%s\" updaterchannel=\"%s\" prodchannel=\"%s\" "
                      "os=\"%s\" arch=\"%s\" nacl_arch=\"%s\"",
                      prod_id.c_str(),                    // "updater"
                      browser_version.c_str(),            // "updaterversion"
                      browser_version.c_str(),            // "prodversion"
                      lang.c_str(),                       // "lang"
                      channel.c_str(),                    // "updaterchannel"
                      channel.c_str(),                    // "prodchannel"
                      UpdateQueryParams::GetOS(),         // "os"
                      UpdateQueryParams::GetArch(),       // "arch"
                      UpdateQueryParams::GetNaclArch());  // "nacl_arch"
#if defined(OS_WIN)
  const bool is_wow64(base::win::OSInfo::GetInstance()->wow64_status() ==
                      base::win::OSInfo::WOW64_ENABLED);
  if (is_wow64)
    base::StringAppendF(&request, " wow64=\"1\"");
#endif
  if (!download_preference.empty())
    base::StringAppendF(&request, " dlpref=\"%s\"",
                        download_preference.c_str());
  if (updater_state_attributes &&
      updater_state_attributes->count(UpdaterState::kIsEnterpriseManaged)) {
    base::StringAppendF(
        &request, " %s=\"%s\"",  // domainjoined
        UpdaterState::kIsEnterpriseManaged,
        (*updater_state_attributes)[UpdaterState::kIsEnterpriseManaged]
            .c_str());
  }
  base::StringAppendF(&request, ">");

  // HW platform information.
  base::StringAppendF(&request, "<hw physmemory=\"%d\"/>",
                      GetPhysicalMemoryGB());  // "physmem" in GB.

  // OS version and platform information.
  const std::string os_version = GetOSVersion();
  const std::string os_sp = GetServicePack();
  base::StringAppendF(
      &request, "<os platform=\"%s\" arch=\"%s\"",
      os_long_name.c_str(),                                    // "platform"
      base::SysInfo().OperatingSystemArchitecture().c_str());  // "arch"
  if (!os_version.empty())
    base::StringAppendF(&request, " version=\"%s\"", os_version.c_str());
  if (!os_sp.empty())
    base::StringAppendF(&request, " sp=\"%s\"", os_sp.c_str());
  base::StringAppendF(&request, "/>");

#if defined(GOOGLE_CHROME_BUILD)
  // Updater state.
  if (updater_state_attributes) {
    base::StringAppendF(&request, "<updater");
    for (const auto& attr : *updater_state_attributes) {
      if (attr.first != UpdaterState::kIsEnterpriseManaged) {
        base::StringAppendF(&request, " %s=\"%s\"", attr.first.c_str(),
                            attr.second.c_str());
      }
    }
    base::StringAppendF(&request, "/>");
  }
#endif  // GOOGLE_CHROME_BUILD

  // The actual payload of the request.
  base::StringAppendF(&request, "%s</request>", request_body.c_str());

  return request;
}

std::map<std::string, std::string> BuildUpdateCheckExtraRequestHeaders(
    scoped_refptr<Configurator> config,
    const std::vector<std::string>& ids,
    bool is_foreground) {
  // This number of extension ids result in an HTTP header length of about 1KB.
  constexpr size_t maxExtensionCount = 30;
  const std::vector<std::string>& app_ids =
      ids.size() <= maxExtensionCount
          ? ids
          : std::vector<std::string>(ids.cbegin(),
                                     ids.cbegin() + maxExtensionCount);
  return std::map<std::string, std::string>{
      {"X-Goog-Update-Updater",
       base::StringPrintf("%s-%s", config->GetProdId().c_str(),
                          config->GetBrowserVersion().GetString().c_str())},
      {"X-Goog-Update-Interactivity", is_foreground ? "fg" : "bg"},
      {"X-Goog-Update-AppId", base::JoinString(app_ids, ",")},
  };
}

std::string BuildUpdateCheckRequest(
    const Configurator& config,
    const std::string& session_id,
    const std::vector<std::string>& ids_checked,
    const IdToComponentPtrMap& components,
    PersistedData* metadata,
    const std::string& additional_attributes,
    bool enabled_component_updates,
    const std::unique_ptr<UpdaterState::Attributes>& updater_state_attributes) {
  const std::string brand(SanitizeBrand(config.GetBrand()));
  std::string app_elements;
  for (const auto& id : ids_checked) {
    DCHECK_EQ(1u, components.count(id));
    const auto& component = *components.at(id);
    const auto& component_id = component.id();
    const auto* crx_component = component.crx_component();

    DCHECK(crx_component);

    const update_client::InstallerAttributes installer_attributes(
        SanitizeInstallerAttributes(crx_component->installer_attributes));
    std::string app("<app ");
    base::StringAppendF(&app, "appid=\"%s\" version=\"%s\"",
                        component_id.c_str(),
                        crx_component->version.GetString().c_str());
    if (!brand.empty())
      base::StringAppendF(&app, " brand=\"%s\"", brand.c_str());
    if (!crx_component->install_source.empty())
      base::StringAppendF(&app, " installsource=\"%s\"",
                          crx_component->install_source.c_str());
    else if (component.is_foreground())
      base::StringAppendF(&app, " installsource=\"ondemand\"");
    if (!crx_component->install_location.empty())
      base::StringAppendF(&app, " installedby=\"%s\"",
                          crx_component->install_location.c_str());
    for (const auto& attr : installer_attributes) {
      base::StringAppendF(&app, " %s=\"%s\"", attr.first.c_str(),
                          attr.second.c_str());
    }
    const auto& cohort = metadata->GetCohort(component_id);
    const auto& cohort_name = metadata->GetCohortName(component_id);
    const auto& cohort_hint = metadata->GetCohortHint(component_id);
    const auto& disabled_reasons = crx_component->disabled_reasons;
    if (!cohort.empty())
      base::StringAppendF(&app, " cohort=\"%s\"", cohort.c_str());
    if (!cohort_name.empty())
      base::StringAppendF(&app, " cohortname=\"%s\"", cohort_name.c_str());
    if (!cohort_hint.empty())
      base::StringAppendF(&app, " cohorthint=\"%s\"", cohort_hint.c_str());
    base::StringAppendF(&app, " enabled=\"%d\">",
                        disabled_reasons.empty() ? 1 : 0);

    for (const int& disabled_reason : disabled_reasons)
      base::StringAppendF(&app, "<disabled reason=\"%d\"/>", disabled_reason);

    base::StringAppendF(&app, "<updatecheck");
    if (crx_component->supports_group_policy_enable_component_updates &&
        !enabled_component_updates) {
      base::StringAppendF(&app, " updatedisabled=\"true\"");
    }
    base::StringAppendF(&app, "/>");

    base::StringAppendF(&app, "<ping");
    if (metadata->GetActiveBit(component_id)) {
      const int date_last_active = metadata->GetDateLastActive(component_id);
      if (date_last_active != kDateUnknown) {
        base::StringAppendF(&app, " ad=\"%d\"", date_last_active);
      } else {
        // Fall back to "day" if "date" is not available.
        base::StringAppendF(&app, " a=\"%d\"",
                            metadata->GetDaysSinceLastActive(component_id));
      }
    }
    const int date_last_rollcall = metadata->GetDateLastRollCall(component_id);
    if (date_last_rollcall != kDateUnknown) {
      base::StringAppendF(&app, " rd=\"%d\"", date_last_rollcall);
    } else {
      // Fall back to "day" if "date" is not available.
      base::StringAppendF(&app, " r=\"%d\"",
                          metadata->GetDaysSinceLastRollCall(component_id));
    }
    base::StringAppendF(&app, " ping_freshness=\"%s\"/>",
                        metadata->GetPingFreshness(component_id).c_str());

    if (!crx_component->fingerprint.empty()) {
      base::StringAppendF(&app,
                          "<packages>"
                          "<package fp=\"%s\"/>"
                          "</packages>",
                          crx_component->fingerprint.c_str());
    }
    base::StringAppendF(&app, "</app>");
    app_elements.append(app);
    VLOG(1) << "Appending to update request: " << app;
  }

  // Include the updater state in the update check request.
  return BuildProtocolRequest(
      session_id, config.GetProdId(), config.GetBrowserVersion().GetString(),
      config.GetChannel(), config.GetLang(), config.GetOSLongName(),
      config.GetDownloadPreference(), app_elements, additional_attributes,
      updater_state_attributes);
}

std::string BuildEventPingRequest(const Configurator& config,
                                  const Component& component) {
  DCHECK(component.state() == ComponentState::kUpdateError ||
         component.state() == ComponentState::kUpToDate ||
         component.state() == ComponentState::kUpdated ||
         component.state() == ComponentState::kUninstalled);

  std::string app =
      base::StringPrintf("<app appid=\"%s\">", component.id().c_str());
  base::StrAppend(&app, component.events());
  app.append("</app>");

  // The ping request does not include any updater state.
  return BuildProtocolRequest(component.session_id(), config.GetProdId(),
                              config.GetBrowserVersion().GetString(),
                              config.GetChannel(), config.GetLang(),
                              config.GetOSLongName(),
                              config.GetDownloadPreference(), app, "", nullptr);
}

}  // namespace update_client
