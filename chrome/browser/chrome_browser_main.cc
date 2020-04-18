// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chrome_browser_main.h"

#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "base/at_exit.h"
#include "base/base_switches.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "base/debug/crash_logging.h"
#include "base/debug/debugger.h"
#include "base/debug/leak_annotations.h"
#include "base/deferred_sequenced_task_runner.h"
#include "base/feature_list.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/memory/scoped_refptr.h"
#include "base/message_loop/message_loop.h"
#include "base/metrics/field_trial.h"
#include "base/metrics/histogram_macros.h"
#include "base/path_service.h"
#include "base/profiler/stack_sampling_profiler.h"
#include "base/run_loop.h"
#include "base/sequenced_task_runner.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/string16.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_piece.h"
#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/sys_info.h"
#include "base/task_scheduler/post_task.h"
#include "base/threading/platform_thread.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/default_tick_clock.h"
#include "base/time/time.h"
#include "base/trace_event/trace_event.h"
#include "base/values.h"
#include "build/build_config.h"
#include "cc/base/switches.h"
#include "chrome/browser/about_flags.h"
#include "chrome/browser/active_use_util.h"
#include "chrome/browser/after_startup_task_utils.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/browser_process_impl.h"
#include "chrome/browser/browser_process_platform_part.h"
#include "chrome/browser/chrome_browser_field_trials.h"
#include "chrome/browser/chrome_browser_main_extra_parts.h"
#include "chrome/browser/component_updater/crl_set_component_installer.h"
#include "chrome/browser/component_updater/file_type_policies_component_installer.h"
#include "chrome/browser/component_updater/mei_preload_component_installer.h"
#include "chrome/browser/component_updater/optimization_hints_component_installer.h"
#include "chrome/browser/component_updater/origin_trials_component_installer.h"
#include "chrome/browser/component_updater/pepper_flash_component_installer.h"
#include "chrome/browser/component_updater/recovery_component_installer.h"
#include "chrome/browser/component_updater/recovery_improved_component_installer.h"
#include "chrome/browser/component_updater/sth_set_component_installer.h"
#include "chrome/browser/component_updater/subresource_filter_component_installer.h"
#include "chrome/browser/component_updater/supervised_user_whitelist_installer.h"
#include "chrome/browser/component_updater/widevine_cdm_component_installer.h"
#include "chrome/browser/defaults.h"
#include "chrome/browser/first_run/first_run.h"
#include "chrome/browser/lifetime/browser_shutdown.h"
#include "chrome/browser/media/webrtc/media_capture_devices_dispatcher.h"
#include "chrome/browser/media/webrtc/webrtc_log_util.h"
#include "chrome/browser/metrics/chrome_metrics_service_accessor.h"
#include "chrome/browser/metrics/expired_histograms_array.h"
#include "chrome/browser/metrics/field_trial_synchronizer.h"
#include "chrome/browser/metrics/renderer_uptime_tracker.h"
#include "chrome/browser/metrics/thread_watcher.h"
#include "chrome/browser/nacl_host/nacl_browser_delegate_impl.h"
#include "chrome/browser/performance_monitor/performance_monitor.h"
#include "chrome/browser/plugins/plugin_prefs.h"
#include "chrome/browser/policy/chrome_browser_policy_connector.h"
#include "chrome/browser/prefs/chrome_command_line_pref_store.h"
#include "chrome/browser/prefs/chrome_pref_service_factory.h"
#include "chrome/browser/prefs/incognito_mode_prefs.h"
#include "chrome/browser/prefs/pref_metrics_service.h"
#include "chrome/browser/process_singleton.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_attributes_entry.h"
#include "chrome/browser/profiles/profile_attributes_storage.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/profiles/profiles_state.h"
#include "chrome/browser/resource_coordinator/render_process_probe.h"
#include "chrome/browser/resource_coordinator/tab_manager.h"
#include "chrome/browser/sessions/chrome_serialized_navigation_driver.h"
#include "chrome/browser/shell_integration.h"
#include "chrome/browser/tracing/background_tracing_field_trial.h"
#include "chrome/browser/tracing/navigation_tracing.h"
#include "chrome/browser/translate/translate_service.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/javascript_dialogs/chrome_javascript_native_dialog_factory.h"
#include "chrome/browser/ui/profile_error_dialog.h"
#include "chrome/browser/ui/startup/bad_flags_prompt.h"
#include "chrome/browser/ui/startup/default_browser_prompt.h"
#include "chrome/browser/ui/startup/startup_browser_creator.h"
#include "chrome/browser/ui/uma_browsing_activity_observer.h"
#include "chrome/browser/ui/webui/chrome_web_ui_controller_factory.h"
#include "chrome/common/buildflags.h"
#include "chrome/common/channel_info.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/common/chrome_features.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/chrome_result_codes.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/crash_keys.h"
#include "chrome/common/env_vars.h"
#include "chrome/common/logging_chrome.h"
#include "chrome/common/media/media_resource_provider.h"
#include "chrome/common/net/net_resource_provider.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/profiling.h"
#include "chrome/common/stack_sampling_configuration.h"
#include "chrome/grit/generated_resources.h"
#include "chrome/installer/util/google_update_settings.h"
#include "components/component_updater/component_updater_service.h"
#include "components/component_updater/crl_set_remover.h"
#include "components/device_event_log/device_event_log.h"
#include "components/flags_ui/pref_service_flags_storage.h"
#include "components/google/core/browser/google_util.h"
#include "components/language/content/browser/geo_language_provider.h"
#include "components/language/core/common/language_experiments.h"
#include "components/language_usage_metrics/language_usage_metrics.h"
#include "components/metrics/call_stack_profile_metrics_provider.h"
#include "components/metrics/call_stack_profile_params.h"
#include "components/metrics/expired_histogram_util.h"
#include "components/metrics/metrics_reporting_default_state.h"
#include "components/metrics/metrics_service.h"
#include "components/metrics_services_manager/metrics_services_manager.h"
#include "components/nacl/browser/nacl_browser.h"
#include "components/nacl/common/buildflags.h"
#include "components/offline_pages/buildflags/buildflags.h"
#include "components/prefs/json_pref_store.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/pref_value_store.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "components/rappor/rappor_service_impl.h"
#include "components/signin/core/browser/profile_management_switches.h"
#include "components/startup_metric_utils/browser/startup_metric_utils.h"
#include "components/tracing/common/tracing_switches.h"
#include "components/translate/core/browser/translate_download_manager.h"
#include "components/variations/field_trial_config/field_trial_util.h"
#include "components/variations/pref_names.h"
#include "components/variations/service/variations_service.h"
#include "components/variations/synthetic_trials_active_group_id_provider.h"
#include "components/variations/variations_associated_data.h"
#include "components/variations/variations_crash_keys.h"
#include "components/variations/variations_http_header_provider.h"
#include "components/variations/variations_switches.h"
#include "components/version_info/version_info.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_types.h"
#include "content/public/browser/site_instance.h"
#include "content/public/browser/webvr_service_provider.h"
#include "content/public/common/content_client.h"
#include "content/public/common/content_features.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/main_function_params.h"
#include "content/public/common/service_manager_connection.h"
#include "device/vr/buildflags/buildflags.h"
#include "extensions/buildflags/buildflags.h"
#include "media/base/localized_strings.h"
#include "media/media_buildflags.h"
#include "net/base/net_module.h"
#include "net/cookies/cookie_monster.h"
#include "net/http/http_network_layer.h"
#include "net/http/http_stream_factory.h"
#include "net/url_request/url_request.h"
#include "printing/buildflags/buildflags.h"
#include "rlz/buildflags/buildflags.h"
#include "services/service_manager/embedder/main_delegate.h"
#include "third_party/blink/public/common/experiments/memory_ablation_experiment.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/layout.h"
#include "ui/base/material_design/material_design_controller.h"
#include "ui/base/resource/resource_bundle.h"

#if defined(OS_ANDROID)
#include "chrome/browser/metrics/thread_watcher_android.h"
#include "ui/base/resource/resource_bundle_android.h"
#else
#include "chrome/browser/feedback/feedback_profile_observer.h"
#include "chrome/browser/resource_coordinator/tab_activity_watcher.h"
#include "chrome/browser/usb/web_usb_detector.h"
#endif  // defined(OS_ANDROID)

#if !defined(OS_ANDROID) && !defined(OS_CHROMEOS)
#include "chrome/browser/first_run/upgrade_util.h"
#endif

#if defined(OS_CHROMEOS)
#include "chrome/browser/chromeos/policy/browser_policy_connector_chromeos.h"
#include "chrome/browser/chromeos/settings/cros_settings.h"
#include "chromeos/chromeos_switches.h"
#include "chromeos/settings/cros_settings_names.h"
#endif  // defined(OS_CHROMEOS)

#if defined(OS_LINUX) && !defined(OS_CHROMEOS)
#include "chrome/browser/first_run/upgrade_util_linux.h"
#endif  // defined(OS_LINUX) && !defined(OS_CHROMEOS)

#if defined(OS_LINUX)
#include "components/crash/content/app/breakpad_linux.h"
#endif

#if defined(OS_MACOSX)
#include <Security/Security.h>

#include "base/mac/scoped_nsautorelease_pool.h"
#include "chrome/browser/mac/keystone_glue.h"
#endif  // defined(OS_MACOSX)

// TODO(port): several win-only methods have been pulled out of this, but
// BrowserMain() as a whole needs to be broken apart so that it's usable by
// other platforms. For now, it's just a stub. This is a serious work in
// progress and should not be taken as an indication of a real refactoring.

#if defined(OS_WIN)
#include "base/trace_event/trace_event_etw_export_win.h"
#include "base/win/win_util.h"
#include "chrome/browser/chrome_browser_main_win.h"
#include "chrome/browser/component_updater/sw_reporter_installer_win.h"
#if defined(GOOGLE_CHROME_BUILD)
#include "chrome/browser/component_updater/third_party_module_list_component_installer_win.h"
#endif  // defined(GOOGLE_CHROME_BUILD)
#include "chrome/browser/downgrade/user_data_downgrade.h"
#include "chrome/browser/first_run/upgrade_util_win.h"
#include "chrome/browser/ui/network_profile_bubble.h"
#include "chrome/browser/ui/views/try_chrome_dialog_win/try_chrome_dialog.h"
#include "chrome/browser/win/browser_util.h"
#include "chrome/browser/win/chrome_select_file_dialog_factory.h"
#include "chrome/install_static/install_util.h"
#include "ui/base/l10n/l10n_util_win.h"
#include "ui/shell_dialogs/select_file_dialog.h"
#endif  // defined(OS_WIN)

#if defined(OS_WIN) || defined(OS_MACOSX) || \
    (defined(OS_LINUX) && !defined(OS_CHROMEOS))
#include "chrome/browser/metrics/desktop_session_duration/desktop_session_duration_tracker.h"
#endif

#if BUILDFLAG(ENABLE_BACKGROUND_MODE)
#include "chrome/browser/background/background_mode_manager.h"
#endif  // BUILDFLAG(ENABLE_BACKGROUND_MODE)

#if BUILDFLAG(ENABLE_CAPTIVE_PORTAL_DETECTION)
#include "chrome/browser/component_updater/ssl_error_assistant_component_installer.h"
#endif  // BUILDFLAG(ENABLE_CAPTIVE_PORTAL_DETECTION)

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "chrome/browser/extensions/startup_helper.h"
#include "extensions/browser/extension_protocols.h"
#include "extensions/common/features/feature_provider.h"
#include "extensions/components/javascript_dialog_extensions_client/javascript_dialog_extension_client_impl.h"
#endif  // BUILDFLAG(ENABLE_EXTENSIONS)

#if BUILDFLAG(ENABLE_NACL)
#include "chrome/browser/component_updater/pnacl_component_installer.h"
#include "components/nacl/browser/nacl_process_host.h"
#endif  // BUILDFLAG(ENABLE_NACL)

#if BUILDFLAG(ENABLE_OFFLINE_PAGES)
#include "chrome/browser/offline_pages/offline_page_info_handler.h"
#endif

