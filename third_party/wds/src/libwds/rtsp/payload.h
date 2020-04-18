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


#ifndef LIBWDS_RTSP_PAYLOAD_H_
#define LIBWDS_RTSP_PAYLOAD_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "libwds/public/logging.h"

#include "libwds/rtsp/property.h"
#include "libwds/rtsp/genericproperty.h"
#include "libwds/rtsp/propertyerrors.h"

namespace wds {
namespace rtsp {

using PropertyMap = std::map<std::string, std::shared_ptr<Property>>;
using PropertyErrorMap = std::map<std::string, std::shared_ptr<PropertyErrors>>;

class Payload {
 public:
  enum Type {
    Properties,
    Requests,
    Errors
  };

  virtual ~Payload();
  virtual std::string ToString() const = 0;

  Type type() const { return type_; }

 protected:
  explicit Payload(Type type) : type_(type) {}
  Type type_;
};

class PropertyMapPayload : public Payload {
 public:
  PropertyMapPayload() : Payload(Payload::Properties) {}

  ~PropertyMapPayload() override;

  std::shared_ptr<Property> GetProperty(const std::string& name) const;
  std::shared_ptr<Property> GetProperty(PropertyType type) const;
  bool HasProperty(PropertyType type) const;
  void AddProperty(const std::shared_ptr<Property>& property);
  const PropertyMap& properties() const { return properties_; }

  std::string ToString() const override;

 private:
  PropertyMap properties_;
};

inline PropertyMapPayload* ToPropertyMapPayload(Payload* payload) {
  if (!payload)
     return nullptr;
  if (payload->type() == Payload::Properties)
    return static_cast<PropertyMapPayload*>(payload);
  WDS_ERROR("Inappropriate payload type");
  return nullptr;
}

class GetParameterPayload : public Payload {
 public:
  GetParameterPayload() : Payload(Payload::Requests) {}
  explicit GetParameterPayload(
      const std::vector<std::string>& properties);
  ~GetParameterPayload() override;

  void AddRequestProperty(const PropertyType& property);
  void AddRequestProperty(const std::string& generic_property);
  const std::vector<std::string>& properties() const {
    return properties_;
  }

  std::string ToString() const override;

 private:
  std::vector<std::string> properties_;
};

inline GetParameterPayload* ToGetParameterPayload(Payload* payload) {
  if (!payload)
    return nullptr;
  if (payload->type() == Payload::Requests)
    return static_cast<GetParameterPayload*>(payload);
  WDS_ERROR("Inappropriate payload type");
  return nullptr;
}

class PropertyErrorPayload : public Payload {
 public:
  PropertyErrorPayload() : Payload(Payload::Errors) {}
  ~PropertyErrorPayload() override;

  std::shared_ptr<PropertyErrors> GetPropertyError(const std::string& name) const;
  std::shared_ptr<PropertyErrors> GetPropertyError(PropertyType type) const;
  void AddPropertyError(const std::shared_ptr<PropertyErrors>& error);
  const PropertyErrorMap& property_errors() const { return property_errors_; }
  std::string ToString() const override;

 private:
  PropertyErrorMap property_errors_;
};

inline PropertyErrorPayload* ToPropertyErrorPayload(Payload* payload) {
  if (!payload)
     return nullptr;
  if (payload->type() == Payload::Errors)
    return static_cast<PropertyErrorPayload*>(payload);
  WDS_ERROR("Inappropriate payload type");
  return nullptr;
}

}  // namespace rtsp
}  // namespace wds

#endif // LIBWDS_RTSP_PAYLOAD_H_
