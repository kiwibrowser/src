// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/aura/event_injector.h"

#include "services/service_manager/public/cpp/connector.h"
#include "services/ui/public/interfaces/constants.mojom.h"
#include "ui/aura/env.h"
#include "ui/aura/mus/window_tree_client.h"
#include "ui/aura/window_tree_host.h"
#include "ui/events/event.h"
#include "ui/events/event_sink.h"

namespace {
std::unique_ptr<ui::Event> MapEvent(const ui::Event& event) {
  if (event.IsScrollEvent()) {
    return std::make_unique<ui::PointerEvent>(
        ui::MouseWheelEvent(*event.AsScrollEvent()));
  }

  if (event.IsMouseEvent())
    return std::make_unique<ui::PointerEvent>(*event.AsMouseEvent());

  if (event.IsTouchEvent())
    return std::make_unique<ui::PointerEvent>(*event.AsTouchEvent());

  return ui::Event::Clone(event);
}

}  // namespace

namespace aura {

EventInjector::EventInjector() {}

EventInjector::~EventInjector() {}

ui::EventDispatchDetails EventInjector::Inject(WindowTreeHost* host,
                                               ui::Event* event) {
  Env* env = Env::GetInstance();
  DCHECK(env);
  DCHECK(host);
  DCHECK(event);

  if (env->mode() == Env::Mode::LOCAL)
    return host->event_sink()->OnEventFromSource(event);

  if (event->IsLocatedEvent()) {
    // The ui-service expects events coming in to have a location matching the
    // root location. The non-ui-service code does this by way of
    // OnEventFromSource() ending up in LocatedEvent::UpdateForRootTransform(),
    // which reset the root_location to match the location.
    event->AsLocatedEvent()->set_root_location_f(
        event->AsLocatedEvent()->location_f());
  }

  if (!event_injector_) {
    env->window_tree_client_->connector()->BindInterface(
        ui::mojom::kServiceName, &event_injector_);
  }
  event_injector_->InjectEvent(
      host->GetDisplayId(), MapEvent(*event),
      base::BindOnce([](bool result) { DCHECK(result); }));
  return ui::EventDispatchDetails();
}

}  // namespace aura