#if BUILDFLAG(ENABLE_PRINT_PREVIEW) && !defined(OFFICIAL_BUILD)
#include "printing/printed_document.h"
#endif

#if BUILDFLAG(ENABLE_PRINT_PREVIEW) && !defined(OS_CHROMEOS)
#include "chrome/browser/printing/cloud_print/cloud_print_proxy_service.h"
#include "chrome/browser/printing/cloud_print/cloud_print_proxy_service_factory.h"
#endif

#if BUILDFLAG(ENABLE_RLZ)
#include "chrome/browser/rlz/chrome_rlz_tracker_delegate.h"
#include "components/rlz/rlz_tracker.h"
#endif  // BUILDFLAG(ENABLE_RLZ)

#if BUILDFLAG(ENABLE_VR)
#include "chrome/browser/vr/service/vr_service_impl.h"
#endif

#if defined(USE_AURA)
#include "chrome/browser/lifetime/application_lifetime.h"
#include "services/service_manager/runner/common/client_util.h"
#include "ui/aura/env.h"
#endif
#include "services/service_manager/public/cpp/connector.h"

using content::BrowserThread;

namespace {

// This function provides some ways to test crash and assertion handling
// behavior of the program.
void HandleTestParameters(const base::CommandLine& command_line) {
  // This parameter causes a null pointer crash (crash reporter trigger).
  if (command_line.HasSwitch(switches::kBrowserCrashTest)) {
    int* bad_pointer = NULL;
    *bad_pointer = 0;
  }
}

#if !defined(OS_ANDROID) && !defined(OS_CHROMEOS)
void AddFirstRunNewTabs(StartupBrowserCreator* browser_creator,
                        const std::vector<GURL>& new_tabs) {
  for (std::vector<GURL>::const_iterator it = new_tabs.begin();
       it != new_tabs.end(); ++it) {
    if (it->is_valid())
      browser_creator->AddFirstRunTab(*it);
  }
}
#endif  // !defined(OS_ANDROID) && !defined(OS_CHROMEOS)

void InitializeLocalState(base::SequencedTaskRunner* local_state_task_runner) {
  TRACE_EVENT0("startup", "ChromeBrowserMainParts::InitializeLocalState")

  // Load local state.  This includes the application locale so we know which
  // locale dll to load.  This also causes local state prefs to be registered.
  PrefService* local_state = g_browser_process->local_state();
  DCHECK(local_state);
#if defined(OS_WIN)
  if (first_run::IsChromeFirstRun()) {
    // During first run we read the google_update registry key to find what
    // language the user selected when downloading the installer. This
    // becomes our default language in the prefs.
    // Other platforms obey the system locale.
    base::string16 install_lang;
    if (GoogleUpdateSettings::GetLanguage(&install_lang)) {
      local_state->SetString(prefs::kApplicationLocale,
                             base::UTF16ToASCII(install_lang));
    }
    bool stats_default;
    if (GoogleUpdateSettings::GetCollectStatsConsentDefault(&stats_default)) {
      // |stats_default| == true means that the default state of consent for the
      // product at the time of install was to report usage statistics, meaning
      // "opt-out".
      metrics::RecordMetricsReportingDefaultState(
          local_state, stats_default ? metrics::EnableMetricsDefault::OPT_OUT
                                     : metrics::EnableMetricsDefault::OPT_IN);
    }
  }
#endif  // defined(OS_WIN)

  // If the local state file for the current profile doesn't exist and the
  // parent profile command line flag is present, then we should inherit some
  // local state from the parent profile.
  // Checking that the local state file for the current profile doesn't exist
  // is the most robust way to determine whether we need to inherit or not
  // since the parent profile command line flag can be present even when the
  // current profile is not a new one, and in that case we do not want to
  // inherit and reset the user's setting.
  //
  // TODO(mnissler): We should probably just instantiate a
  // JSONPrefStore here instead of an entire PrefService. Once this is
  // addressed, the call to browser_prefs::RegisterLocalState can move
  // to chrome_prefs::CreateLocalState.
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  if (command_line->HasSwitch(switches::kParentProfile)) {
    base::FilePath local_state_path;
    base::PathService::Get(chrome::FILE_LOCAL_STATE, &local_state_path);
    bool local_state_file_exists = base::PathExists(local_state_path);
    if (!local_state_file_exists) {
      base::FilePath parent_profile =
          command_line->GetSwitchValuePath(switches::kParentProfile);
      scoped_refptr<PrefRegistrySimple> registry =
          base::MakeRefCounted<PrefRegistrySimple>();
      registry->RegisterStringPref(prefs::kApplicationLocale, std::string());
      const std::unique_ptr<PrefService> parent_local_state =
          chrome_prefs::CreateLocalState(parent_profile,
                                         local_state_task_runner,
                                         g_browser_process->policy_service(),
                                         std::move(registry), false, nullptr);
      // Right now, we only inherit the locale setting from the parent profile.
      local_state->SetString(
          prefs::kApplicationLocale,
          parent_local_state->GetString(prefs::kApplicationLocale));
    }
  }

#if defined(OS_CHROMEOS)
  if (command_line->HasSwitch(chromeos::switches::kLoginManager)) {
    std::string owner_locale = local_state->GetString(prefs::kOwnerLocale);
    // Ensure that we start with owner's locale.
    if (!owner_locale.empty() &&
        local_state->GetString(prefs::kApplicationLocale) != owner_locale &&
        !local_state->IsManagedPreference(prefs::kApplicationLocale)) {
      local_state->SetString(prefs::kApplicationLocale, owner_locale);
    }
  }
#endif  // defined(OS_CHROMEOS)
}

void ConvertFlagsToSwitches() {
#if !defined(OS_CHROMEOS)
  // Convert active flags into switches. This needs to be done before
  // ui::ResourceBundle::InitSharedInstanceWithLocale as some loaded resources
  // are affected by experiment flags (--touch-optimized-ui in particular). On
  // ChromeOS system level flags are applied from the device settings from the
  // session manager.
  DCHECK(!ui::ResourceBundle::HasSharedInstance());
  TRACE_EVENT0("startup",
               "ChromeBrowserMainParts::PreCreateThreadsImpl:ConvertFlags");
  flags_ui::PrefServiceFlagsStorage flags_storage(
      g_browser_process->local_state());
  about_flags::ConvertFlagsToSwitches(&flags_storage,
                                      base::CommandLine::ForCurrentProcess(),
                                      flags_ui::kAddSentinels);
#endif  // !defined(OS_CHROMEOS)
}

// Initializes the primary profile, possibly doing some user prompting to pick
// a fallback profile. Returns the newly created profile, or NULL if startup
// should not continue.
Profile* CreatePrimaryProfile(const content::MainFunctionParams& parameters,
                              const base::FilePath& user_data_dir,
                              const base::CommandLine& parsed_command_line) {
  TRACE_EVENT0("startup", "ChromeBrowserMainParts::CreateProfile")

  base::Time start = base::Time::Now();

  bool set_last_used_profile = false;

// If the browser is launched due to activation on Windows native
// notification, the profile id encoded in the notification launch id should
// be chosen over all others.
#if defined(OS_WIN)
  if (parsed_command_line.HasSwitch(switches::kNotificationLaunchId)) {
    profiles::SetLastUsedProfile(
        base::UTF16ToUTF8(parsed_command_line.GetSwitchValueNative(
            switches::kNotificationLaunchId)));
    set_last_used_profile = true;
  }
#endif  // defined(OS_WIN)

  bool profile_dir_specified =
      profiles::IsMultipleProfilesEnabled() &&
      parsed_command_line.HasSwitch(switches::kProfileDirectory);
  if (!set_last_used_profile && profile_dir_specified) {
    profiles::SetLastUsedProfile(
        parsed_command_line.GetSwitchValueASCII(switches::kProfileDirectory));
    set_last_used_profile = true;
  }

  if (set_last_used_profile) {
    // Clear kProfilesLastActive since the user only wants to launch a specific
    // profile.
    ListPrefUpdate update(g_browser_process->local_state(),
                          prefs::kProfilesLastActive);
    base::ListValue* profile_list = update.Get();
    profile_list->Clear();
  }

  Profile* profile = nullptr;
#if defined(OS_CHROMEOS) || defined(OS_ANDROID)
  // On ChromeOS and Android the ProfileManager will use the same path as the
  // one we got passed. CreateInitialProfile will therefore use the correct path
  // automatically.
  DCHECK_EQ(user_data_dir.value(),
            g_browser_process->profile_manager()->user_data_dir().value());
  profile = ProfileManager::CreateInitialProfile();

  // TODO(port): fix this. See comments near the definition of |user_data_dir|.
  // It is better to CHECK-fail here than it is to silently exit because of
  // missing code in the above test.
  CHECK(profile) << "Cannot get default profile.";

#else
  profile = GetStartupProfile(user_data_dir, parsed_command_line);

  if (!profile && !set_last_used_profile)
    profile = GetFallbackStartupProfile();

  if (!profile) {
    ProfileErrorType error_type =
        profile_dir_specified ? ProfileErrorType::CREATE_FAILURE_SPECIFIED
                              : ProfileErrorType::CREATE_FAILURE_ALL;

    // TODO(lwchkg): What diagnostics do you want to include in the feedback
    // report when an error occurs?
    ShowProfileErrorDialog(error_type, IDS_COULDNT_STARTUP_PROFILE_ERROR,
                           "Error creating primary profile.");
    return nullptr;
  }
#endif  // defined(OS_CHROMEOS) || defined(OS_ANDROID)

  UMA_HISTOGRAM_LONG_TIMES(
      "Startup.CreateFirstProfile", base::Time::Now() - start);
  return profile;
}

#if defined(OS_MACOSX)
OSStatus KeychainCallback(SecKeychainEvent keychain_event,
                          SecKeychainCallbackInfo* info, void* context) {
  return noErr;
}
#endif  // defined(OS_MACOSX)

void RegisterComponentsForUpdate(PrefService* profile_prefs) {
  auto* const cus = g_browser_process->component_updater();

  if (base::FeatureList::IsEnabled(features::kImprovedRecoveryComponent))
    RegisterRecoveryImprovedComponent(cus, g_browser_process->local_state());
  else
    RegisterRecoveryComponent(cus, g_browser_process->local_state());

#if !defined(OS_ANDROID)
  RegisterPepperFlashComponent(cus);
#endif  // !defined(OS_ANDROID)

#if BUILDFLAG(ENABLE_LIBRARY_CDMS)
  RegisterWidevineCdmComponent(cus);
#endif  // BUILDFLAG(ENABLE_LIBRARY_CDMS)

#if BUILDFLAG(ENABLE_NACL) && !defined(OS_ANDROID)
#if defined(OS_CHROMEOS)
  // PNaCl on Chrome OS is on rootfs and there is no need to download it. But
  // Chrome4ChromeOS on Linux doesn't contain PNaCl so enable component
  // installer when running on Linux. See crbug.com/422121 for more details.
  if (!base::SysInfo::IsRunningOnChromeOS())
#endif  // defined(OS_CHROMEOS)
    RegisterPnaclComponent(cus);
#endif  // BUILDFLAG(ENABLE_NACL) && !defined(OS_ANDROID)

  component_updater::SupervisedUserWhitelistInstaller* whitelist_installer =
      g_browser_process->supervised_user_whitelist_installer();
  whitelist_installer->RegisterComponents();

  RegisterSubresourceFilterComponent(cus);
  RegisterOptimizationHintsComponent(cus, profile_prefs);

  base::FilePath path;
  if (base::PathService::Get(chrome::DIR_USER_DATA, &path)) {
    // The CRLSet component previously resided in a different location: delete
    // the old file.
    component_updater::DeleteLegacyCRLSet(path);
#if !defined(OS_CHROMEOS) && !defined(OS_ANDROID)
    // CRLSetFetcher attempts to load a CRL set from either the local disk or
    // network.
    // For Chrome OS this registration is delayed until user login.
    // On Android, we do not register at all.
    component_updater::RegisterCRLSetComponent(cus, path);
#endif  // !defined(OS_CHROMEOS) && !defined(OS_ANDROID)

#if !defined(OS_CHROMEOS) && !defined(OS_ANDROID)
    // Registration of the STH set fetcher here is not done for:
    // Android: Because the story around CT on Mobile is not finalized yet.
    // Chrome OS: On Chrome OS this registration is delayed until user login.
    RegisterSTHSetComponent(cus, path);
#endif  // !defined(OS_CHROMEOS) && !defined(OS_ANDROID)
    RegisterOriginTrialsComponent(cus, path);

    RegisterFileTypePoliciesComponent(cus, path);

#if BUILDFLAG(ENABLE_CAPTIVE_PORTAL_DETECTION)
    RegisterSSLErrorAssistantComponent(cus, path);
#endif
  }

  RegisterMediaEngagementPreloadComponent(cus, base::OnceClosure());

#if defined(OS_WIN)
  // SwReporter is only needed for official builds.  However, to enable testing
  // on chromium build bots, it is always registered here and
  // RegisterSwReporterComponent() has support for running only in official
  // builds or tests.
  RegisterSwReporterComponent(cus);
#if defined(GOOGLE_CHROME_BUILD)
  RegisterThirdPartyModuleListComponent(cus);
#endif  // defined(GOOGLE_CHROME_BUILD)
#endif  // defined(OS_WIN)
}

#if !defined(OS_ANDROID)
bool ProcessSingletonNotificationCallback(
    const base::CommandLine& command_line,
    const base::FilePath& current_directory) {
  // Drop the request if the browser process is already in shutdown path.
  if (!g_browser_process || g_browser_process->IsShuttingDown())
    return false;

  if (command_line.HasSwitch(switches::kOriginalProcessStartTime)) {
    std::string start_time_string =
        command_line.GetSwitchValueASCII(switches::kOriginalProcessStartTime);
    int64_t remote_start_time;
    if (base::StringToInt64(start_time_string, &remote_start_time)) {
      base::TimeDelta elapsed =
          base::Time::Now() - base::Time::FromInternalValue(remote_start_time);
      if (command_line.HasSwitch(switches::kFastStart)) {
        UMA_HISTOGRAM_LONG_TIMES(
            "Startup.WarmStartTimeFromRemoteProcessStartFast", elapsed);
      } else {
        UMA_HISTOGRAM_LONG_TIMES(
            "Startup.WarmStartTimeFromRemoteProcessStart", elapsed);
      }
    }
  }

  g_browser_process->platform_part()->PlatformSpecificCommandLineProcessing(
      command_line);

  base::FilePath user_data_dir =
      g_browser_process->profile_manager()->user_data_dir();
  base::FilePath startup_profile_dir =
      GetStartupProfilePath(user_data_dir, command_line);

  StartupBrowserCreator::ProcessCommandLineAlreadyRunning(
      command_line, current_directory, startup_profile_dir);

  // Record now as the last successful chrome start.
  if (ShouldRecordActiveUse(command_line))
    GoogleUpdateSettings::SetLastRunTime();

  return true;
}
#endif  // !defined(OS_ANDROID)

class ScopedMainMessageLoopRunEvent {
 public:
  ScopedMainMessageLoopRunEvent() {
    TRACE_EVENT_ASYNC_BEGIN0(
        "toplevel", "ChromeBrowserMainParts::MainMessageLoopRun", this);
  }
  ~ScopedMainMessageLoopRunEvent() {
    TRACE_EVENT_ASYNC_END0("toplevel",
                           "ChromeBrowserMainParts::MainMessageLoopRun", this);
  }
};

// Check if the policy to allow WebDriver to override Site Isolation is set and
// if the user actually wants to use WebDriver which will cause us to skip
// even checking if Site Isolation should be turned on by policy.
bool IsWebDriverOverridingPolicy(PrefService* local_state) {
  auto* command_line = base::CommandLine::ForCurrentProcess();
  return (!command_line->HasSwitch(switches::kEnableAutomation) ||
          !(local_state->IsManagedPreference(
                prefs::kWebDriverOverridesIncompatiblePolicies) &&
            local_state->GetBoolean(
                prefs::kWebDriverOverridesIncompatiblePolicies)));
}

// The initial read is done synchronously, the TaskPriority is thus only used
// for flushes to disks and BACKGROUND is therefore appropriate. Priority of
// remaining BACKGROUND+BLOCK_SHUTDOWN tasks is bumped by the TaskScheduler on
// shutdown. However, some shutdown use cases happen without
// TaskScheduler::Shutdown() (e.g. ChromeRestartRequest::Start() and
// BrowserProcessImpl::EndSession()) and we must thus unfortunately make this
// USER_VISIBLE until we solve https://crbug.com/747495 to allow bumping
// priority of a sequence on demand.
scoped_refptr<base::SequencedTaskRunner> CreateLocalStateTaskRunner() {
  return base::CreateSequencedTaskRunnerWithTraits(
      {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
       base::TaskShutdownBehavior::BLOCK_SHUTDOWN});
}

// Initializes the shared instance of ResourceBundle and returns the locale. An
// empty string return value indicates failure.
std::string InitResourceBundleAndDetermineLocale(
    const content::MainFunctionParams& params,
    std::unique_ptr<ui::DataPack> data_pack) {
#if defined(OS_MACOSX)
  // TODO(markusheintz): Read preference pref::kApplicationLocale in order
  // to enforce the application locale.
  // Tests always get en-US.
  std::string locale = params.ui_task ? "en-US" : std::string();
#else
  std::string locale =
      g_browser_process->local_state()->GetString(prefs::kApplicationLocale);
#endif

  TRACE_EVENT0("startup",
               "ChromeBrowserMainParts::InitResourceBundleAndDetermineLocale");
  // On a POSIX OS other than ChromeOS, the parameter that is passed to the
  // method InitSharedInstance is ignored.
  locale = ui::ResourceBundle::InitSharedInstanceWithLocale(
      locale, nullptr, ui::ResourceBundle::LOAD_COMMON_RESOURCES);

  if (data_pack) {
    ui::ResourceBundle::GetSharedInstance().AddDataPack(std::move(data_pack));
  } else {
    LOG(ERROR) << "Failed to load resources.pak\n"
               << "Some features may not be available.";
  }

  return locale;
}

bool IsSiteIsolationEnterprisePolicyApplicable() {
#if defined(OS_ANDROID)
  // https://crbug.com/844118: Limiting policy to devices with > 1GB RAM.
  // Using 1077 rather than 1024 because 1) it helps ensure that devices with
  // exactly 1GB of RAM won't get included because of inaccuracies or off-by-one
  // errors and 2) this is the bucket boundary in Memory.Stats.Win.TotalPhys2.
  bool have_enough_memory = base::SysInfo::AmountOfPhysicalMemoryMB() > 1077;
  return have_enough_memory;
#else
  return true;
#endif
}

}  // namespace

