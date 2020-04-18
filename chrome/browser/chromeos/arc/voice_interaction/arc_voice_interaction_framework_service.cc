// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/arc/voice_interaction/arc_voice_interaction_framework_service.h"

#include <utility>
#include <vector>

#include "ash/public/cpp/shell_window_ids.h"
#include "ash/shell.h"
#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/command_line.h"
#include "base/containers/flat_set.h"
#include "base/logging.h"
#include "base/memory/ref_counted.h"
#include "base/memory/singleton.h"
#include "base/metrics/histogram_macros.h"
#include "base/metrics/user_metrics.h"
#include "base/metrics/user_metrics_action.h"
#include "base/task_scheduler/post_task.h"
#include "chrome/browser/chromeos/arc/arc_util.h"
#include "chrome/browser/chromeos/arc/boot_phase_monitor/arc_boot_phase_monitor_bridge.h"
#include "chrome/browser/chromeos/arc/voice_interaction/highlighter_controller_client.h"
#include "chrome/browser/chromeos/arc/voice_interaction/voice_interaction_controller_client.h"
#include "chrome/browser/chromeos/ash_config.h"
#include "chrome/browser/chromeos/login/helper.h"
#include "chrome/browser/chromeos/login/ui/login_display_host_webui.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/webui/chromeos/login/oobe_ui.h"
#include "chrome/common/pref_names.h"
#include "chromeos/chromeos_switches.h"
#include "components/arc/arc_bridge_service.h"
#include "components/arc/arc_browser_context_keyed_service_factory_base.h"
#include "components/arc/arc_prefs.h"
#include "components/arc/arc_util.h"
#include "components/arc/connection_holder.h"
#include "components/session_manager/core/session_manager.h"
#include "content/public/browser/browser_thread.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/aura/client/aura_constants.h"
#include "ui/aura/window.h"
#include "ui/compositor/layer.h"
#include "ui/compositor/layer_owner.h"
#include "ui/compositor/layer_tree_owner.h"
#include "ui/gfx/codec/jpeg_codec.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/image/image_util.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/snapshot/snapshot.h"
#include "ui/snapshot/snapshot_aura.h"
#include "ui/wm/core/window_util.h"
#include "ui/wm/public/activation_client.h"

namespace mojo {

// Map VoiceInteractionState from arc::mojom into ash::mojom. The duplicate
// definition is because we do not want to use extensible widely.
// (crbug.com/731893).
template <>
struct TypeConverter<ash::mojom::VoiceInteractionState,
                     arc::mojom::VoiceInteractionState> {
  static ash::mojom::VoiceInteractionState Convert(
      arc::mojom::VoiceInteractionState state) {
    switch (state) {
      case arc::mojom::VoiceInteractionState::NOT_READY:
        return ash::mojom::VoiceInteractionState::NOT_READY;
      case arc::mojom::VoiceInteractionState::STOPPED:
        return ash::mojom::VoiceInteractionState::STOPPED;
      case arc::mojom::VoiceInteractionState::RUNNING:
        return ash::mojom::VoiceInteractionState::RUNNING;
    }

    NOTREACHED() << "Invalid state: " << static_cast<int>(state);
    return ash::mojom::VoiceInteractionState::NOT_READY;
  }
};

}  // namespace mojo

