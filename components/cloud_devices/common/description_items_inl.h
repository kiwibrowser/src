// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CLOUD_DEVICES_COMMON_DESCRIPTION_DESCRIPTION_ITEMS_INL_H_
#define COMPONENTS_CLOUD_DEVICES_COMMON_DESCRIPTION_DESCRIPTION_ITEMS_INL_H_

#include <stddef.h>

#include <memory>
#include <utility>
#include <vector>

#include "base/numerics/safe_conversions.h"
#include "base/values.h"
#include "components/cloud_devices/common/description_items.h"

// Implementation of templates defined in header file.
// This file should be included from CC file with implementation of device
// specific capabilities.

namespace cloud_devices {

template <class Option, class Traits>
ListCapability<Option, Traits>::ListCapability() {
  Reset();
}

template <class Option, class Traits>
ListCapability<Option, Traits>::~ListCapability() {
}

template <class Option, class Traits>
bool ListCapability<Option, Traits>::IsValid() const {
  if (empty())
    return false;  // This type of capabilities can't be empty.
  for (size_t i = 0; i < options_.size(); ++i) {
    if (!Traits::IsValid(options_[i]))
      return false;
  }
  return true;
}

template <class Option, class Traits>
bool ListCapability<Option, Traits>::LoadFrom(
    const CloudDeviceDescription& description) {
  Reset();
  const base::ListValue* options =
      description.GetListItem(Traits::GetCapabilityPath());
  if (!options)
    return false;
  for (size_t i = 0; i < options->GetSize(); ++i) {
    const base::DictionaryValue* option_value = NULL;
    if (!options->GetDictionary(i, &option_value))
      return false;  // Every entry must be a dictionary.
    Option option;
    if (!Traits::Load(*option_value, &option))
      return false;
    AddOption(option);
  }
  return IsValid();
}

template <class Option, class Traits>
void ListCapability<Option, Traits>::SaveTo(
    CloudDeviceDescription* description) const {
  DCHECK(IsValid());
  base::ListValue* options_list =
      description->CreateListItem(Traits::GetCapabilityPath());
  for (size_t i = 0; i < options_.size(); ++i) {
    std::unique_ptr<base::DictionaryValue> option_value(
        new base::DictionaryValue);
    Traits::Save(options_[i], option_value.get());
    options_list->Append(std::move(option_value));
  }
}

template <class Option, class Traits>
SelectionCapability<Option, Traits>::SelectionCapability() {
  Reset();
}

template <class Option, class Traits>
SelectionCapability<Option, Traits>::~SelectionCapability() {
}

template <class Option, class Traits>
bool SelectionCapability<Option, Traits>::IsValid() const {
  if (empty())
    return false;  // This type of capabilities can't be empty
  for (size_t i = 0; i < options_.size(); ++i) {
    if (!Traits::IsValid(options_[i]))
      return false;
  }
  return default_idx_ >= 0 && default_idx_ < base::checked_cast<int>(size());
}

template <class Option, class Traits>
bool SelectionCapability<Option, Traits>::LoadFrom(
    const CloudDeviceDescription& description) {
  Reset();
  const base::DictionaryValue* item =
      description.GetItem(Traits::GetCapabilityPath());
  if (!item)
    return false;
  const base::ListValue* options = NULL;
  if (!item->GetList(json::kKeyOption, &options))
    return false;
  for (size_t i = 0; i < options->GetSize(); ++i) {
    const base::DictionaryValue* option_value = NULL;
    if (!options->GetDictionary(i, &option_value))
      return false;  // Every entry must be a dictionary.
    Option option;
    if (!Traits::Load(*option_value, &option))
      return false;
    bool is_default = false;
    option_value->GetBoolean(json::kKeyIsDefault, &is_default);
    if (is_default && default_idx_ >= 0) {
      return false;  // Multiple defaults.
    }
    AddDefaultOption(option, is_default);
  }
  return IsValid();
}

template <class Option, class Traits>
void SelectionCapability<Option, Traits>::SaveTo(
    CloudDeviceDescription* description) const {
  DCHECK(IsValid());
  auto options_list = std::make_unique<base::ListValue>();
  for (size_t i = 0; i < options_.size(); ++i) {
    auto option_value = std::make_unique<base::DictionaryValue>();
    if (base::checked_cast<int>(i) == default_idx_)
      option_value->SetBoolean(json::kKeyIsDefault, true);
    Traits::Save(options_[i], option_value.get());
    options_list->Append(std::move(option_value));
  }
  description->CreateItem(Traits::GetCapabilityPath())
      ->Set(json::kKeyOption, std::move(options_list));
}

template <class Traits>
BooleanCapability<Traits>::BooleanCapability() {
  Reset();
}

template <class Traits>
BooleanCapability<Traits>::~BooleanCapability() {
}

template <class Traits>
bool BooleanCapability<Traits>::LoadFrom(
    const CloudDeviceDescription& description) {
  Reset();
  const base::DictionaryValue* dict =
      description.GetItem(Traits::GetCapabilityPath());
  if (!dict)
    return false;
  default_value_ = Traits::kDefault;
  dict->GetBoolean(json::kKeyDefault, &default_value_);
  return true;
}

template <class Traits>
void BooleanCapability<Traits>::SaveTo(
    CloudDeviceDescription* description) const {
  base::DictionaryValue* dict =
      description->CreateItem(Traits::GetCapabilityPath());
  if (default_value_ != Traits::kDefault)
    dict->SetBoolean(json::kKeyDefault, default_value_);
}

template <class Traits>
bool EmptyCapability<Traits>::LoadFrom(
    const CloudDeviceDescription& description) {
  return description.GetItem(Traits::GetCapabilityPath()) != NULL;
}

template <class Traits>
void EmptyCapability<Traits>::SaveTo(
    CloudDeviceDescription* description) const {
  description->CreateItem(Traits::GetCapabilityPath());
}

template <class Option, class Traits>
ValueCapability<Option, Traits>::ValueCapability() {
  Reset();
}

template <class Option, class Traits>
ValueCapability<Option, Traits>::~ValueCapability() {
}

template <class Option, class Traits>
bool ValueCapability<Option, Traits>::IsValid() const {
  return Traits::IsValid(value());
}

template <class Option, class Traits>
bool ValueCapability<Option, Traits>::LoadFrom(
    const CloudDeviceDescription& description) {
  Reset();
  const base::DictionaryValue* option_value =
      description.GetItem(Traits::GetCapabilityPath());
  if (!option_value)
    return false;
  Option option;
  if (!Traits::Load(*option_value, &option))
    return false;
  set_value(option);
  return IsValid();
}

template <class Option, class Traits>
void ValueCapability<Option, Traits>::SaveTo(
    CloudDeviceDescription* description) const {
  DCHECK(IsValid());
  Traits::Save(value(), description->CreateItem(Traits::GetCapabilityPath()));
}

template <class Option, class Traits>
TicketItem<Option, Traits>::TicketItem() {
  Reset();
}

template <class Option, class Traits>
TicketItem<Option, Traits>::~TicketItem() {
}

template <class Option, class Traits>
bool TicketItem<Option, Traits>::IsValid() const {
  return Traits::IsValid(value());
}

template <class Option, class Traits>
bool TicketItem<Option, Traits>::LoadFrom(
    const CloudDeviceDescription& description) {
  Reset();
  const base::DictionaryValue* option_value =
      description.GetItem(Traits::GetTicketItemPath());
  if (!option_value)
    return false;
  Option option;
  if (!Traits::Load(*option_value, &option))
    return false;
  set_value(option);
  return IsValid();
}

template <class Option, class Traits>
void TicketItem<Option, Traits>::SaveTo(
    CloudDeviceDescription* description) const {
  DCHECK(IsValid());
  Traits::Save(value(), description->CreateItem(Traits::GetTicketItemPath()));
}

}  // namespace cloud_devices

#endif  // COMPONENTS_CLOUD_DEVICES_COMMON_DESCRIPTION_DESCRIPTION_ITEMS_INL_H_