namespace chrome_browser {

// This error message is not localized because we failed to load the
// localization data files.
#if defined(OS_WIN)
const char kMissingLocaleDataTitle[] = "Missing File Error";
#endif  // defined(OS_WIN)

#if defined(OS_WIN)
// TODO(port) This should be used on Linux Aura as well. http://crbug.com/338969
const char kMissingLocaleDataMessage[] =
    "Unable to find locale data files. Please reinstall.";
#endif  // defined(OS_WIN)

}  // namespace chrome_browser

#if !defined(OS_ANDROID)
// A TaskRunner that defers tasks until the real task runner is up and running.
// This is used during early initialization, before the real task runner has
// been created. DeferringTaskRunner has the following states.
//
// . kInstalled: the initial state. Tasks are added to |deferred_runner_|. In
//   this state this is installed as the active ThreadTaskRunnerHandle.
// . kWaitingForMainToStart: this is no longer the active
//   ThreadTaskRunnerHandle, tasks are still deferred though.
// . kMessageLoopRunning: last state, all deferred tasks are flushed to the
//   active ThreadTaskRunnerHandle.
//
// The state changes by calling AdvanceToNextState(), and state changes in
// order.
class ChromeBrowserMainParts::DeferringTaskRunner
    : public base::SingleThreadTaskRunner {
 public:
  DeferringTaskRunner() {
    deferred_runner_ =
        base::MakeRefCounted<base::DeferredSequencedTaskRunner>();
    thread_task_runner_handle_ =
        std::make_unique<base::ThreadTaskRunnerHandle>(this);
  }

  bool is_at_end_state() const {
    base::AutoLock lock(lock_);
    return state_ == State::kMessageLoopRunning;
  }

  // See description above class for details.
  void AdvanceToNextState() {
    std::unique_ptr<base::ThreadTaskRunnerHandle> thread_task_runner_handle;
    {
      base::AutoLock lock(lock_);
      switch (state_) {
        case State::kInstalled:
          state_ = State::kWaitingForMainToStart;
          // ~ThreadTaskRunnerHandle calls back to
          // RunsTasksInCurrentSequence(), so destruction has to happen when
          // the lock isn't held.
          thread_task_runner_handle = std::move(thread_task_runner_handle_);
          break;

        case State::kWaitingForMainToStart:
          state_ = State::kMessageLoopRunning;
          deferred_runner_->StartWithTaskRunner(
              base::ThreadTaskRunnerHandle::Get());
          deferred_runner_ = nullptr;
          main_thread_task_runner_ = base::ThreadTaskRunnerHandle::Get();
          break;

        case State::kMessageLoopRunning:
          NOTREACHED();
      }
    }
  }

  // base::SequencedTaskRunner:
  bool PostDelayedTask(const base::Location& from_here,
                       base::OnceClosure task,
                       base::TimeDelta delay) override {
    base::AutoLock lock(lock_);
    switch (state_) {
      case State::kInstalled:
      case State::kWaitingForMainToStart:
        return deferred_runner_->PostDelayedTask(from_here, std::move(task),
                                                 delay);

      case State::kMessageLoopRunning:
        return main_thread_task_runner_->PostDelayedTask(
            from_here, std::move(task), delay);
    }
    NOTREACHED();
    return false;
  }
  bool RunsTasksInCurrentSequence() const override {
    base::AutoLock lock(lock_);
    switch (state_) {
      case State::kInstalled:
      case State::kWaitingForMainToStart:
        return deferred_runner_->RunsTasksInCurrentSequence();

      case State::kMessageLoopRunning:
        return main_thread_task_runner_->RunsTasksInCurrentSequence();
    }
    NOTREACHED();
    return false;
  }
  bool PostNonNestableDelayedTask(const base::Location& from_here,
                                  base::OnceClosure task,
                                  base::TimeDelta delay) override {
    base::AutoLock lock(lock_);
    switch (state_) {
      case State::kInstalled:
      case State::kWaitingForMainToStart:
        return deferred_runner_->PostNonNestableDelayedTask(
            from_here, std::move(task), delay);

      case State::kMessageLoopRunning:
        return main_thread_task_runner_->PostNonNestableDelayedTask(
            from_here, std::move(task), delay);
    }
    NOTREACHED();
    return false;
  }

 private:
  // See class description for details on states.
  enum class State {
    kInstalled,
    kWaitingForMainToStart,
    kMessageLoopRunning,
  };

  ~DeferringTaskRunner() override = default;

  // |lock_| protects *all* members.
  mutable base::Lock lock_;
  scoped_refptr<base::DeferredSequencedTaskRunner> deferred_runner_;
  scoped_refptr<SingleThreadTaskRunner> main_thread_task_runner_;
  std::unique_ptr<base::ThreadTaskRunnerHandle> thread_task_runner_handle_;
  State state_ = State::kInstalled;

  DISALLOW_COPY_AND_ASSIGN(DeferringTaskRunner);
};
#endif

// BrowserMainParts ------------------------------------------------------------

