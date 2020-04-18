// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_GCM_DRIVER_REGISTRATION_INFO_H_
#define COMPONENTS_GCM_DRIVER_REGISTRATION_INFO_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/memory/linked_ptr.h"
#include "base/time/time.h"

namespace gcm  {

// Encapsulates the information needed to register with the server.
struct RegistrationInfo {
  enum RegistrationType {
    GCM_REGISTRATION,
    INSTANCE_ID_TOKEN
  };

  // Returns the appropriate RegistrationInfo instance based on the serialized
  // key and value.
  // |registration_id| can be NULL if no interest to it.
  static std::unique_ptr<RegistrationInfo> BuildFromString(
      const std::string& serialized_key,
      const std::string& serialized_value,
      std::string* registration_id);

  RegistrationInfo();
  virtual ~RegistrationInfo();

  // Returns the type of the registration info.
  virtual RegistrationType GetType() const = 0;

  // For persisting to the store. Depending on the type, part of the
  // registration info is written as key. The remaining of the registration
  // info plus the registration ID are written as value.
  virtual std::string GetSerializedKey() const = 0;
  virtual std::string GetSerializedValue(
      const std::string& registration_id) const = 0;
  // |registration_id| can be NULL if it is of no interest to the caller.
  virtual bool Deserialize(const std::string& serialized_key,
                           const std::string& serialized_value,
                           std::string* registration_id) = 0;

  // Every registration is associated with an application.
  std::string app_id;
  base::Time last_validated;
};

// For GCM registration.
struct GCMRegistrationInfo : public RegistrationInfo {
  GCMRegistrationInfo();
  ~GCMRegistrationInfo() override;

  // Converts from the base type;
  static const GCMRegistrationInfo* FromRegistrationInfo(
      const RegistrationInfo* registration_info);
  static GCMRegistrationInfo* FromRegistrationInfo(
      RegistrationInfo* registration_info);

  // RegistrationInfo overrides:
  RegistrationType GetType() const override;
  std::string GetSerializedKey() const override;
  std::string GetSerializedValue(
      const std::string& registration_id) const override;
  bool Deserialize(const std::string& serialized_key,
                   const std::string& serialized_value,
                   std::string* registration_id) override;

  // List of IDs of the servers that are allowed to send the messages to the
  // application. These IDs are assigned by the Google API Console.
  std::vector<std::string> sender_ids;
};

// For InstanceID token retrieval.
struct InstanceIDTokenInfo : public RegistrationInfo {
  InstanceIDTokenInfo();
  ~InstanceIDTokenInfo() override;

  // Converts from the base type;
  static const InstanceIDTokenInfo* FromRegistrationInfo(
      const RegistrationInfo* registration_info);
  static InstanceIDTokenInfo* FromRegistrationInfo(
      RegistrationInfo* registration_info);

  // RegistrationInfo overrides:
  RegistrationType GetType() const override;
  std::string GetSerializedKey() const override;
  std::string GetSerializedValue(
      const std::string& registration_id) const override;
  bool Deserialize(const std::string& serialized_key,
                   const std::string& serialized_value,
                   std::string* registration_id) override;

  // Entity that is authorized to access resources associated with the Instance
  // ID. It can be another Instance ID or a project ID assigned by the Google
  // API Console.
  std::string authorized_entity;

  // Authorized actions that the authorized entity can take.
  // E.g. for sending GCM messages, 'GCM' scope should be used.
  std::string scope;

  // Allows including a small number of string key/value pairs that will be
  // associated with the token and may be used in processing the request. These
  // are not serialized/deserialized.
  std::map<std::string, std::string> options;
};

struct RegistrationInfoComparer {
  bool operator()(const linked_ptr<RegistrationInfo>& a,
                  const linked_ptr<RegistrationInfo>& b) const;
};

// Collection of registration info.
// Map from RegistrationInfo instance to registration ID.
typedef std::map<linked_ptr<RegistrationInfo>,
                std::string,
                RegistrationInfoComparer> RegistrationInfoMap;

// Returns true if a GCM registration for |app_id| exists in |map|.
bool ExistsGCMRegistrationInMap(const RegistrationInfoMap& map,
                                const std::string& app_id);

}  // namespace gcm

#endif  // COMPONENTS_GCM_DRIVER_REGISTRATION_INFO_H_