namespace arc {

namespace {

using LayerSet = base::flat_set<const ui::Layer*>;

// Time out for a context query from container since user initiated
// interaction. This must be strictly less than
// kMaxTimeSinceUserInteractionForHistogram so that the histogram
// could cover the range of normal operations.
constexpr base::TimeDelta kAllowedTimeSinceUserInteraction =
    base::TimeDelta::FromSeconds(2);
constexpr base::TimeDelta kMaxTimeSinceUserInteractionForHistogram =
    base::TimeDelta::FromSeconds(5);

constexpr int32_t kContextRequestMaxRemainingCount = 2;

std::unique_ptr<ui::LayerTreeOwner> CreateLayerTreeForSnapshot(
    aura::Window* root_window) {
  LayerSet blocked_layers;
  for (auto* browser : *BrowserList::GetInstance()) {
    if (browser->profile()->IsOffTheRecord())
      blocked_layers.insert(browser->window()->GetNativeWindow()->layer());
  }

  LayerSet excluded_layers;
  // Exclude metalayer-related layers. This will also include other layers
  // under kShellWindowId_OverlayContainer which is fine.
  // TODO(crbug.com/757012): Mash support.
  if (chromeos::GetAshConfig() != ash::Config::MASH) {
    aura::Window* overlay_container = ash::Shell::GetContainer(
        root_window, ash::kShellWindowId_OverlayContainer);
    if (overlay_container != nullptr)
      excluded_layers.insert(overlay_container->layer());
  }

  auto layer_tree_owner = ::wm::RecreateLayersWithClosure(
      root_window, base::BindRepeating(
                       [](LayerSet blocked_layers, LayerSet excluded_layers,
                          ui::LayerOwner* owner) -> std::unique_ptr<ui::Layer> {
                         // Parent layer is excluded meaning that it's pointless
                         // to clone current child and all its descendants. This
                         // reduces the number of layers to create.
                         if (blocked_layers.count(owner->layer()->parent()))
                           return nullptr;
                         if (blocked_layers.count(owner->layer())) {
                           auto layer = std::make_unique<ui::Layer>(
                               ui::LayerType::LAYER_SOLID_COLOR);
                           layer->SetBounds(owner->layer()->bounds());
                           layer->SetColor(SK_ColorBLACK);
                           return layer;
                         }
                         if (excluded_layers.count(owner->layer()))
                           return nullptr;
                         return owner->RecreateLayer();
                       },
                       std::move(blocked_layers), std::move(excluded_layers)));

  // layer_tree_owner cannot be null since we are starting off from the root
  // window, which could never be incognito.
  DCHECK(layer_tree_owner);

  auto* root_layer = layer_tree_owner->root();
  // The root layer might have a scaling transform applied (if the user has
  // changed the UI scale via Ctrl-Shift-Plus/Minus).
  // Clear the transform so that the snapshot is taken at 1:1 scale relative
  // to screen pixels.
  root_layer->SetTransform(gfx::Transform());
  root_window->layer()->Add(root_layer);
  root_window->layer()->StackAtBottom(root_layer);
  return layer_tree_owner;
}

void EncodeAndReturnImage(
    ArcVoiceInteractionFrameworkService::CaptureFullscreenCallback callback,
    std::unique_ptr<ui::LayerTreeOwner> old_layer_owner,
    gfx::Image image) {
  old_layer_owner.reset();
  base::PostTaskWithTraitsAndReplyWithResult(
      FROM_HERE,
      base::TaskTraits{base::MayBlock(), base::TaskPriority::USER_BLOCKING},
      // We use SkBitmap here to avoid passing in gfx::Image directly, which
      // shares a single gfx::ImageStorage that's not threadsafe.
      // Alternatively, we could also pass in |image| with std::move().
      base::BindOnce(
          [](SkBitmap image) -> std::vector<uint8_t> {
            std::vector<uint8_t> res;
            gfx::JPEGCodec::Encode(image, 100, &res);
            return res;
          },
          image.AsBitmap()),
      std::move(callback));
}

// Singleton factory for ArcVoiceInteractionFrameworkService.
class ArcVoiceInteractionFrameworkServiceFactory
    : public internal::ArcBrowserContextKeyedServiceFactoryBase<
          ArcVoiceInteractionFrameworkService,
          ArcVoiceInteractionFrameworkServiceFactory> {
 public:
  // Factory name used by ArcBrowserContextKeyedServiceFactoryBase.
  static constexpr const char* kName =
      "ArcVoiceInteractionFrameworkServiceFactory";

  static ArcVoiceInteractionFrameworkServiceFactory* GetInstance() {
    return base::Singleton<ArcVoiceInteractionFrameworkServiceFactory>::get();
  }

 private:
  friend base::DefaultSingletonTraits<
      ArcVoiceInteractionFrameworkServiceFactory>;
  ArcVoiceInteractionFrameworkServiceFactory() = default;
  ~ArcVoiceInteractionFrameworkServiceFactory() override = default;

  // BrowserContextKeyedServiceFactory override:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override {
    if (!chromeos::switches::IsVoiceInteractionEnabled())
      return nullptr;
    return ArcBrowserContextKeyedServiceFactoryBase::BuildServiceInstanceFor(
        context);
  }
};

}  // namespace