ChromeBrowserMainParts::ChromeBrowserMainParts(
    const content::MainFunctionParams& parameters,
    std::unique_ptr<ui::DataPack> data_pack)
    : parameters_(parameters),
      parsed_command_line_(parameters.command_line),
      result_code_(service_manager::RESULT_CODE_NORMAL_EXIT),
      ui_thread_profiler_(ThreadProfiler::CreateAndStartOnMainThread()),
      should_call_pre_main_loop_start_startup_on_variations_service_(
          !parameters.ui_task),
      profile_(NULL),
      run_message_loop_(true),
      service_manifest_data_pack_(std::move(data_pack)) {
  // If we're running tests (ui_task is non-null).
  if (parameters.ui_task)
    browser_defaults::enable_help_app = false;

  // Chrome disallows cookies by default. All code paths that want to use
  // cookies need to go through one of Chrome's URLRequestContexts which have
  // a ChromeNetworkDelegate attached that selectively allows cookies again.
  net::URLRequest::SetDefaultCookiePolicyToBlock();

#if !defined(OS_ANDROID)
  startup_watcher_ = std::make_unique<StartupTimeBomb>();
  shutdown_watcher_ = std::make_unique<ShutdownWatcherHelper>();

  // This needs to be created in the constructor as Chrome OS now creates some
  // classes that depend upon a ThreadTaskRunnerHandle in
  // PreEarlyInitialization().
  initial_task_runner_ = base::MakeRefCounted<DeferringTaskRunner>();
#endif  // !defined(OS_ANDROID)
}

ChromeBrowserMainParts::~ChromeBrowserMainParts() {
  for (int i = static_cast<int>(chrome_extra_parts_.size())-1; i >= 0; --i)
    delete chrome_extra_parts_[i];
  chrome_extra_parts_.clear();
}

void ChromeBrowserMainParts::SetupFieldTrials() {
  // Initialize FieldTrialList to support FieldTrials. This is intentionally
  // leaked since it needs to live for the duration of the browser process and
  // there's no benefit in cleaning it up at exit.
  base::FieldTrialList* leaked_field_trial_list = new base::FieldTrialList(
      browser_process_->GetMetricsServicesManager()->CreateEntropyProvider());
  ANNOTATE_LEAKING_OBJECT_PTR(leaked_field_trial_list);
  ignore_result(leaked_field_trial_list);

  auto feature_list = std::make_unique<base::FeatureList>();

  // Associate parameters chosen in about:flags and create trial/group for them.
  flags_ui::PrefServiceFlagsStorage flags_storage(
      g_browser_process->local_state());
  std::vector<std::string> variation_ids =
      about_flags::RegisterAllFeatureVariationParameters(
          &flags_storage, feature_list.get());

  std::set<std::string> unforceable_field_trials;
#if defined(OFFICIAL_BUILD)
  unforceable_field_trials.insert("SettingsEnforcement");
#endif  // defined(OFFICIAL_BUILD)

  variations::VariationsService* variations_service =
      browser_process_->variations_service();
  variations_service->SetupFieldTrials(
      cc::switches::kEnableGpuBenchmarking, switches::kEnableFeatures,
      switches::kDisableFeatures, unforceable_field_trials, variation_ids,
      std::move(feature_list), &browser_field_trials_);
  variations::InitCrashKeys();

  // Initialize FieldTrialSynchronizer system. This is a singleton and is used
  // for posting tasks via base::Bind. Its deleted when it goes out of scope.
  // Even though base::Bind does AddRef and Release, the object will not be
  // deleted after the Task is executed.
  field_trial_synchronizer_ = new FieldTrialSynchronizer();
}

void ChromeBrowserMainParts::SetupMetrics() {
  TRACE_EVENT0("startup", "ChromeBrowserMainParts::SetupMetrics");
  metrics::MetricsService* metrics = browser_process_->metrics_service();
  metrics->synthetic_trial_registry()->AddSyntheticTrialObserver(
      variations::VariationsHttpHeaderProvider::GetInstance());
  metrics->synthetic_trial_registry()->AddSyntheticTrialObserver(
      variations::SyntheticTrialsActiveGroupIdProvider::GetInstance());
  // Now that field trials have been created, initializes metrics recording.
  metrics->InitializeMetricsRecordingState();
}

void ChromeBrowserMainParts::StartMetricsRecording() {
  TRACE_EVENT0("startup", "ChromeBrowserMainParts::StartMetricsRecording");

  g_browser_process->metrics_service()->CheckForClonedInstall();

#if defined(OS_WIN)
  // The last live timestamp is used to assess whether a browser crash occurred
  // due to a full system crash. Update the last live timestamp on a slow
  // schedule to get the bast possible accuracy for the assessment.
  g_browser_process->metrics_service()->StartUpdatingLastLiveTimestamp();
#endif

  // Register a synthetic field trial for the sampling profiler configuration
  // that was already chosen.
  std::string trial_name, group_name;
  if (StackSamplingConfiguration::Get()->GetSyntheticFieldTrial(&trial_name,
                                                                &group_name)) {
    ChromeMetricsServiceAccessor::RegisterSyntheticFieldTrial(trial_name,
                                                              group_name);
  }

  g_browser_process->GetMetricsServicesManager()->UpdateUploadPermissions(true);
}

void ChromeBrowserMainParts::RecordBrowserStartupTime() {
  // Don't record any metrics if UI was displayed before this point e.g.
  // warning dialogs or browser was started in background mode.
  if (startup_metric_utils::WasMainWindowStartupInterrupted())
    return;

  bool is_first_run = false;
#if !defined(OS_ANDROID)
  // On Android, first run is handled in Java code, and the C++ side of Chrome
  // doesn't know if this is the first run. This will cause some inaccuracy in
  // the UMA statistics, but this should be minor (first runs are rare).
  is_first_run = first_run::IsChromeFirstRun();
#endif  // defined(OS_ANDROID)

  // Record collected startup metrics.
  startup_metric_utils::RecordBrowserMainMessageLoopStart(
      base::TimeTicks::Now(), is_first_run, g_browser_process->local_state());
}

void ChromeBrowserMainParts::SetupOriginTrialsCommandLine(
    PrefService* local_state) {
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  if (!command_line->HasSwitch(switches::kOriginTrialPublicKey)) {
    std::string new_public_key =
        local_state->GetString(prefs::kOriginTrialPublicKey);
    if (!new_public_key.empty()) {
      command_line->AppendSwitchASCII(
          switches::kOriginTrialPublicKey,
          local_state->GetString(prefs::kOriginTrialPublicKey));
    }
  }
  if (!command_line->HasSwitch(switches::kOriginTrialDisabledFeatures)) {
    const base::ListValue* override_disabled_feature_list =
        local_state->GetList(prefs::kOriginTrialDisabledFeatures);
    if (override_disabled_feature_list) {
      std::vector<base::StringPiece> disabled_features;
      base::StringPiece disabled_feature;
      for (const auto& item : *override_disabled_feature_list) {
        if (item.GetAsString(&disabled_feature)) {
          disabled_features.push_back(disabled_feature);
        }
      }
      if (!disabled_features.empty()) {
        const std::string override_disabled_features =
            base::JoinString(disabled_features, "|");
        command_line->AppendSwitchASCII(switches::kOriginTrialDisabledFeatures,
                                        override_disabled_features);
      }
    }
  }
  if (!command_line->HasSwitch(switches::kOriginTrialDisabledTokens)) {
    const base::ListValue* disabled_token_list =
        local_state->GetList(prefs::kOriginTrialDisabledTokens);
    if (disabled_token_list) {
      std::vector<base::StringPiece> disabled_tokens;
      base::StringPiece disabled_token;
      for (const auto& item : *disabled_token_list) {
        if (item.GetAsString(&disabled_token)) {
          disabled_tokens.push_back(disabled_token);
        }
      }
      if (!disabled_tokens.empty()) {
        const std::string disabled_token_switch =
            base::JoinString(disabled_tokens, "|");
        command_line->AppendSwitchASCII(switches::kOriginTrialDisabledTokens,
                                        disabled_token_switch);
      }
    }
  }
}

// -----------------------------------------------------------------------------
// TODO(viettrungluu): move more/rest of BrowserMain() into BrowserMainParts.

#if defined(OS_WIN)
#define DLLEXPORT __declspec(dllexport)

// We use extern C for the prototype DLLEXPORT to avoid C++ name mangling.
extern "C" {
DLLEXPORT void __cdecl RelaunchChromeBrowserWithNewCommandLineIfNeeded();
}

DLLEXPORT void __cdecl RelaunchChromeBrowserWithNewCommandLineIfNeeded() {
  // Need an instance of AtExitManager to handle singleton creations and
  // deletions.  We need this new instance because, the old instance created
  // in ChromeMain() got destructed when the function returned.
  base::AtExitManager exit_manager;
  upgrade_util::RelaunchChromeBrowserWithNewCommandLineIfNeeded();
}
#endif

// content::BrowserMainParts implementation ------------------------------------

bool ChromeBrowserMainParts::ShouldContentCreateFeatureList() {
  // Chrome creates the FeatureList, so no need for content to do the same.
  return false;
}

int ChromeBrowserMainParts::PreEarlyInitialization() {
  TRACE_EVENT0("startup", "ChromeBrowserMainParts::PreEarlyInitialization");
  for (size_t i = 0; i < chrome_extra_parts_.size(); ++i)
    chrome_extra_parts_[i]->PreEarlyInitialization();

  // Create BrowserProcess in PreEarlyInitialization() so that we can load
  // field trials (and all it depends upon).
  scoped_refptr<base::SequencedTaskRunner> local_state_task_runner =
      CreateLocalStateTaskRunner();
  browser_process_ =
      std::make_unique<BrowserProcessImpl>(local_state_task_runner.get());

  bool failed_to_load_resource_bundle = false;
  const int load_local_state_result = LoadLocalState(
      local_state_task_runner.get(), &failed_to_load_resource_bundle);
  if (load_local_state_result == chrome::RESULT_CODE_MISSING_DATA &&
      failed_to_load_resource_bundle) {
    if (base::CommandLine::ForCurrentProcess()->HasSwitch(
            switches::kNoErrorDialogs)) {
      return chrome::RESULT_CODE_MISSING_DATA;
    }
    // Continue on and show error later (once UI has been initialized and main
    // message loop is running).
    return service_manager::RESULT_CODE_NORMAL_EXIT;
  }
  return load_local_state_result;
}

void ChromeBrowserMainParts::PostEarlyInitialization() {
  TRACE_EVENT0("startup", "ChromeBrowserMainParts::PostEarlyInitialization");
  for (size_t i = 0; i < chrome_extra_parts_.size(); ++i)
    chrome_extra_parts_[i]->PostEarlyInitialization();
}

void ChromeBrowserMainParts::ToolkitInitialized() {
  TRACE_EVENT0("startup", "ChromeBrowserMainParts::ToolkitInitialized");
  for (size_t i = 0; i < chrome_extra_parts_.size(); ++i)
    chrome_extra_parts_[i]->ToolkitInitialized();
}

void ChromeBrowserMainParts::PreMainMessageLoopStart() {
#if !defined(OS_ANDROID)
  initial_task_runner_->AdvanceToNextState();
#endif

  TRACE_EVENT0("startup", "ChromeBrowserMainParts::PreMainMessageLoopStart");

  for (size_t i = 0; i < chrome_extra_parts_.size(); ++i)
    chrome_extra_parts_[i]->PreMainMessageLoopStart();
}

void ChromeBrowserMainParts::PostMainMessageLoopStart() {
  TRACE_EVENT0("startup", "ChromeBrowserMainParts::PostMainMessageLoopStart");

  ui_thread_profiler_->SetMainThreadTaskRunner(
      base::ThreadTaskRunnerHandle::Get());

  // device_event_log must be initialized after the message loop. Calls to
  // {DEVICE}_LOG prior to here will only be logged with VLOG. Some
  // platforms (e.g. chromeos) may have already initialized this.
  if (!device_event_log::IsInitialized())
    device_event_log::Initialize(0 /* default max entries */);

  for (size_t i = 0; i < chrome_extra_parts_.size(); ++i)
    chrome_extra_parts_[i]->PostMainMessageLoopStart();
}

