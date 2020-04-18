// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <unordered_set>

#include "base/lazy_instance.h"
#include "ui/accessibility/platform/ax_unique_id.h"

namespace ui {

namespace {

base::LazyInstance<std::unordered_set<int32_t>>::Leaky g_assigned_ids =
    LAZY_INSTANCE_INITIALIZER;

}  // namespace

AXUniqueId::AXUniqueId(const int32_t max_id) : id_(GetNextAXUniqueId(max_id)) {}

AXUniqueId::~AXUniqueId() {
  g_assigned_ids.Get().erase(id_);
}

bool AXUniqueId::operator==(const AXUniqueId& other) const {
  return Get() == other.Get();
}

bool AXUniqueId::operator!=(const AXUniqueId& other) const {
  return !(*this == other);
}

bool AXUniqueId::IsAssigned(int32_t id) const {
  auto id_map = g_assigned_ids.Get();
  return id_map.find(id) != id_map.end();
}

int32_t AXUniqueId::GetNextAXUniqueId(const int32_t max_id) {
  static int32_t current_id = 0;
  static bool has_wrapped = false;

  const int32_t prev_id = current_id;

  while (true) {
    if (current_id == max_id) {
      current_id = 1;
      has_wrapped = true;
    } else {
      current_id++;
    }
    if (current_id == prev_id)
      LOG(FATAL) << "Over 2 billion active ids, something is wrong.";
    if (!has_wrapped || !IsAssigned(current_id))
      break;
  }

  g_assigned_ids.Get().insert(current_id);

  return current_id;
}

}  // namespace ui
