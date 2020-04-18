// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/resource_coordinator/coordination_unit/coordination_unit_base.h"

#include <unordered_map>

#include "services/resource_coordinator/observers/coordination_unit_graph_observer.h"
#include "services/resource_coordinator/public/cpp/coordination_unit_id.h"

namespace resource_coordinator {

namespace {

using CUIDMap = std::unordered_map<CoordinationUnitID,
                                   std::unique_ptr<CoordinationUnitBase>>;

CUIDMap& g_cu_map() {
  static CUIDMap* instance = new CUIDMap();
  return *instance;
}

}  // namespace

// static
CoordinationUnitBase* CoordinationUnitBase::AddNewCoordinationUnit(
    std::unique_ptr<CoordinationUnitBase> new_cu) {
  auto it = g_cu_map().emplace(new_cu->id(), std::move(new_cu));
  DCHECK(it.second);  // Inserted successfully
  return it.first->second.get();
}

// static
std::vector<CoordinationUnitBase*>
CoordinationUnitBase::GetCoordinationUnitsOfType(CoordinationUnitType type) {
  std::vector<CoordinationUnitBase*> results;
  for (auto& el : g_cu_map()) {
    if (el.first.type == type)
      results.push_back(el.second.get());
  }
  return results;
}

// static
void CoordinationUnitBase::AssertNoActiveCoordinationUnits() {
  CHECK(g_cu_map().empty());
}

// static
void CoordinationUnitBase::ClearAllCoordinationUnits() {
  g_cu_map().clear();
}

// static
CoordinationUnitBase* CoordinationUnitBase::GetCoordinationUnitByID(
    const CoordinationUnitID cu_id) {
  auto cu_iter = g_cu_map().find(cu_id);
  return cu_iter != g_cu_map().end() ? cu_iter->second.get() : nullptr;
}

CoordinationUnitBase::CoordinationUnitBase(const CoordinationUnitID& id)
    : id_(id.type, id.id) {}

CoordinationUnitBase::~CoordinationUnitBase() = default;

void CoordinationUnitBase::Destruct() {
  size_t erased_count = g_cu_map().erase(id_);
  // After this point |this| is destructed and should not be accessed anymore.
  DCHECK_EQ(erased_count, 1u);
}

void CoordinationUnitBase::BeforeDestroyed() {
  for (auto& observer : observers_)
    observer.OnBeforeCoordinationUnitDestroyed(this);
}

void CoordinationUnitBase::AddObserver(
    CoordinationUnitGraphObserver* observer) {
  observers_.AddObserver(observer);
}

void CoordinationUnitBase::RemoveObserver(
    CoordinationUnitGraphObserver* observer) {
  observers_.RemoveObserver(observer);
}

bool CoordinationUnitBase::GetProperty(const mojom::PropertyType property_type,
                                       int64_t* result) const {
  auto value_it = properties_.find(property_type);
  if (value_it != properties_.end()) {
    *result = value_it->second;
    return true;
  }
  return false;
}

int64_t CoordinationUnitBase::GetPropertyOrDefault(
    const mojom::PropertyType property_type, int64_t default_value) const {
  int64_t value = 0;
  if (GetProperty(property_type, &value))
    return value;
  return default_value;
}

void CoordinationUnitBase::OnEventReceived(mojom::Event event) {
  for (auto& observer : observers())
    observer.OnEventReceived(this, event);
}

void CoordinationUnitBase::OnPropertyChanged(mojom::PropertyType property_type,
                                             int64_t value) {
  for (auto& observer : observers())
    observer.OnPropertyChanged(this, property_type, value);
}

void CoordinationUnitBase::SendEvent(mojom::Event event) {
  OnEventReceived(event);
}

void CoordinationUnitBase::SetProperty(mojom::PropertyType property_type,
                                       int64_t value) {
  // The |CoordinationUnitGraphObserver| API specification dictates that
  // the property is guarranteed to be set on the |CoordinationUnitBase|
  // and propagated to the appropriate associated |CoordianationUnitBase|
  // before |OnPropertyChanged| is invoked on all of the registered observers.
  properties_[property_type] = value;
  OnPropertyChanged(property_type, value);
}

}  // namespace resource_coordinator