// static
ArcVoiceInteractionFrameworkService*
ArcVoiceInteractionFrameworkService::GetForBrowserContext(
    content::BrowserContext* context) {
  return ArcVoiceInteractionFrameworkServiceFactory::GetForBrowserContext(
      context);
}

KeyedServiceBaseFactory* ArcVoiceInteractionFrameworkService::GetFactory() {
  return ArcVoiceInteractionFrameworkServiceFactory::GetInstance();
}

ArcVoiceInteractionFrameworkService::ArcVoiceInteractionFrameworkService(
    content::BrowserContext* context,
    ArcBridgeService* bridge_service)
    : context_(context),
      arc_bridge_service_(bridge_service),
      highlighter_client_(std::make_unique<HighlighterControllerClient>(this)),
      weak_ptr_factory_(this) {
  arc_bridge_service_->voice_interaction_framework()->SetHost(this);
  arc_bridge_service_->voice_interaction_framework()->AddObserver(this);
  ArcSessionManager::Get()->AddObserver(this);
  chromeos::CrasAudioHandler::Get()->AddAudioObserver(this);
}

ArcVoiceInteractionFrameworkService::~ArcVoiceInteractionFrameworkService() {
  chromeos::CrasAudioHandler::Get()->RemoveAudioObserver(this);
  ArcSessionManager::Get()->RemoveObserver(this);
  arc_bridge_service_->voice_interaction_framework()->RemoveObserver(this);
  arc_bridge_service_->voice_interaction_framework()->SetHost(nullptr);
}

void ArcVoiceInteractionFrameworkService::OnConnectionReady() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (is_request_pending_) {
    is_request_pending_ = false;
    if (is_pending_request_toggle_) {
      mojom::VoiceInteractionFrameworkInstance* framework_instance =
          ARC_GET_INSTANCE_FOR_METHOD(
              arc_bridge_service_->voice_interaction_framework(),
              ToggleVoiceInteractionSession);
      DCHECK(framework_instance);
      framework_instance->ToggleVoiceInteractionSession(IsHomescreenActive());
    } else {
      mojom::VoiceInteractionFrameworkInstance* framework_instance =
          ARC_GET_INSTANCE_FOR_METHOD(
              arc_bridge_service_->voice_interaction_framework(),
              StartVoiceInteractionSession);
      DCHECK(framework_instance);
      framework_instance->StartVoiceInteractionSession(IsHomescreenActive());
    }
  }

  highlighter_client_->Attach();
}

void ArcVoiceInteractionFrameworkService::OnConnectionClosed() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  highlighter_client_->Detach();
}

void ArcVoiceInteractionFrameworkService::CaptureFullscreen(
    CaptureFullscreenCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (!ValidateTimeSinceUserInteraction()) {
    std::move(callback).Run(std::vector<uint8_t>{});
    return;
  }

  // Since ARC currently only runs in primary display, we restrict
  // the screenshot to it.
  // TODO(crbug.com/757012): Mash support.
  if (chromeos::GetAshConfig() == ash::Config::MASH) {
    std::move(callback).Run(std::vector<uint8_t>{});
    return;
  }
  aura::Window* window = ash::Shell::GetPrimaryRootWindow();
  DCHECK(window);

  auto old_layer_owner = CreateLayerTreeForSnapshot(window);
  ui::GrabLayerSnapshotAsync(
      old_layer_owner->root(), gfx::Rect(window->bounds().size()),
      base::Bind(&EncodeAndReturnImage, base::Passed(std::move(callback)),
                 base::Passed(std::move(old_layer_owner))));
}