int ChromeBrowserMainParts::PreCreateThreads() {
  // IMPORTANT
  // Calls in this function should not post tasks or create threads as
  // components used to handle those tasks are not yet available. This work
  // should be deferred to PreMainMessageLoopRunImpl.

  TRACE_EVENT0("startup", "ChromeBrowserMainParts::PreCreateThreads");
  result_code_ = PreCreateThreadsImpl();

  if (result_code_ == service_manager::RESULT_CODE_NORMAL_EXIT) {
#if !defined(OS_ANDROID)
    // These members must be initialized before exiting this function normally.
    DCHECK(master_prefs_.get());
    DCHECK(browser_creator_.get());
#endif  // !defined(OS_ANDROID)
    for (size_t i = 0; i < chrome_extra_parts_.size(); ++i)
      chrome_extra_parts_[i]->PreCreateThreads();
  }

  // Create an instance of GpuModeManager to watch gpu mode pref change.
  g_browser_process->gpu_mode_manager();

  return result_code_;
}

int ChromeBrowserMainParts::LoadLocalState(
    base::SequencedTaskRunner* local_state_task_runner,
    bool* failed_to_load_resource_bundle) {
  *failed_to_load_resource_bundle = false;
  if (!base::PathService::Get(chrome::DIR_USER_DATA, &user_data_dir_))
    return chrome::RESULT_CODE_MISSING_DATA;

  InitializeLocalState(local_state_task_runner);

  ConvertFlagsToSwitches();

  browser_process_->local_state()->UpdateCommandLinePrefStore(
      new ChromeCommandLinePrefStore(base::CommandLine::ForCurrentProcess()));

#if !defined(OS_ANDROID)
  master_prefs_ = std::make_unique<first_run::MasterPrefs>();
#endif

  // First run prefs may use the ResourceBundle (and get data from it), so this
  // needs to be before ApplyFirstRunPrefs().
  std::string locale = InitResourceBundleAndDetermineLocale(
      parameters(), std::move(service_manifest_data_pack_));

  if (locale.empty()) {
    *failed_to_load_resource_bundle = true;
    return chrome::RESULT_CODE_MISSING_DATA;
  }

  const int apply_first_run_result = ApplyFirstRunPrefs();
  if (apply_first_run_result != service_manager::RESULT_CODE_NORMAL_EXIT)
    return apply_first_run_result;

  browser_process_->SetApplicationLocale(locale);

  SetupOriginTrialsCommandLine(browser_process_->local_state());

  // Initialize field trials now that the Local State file has been read. This
  // is done as soon as possible (here), so that code using the base::Feature
  // API can be used from this point on. Field trials are also needed by
  // IOThread's initialization in BrowserProcess:PreCreateThreads. Metrics
  // initialization is handled in PreMainMessageLoopRunImpl since it posts
  // tasks.
  SetupFieldTrials();

  metrics::EnableExpiryChecker(chrome_metrics::kExpiredHistogramsHashes,
                               chrome_metrics::kNumExpiredHistograms);

  return service_manager::RESULT_CODE_NORMAL_EXIT;
}

int ChromeBrowserMainParts::ApplyFirstRunPrefs() {
// Android does first run in Java instead of native.
// Chrome OS has its own out-of-box-experience code.
#if !defined(OS_ANDROID) && !defined(OS_CHROMEOS)
  // On first run, we need to process the predictor preferences before the
  // browser's profile_manager object is created, but after ResourceBundle
  // is initialized.
  if (!first_run::IsChromeFirstRun())
    return service_manager::RESULT_CODE_NORMAL_EXIT;

  first_run::ProcessMasterPreferencesResult pmp_result =
      first_run::ProcessMasterPreferences(user_data_dir_, master_prefs_.get());
  if (pmp_result == first_run::EULA_EXIT_NOW)
    return chrome::RESULT_CODE_EULA_REFUSED;

  // TODO(macourteau): refactor preferences that are copied from
  // master_preferences into local_state, as a "local_state" section in
  // master preferences. If possible, a generic solution would be preferred
  // over a copy one-by-one of specific preferences. Also see related TODO
  // in first_run.h.

  PrefService* local_state = g_browser_process->local_state();
  // Store the initial VariationsService seed in local state, if it exists
  // in master prefs.
  if (!master_prefs_->compressed_variations_seed.empty()) {
    local_state->SetString(variations::prefs::kVariationsCompressedSeed,
                           master_prefs_->compressed_variations_seed);
    if (!master_prefs_->variations_seed_signature.empty()) {
      local_state->SetString(variations::prefs::kVariationsSeedSignature,
                             master_prefs_->variations_seed_signature);
    }
    // Set the variation seed date to the current system time. If the user's
    // clock is incorrect, this may cause some field trial expiry checks to
    // not do the right thing until the next seed update from the server,
    // when this value will be updated.
    local_state->SetInt64(variations::prefs::kVariationsSeedDate,
                          base::Time::Now().ToInternalValue());
  }

  if (!master_prefs_->suppress_default_browser_prompt_for_version.empty()) {
    local_state->SetString(
        prefs::kBrowserSuppressDefaultBrowserPrompt,
        master_prefs_->suppress_default_browser_prompt_for_version);
  }

#if defined(OS_WIN)
  if (!master_prefs_->welcome_page_on_os_upgrade_enabled)
    local_state->SetBoolean(prefs::kWelcomePageOnOSUpgradeEnabled, false);
#endif
#endif  // !defined(OS_ANDROID) && !defined(OS_CHROMEOS)
  return service_manager::RESULT_CODE_NORMAL_EXIT;
}

int ChromeBrowserMainParts::PreCreateThreadsImpl() {
  TRACE_EVENT0("startup", "ChromeBrowserMainParts::PreCreateThreadsImpl")
  run_message_loop_ = false;

  if (browser_process_->GetApplicationLocale().empty()) {
    ShowMissingLocaleMessageBox();
    return chrome::RESULT_CODE_MISSING_DATA;
  }

#if !defined(OS_ANDROID)
  chrome::MaybeShowInvalidUserDataDirWarningDialog();
#endif  // !defined(OS_ANDROID)

  DCHECK(!user_data_dir_.empty());

  // Force MediaCaptureDevicesDispatcher to be created on UI thread.
  MediaCaptureDevicesDispatcher::GetInstance();

  // Android's first run is done in Java instead of native.
#if !defined(OS_ANDROID)
  process_singleton_.reset(new ChromeProcessSingleton(
      user_data_dir_, base::Bind(&ProcessSingletonNotificationCallback)));

  // Cache first run state early.
  first_run::IsChromeFirstRun();

  // The real ThreadTaskRunnerHandle has been installed. Flush all the tasks
  // and release the ref to |initial_task_runner_| so that it may be destroyed.
  initial_task_runner_->AdvanceToNextState();
  DCHECK(initial_task_runner_->is_at_end_state());
  initial_task_runner_ = nullptr;
#endif  // !defined(OS_ANDROID)

  PrefService* local_state = browser_process_->local_state();

#if defined(OS_CHROMEOS)
  chromeos::CrosSettings::Initialize();
#endif  // defined(OS_CHROMEOS)

  {
    TRACE_EVENT0(
        "startup",
        "ChromeBrowserMainParts::PreCreateThreadsImpl:InitBrowserProcessImpl");
    browser_process_->Init();
  }

#if !defined(OS_ANDROID)
  // These members must be initialized before returning from this function.
  // Android doesn't use StartupBrowserCreator.
  browser_creator_.reset(new StartupBrowserCreator);
  // TODO(yfriedman): Refactor Android to re-use UMABrowsingActivityObserver
  chrome::UMABrowsingActivityObserver::Init();
#endif  // !defined(OS_ANDROID)

  // The MaterialDesignController needs to look at command line flags, which
  // are not available until this point. Now that they are, proceed with
  // initializing the MaterialDesignController.
  ui::MaterialDesignController::Initialize();

#if defined(OS_WIN)
  // This is needed to enable ETW exporting when requested in about:flags.
  // Normally, we enable it in ContentMainRunnerImpl::Initialize when the flag
  // is present on the command line but flags in about:flags are converted only
  // after this function runs. Note that this starts exporting later which
  // affects tracing the browser startup. Also, this is only relevant for the
  // browser process, as other processes will get all the flags on their command
  // line regardless of the origin (command line or about:flags).
  if (parsed_command_line().HasSwitch(switches::kTraceExportEventsToETW))
    base::trace_event::TraceEventETWExport::EnableETWExport();
#endif  // OS_WIN

  // Reset the command line in the crash report details, since we may have
  // just changed it to include experiments.
  crash_keys::SetCrashKeysFromCommandLine(
      *base::CommandLine::ForCurrentProcess());

  browser_process_->browser_policy_connector()->OnResourceBundleCreated();

// Android does first run in Java instead of native.
// Chrome OS has its own out-of-box-experience code.
#if !defined(OS_ANDROID) && !defined(OS_CHROMEOS)
  if (first_run::IsChromeFirstRun()) {
    if (!parsed_command_line().HasSwitch(switches::kApp) &&
        !parsed_command_line().HasSwitch(switches::kAppId) &&
        !parsed_command_line().HasSwitch(switches::kShowAppList)) {
      AddFirstRunNewTabs(browser_creator_.get(), master_prefs_->new_tabs);
    }

#if defined(OS_MACOSX) || defined(OS_LINUX)
    // Create directory for user-level Native Messaging manifest files. This
    // makes it less likely that the directory will be created by third-party
    // software with incorrect owner or permission. See crbug.com/725513 .
    base::FilePath user_native_messaging_dir;
    CHECK(base::PathService::Get(chrome::DIR_USER_NATIVE_MESSAGING,
                                 &user_native_messaging_dir));
    if (!base::PathExists(user_native_messaging_dir))
      base::CreateDirectory(user_native_messaging_dir);
#endif  // defined(OS_MACOSX) || defined(OS_LINUX)
  }
#endif  // !defined(OS_ANDROID) && !defined(OS_CHROMEOS)

#if defined(OS_LINUX) || defined(OS_OPENBSD)
  // Set the product channel for crash reports.
  breakpad::SetChannelCrashKey(chrome::GetChannelName());
#endif  // defined(OS_LINUX) || defined(OS_OPENBSD)

#if defined(OS_MACOSX)
  // Get the Keychain API to register for distributed notifications on the main
  // thread, which has a proper CFRunloop, instead of later on the I/O thread,
  // which doesn't. This ensures those notifications will get delivered
  // properly. See issue 37766.
  // (Note that the callback mask here is empty. I don't want to register for
  // any callbacks, I just want to initialize the mechanism.)
  SecKeychainAddCallback(&KeychainCallback, 0, NULL);
#endif  // defined(OS_MACOSX)

#if BUILDFLAG(ENABLE_VR)
  content::WebvrServiceProvider::SetWebvrServiceCallback(
      base::Bind(&vr::VRServiceImpl::Create));
#endif

  // Enable Navigation Tracing only if a trace upload url is specified.
  if (parsed_command_line_.HasSwitch(switches::kEnableNavigationTracing) &&
      parsed_command_line_.HasSwitch(switches::kTraceUploadURL)) {
    tracing::SetupNavigationTracing();
  }

#if defined(OS_WIN) || defined(OS_MACOSX) || \
    (defined(OS_LINUX) && !defined(OS_CHROMEOS))
  metrics::DesktopSessionDurationTracker::Initialize();
#endif
  metrics::RendererUptimeTracker::Initialize();

  if (IsWebDriverOverridingPolicy(local_state)) {
    auto* command_line = base::CommandLine::ForCurrentProcess();
    // Add Site Isolation switches as dictated by policy.
    if (local_state->GetBoolean(prefs::kSitePerProcess) &&
        IsSiteIsolationEnterprisePolicyApplicable() &&
        !command_line->HasSwitch(switches::kSitePerProcess)) {
      command_line->AppendSwitch(switches::kSitePerProcess);
    }
    // We apply the flag always when it differs from the command line state,
    // because we don't want the command-line switch to take precedence over
    // enterprise policy. (This behavior is in harmony with other enterprise
    // policy settings.)
    if (local_state->HasPrefPath(prefs::kIsolateOrigins) &&
        IsSiteIsolationEnterprisePolicyApplicable() &&
        (!command_line->HasSwitch(switches::kIsolateOrigins) ||
         command_line->GetSwitchValueASCII(switches::kIsolateOrigins) !=
             local_state->GetString(prefs::kIsolateOrigins))) {
      command_line->AppendSwitchASCII(
          switches::kIsolateOrigins,
          local_state->GetString(prefs::kIsolateOrigins));
    }
  }

  // The admin should also be able to use these policies to override trials that
  // will try to turn site isolation on per default.
  // Note that disabling either SitePerProcess or IsolateOrigins via policy will
  // disable both types of field trials.
  if ((local_state->IsManagedPreference(prefs::kSitePerProcess) &&
       !local_state->GetBoolean(prefs::kSitePerProcess)) ||
      (local_state->IsManagedPreference(prefs::kIsolateOrigins) &&
       local_state->GetString(prefs::kIsolateOrigins).empty())) {
    base::CommandLine::ForCurrentProcess()->AppendSwitch(
        switches::kDisableSiteIsolationTrials);
  }

  // ChromeOS needs ui::ResourceBundle::InitSharedInstance to be called before
  // this.
  browser_process_->PreCreateThreads(parsed_command_line());

  // This must occur in PreCreateThreads() because it initializes global state
  // which is then read by all threads without synchronization. It must be after
  // browser_process_->PreCreateThreads() as that instantiates the IOThread
  // which is used in SetupMetrics().
  SetupMetrics();

  return service_manager::RESULT_CODE_NORMAL_EXIT;
}

