// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/browser/ui/aura/accessibility/automation_manager_aura.h"

#include <stddef.h>

#include "base/memory/singleton.h"
#include "build/build_config.h"
#include "chromecast/browser/extensions/api/automation_internal/automation_event_router.h"
#include "chromecast/common/extensions_api/automation_api_constants.h"
#include "chromecast/common/extensions_api/cast_extension_messages.h"
#include "content/public/browser/ax_event_notification_details.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/render_frame_host.h"
#include "ui/accessibility/ax_action_data.h"
#include "ui/accessibility/ax_enums.mojom.h"
#include "ui/accessibility/ax_tree_id_registry.h"
#include "ui/accessibility/platform/aura_window_properties.h"
#include "ui/aura/env.h"
#include "ui/aura/window.h"
#include "ui/aura/window_tree_host.h"
#include "ui/views/accessibility/ax_aura_obj_wrapper.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

using content::BrowserContext;
using extensions::cast::AutomationEventRouter;

namespace {

// Returns default browser context for sending events in case it was not
// provided.
BrowserContext* GetDefaultEventContext() {
  // TODO(rmrossi) Temporary stub
  return nullptr;
}

}  // namespace

// static
AutomationManagerAura* AutomationManagerAura::GetInstance() {
  return base::Singleton<AutomationManagerAura>::get();
}

void AutomationManagerAura::Enable(BrowserContext* context) {
  enabled_ = true;
  Reset(false);

  SendEvent(context, current_tree_->GetRoot(), ax::mojom::Event::kLoadComplete);
  views::AXAuraObjCache::GetInstance()->SetDelegate(this);
}

void AutomationManagerAura::Disable() {
  enabled_ = false;
  Reset(true);
}

void AutomationManagerAura::HandleEvent(BrowserContext* context,
                                        views::View* view,
                                        ax::mojom::Event event_type) {
  if (!enabled_)
    return;

  views::AXAuraObjWrapper* aura_obj = view ?
      views::AXAuraObjCache::GetInstance()->GetOrCreate(view) :
      current_tree_->GetRoot();
  SendEvent(nullptr, aura_obj, event_type);
}

void AutomationManagerAura::HandleAlert(content::BrowserContext* context,
                                        const std::string& text) {
  if (!enabled_)
    return;

  views::AXAuraObjWrapper* obj =
      static_cast<AXRootObjWrapper*>(current_tree_->GetRoot())
          ->GetAlertForText(text);
  SendEvent(context, obj, ax::mojom::Event::kAlert);
}

void AutomationManagerAura::PerformAction(const ui::AXActionData& data) {
  CHECK(enabled_);

  current_tree_->HandleAccessibleAction(data);
}

void AutomationManagerAura::OnChildWindowRemoved(
    views::AXAuraObjWrapper* parent) {
  if (!enabled_)
    return;

  if (!parent)
    parent = current_tree_->GetRoot();

  SendEvent(nullptr, parent, ax::mojom::Event::kChildrenChanged);
}

void AutomationManagerAura::OnEvent(views::AXAuraObjWrapper* aura_obj,
                                    ax::mojom::Event event_type) {
  SendEvent(nullptr, aura_obj, event_type);
}

AutomationManagerAura::AutomationManagerAura()
    : AXHostDelegate(extensions::api::automation::kDesktopTreeID),
      enabled_(false),
      processing_events_(false) {}

AutomationManagerAura::~AutomationManagerAura() {
}

void AutomationManagerAura::Reset(bool reset_serializer) {
  if (!current_tree_)
    current_tree_.reset(new AXTreeSourceAura());
  reset_serializer ? current_tree_serializer_.reset()
                   : current_tree_serializer_.reset(
                         new AuraAXTreeSerializer(current_tree_.get()));
}

void AutomationManagerAura::SendEvent(BrowserContext* context,
                                      views::AXAuraObjWrapper* aura_obj,
                                      ax::mojom::Event event_type) {
  if (!current_tree_serializer_)
    return;

  if (!context)
    context = GetDefaultEventContext();

  //if (!context) {
  //  LOG(WARNING) << "Accessibility notification but no browser context";
  //  return;
  //}

  if (processing_events_) {
    pending_events_.push_back(std::make_pair(aura_obj, event_type));
    return;
  }
  processing_events_ = true;

  std::vector<ExtensionMsg_AccessibilityEventParams> events;
  events.emplace_back(ExtensionMsg_AccessibilityEventParams());
  ExtensionMsg_AccessibilityEventParams& params = events.back();
  if (!current_tree_serializer_->SerializeChanges(aura_obj, &params.update)) {
    LOG(ERROR) << "Unable to serialize one accessibility event.";
    return;
  }

  // Make sure the focused node is serialized.
  views::AXAuraObjWrapper* focus =
      views::AXAuraObjCache::GetInstance()->GetFocus();
  if (focus)
    current_tree_serializer_->SerializeChanges(focus, &params.update);

  params.tree_id = 0;
  params.id = aura_obj->GetUniqueId().Get();
  params.event_type = event_type;
  params.mouse_location = aura::Env::GetInstance()->last_mouse_location();

  AutomationEventRouter* router = AutomationEventRouter::GetInstance();
  router->DispatchAccessibilityEvents(events);

  processing_events_ = false;
  auto pending_events_copy = pending_events_;
  pending_events_.clear();
  for (size_t i = 0; i < pending_events_copy.size(); ++i) {
    SendEvent(context,
              pending_events_copy[i].first,
              pending_events_copy[i].second);
  }
}
