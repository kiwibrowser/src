// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/arc/boot_phase_monitor/arc_boot_phase_monitor_bridge.h"

#include <utility>

#include "base/bind.h"
#include "base/logging.h"
#include "base/memory/singleton.h"
#include "base/metrics/histogram_macros.h"
#include "chrome/browser/chromeos/arc/arc_util.h"
#include "chrome/browser/chromeos/arc/boot_phase_monitor/arc_instance_throttle.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/sessions/session_restore.h"
#include "chrome/browser/ui/ash/multi_user/multi_user_util.h"
#include "chromeos/cryptohome/cryptohome_parameters.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/session_manager_client.h"
#include "components/arc/arc_bridge_service.h"
#include "components/arc/arc_browser_context_keyed_service_factory_base.h"
#include "components/arc/arc_prefs.h"
#include "components/arc/arc_util.h"
#include "extensions/browser/extension_system.h"
#include "extensions/browser/extension_system_provider.h"
#include "extensions/browser/extensions_browser_client.h"
#include "extensions/common/one_shot_event.h"

namespace arc {
namespace {

void OnEmitArcBooted(bool success) {
  if (!success)
    VLOG(1) << "Failed to emit arc booted signal.";
}

class DefaultDelegateImpl : public ArcBootPhaseMonitorBridge::Delegate {
 public:
  DefaultDelegateImpl() = default;
  ~DefaultDelegateImpl() override = default;

  void DisableCpuRestriction() override {
    SetArcCpuRestriction(false /* do_restrict */);
  }

  void RecordFirstAppLaunchDelayUMA(base::TimeDelta delta) override {
    VLOG(2) << "Launching the first app took "
            << delta.InMillisecondsRoundedUp() << " ms.";
    UMA_HISTOGRAM_CUSTOM_TIMES("Arc.FirstAppLaunchDelay.TimeDelta", delta,
                               base::TimeDelta::FromMilliseconds(1),
                               base::TimeDelta::FromMinutes(2), 50);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(DefaultDelegateImpl);
};

// Singleton factory for ArcBootPhaseMonitorBridge.
class ArcBootPhaseMonitorBridgeFactory
    : public internal::ArcBrowserContextKeyedServiceFactoryBase<
          ArcBootPhaseMonitorBridge,
          ArcBootPhaseMonitorBridgeFactory> {
 public:
  // Factory name used by ArcBrowserContextKeyedServiceFactoryBase.
  static constexpr const char* kName = "ArcBootPhaseMonitorBridgeFactory";

  static ArcBootPhaseMonitorBridgeFactory* GetInstance() {
    return base::Singleton<ArcBootPhaseMonitorBridgeFactory>::get();
  }

 private:
  friend base::DefaultSingletonTraits<ArcBootPhaseMonitorBridgeFactory>;

  ArcBootPhaseMonitorBridgeFactory() {
    DependsOn(extensions::ExtensionsBrowserClient::Get()
                  ->GetExtensionSystemFactory());
  }

  ~ArcBootPhaseMonitorBridgeFactory() override = default;
};

}  // namespace

// static
ArcBootPhaseMonitorBridge* ArcBootPhaseMonitorBridge::GetForBrowserContext(
    content::BrowserContext* context) {
  return ArcBootPhaseMonitorBridgeFactory::GetForBrowserContext(context);
}

// static
ArcBootPhaseMonitorBridge*
ArcBootPhaseMonitorBridge::GetForBrowserContextForTesting(
    content::BrowserContext* context) {
  return ArcBootPhaseMonitorBridgeFactory::GetForBrowserContextForTesting(
      context);
}

// static
void ArcBootPhaseMonitorBridge::RecordFirstAppLaunchDelayUMA(
    content::BrowserContext* context) {
  auto* bridge = arc::ArcBootPhaseMonitorBridge::GetForBrowserContext(context);
  if (bridge)
    bridge->RecordFirstAppLaunchDelayUMAInternal();
}

ArcBootPhaseMonitorBridge::ArcBootPhaseMonitorBridge(
    content::BrowserContext* context,
    ArcBridgeService* bridge_service)
    : context_(context),
      arc_bridge_service_(bridge_service),
      account_id_(multi_user_util::GetAccountIdFromProfile(
          Profile::FromBrowserContext(context))),
      // Set the default delegate. Unit tests may use a different one.
      delegate_(std::make_unique<DefaultDelegateImpl>()),
      weak_ptr_factory_(this) {
  arc_bridge_service_->boot_phase_monitor()->SetHost(this);
  auto* arc_session_manager = ArcSessionManager::Get();
  DCHECK(arc_session_manager);
  arc_session_manager->AddObserver(this);
  SessionRestore::AddObserver(this);

  auto* profile = Profile::FromBrowserContext(context);
  auto* extension_system = extensions::ExtensionSystem::Get(profile);
  DCHECK(extension_system);
  extension_system->ready().Post(
      FROM_HERE, base::Bind(&ArcBootPhaseMonitorBridge::OnExtensionsReady,
                            weak_ptr_factory_.GetWeakPtr()));

  // Initialize |enabled_by_policy_| now.
  OnArcPlayStoreEnabledChanged(IsArcPlayStoreEnabledForProfile(profile));
}

ArcBootPhaseMonitorBridge::~ArcBootPhaseMonitorBridge() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  arc_bridge_service_->boot_phase_monitor()->SetHost(nullptr);
  auto* arc_session_manager = ArcSessionManager::Get();
  DCHECK(arc_session_manager);
  arc_session_manager->RemoveObserver(this);
  SessionRestore::RemoveObserver(this);
}

void ArcBootPhaseMonitorBridge::RecordFirstAppLaunchDelayUMAInternal() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  if (first_app_launch_delay_recorded_)
    return;
  first_app_launch_delay_recorded_ = true;