void ArcVoiceInteractionFrameworkService::SetVoiceInteractionState(
    arc::mojom::VoiceInteractionState voice_interaction_state) {
  ash::mojom::VoiceInteractionState state =
      mojo::ConvertTo<ash::mojom::VoiceInteractionState>(
          voice_interaction_state);
  DCHECK_NE(state_, state);
  // Assume voice interaction state changing from NOT_READY to a state other
  // than ready indicates container boot complete and it's safe to synchronize
  // voice interaction flags. VoiceInteractionEnabled is locked at true in
  // Android side so we don't need to synchronize it here.
  if (state_ == ash::mojom::VoiceInteractionState::NOT_READY) {
    PrefService* prefs = Profile::FromBrowserContext(context_)->GetPrefs();
    bool value_prop_accepted =
        prefs->GetBoolean(prefs::kArcVoiceInteractionValuePropAccepted);

    bool enable_voice_interaction =
        value_prop_accepted &&
        prefs->GetBoolean(prefs::kVoiceInteractionEnabled);
    SetVoiceInteractionEnabled(enable_voice_interaction, base::DoNothing());

    SetVoiceInteractionContextEnabled(
        enable_voice_interaction &&
        prefs->GetBoolean(prefs::kVoiceInteractionContextEnabled));
  }

  // If voice session stopped running, we also stop the assist layer session.
  if (state_ == ash::mojom::VoiceInteractionState::RUNNING)
    highlighter_client_->Exit();

  state_ = state;
  arc::VoiceInteractionControllerClient::Get()->NotifyStatusChanged(state);
}

void ArcVoiceInteractionFrameworkService::ShowMetalayer() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  NotifyMetalayerStatusChanged(true);
}

void ArcVoiceInteractionFrameworkService::HideMetalayer() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  NotifyMetalayerStatusChanged(false);
}

void ArcVoiceInteractionFrameworkService::OnArcPlayStoreEnabledChanged(
    bool enabled) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (enabled)
    return;

  SetVoiceInteractionSetupCompletedInternal(false);
  SetVoiceInteractionEnabled(false, base::DoNothing());
  SetVoiceInteractionContextEnabled(false);
}

void ArcVoiceInteractionFrameworkService::OnHotwordTriggered(uint64_t tv_sec,
                                                             uint64_t tv_nsec) {
  InitiateUserInteraction(false /* is_toggle */);
}

void ArcVoiceInteractionFrameworkService::StartVoiceInteractionSetupWizard() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  // This screen is shown after the Just-A-Sec screen, which blocks until
  // application sync'd is received. At that point, framework service should
  // already be initialized. Here we get a method that is defined in version 1
  // to ensure the connection is established.
  // TODO(muyuanli): This is a hack for backward compatibility and should be
  // removed once Android side is checked in and stablized, then we should
  // DCHECK the |setting_applied| parameter in the lambda. See
  // crbug.com/768935.
  DCHECK(ARC_GET_INSTANCE_FOR_METHOD(
      arc_bridge_service_->voice_interaction_framework(),
      StartVoiceInteractionSession));
  SetVoiceInteractionEnabled(
      true, base::BindOnce(
                [](base::OnceClosure next, bool setting_applied) {
                  if (!setting_applied)
                    DVLOG(1) << "Not synchronizing settings: version mismatch";
                  std::move(next).Run();
                },
                base::BindOnce(&ArcVoiceInteractionFrameworkService::
                                   StartVoiceInteractionSetupWizardActivity,
                               weak_ptr_factory_.GetWeakPtr())));
}

void ArcVoiceInteractionFrameworkService::ShowVoiceInteractionSettings() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  arc::mojom::VoiceInteractionFrameworkInstance* framework_instance =
      ARC_GET_INSTANCE_FOR_METHOD(
          arc_bridge_service_->voice_interaction_framework(),
          ShowVoiceInteractionSettings);
  if (!framework_instance)
    return;
  framework_instance->ShowVoiceInteractionSettings();
}

void ArcVoiceInteractionFrameworkService::NotifyMetalayerStatusChanged(
    bool visible) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  mojom::VoiceInteractionFrameworkInstance* framework_instance =
      ARC_GET_INSTANCE_FOR_METHOD(
          arc_bridge_service_->voice_interaction_framework(),
          SetMetalayerVisibility);
  if (!framework_instance)
    return;
  framework_instance->SetMetalayerVisibility(visible);
}