void ChromeBrowserMainParts::PostCreateThreads() {
  // This task should be posted after the IO thread starts, and prior to the
  // base version of the function being invoked. It is functionally okay to post
  // this task in method ChromeBrowserMainParts::BrowserThreadsStarted() which
  // we also need to add in this class, and call this method at the very top of
  // BrowserMainLoop::InitializeMainThread(). PostCreateThreads is preferred to
  // BrowserThreadsStarted as it matches the PreCreateThreads and CreateThreads
  // stages.
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(&ThreadProfiler::StartOnChildThread,
                     metrics::CallStackProfileParams::IO_THREAD));
}

void ChromeBrowserMainParts::ServiceManagerConnectionStarted(
    content::ServiceManagerConnection* connection) {
  // This should be called after the creation of the tracing controller. The
  // tracing controller is created when the service manager connection is
  // started.
  tracing::SetupBackgroundTracingFieldTrial();

  for (size_t i = 0; i < chrome_extra_parts_.size(); ++i)
    chrome_extra_parts_[i]->ServiceManagerConnectionStarted(connection);
}

void ChromeBrowserMainParts::PreMainMessageLoopRun() {
#if defined(USE_AURA)
  if (content::ServiceManagerConnection::GetForProcess() &&
      service_manager::ServiceManagerIsRemote()) {
    content::ServiceManagerConnection::GetForProcess()->
        SetConnectionLostClosure(base::Bind(&chrome::SessionEnding));
  }
#endif
  TRACE_EVENT0("startup", "ChromeBrowserMainParts::PreMainMessageLoopRun");

  result_code_ = PreMainMessageLoopRunImpl();

  for (size_t i = 0; i < chrome_extra_parts_.size(); ++i)
    chrome_extra_parts_[i]->PreMainMessageLoopRun();
}

// PreMainMessageLoopRun calls these extra stages in the following order:
//  PreMainMessageLoopRunImpl()
//   ... initial setup, including browser_process_ setup.
//   PreProfileInit()
//   ... additional setup, including CreateProfile()
//   PostProfileInit()
//   ... additional setup
//   PreBrowserStart()
//   ... browser_creator_->Start (OR parameters().ui_task->Run())
//   PostBrowserStart()

void ChromeBrowserMainParts::PreProfileInit() {
  TRACE_EVENT0("startup", "ChromeBrowserMainParts::PreProfileInit");

  for (size_t i = 0; i < chrome_extra_parts_.size(); ++i)
    chrome_extra_parts_[i]->PreProfileInit();

#if !defined(OS_ANDROID)
  // Initialize the feedback uploader so it can setup notifications for profile
  // creation.
  feedback::FeedbackProfileObserver::Initialize();

  // Ephemeral profiles may have been left behind if the browser crashed.
  g_browser_process->profile_manager()->CleanUpEphemeralProfiles();
  // Files of deleted profiles can also be left behind after a crash.
  g_browser_process->profile_manager()->CleanUpDeletedProfiles();
#endif  // !defined(OS_ANDROID)

#if BUILDFLAG(ENABLE_EXTENSIONS)
  javascript_dialog_extensions_client::InstallClient();
#endif  // BUILDFLAG(ENABLE_EXTENSIONS)

  InstallChromeJavaScriptNativeDialogFactory();
}

void ChromeBrowserMainParts::PostProfileInit() {
  TRACE_EVENT0("startup", "ChromeBrowserMainParts::PostProfileInit");

  g_browser_process->CreateDevToolsProtocolHandler();
  if (parsed_command_line().HasSwitch(::switches::kAutoOpenDevToolsForTabs))
    g_browser_process->CreateDevToolsAutoOpener();

  for (size_t i = 0; i < chrome_extra_parts_.size(); ++i)
    chrome_extra_parts_[i]->PostProfileInit();
}

void ChromeBrowserMainParts::PreBrowserStart() {
  TRACE_EVENT0("startup", "ChromeBrowserMainParts::PreBrowserStart");
  for (size_t i = 0; i < chrome_extra_parts_.size(); ++i)
    chrome_extra_parts_[i]->PreBrowserStart();

// Start the tab manager here so that we give the most amount of time for the
// other services to start up before we start adjusting the oom priority.
#if defined(OS_WIN) || defined(OS_MACOSX) || defined(OS_LINUX)
  g_browser_process->GetTabManager()->Start();
#endif

  // The RulesetService will make the filtering rules available to renderers
  // immediately after its construction, provided that the rules are already
  // available at no cost in an indexed format. This enables activating
  // subresource filtering, if needed, also for page loads on start-up.
  g_browser_process->subresource_filter_ruleset_service();
}

void ChromeBrowserMainParts::PostBrowserStart() {
  TRACE_EVENT0("startup", "ChromeBrowserMainParts::PostBrowserStart");
  for (size_t i = 0; i < chrome_extra_parts_.size(); ++i)
    chrome_extra_parts_[i]->PostBrowserStart();
#if !defined(OS_ANDROID)
  // Allow ProcessSingleton to process messages.
  process_singleton_->Unlock();
#endif  // !defined(OS_ANDROID)
  // Set up a task to delete old WebRTC log files for all profiles. Use a delay
  // to reduce the impact on startup time.
  BrowserThread::PostDelayedTask(
      BrowserThread::UI, FROM_HERE,
      base::BindOnce(&WebRtcLogUtil::DeleteOldWebRtcLogFilesForAllProfiles),
      base::TimeDelta::FromMinutes(1));

#if !defined(OS_ANDROID)
  if (base::FeatureList::IsEnabled(features::kWebUsb)) {
    web_usb_detector_.reset(new WebUsbDetector());
    BrowserThread::PostAfterStartupTask(
        FROM_HERE, BrowserThread::GetTaskRunnerForThread(BrowserThread::UI),
        base::BindOnce(&WebUsbDetector::Initialize,
                       base::Unretained(web_usb_detector_.get())));
  }
  if (base::FeatureList::IsEnabled(features::kTabMetricsLogging)) {
    // Initialize the TabActivityWatcher to begin logging tab activity events.
    resource_coordinator::TabActivityWatcher::GetInstance();
  }
#endif

  // At this point, StartupBrowserCreator::Start has run creating initial
  // browser windows and tabs, but no progress has been made in loading
  // content as the main message loop hasn't started processing tasks yet.
  // We setup to observe to the initial page load here to defer running
  // task posted via PostAfterStartupTask until its complete.
  AfterStartupTaskUtils::StartMonitoringStartup();
}

