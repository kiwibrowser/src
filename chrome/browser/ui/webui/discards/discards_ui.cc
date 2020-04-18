// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/discards/discards_ui.h"

#include <utility>
#include <vector>

#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/resource_coordinator/discard_reason.h"
#include "chrome/browser/resource_coordinator/lifecycle_unit.h"
#include "chrome/browser/resource_coordinator/tab_activity_watcher.h"
#include "chrome/browser/resource_coordinator/tab_lifecycle_unit_external.h"
#include "chrome/browser/resource_coordinator/tab_manager.h"
#include "chrome/browser/resource_coordinator/time.h"
#include "chrome/browser/ui/webui/discards/discards.mojom.h"
#include "chrome/common/webui_url_constants.h"
#include "chrome/grit/browser_resources.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/browser/web_ui_message_handler.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "ui/resources/grit/ui_resources.h"

namespace {

resource_coordinator::DiscardReason GetDiscardReason(bool urgent) {
  return urgent ? resource_coordinator::DiscardReason::kUrgent
                : resource_coordinator::DiscardReason::kProactive;
}

mojom::LifecycleUnitVisibility GetLifecycleUnitVisibility(
    content::Visibility visibility) {
  switch (visibility) {
    case content::Visibility::HIDDEN:
      return mojom::LifecycleUnitVisibility::HIDDEN;
    case content::Visibility::OCCLUDED:
      return mojom::LifecycleUnitVisibility::OCCLUDED;
    case content::Visibility::VISIBLE:
      return mojom::LifecycleUnitVisibility::VISIBLE;
  }
#if defined(COMPILER_MSVC)
  NOTREACHED();
  return mojom::LifecycleUnitVisibility::VISIBLE;
#endif
}

resource_coordinator::LifecycleUnit* GetLifecycleUnitById(int32_t id) {
  for (resource_coordinator::LifecycleUnit* lifecycle_unit :
       g_browser_process->GetTabManager()->GetSortedLifecycleUnits()) {
    if (lifecycle_unit->GetID() == id)
      return lifecycle_unit;
  }
  return nullptr;
}

class DiscardsDetailsProviderImpl : public mojom::DiscardsDetailsProvider {
 public:
  // This instance is deleted when the supplied pipe is destroyed.
  DiscardsDetailsProviderImpl(
      mojo::InterfaceRequest<mojom::DiscardsDetailsProvider> request)
      : binding_(this, std::move(request)) {}

  ~DiscardsDetailsProviderImpl() override {}

  // mojom::DiscardsDetailsProvider overrides:
  void GetTabDiscardsInfo(GetTabDiscardsInfoCallback callback) override {
    resource_coordinator::TabManager* tab_manager =
        g_browser_process->GetTabManager();
    const resource_coordinator::LifecycleUnitVector lifecycle_units =
        tab_manager->GetSortedLifecycleUnits();

    std::vector<mojom::TabDiscardsInfoPtr> infos;
    infos.reserve(lifecycle_units.size());

    const base::TimeTicks now = resource_coordinator::NowTicks();

    // Convert the LifecycleUnits to a vector of TabDiscardsInfos.
    size_t rank = 1;
    for (auto* lifecycle_unit : lifecycle_units) {
      mojom::TabDiscardsInfoPtr info(mojom::TabDiscardsInfo::New());

      resource_coordinator::TabLifecycleUnitExternal*
          tab_lifecycle_unit_external =
              lifecycle_unit->AsTabLifecycleUnitExternal();
      content::WebContents* contents =
          tab_lifecycle_unit_external->GetWebContents();

      info->tab_url = contents->GetLastCommittedURL().spec();
      info->title = base::UTF16ToUTF8(lifecycle_unit->GetTitle());
      info->visibility =
          GetLifecycleUnitVisibility(lifecycle_unit->GetVisibility());
      info->is_media = tab_lifecycle_unit_external->IsMediaTab();
      info->is_frozen = tab_lifecycle_unit_external->IsFrozen();
      info->is_discarded = tab_lifecycle_unit_external->IsDiscarded();
      info->discard_count = tab_lifecycle_unit_external->GetDiscardCount();
      info->utility_rank = rank++;
      const base::TimeTicks last_focused_time =
          lifecycle_unit->GetLastFocusedTime();
      const base::TimeDelta elapsed =
          (last_focused_time == base::TimeTicks::Max())
              ? base::TimeDelta()
              : (now - last_focused_time);
      info->last_active_seconds = static_cast<int32_t>(elapsed.InSeconds());
      info->is_auto_discardable =
          tab_lifecycle_unit_external->IsAutoDiscardable();
      info->id = lifecycle_unit->GetID();
      base::Optional<float> reactivation_score =
          resource_coordinator::TabActivityWatcher::GetInstance()
              ->CalculateReactivationScore(contents);
      info->has_reactivation_score = reactivation_score.has_value();
      if (info->has_reactivation_score)
        info->reactivation_score = reactivation_score.value();

      infos.push_back(std::move(info));
    }

    std::move(callback).Run(std::move(infos));
  }