void ArcVoiceInteractionFrameworkService::SetVoiceInteractionEnabled(
    bool enable,
    VoiceInteractionSettingCompleteCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  PrefService* prefs = Profile::FromBrowserContext(context_)->GetPrefs();

  prefs->SetBoolean(prefs::kVoiceInteractionEnabled, enable);
  if (!enable)
    prefs->SetBoolean(prefs::kVoiceInteractionContextEnabled, false);

  mojom::VoiceInteractionFrameworkInstance* framework_instance =
      ARC_GET_INSTANCE_FOR_METHOD(
          arc_bridge_service_->voice_interaction_framework(),
          SetVoiceInteractionEnabled);
  if (!framework_instance) {
    std::move(callback).Run(false);
    return;
  }
  framework_instance->SetVoiceInteractionEnabled(
      enable, base::BindOnce(std::move(callback), true));
}

void ArcVoiceInteractionFrameworkService::SetVoiceInteractionContextEnabled(
    bool enable) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  PrefService* prefs = Profile::FromBrowserContext(context_)->GetPrefs();
  prefs->SetBoolean(prefs::kVoiceInteractionContextEnabled, enable);

  mojom::VoiceInteractionFrameworkInstance* framework_instance =
      ARC_GET_INSTANCE_FOR_METHOD(
          arc_bridge_service_->voice_interaction_framework(),
          SetVoiceInteractionContextEnabled);
  if (!framework_instance)
    return;
  framework_instance->SetVoiceInteractionContextEnabled(enable);
}

void ArcVoiceInteractionFrameworkService::SetVoiceInteractionSetupCompleted() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  SetVoiceInteractionSetupCompletedInternal(true);
  SetVoiceInteractionEnabled(true, base::DoNothing());
  SetVoiceInteractionContextEnabled(true);
}

void ArcVoiceInteractionFrameworkService::StartSessionFromUserInteraction(
    const gfx::Rect& rect) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (!InitiateUserInteraction(false /* is_toggle */))
    return;

  if (rect.IsEmpty()) {
    mojom::VoiceInteractionFrameworkInstance* framework_instance =
        ARC_GET_INSTANCE_FOR_METHOD(
            arc_bridge_service_->voice_interaction_framework(),
            StartVoiceInteractionSession);
    DCHECK(framework_instance);
    framework_instance->StartVoiceInteractionSession(IsHomescreenActive());
  } else {
    mojom::VoiceInteractionFrameworkInstance* framework_instance =
        ARC_GET_INSTANCE_FOR_METHOD(
            arc_bridge_service_->voice_interaction_framework(),
            StartVoiceInteractionSessionForRegion);
    DCHECK(framework_instance);
    framework_instance->StartVoiceInteractionSessionForRegion(rect);
  }
  VLOG(1) << "Sent voice interaction request.";
}

void ArcVoiceInteractionFrameworkService::ToggleSessionFromUserInteraction() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (!InitiateUserInteraction(true /* is_toggle */))
    return;

  mojom::VoiceInteractionFrameworkInstance* framework_instance =
      ARC_GET_INSTANCE_FOR_METHOD(
          arc_bridge_service_->voice_interaction_framework(),
          ToggleVoiceInteractionSession);
  DCHECK(framework_instance);
  framework_instance->ToggleVoiceInteractionSession(IsHomescreenActive());
}

bool ArcVoiceInteractionFrameworkService::ValidateTimeSinceUserInteraction() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (!context_request_remaining_count_) {
    // Allowed number of requests used up. But we still have additional
    // requests. It's likely that there is something malicious going on.
    LOG(ERROR) << "Illegal context request from container.";
    UMA_HISTOGRAM_BOOLEAN("VoiceInteraction.IllegalContextRequest", true);
    return false;
  }
  auto elapsed = base::TimeTicks::Now() - user_interaction_start_time_;
  elapsed = elapsed > kMaxTimeSinceUserInteractionForHistogram
                ? kMaxTimeSinceUserInteractionForHistogram
                : elapsed;

  UMA_HISTOGRAM_CUSTOM_COUNTS(
      "VoiceInteraction.UserInteractionToRequestArrival",
      elapsed.InMilliseconds(), 1,
      kMaxTimeSinceUserInteractionForHistogram.InMilliseconds(), 20);

  if (elapsed > kAllowedTimeSinceUserInteraction) {
    LOG(ERROR) << "Timed out since last user interaction.";
    context_request_remaining_count_ = 0;
    return false;
  }

  context_request_remaining_count_--;
  return true;
}