int ChromeBrowserMainParts::PreMainMessageLoopRunImpl() {
  TRACE_EVENT0("startup", "ChromeBrowserMainParts::PreMainMessageLoopRunImpl");

  SCOPED_UMA_HISTOGRAM_LONG_TIMER("Startup.PreMainMessageLoopRunImplLongTime");
  const base::TimeTicks start_time_step1 = base::TimeTicks::Now();

  // Can't be in SetupFieldTrials() because it needs a task runner.
  blink::MemoryAblationExperiment::MaybeStart(
      BrowserThread::GetTaskRunnerForThread(BrowserThread::IO));

#if defined(OS_WIN)
  // Windows parental controls calls can be slow, so we do an early init here
  // that calculates this value off of the UI thread.
  IncognitoModePrefs::InitializePlatformParentalControls();
#endif

  // Android updates the metrics service dynamically depending on whether the
  // application is in the foreground or not. Do not start here.
#if !defined(OS_ANDROID)
  // Now that the file thread has been started, start recording.
  StartMetricsRecording();
#endif  // !defined(OS_ANDROID)

  if (!base::debug::BeingDebugged()) {
    // Create watchdog thread after creating all other threads because it will
    // watch the other threads and they must be running.
    browser_process_->watchdog_thread();
  }

  // Do any initializating in the browser process that requires all threads
  // running.
  browser_process_->PreMainMessageLoopRun();

  // Record last shutdown time into a histogram.
  browser_shutdown::ReadLastShutdownInfo();

#if defined(OS_WIN)
  // On Windows, we use our startup as an opportunity to do upgrade/uninstall
  // tasks.  Those care whether the browser is already running.  On Linux/Mac,
  // upgrade/uninstall happen separately.
  bool already_running = browser_util::IsBrowserAlreadyRunning();

  // If the command line specifies 'uninstall' then we need to work here
  // unless we detect another chrome browser running.
  if (parsed_command_line().HasSwitch(switches::kUninstall)) {
    return DoUninstallTasks(already_running);
  }

  if (parsed_command_line().HasSwitch(switches::kHideIcons) ||
      parsed_command_line().HasSwitch(switches::kShowIcons)) {
    return ChromeBrowserMainPartsWin::HandleIconsCommands(
        parsed_command_line_);
  }

  ui::SelectFileDialog::SetFactory(new ChromeSelectFileDialogFactory());
#endif  // defined(OS_WIN)

  if (parsed_command_line().HasSwitch(switches::kMakeDefaultBrowser)) {
    bool is_managed = g_browser_process->local_state()->IsManagedPreference(
        prefs::kDefaultBrowserSettingEnabled);
    if (is_managed && !g_browser_process->local_state()->GetBoolean(
        prefs::kDefaultBrowserSettingEnabled)) {
      return static_cast<int>(chrome::RESULT_CODE_ACTION_DISALLOWED_BY_POLICY);
    }

    return shell_integration::SetAsDefaultBrowser()
               ? static_cast<int>(service_manager::RESULT_CODE_NORMAL_EXIT)
               : static_cast<int>(chrome::RESULT_CODE_SHELL_INTEGRATION_FAILED);
  }

#if defined(USE_AURA)
  // Make sure aura::Env has been initialized.
  CHECK(aura::Env::GetInstance());
#endif  // defined(USE_AURA)

  // Android doesn't support extensions and doesn't implement ProcessSingleton.
#if !defined(OS_ANDROID)
  // If the command line specifies --pack-extension, attempt the pack extension
  // startup action and exit.
  if (parsed_command_line().HasSwitch(switches::kPackExtension)) {
    extensions::StartupHelper extension_startup_helper;
    if (extension_startup_helper.PackExtension(parsed_command_line()))
      return service_manager::RESULT_CODE_NORMAL_EXIT;
    return chrome::RESULT_CODE_PACK_EXTENSION_ERROR;
  }

  // When another process is running, use that process instead of starting a
  // new one. NotifyOtherProcess will currently give the other process up to
  // 20 seconds to respond. Note that this needs to be done before we attempt
  // to read the profile.
  notify_result_ = process_singleton_->NotifyOtherProcessOrCreate();
  UMA_HISTOGRAM_ENUMERATION("Chrome.ProcessSingleton.NotifyResult",
                            notify_result_,
                            ProcessSingleton::kNumNotifyResults);
  switch (notify_result_) {
    case ProcessSingleton::PROCESS_NONE:
      // No process already running, fall through to starting a new one.
      g_browser_process->platform_part()->PlatformSpecificCommandLineProcessing(
          *base::CommandLine::ForCurrentProcess());
      break;

    case ProcessSingleton::PROCESS_NOTIFIED:
      printf("%s\n", base::SysWideToNativeMB(
                         base::UTF16ToWide(l10n_util::GetStringUTF16(
                             IDS_USED_EXISTING_BROWSER)))
                         .c_str());

      // Having a differentiated return type for testing allows for tests to
      // verify proper handling of some switches. When not testing, stick to
      // the standard Unix convention of returning zero when things went as
      // expected.
      if (parsed_command_line().HasSwitch(switches::kTestType))
        return chrome::RESULT_CODE_NORMAL_EXIT_PROCESS_NOTIFIED;
      return service_manager::RESULT_CODE_NORMAL_EXIT;

    case ProcessSingleton::PROFILE_IN_USE:
      return chrome::RESULT_CODE_PROFILE_IN_USE;

    case ProcessSingleton::LOCK_ERROR:
      LOG(ERROR) << "Failed to create a ProcessSingleton for your profile "
                    "directory. This means that running multiple instances "
                    "would start multiple browser processes rather than "
                    "opening a new window in the existing process. Aborting "
                    "now to avoid profile corruption.";
      return chrome::RESULT_CODE_PROFILE_IN_USE;
  }

  // Handle special early return paths (which couldn't be processed even earlier
  // as they require the process singleton to be held) first.

  std::string try_chrome =
      parsed_command_line().GetSwitchValueASCII(switches::kTryChromeAgain);

  // The TryChromeDialog may be aborted by a rendezvous from another browser
  // process (e.g., a launch via Chrome's taskbar icon or some such). In this
  // case, browser startup should continue without processing the original
  // command line (the one with --try-chrome-again), but rather with the command
  // line from the other process (handled in
  // ProcessSingletonNotificationCallback thanks to the ProcessSingleton). This
  // variable is cleared in that particular case, leading to a bypass of the
  // StartupBrowserCreator.
  bool process_command_line = true;
  if (!try_chrome.empty()) {
#if defined(OS_WIN)
    // Setup.exe has determined that we need to run a retention experiment
    // and has lauched chrome to show the experiment UI. It is guaranteed that
    // no other Chrome is currently running as the process singleton was
    // successfully grabbed above.
    int try_chrome_int;
    base::StringToInt(try_chrome, &try_chrome_int);
    TryChromeDialog::Result answer = TryChromeDialog::Show(
        try_chrome_int,
        base::Bind(&ChromeProcessSingleton::SetModalDialogNotificationHandler,
                   base::Unretained(process_singleton_.get())));
    switch (answer) {
      case TryChromeDialog::NOT_NOW:
        return chrome::RESULT_CODE_NORMAL_EXIT_CANCEL;
      case TryChromeDialog::OPEN_CHROME_WELCOME:
        browser_creator_->set_welcome_back_page(
            StartupBrowserCreator::WelcomeBackPage::kWelcomeStandard);
        break;
      case TryChromeDialog::OPEN_CHROME_WELCOME_WIN10:
        browser_creator_->set_welcome_back_page(
            StartupBrowserCreator::WelcomeBackPage::kWelcomeWin10);
        break;
      case TryChromeDialog::OPEN_CHROME_DEFAULT:
        break;
      case TryChromeDialog::OPEN_CHROME_DEFER:
        process_command_line = false;
        break;
    }
#else
    // We don't support retention experiments on Mac or Linux.
    return service_manager::RESULT_CODE_NORMAL_EXIT;
#endif  // defined(OS_WIN)
  }
#endif  // !defined(OS_ANDROID)

#if defined(OS_WIN)
  // Do the tasks if chrome has been upgraded while it was last running.
  if (!already_running && upgrade_util::DoUpgradeTasks(parsed_command_line()))
    return service_manager::RESULT_CODE_NORMAL_EXIT;

  // Check if there is any machine level Chrome installed on the current
  // machine. If yes and the current Chrome process is user level, we do not
  // allow the user level Chrome to run. So we notify the user and uninstall
  // user level Chrome.
  // Note this check needs to happen here (after the process singleton was
  // obtained but before potentially creating the first run sentinel).
  if (ChromeBrowserMainPartsWin::CheckMachineLevelInstall())
    return chrome::RESULT_CODE_MACHINE_LEVEL_INSTALL_EXISTS;
#endif  // defined(OS_WIN)

  // Desktop construction occurs here, (required before profile creation).
  PreProfileInit();

  // Profile creation ----------------------------------------------------------

  metrics::MetricsService::SetExecutionPhase(
      metrics::ExecutionPhase::CREATE_PROFILE,
      g_browser_process->local_state());

  UMA_HISTOGRAM_TIMES("Startup.PreMainMessageLoopRunImplStep1Time",
                      base::TimeTicks::Now() - start_time_step1);

  // This step is costly and is already measured in Startup.CreateFirstProfile
  // and more directly Profile.CreateAndInitializeProfile.
  profile_ = CreatePrimaryProfile(parameters(),
                                  user_data_dir_,
                                  parsed_command_line());
  if (!profile_)
    return service_manager::RESULT_CODE_NORMAL_EXIT;

#if !defined(OS_ANDROID)
  const base::TimeTicks start_time_step2 = base::TimeTicks::Now();
  // The first run sentinel must be created after the process singleton was
  // grabbed and no early return paths were otherwise hit above.
  first_run::CreateSentinelIfNeeded();
#endif  // !defined(OS_ANDROID)

#if BUILDFLAG(ENABLE_BACKGROUND_MODE)
  // Autoload any profiles which are running background apps.
  // TODO(rlp): Do this on a separate thread. See http://crbug.com/99075.
  browser_process_->profile_manager()->AutoloadProfiles();
#endif  // BUILDFLAG(ENABLE_BACKGROUND_MODE)
  // Post-profile init ---------------------------------------------------------

  TranslateService::Initialize();
  if (base::FeatureList::IsEnabled(features::kGeoLanguage) ||
      language::GetOverrideLanguageModel() ==
          language::OverrideLanguageModel::GEO) {
    language::GeoLanguageProvider::GetInstance()->StartUp(
        content::ServiceManagerConnection::GetForProcess()
            ->GetConnector()
            ->Clone());
  }

  // Needs to be done before PostProfileInit, since login manager on CrOS is
  // called inside PostProfileInit.
  content::WebUIControllerFactory::RegisterFactory(
      ChromeWebUIControllerFactory::GetInstance());

#if BUILDFLAG(ENABLE_NACL)
  // NaClBrowserDelegateImpl is accessed inside PostProfileInit().
  // So make sure to create it before that.
  nacl::NaClBrowser::SetDelegate(std::make_unique<NaClBrowserDelegateImpl>(
      browser_process_->profile_manager()));
#endif  // BUILDFLAG(ENABLE_NACL)

  // TODO(stevenjb): Move WIN and MACOSX specific code to appropriate Parts.
  // (requires supporting early exit).
  PostProfileInit();

#if !defined(OS_ANDROID) && !defined(OS_CHROMEOS)
  // Execute first run specific code after the PrefService has been initialized
  // and preferences have been registered since some of the import code depends
  // on preferences.
  if (first_run::IsChromeFirstRun()) {
    first_run::AutoImport(profile_, master_prefs_->import_bookmarks_path);

    // Note: this can pop the first run consent dialog on linux.
    first_run::DoPostImportTasks(profile_,
                                 master_prefs_->make_chrome_default_for_user);

    // The first run dialog is modal, and spins a RunLoop, which could receive
    // a SIGTERM, and call chrome::AttemptExit(). Exit cleanly in that case.
    if (browser_shutdown::IsTryingToQuit())
      return service_manager::RESULT_CODE_NORMAL_EXIT;

    if (!master_prefs_->suppress_first_run_default_browser_prompt) {
      browser_creator_->set_show_main_browser_window(
          !ShowFirstRunDefaultBrowserPrompt(profile_));
    } else {
      browser_creator_->set_is_default_browser_dialog_suppressed(true);
    }
  }
#endif  // !defined(OS_ANDROID) && !defined(OS_CHROMEOS)

#if defined(OS_WIN)
  // Sets things up so that if we crash from this point on, a dialog will
  // popup asking the user to restart chrome. It is done this late to avoid
  // testing against a bunch of special cases that are taken care early on.
  ChromeBrowserMainPartsWin::PrepareRestartOnCrashEnviroment(
      parsed_command_line());

  // Registers Chrome with the Windows Restart Manager, which will restore the
  // Chrome session when the computer is restarted after a system update.
  // This could be run as late as WM_QUERYENDSESSION for system update reboots,
  // but should run on startup if extended to handle crashes/hangs/patches.
  // Also, better to run once here than once for each HWND's WM_QUERYENDSESSION.
  ChromeBrowserMainPartsWin::RegisterApplicationRestart(parsed_command_line());

  // Verify that the profile is not on a network share and if so prepare to show
  // notification to the user.
  if (NetworkProfileBubble::ShouldCheckNetworkProfile(profile_)) {
    base::PostTaskWithTraits(
        FROM_HERE, {base::MayBlock()},
        base::Bind(&NetworkProfileBubble::CheckNetworkProfile,
                   profile_->GetPath()));
  }
#endif  // defined(OS_WIN)

#if BUILDFLAG(ENABLE_RLZ) && !defined(OS_CHROMEOS)
  // Init the RLZ library. This just binds the dll and schedules a task on the
  // file thread to be run sometime later. If this is the first run we record
  // the installation event.
  int ping_delay =
      profile_->GetPrefs()->GetInteger(prefs::kRlzPingDelaySeconds);
  // Negative ping delay means to send ping immediately after a first search is
  // recorded.
  rlz::RLZTracker::SetRlzDelegate(
      base::WrapUnique(new ChromeRLZTrackerDelegate));
  rlz::RLZTracker::InitRlzDelayed(
      first_run::IsChromeFirstRun(), ping_delay < 0,
      base::TimeDelta::FromSeconds(abs(ping_delay)),
      ChromeRLZTrackerDelegate::IsGoogleDefaultSearch(profile_),
      ChromeRLZTrackerDelegate::IsGoogleHomepage(profile_),
      ChromeRLZTrackerDelegate::IsGoogleInStartpages(profile_));
#endif  // BUILDFLAG(ENABLE_RLZ) && !defined(OS_CHROMEOS)

  // Configure modules that need access to resources.
  net::NetModule::SetResourceProvider(chrome_common_net::NetResourceProvider);
  media::SetLocalizedStringProvider(
      chrome_common_media::LocalizedStringProvider);

  // In unittest mode, this will do nothing.  In normal mode, this will create
  // the global IntranetRedirectDetector instance, which will promptly go to
  // sleep for seven seconds (to avoid slowing startup), and wake up afterwards
  // to see if it should do anything else.
  //
  // A simpler way of doing all this would be to have some function which could
  // give the time elapsed since startup, and simply have this object check that
  // when asked to initialize itself, but this doesn't seem to exist.
  //
  // This can't be created in the BrowserProcessImpl constructor because it
  // needs to read prefs that get set after that runs.
  browser_process_->intranet_redirect_detector();

#if BUILDFLAG(ENABLE_PRINT_PREVIEW) && !defined(OFFICIAL_BUILD)
  if (parsed_command_line().HasSwitch(switches::kDebugPrint)) {
    base::FilePath path =
        parsed_command_line().GetSwitchValuePath(switches::kDebugPrint);
    if (!path.empty())
      printing::PrintedDocument::SetDebugDumpPath(path);
  }
#endif  // BUILDFLAG(ENABLE_PRINT_PREVIEW) && !defined(OFFICIAL_BUILD)

  HandleTestParameters(parsed_command_line());
  browser_process_->metrics_service()->RecordBreakpadHasDebugger(
      base::debug::BeingDebugged());

  language_usage_metrics::LanguageUsageMetrics::RecordAcceptLanguages(
      profile_->GetPrefs()->GetString(prefs::kAcceptLanguages));
  language_usage_metrics::LanguageUsageMetrics::RecordApplicationLanguage(
      browser_process_->GetApplicationLocale());

// StartupTimeBomb is disabled on Android, see https://crbug.com/366699.
#if !defined(OS_ANDROID)
  // Start watching for hangs during startup. We disarm this hang detector when
  // ThreadWatcher takes over or when browser is shutdown or when
  // startup_watcher_ is deleted.
  metrics::MetricsService::SetExecutionPhase(
      metrics::ExecutionPhase::STARTUP_TIMEBOMB_ARM,
      g_browser_process->local_state());
  startup_watcher_->Arm(base::TimeDelta::FromSeconds(600));
#endif  // !defined(OS_ANDROID)

// On mobile, need for clean shutdown arises only when the application comes
// to foreground (i.e. MetricsService::OnAppEnterForeground is called).
// http://crbug.com/179143
#if !defined(OS_ANDROID)
  // Start watching for a hang.
  browser_process_->metrics_service()->LogNeedForCleanShutdown();
#endif  // !defined(OS_ANDROID)

#if BUILDFLAG(ENABLE_PRINT_PREVIEW) && !defined(OS_CHROMEOS)
  // Create the instance of the cloud print proxy service so that it can launch
  // the service process if needed. This is needed because the service process
  // might have shutdown because an update was available.
  // TODO(torne): this should maybe be done with
  // BrowserContextKeyedServiceFactory::ServiceIsCreatedWithBrowserContext()
  // instead?
  CloudPrintProxyServiceFactory::GetForProfile(profile_);
#endif

  // Start watching all browser threads for responsiveness.
  metrics::MetricsService::SetExecutionPhase(
      metrics::ExecutionPhase::THREAD_WATCHER_START,
      g_browser_process->local_state());
  ThreadWatcherList::StartWatchingAll(parsed_command_line());

  // This has to come before the first GetInstance() call. PreBrowserStart()
  // seems like a reasonable place to put this, except on Android,
  // OfflinePageInfoHandler::Register() below calls GetInstance().
  // TODO(thestig): See if the Android code below can be moved to later.
  sessions::ContentSerializedNavigationDriver::SetInstance(
      ChromeSerializedNavigationDriver::GetInstance());

#if defined(OS_ANDROID)
  ThreadWatcherAndroid::RegisterApplicationStatusListener();
#endif  // defined(OS_ANDROID)

#if BUILDFLAG(ENABLE_OFFLINE_PAGES)
  offline_pages::OfflinePageInfoHandler::Register();
#endif

#if BUILDFLAG(ENABLE_NACL)
  BrowserThread::PostTask(BrowserThread::IO, FROM_HERE,
                          base::BindOnce(nacl::NaClProcessHost::EarlyStartup));
#endif  // BUILDFLAG(ENABLE_NACL)

  // Make sure initial prefs are recorded
  PrefMetricsService::Factory::GetForProfile(profile_);

  PreBrowserStart();

  if (!parsed_command_line().HasSwitch(switches::kDisableComponentUpdate))
    RegisterComponentsForUpdate(profile_->GetPrefs());

#if defined(OS_ANDROID)
  variations::VariationsService* variations_service =
      browser_process_->variations_service();
  if (variations_service) {
    // Just initialize the policy prefs service here. Variations seed fetching
    // will be initialized when the app enters foreground mode.
    variations_service->set_policy_pref_service(profile_->GetPrefs());
  }

#else
  // Most general initialization is behind us, but opening a
  // tab and/or session restore and such is still to be done.
  base::TimeTicks browser_open_start = base::TimeTicks::Now();

  // We are in regular browser boot sequence. Open initial tabs and enter the
  // main message loop.
  std::vector<Profile*> last_opened_profiles;
#if !defined(OS_CHROMEOS)
  // On ChromeOS multiple profiles doesn't apply, and will break if we load
  // them this early as the cryptohome hasn't yet been mounted (which happens
  // only once we log in).
  last_opened_profiles =
      g_browser_process->profile_manager()->GetLastOpenedProfiles();
#endif  // defined(OS_CHROMEOS)

  UMA_HISTOGRAM_TIMES("Startup.PreMainMessageLoopRunImplStep2Time",
                      base::TimeTicks::Now() - start_time_step2);

  // This step is costly and is already measured in
  // Startup.StartupBrowserCreator_Start.
  // See the comment above for an explanation of |process_command_line|.
  const bool started =
      !process_command_line ||
      browser_creator_->Start(parsed_command_line(), base::FilePath(), profile_,
                              last_opened_profiles);
  const base::TimeTicks start_time_step3 = base::TimeTicks::Now();
  if (started) {
#if defined(OS_WIN) || (defined(OS_LINUX) && !defined(OS_CHROMEOS))
    // Initialize autoupdate timer. Timer callback costs basically nothing
    // when browser is not in persistent mode, so it's OK to let it ride on
    // the main thread. This needs to be done here because we don't want
    // to start the timer when Chrome is run inside a test harness.
    browser_process_->StartAutoupdateTimer();
#endif  // defined(OS_WIN) || (defined(OS_LINUX) && !defined(OS_CHROMEOS))

#if defined(OS_LINUX) && !defined(OS_CHROMEOS)
    // On Linux, the running exe will be updated if an upgrade becomes
    // available while the browser is running.  We need to save the last
    // modified time of the exe, so we can compare to determine if there is
    // an upgrade while the browser is kept alive by a persistent extension.
    upgrade_util::SaveLastModifiedTimeOfExe();
#endif  // defined(OS_LINUX) && !defined(OS_CHROMEOS)

    // Record now as the last successful chrome start.
    if (ShouldRecordActiveUse(parsed_command_line()))
      GoogleUpdateSettings::SetLastRunTime();

#if defined(OS_MACOSX)
    // Call Recycle() here as late as possible, before going into the loop
    // because Start() will add things to it while creating the main window.
    if (parameters().autorelease_pool)
      parameters().autorelease_pool->Recycle();
#endif  // defined(OS_MACOSX)

    const base::TimeDelta delta = base::TimeTicks::Now() - browser_open_start;
    startup_metric_utils::RecordBrowserOpenTabsDelta(delta);

    if (should_call_pre_main_loop_start_startup_on_variations_service_) {
      // Request new variations seed information from server.
      variations::VariationsService* variations_service =
          browser_process_->variations_service();
      if (variations_service)
        variations_service->PerformPreMainMessageLoopStartup();
    }
  }
  run_message_loop_ = started;
  browser_creator_.reset();
#endif  // !defined(OS_ANDROID)

  PostBrowserStart();

  if (parameters().ui_task) {
    parameters().ui_task->Run();
    delete parameters().ui_task;
    run_message_loop_ = false;
  }

#if defined(OS_WIN)
  // Clean up old user data directory and disk cache directory.
  downgrade::DeleteMovedUserDataSoon();
#endif

#if defined(OS_ANDROID)
  // We never run the C++ main loop on Android, since the UI thread message
  // loop is controlled by the OS, so this is as close as we can get to
  // the start of the main loop.
  if (result_code_ <= 0) {
    RecordBrowserStartupTime();
  }
#endif  // defined(OS_ANDROID)

#if !defined(OS_ANDROID)
  UMA_HISTOGRAM_TIMES("Startup.PreMainMessageLoopRunImplStep3Time",
                      base::TimeTicks::Now() - start_time_step3);
#endif  // !defined(OS_ANDROID)

  return result_code_;
}