  void SetAutoDiscardable(int32_t id,
                          bool is_auto_discardable,
                          SetAutoDiscardableCallback callback) override {
    auto* lifecycle_unit = GetLifecycleUnitById(id);
    if (lifecycle_unit) {
      auto* tab_lifecycle_unit_external =
          lifecycle_unit->AsTabLifecycleUnitExternal();
      if (tab_lifecycle_unit_external)
        tab_lifecycle_unit_external->SetAutoDiscardable(is_auto_discardable);
    }
    std::move(callback).Run();
  }

  void DiscardById(int32_t id,
                   bool urgent,
                   DiscardByIdCallback callback) override {
    auto* lifecycle_unit = GetLifecycleUnitById(id);
    if (lifecycle_unit)
      lifecycle_unit->Discard(GetDiscardReason(urgent));
    std::move(callback).Run();
  }

  void FreezeById(int32_t id) override {
    auto* lifecycle_unit = GetLifecycleUnitById(id);
    if (lifecycle_unit)
      lifecycle_unit->Freeze();
  }

  void Discard(bool urgent, DiscardCallback callback) override {
    resource_coordinator::TabManager* tab_manager =
        g_browser_process->GetTabManager();
    tab_manager->DiscardTab(GetDiscardReason(urgent));
    std::move(callback).Run();
  }

 private:
  mojo::Binding<mojom::DiscardsDetailsProvider> binding_;

  DISALLOW_COPY_AND_ASSIGN(DiscardsDetailsProviderImpl);
};

}  // namespace

DiscardsUI::DiscardsUI(content::WebUI* web_ui)
    : ui::MojoWebUIController(web_ui) {
  std::unique_ptr<content::WebUIDataSource> source(
      content::WebUIDataSource::Create(chrome::kChromeUIDiscardsHost));

  source->AddResourcePath("discards.css", IDR_ABOUT_DISCARDS_CSS);
  source->AddResourcePath("discards.js", IDR_ABOUT_DISCARDS_JS);
  // Full paths (relative to src) are important for Mojom generated files.
  source->AddResourcePath("chrome/browser/ui/webui/discards/discards.mojom.js",
                          IDR_ABOUT_DISCARDS_MOJO_JS);
  source->SetDefaultResource(IDR_ABOUT_DISCARDS_HTML);

  Profile* profile = Profile::FromWebUI(web_ui);
  content::WebUIDataSource::Add(profile, source.release());
  AddHandlerToRegistry(base::BindRepeating(
      &DiscardsUI::BindDiscardsDetailsProvider, base::Unretained(this)));
}

DiscardsUI::~DiscardsUI() {}

void DiscardsUI::BindDiscardsDetailsProvider(
    mojom::DiscardsDetailsProviderRequest request) {
  ui_handler_ =
      std::make_unique<DiscardsDetailsProviderImpl>(std::move(request));
}