  if (boot_completed_) {
    VLOG(2) << "ARC has already fully started. Recording the UMA now.";
    if (delegate_)
      delegate_->RecordFirstAppLaunchDelayUMA(base::TimeDelta());
    return;
  }
  app_launch_time_ = base::TimeTicks::Now();
}

void ArcBootPhaseMonitorBridge::OnBootCompleted() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  VLOG(2) << "OnBootCompleted";
  boot_completed_ = true;

  chromeos::SessionManagerClient* session_manager_client =
      chromeos::DBusThreadManager::Get()->GetSessionManagerClient();
  session_manager_client->EmitArcBooted(cryptohome::Identification(account_id_),
                                        base::BindOnce(&OnEmitArcBooted));

  ArcSessionManager* arc_session_manager = ArcSessionManager::Get();
  DCHECK(arc_session_manager);
  if (arc_session_manager->is_directly_started()) {
    // Unless this is opt-in boot, start monitoring window activation changes to
    // prioritize/throttle the container when needed.
    throttle_ = std::make_unique<ArcInstanceThrottle>();
    VLOG(2) << "ArcInstanceThrottle created in OnBootCompleted()";
  }

  if (!app_launch_time_.is_null() && delegate_) {
    delegate_->RecordFirstAppLaunchDelayUMA(base::TimeTicks::Now() -
                                            app_launch_time_);
  }
}

void ArcBootPhaseMonitorBridge::OnArcPlayStoreEnabledChanged(bool enabled) {
  auto* profile = Profile::FromBrowserContext(context_);
  enabled_by_policy_ =
      enabled && IsArcPlayStoreEnabledPreferenceManagedForProfile(profile);
  if (enabled_by_policy_)
    MaybeDisableCpuRestriction();
}

void ArcBootPhaseMonitorBridge::OnArcInitialStart() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  // ARC apps for opt-in finished doing their jobs. Start the throttle.
  throttle_ = std::make_unique<ArcInstanceThrottle>();
  VLOG(2) << "ArcInstanceThrottle created in OnArcInitialStart()";
}

void ArcBootPhaseMonitorBridge::OnArcSessionStopped(ArcStopReason stop_reason) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  // Remove the throttle so that the window observer won't interfere with the
  // container startup when the user opts in to ARC.
  Reset();
  VLOG(2) << "ArcInstanceThrottle has been removed in OnArcSessionStopped()";
}

void ArcBootPhaseMonitorBridge::OnArcSessionRestarting() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  // Remove the throttle so that the window observer won't interfere with the
  // container restart.
  Reset();
  VLOG(2) << "ArcInstanceThrottle has been removed in OnArcSessionRestarting()";

  // We assume that a crash tends to happen while the user is actively using
  // the instance. For that reason, we try to restart the instance without the
  // restricted cgroups.
  if (delegate_)
    delegate_->DisableCpuRestriction();
}

void ArcBootPhaseMonitorBridge::OnSessionRestoreFinishedLoadingTabs() {
  VLOG(2) << "All tabs have been restored";
  if (throttle_)
    return;
  // |throttle_| is not available. This means either of the following:
  // 1) This is an opt-in boot, and OnArcInitialStart() hasn't been called.
  // 2) This is not an opt-in boot, and OnBootCompleted() hasn't been called.
  // In both cases, relax the restriction to let the instance fully start.
  VLOG(2) << "Allowing the instance to use more CPU resources";
  if (delegate_)
    delegate_->DisableCpuRestriction();
}

void ArcBootPhaseMonitorBridge::OnExtensionsReady() {
  VLOG(2) << "All extensions are loaded";
  extensions_ready_ = true;
  MaybeDisableCpuRestriction();
}

void ArcBootPhaseMonitorBridge::MaybeDisableCpuRestriction() {
  if (throttle_)
    return;
  if (!extensions_ready_ || !enabled_by_policy_)
    return;

  VLOG(1) << "ARC is enabled by policy. "
          << "Allowing the instance to use more CPU resources";
  if (delegate_)
    delegate_->DisableCpuRestriction();
}

void ArcBootPhaseMonitorBridge::Reset() {
  throttle_.reset();
  app_launch_time_ = base::TimeTicks();
  first_app_launch_delay_recorded_ = false;
  boot_completed_ = false;
  enabled_by_policy_ = false;

  // Do not reset |extensions_ready_| here. That variable is not tied to the
  // instance.
}

void ArcBootPhaseMonitorBridge::SetDelegateForTesting(
    std::unique_ptr<Delegate> delegate) {
  delegate_ = std::move(delegate);
}

}  // namespace arc