bool ChromeBrowserMainParts::MainMessageLoopRun(int* result_code) {
  // Trace the entry and exit of this method. We don't use the TRACE_EVENT0
  // macro because the tracing infrastructure doesn't expect a synchronous event
  // around the main loop of a thread.
  ScopedMainMessageLoopRunEvent scoped_main_message_loop_run_event;
#if defined(OS_ANDROID)
  // Chrome on Android does not use default MessageLoop. It has its own
  // Android specific MessageLoop
  NOTREACHED();
  return true;
#else
  // Set the result code set in PreMainMessageLoopRun or set above.
  *result_code = result_code_;
  if (!run_message_loop_)
    return true;  // Don't run the default message loop.

  // These should be invoked as close to the start of the browser's
  // UI thread message loop as possible to get a stable measurement
  // across versions.
  RecordBrowserStartupTime();

  DCHECK(base::MessageLoopForUI::IsCurrent());
  base::RunLoop run_loop;

  performance_monitor::PerformanceMonitor::GetInstance()->StartGatherCycle();

  resource_coordinator::RenderProcessProbe::GetInstance()->StartGatherCycle();

  metrics::MetricsService::SetExecutionPhase(
      metrics::ExecutionPhase::MAIN_MESSAGE_LOOP_RUN,
      g_browser_process->local_state());
  run_loop.Run();

  return true;
#endif  // defined(OS_ANDROID)
}

void ChromeBrowserMainParts::PostMainMessageLoopRun() {
  TRACE_EVENT0("startup", "ChromeBrowserMainParts::PostMainMessageLoopRun");
#if defined(OS_ANDROID)
  // Chrome on Android does not use default MessageLoop. It has its own
  // Android specific MessageLoop
  NOTREACHED();
#else
  // Start watching for jank during shutdown. It gets disarmed when
  // |shutdown_watcher_| object is destructed.
  metrics::MetricsService::SetExecutionPhase(
      metrics::ExecutionPhase::SHUTDOWN_TIMEBOMB_ARM,
      g_browser_process->local_state());
  shutdown_watcher_->Arm(base::TimeDelta::FromSeconds(300));

  // Disarm the startup hang detector time bomb if it is still Arm'ed.
  startup_watcher_->Disarm();

  web_usb_detector_.reset();

  for (size_t i = 0; i < chrome_extra_parts_.size(); ++i)
    chrome_extra_parts_[i]->PostMainMessageLoopRun();

  // Some tests don't set parameters.ui_task, so they started translate
  // language fetch that was never completed so we need to cleanup here
  // otherwise it will be done by the destructor in a wrong thread.
  TranslateService::Shutdown(parameters().ui_task == NULL);

  if (notify_result_ == ProcessSingleton::PROCESS_NONE)
    process_singleton_->Cleanup();

  // Stop all tasks that might run on WatchDogThread.
  ThreadWatcherList::StopWatchingAll();

  browser_process_->metrics_service()->Stop();

  restart_last_session_ = browser_shutdown::ShutdownPreThreadsStop();
  browser_process_->StartTearDown();
#endif  // defined(OS_ANDROID)
}

void ChromeBrowserMainParts::PreShutdown() {
  base::StackSamplingProfiler::SetProcessMilestone(
      metrics::CallStackProfileMetricsProvider::SHUTDOWN_START);
}

void ChromeBrowserMainParts::PostDestroyThreads() {
#if defined(OS_ANDROID)
  // On Android, there is no quit/exit. So the browser's main message loop will
  // not finish.
  NOTREACHED();
#else
  int restart_flags = restart_last_session_
                          ? browser_shutdown::RESTART_LAST_SESSION
                          : browser_shutdown::NO_FLAGS;

#if BUILDFLAG(ENABLE_BACKGROUND_MODE)
  if (restart_flags) {
    restart_flags |= BackgroundModeManager::should_restart_in_background()
                         ? browser_shutdown::RESTART_IN_BACKGROUND
                         : browser_shutdown::NO_FLAGS;
  }
#endif  // BUILDFLAG(ENABLE_BACKGROUND_MODE)

  browser_process_->PostDestroyThreads();
  // browser_shutdown takes care of deleting browser_process, so we need to
  // release it.
  ignore_result(browser_process_.release());

  browser_shutdown::ShutdownPostThreadsStop(restart_flags);
  master_prefs_.reset();
  process_singleton_.reset();
  device_event_log::Shutdown();

  // We need to do this check as late as possible, but due to modularity, this
  // may be the last point in Chrome.  This would be more effective if done at
  // a higher level on the stack, so that it is impossible for an early return
  // to bypass this code.  Perhaps we need a *final* hook that is called on all
  // paths from content/browser/browser_main.
  CHECK(metrics::MetricsService::UmaMetricsProperlyShutdown());

#if defined(OS_CHROMEOS)
  chromeos::CrosSettings::Shutdown();
#endif  // defined(OS_CHROMEOS)
#endif  // defined(OS_ANDROID)
}

// Public members:

void ChromeBrowserMainParts::AddParts(ChromeBrowserMainExtraParts* parts) {
  chrome_extra_parts_.push_back(parts);
}
