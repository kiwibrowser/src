/*
 * This file is part of Wireless Display Software for Linux OS
 *
 * Copyright (C) 2014 Intel Corporation.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */


#include "libwds/rtsp/payload.h"

namespace wds {
namespace rtsp {

Payload::~Payload() {
}

PropertyMapPayload::~PropertyMapPayload() {
}

std::shared_ptr<Property> PropertyMapPayload::GetProperty(
    const std::string& name) const {
  auto property = properties_.find(name);
  if (property != properties_.end())
    return property->second;
  return nullptr;
}

std::shared_ptr<Property> PropertyMapPayload::GetProperty(
    PropertyType type) const {
  if (type == GenericPropertyType)
    return nullptr;

  return GetProperty(GetPropertyName(type));
}

bool PropertyMapPayload::HasProperty(PropertyType type) const {
  return properties_.find(GetPropertyName(type)) != properties_.end();
}

void PropertyMapPayload::AddProperty(
    const std::shared_ptr<Property>& property) {
  properties_[property->GetName()] = property;
}

std::string PropertyMapPayload::ToString() const {
  std::string ret;
  for (auto it : properties_) {
    if (auto property = it.second) {
      ret += property->ToString();
      ret += "\r\n";
    }
  }

  return ret;
}

GetParameterPayload::GetParameterPayload(const std::vector<std::string>& properties)
   : Payload(Payload::Requests),
     properties_(properties) {
}

GetParameterPayload::~GetParameterPayload() {
}

void GetParameterPayload::AddRequestProperty(const PropertyType& type) {
  properties_.push_back(GetPropertyName(type));
}

void GetParameterPayload::AddRequestProperty(
    const std::string& generic_property) {
  properties_.push_back(generic_property);
}

std::string GetParameterPayload::ToString() const {
  std::string ret;
  for (const std::string& property : properties_) {
    ret += property;
    ret += "\r\n";
  }

  return ret;
}

PropertyErrorPayload::~PropertyErrorPayload() {
}

std::shared_ptr<PropertyErrors> PropertyErrorPayload::GetPropertyError(
    const std::string& name) const {
  auto property = property_errors_.find(name);
  if (property != property_errors_.end())
    return property->second;
  return nullptr;
}

std::shared_ptr<PropertyErrors> PropertyErrorPayload::GetPropertyError(
    PropertyType type) const {
  if (type == GenericPropertyType)
    return nullptr;
  return GetPropertyError(GetPropertyName(type));
}

void PropertyErrorPayload::AddPropertyError(const std::shared_ptr<PropertyErrors>& errors) {
  if (errors->type() == GenericPropertyType) {
    property_errors_[errors->generic_property_name()] = errors;
  } else {
    property_errors_[GetPropertyName(errors->type())] = errors;
  }
}

std::string PropertyErrorPayload::ToString() const {
  std::string ret;
  for (auto it = property_errors_.rbegin();
       it != property_errors_.rend(); ++it) {
    ret += it->second->ToString();
    ret += "\r\n";
  }

  return ret;
}

}  // namespace rtsp
}  // namespace wds