void ArcVoiceInteractionFrameworkService::StartVoiceInteractionOobe() {
  if (chromeos::LoginDisplayHost::default_host())
    return;
  // The display host will be destructed at the end of OOBE flow.
  chromeos::LoginDisplayHostWebUI* display_host =
      new chromeos::LoginDisplayHostWebUI();
  display_host->StartVoiceInteractionOobe();
}

bool ArcVoiceInteractionFrameworkService::InitiateUserInteraction(
    bool is_toggle) {
  VLOG(1) << "Start voice interaction.";
  PrefService* prefs = Profile::FromBrowserContext(context_)->GetPrefs();
  if (!prefs->GetBoolean(prefs::kArcVoiceInteractionValuePropAccepted) ||
      arc::IsArcTermsOfServiceOobeNegotiationNeeded()) {
    VLOG(1) << "Voice interaction feature or ARC not accepted.";
    should_start_runtime_flow_ = true;
    // If voice interaction value prop has not been accepted, show the value
    // prop OOBE page again.
    StartVoiceInteractionOobe();
    return false;
  }

  if (!prefs->GetBoolean(prefs::kVoiceInteractionEnabled))
    return false;

  if (state_ == ash::mojom::VoiceInteractionState::NOT_READY) {
    // If the container side is not ready, we will be waiting for a while.
    arc::VoiceInteractionControllerClient::Get()->NotifyStatusChanged(
        ash::mojom::VoiceInteractionState::NOT_READY);
  }

  ArcBootPhaseMonitorBridge::RecordFirstAppLaunchDelayUMA(context_);
  if (!arc_bridge_service_->voice_interaction_framework()->IsConnected()) {
    VLOG(1) << "Instance not ready.";
    SetArcCpuRestriction(false);
    is_request_pending_ = true;
    is_pending_request_toggle_ = is_toggle;
    return false;
  }

  user_interaction_start_time_ = base::TimeTicks::Now();
  context_request_remaining_count_ = kContextRequestMaxRemainingCount;
  return true;
}

void ArcVoiceInteractionFrameworkService::
    SetVoiceInteractionSetupCompletedInternal(bool completed) {
  PrefService* prefs = Profile::FromBrowserContext(context_)->GetPrefs();
  prefs->SetBoolean(prefs::kArcVoiceInteractionValuePropAccepted, completed);
}

bool ArcVoiceInteractionFrameworkService::IsHomescreenActive() {
  // Homescreen is considered to be active if there are no active windows.
  // TODO(crbug.com/757012): Mash support.
  if (chromeos::GetAshConfig() == ash::Config::MASH)
    return false;
  return !ash::Shell::Get()->activation_client()->GetActiveWindow();
}

void ArcVoiceInteractionFrameworkService::
    StartVoiceInteractionSetupWizardActivity() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  arc::mojom::VoiceInteractionFrameworkInstance* framework_instance =
      ARC_GET_INSTANCE_FOR_METHOD(
          arc_bridge_service_->voice_interaction_framework(),
          StartVoiceInteractionSetupWizard);

  if (!framework_instance)
    return;

  if (should_start_runtime_flow_) {
    should_start_runtime_flow_ = false;
    VLOG(1) << "Starting runtime setup flow.";
    framework_instance->StartVoiceInteractionSession(IsHomescreenActive());
    return;
  }
  framework_instance->StartVoiceInteractionSetupWizard();
}

std::unique_ptr<ui::LayerTreeOwner>
ArcVoiceInteractionFrameworkService::CreateLayerTreeForSnapshotForTesting(
    aura::Window* root_window) const {
  return CreateLayerTreeForSnapshot(root_window);
}

}  // namespace arc
