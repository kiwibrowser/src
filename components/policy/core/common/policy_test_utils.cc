// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/policy/core/common/policy_test_utils.h"

#include <string>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/callback.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/strings/sys_string_conversions.h"
#include "base/values.h"
#include "build/build_config.h"
#include "components/policy/core/common/policy_bundle.h"

#if defined(OS_IOS) || defined(OS_MACOSX)
#include <CoreFoundation/CoreFoundation.h>

#include "base/mac/scoped_cftyperef.h"
#endif

namespace policy {

PolicyDetailsMap::PolicyDetailsMap() {}

PolicyDetailsMap::~PolicyDetailsMap() {}

GetChromePolicyDetailsCallback PolicyDetailsMap::GetCallback() const {
  return base::Bind(&PolicyDetailsMap::Lookup, base::Unretained(this));
}

void PolicyDetailsMap::SetDetails(const std::string& policy,
                                  const PolicyDetails* details) {
  map_[policy] = details;
}

const PolicyDetails* PolicyDetailsMap::Lookup(const std::string& policy) const {
  PolicyDetailsMapping::const_iterator it = map_.find(policy);
  return it == map_.end() ? NULL : it->second;
}

bool PolicyServiceIsEmpty(const PolicyService* service) {
  const PolicyMap& map = service->GetPolicies(
      PolicyNamespace(POLICY_DOMAIN_CHROME, std::string()));
  if (!map.empty()) {
    base::DictionaryValue dict;
    for (PolicyMap::const_iterator it = map.begin(); it != map.end(); ++it)
      dict.SetKey(it->first, it->second.value->Clone());
    LOG(WARNING) << "There are pre-existing policies in this machine: " << dict;
  }
  return map.empty();
}

#if defined(OS_IOS) || defined(OS_MACOSX)
CFPropertyListRef ValueToProperty(const base::Value& value) {
  switch (value.type()) {
    case base::Value::Type::NONE:
      return kCFNull;

    case base::Value::Type::BOOLEAN: {
      bool bool_value;
      if (value.GetAsBoolean(&bool_value))
        return bool_value ? kCFBooleanTrue : kCFBooleanFalse;
      break;
    }

    case base::Value::Type::INTEGER: {
      int int_value;
      if (value.GetAsInteger(&int_value)) {
        return CFNumberCreate(
            kCFAllocatorDefault, kCFNumberIntType, &int_value);
      }
      break;
    }

    case base::Value::Type::DOUBLE: {
      double double_value;
      if (value.GetAsDouble(&double_value)) {
        return CFNumberCreate(
            kCFAllocatorDefault, kCFNumberDoubleType, &double_value);
      }
      break;
    }

    case base::Value::Type::STRING: {
      std::string string_value;
      if (value.GetAsString(&string_value))
        return base::SysUTF8ToCFStringRef(string_value);
      break;
    }

    case base::Value::Type::DICTIONARY: {
      const base::DictionaryValue* dict_value;
      if (value.GetAsDictionary(&dict_value)) {
        // |dict| is owned by the caller.
        CFMutableDictionaryRef dict =
            CFDictionaryCreateMutable(kCFAllocatorDefault,
                                      dict_value->size(),
                                      &kCFTypeDictionaryKeyCallBacks,
                                      &kCFTypeDictionaryValueCallBacks);
        for (base::DictionaryValue::Iterator iterator(*dict_value);
             !iterator.IsAtEnd(); iterator.Advance()) {
          // CFDictionaryAddValue() retains both |key| and |value|, so make sure
          // the references are balanced.
          base::ScopedCFTypeRef<CFStringRef> key(
              base::SysUTF8ToCFStringRef(iterator.key()));
          base::ScopedCFTypeRef<CFPropertyListRef> cf_value(
              ValueToProperty(iterator.value()));
          if (cf_value)
            CFDictionaryAddValue(dict, key, cf_value);
        }
        return dict;
      }
      break;
    }

    case base::Value::Type::LIST: {
      const base::ListValue* list;
      if (value.GetAsList(&list)) {
        CFMutableArrayRef array =
            CFArrayCreateMutable(NULL, list->GetSize(), &kCFTypeArrayCallBacks);
        for (const auto& entry : *list) {
          // CFArrayAppendValue() retains |cf_value|, so make sure the reference
          // created by ValueToProperty() is released.
          base::ScopedCFTypeRef<CFPropertyListRef> cf_value(
              ValueToProperty(entry));
          if (cf_value)
            CFArrayAppendValue(array, cf_value);
        }
        return array;
      }
      break;
    }

    case base::Value::Type::BINARY:
      // This type isn't converted (though it can be represented as CFData)
      // because there's no equivalent JSON type, and policy values can only
      // take valid JSON values.
      break;
  }

  return NULL;
}
#endif  // defined(OS_IOS) || defined(OS_MACOSX)

}  // namespace policy

std::ostream& operator<<(std::ostream& os,
                         const policy::PolicyBundle& bundle) {
  os << "{" << std::endl;
  for (policy::PolicyBundle::const_iterator iter = bundle.begin();
       iter != bundle.end(); ++iter) {
    os << "  \"" << iter->first << "\": " << *iter->second << "," << std::endl;
  }
  os << "}";
  return os;
}

std::ostream& operator<<(std::ostream& os, policy::PolicyScope scope) {
  switch (scope) {
    case policy::POLICY_SCOPE_USER: {
      os << "POLICY_SCOPE_USER";
      break;
    }
    case policy::POLICY_SCOPE_MACHINE: {
      os << "POLICY_SCOPE_MACHINE";
      break;
    }
    default: {
      os << "POLICY_SCOPE_UNKNOWN(" << int(scope) << ")";
    }
  }
  return os;
}

std::ostream& operator<<(std::ostream& os, policy::PolicyLevel level) {
  switch (level) {
    case policy::POLICY_LEVEL_RECOMMENDED: {
      os << "POLICY_LEVEL_RECOMMENDED";
      break;
    }
    case policy::POLICY_LEVEL_MANDATORY: {
      os << "POLICY_LEVEL_MANDATORY";
      break;
    }
    default: {
      os << "POLICY_LEVEL_UNKNOWN(" << int(level) << ")";
    }
  }
  return os;
}

std::ostream& operator<<(std::ostream& os, policy::PolicyDomain domain) {
  switch (domain) {
    case policy::POLICY_DOMAIN_CHROME: {
      os << "POLICY_DOMAIN_CHROME";
      break;
    }
    case policy::POLICY_DOMAIN_EXTENSIONS: {
      os << "POLICY_DOMAIN_EXTENSIONS";
      break;
    }
    case policy::POLICY_DOMAIN_SIGNIN_EXTENSIONS: {
      os << "POLICY_DOMAIN_SIGNIN_EXTENSIONS";
      break;
    }
    default: {
      os << "POLICY_DOMAIN_UNKNOWN(" << int(domain) << ")";
    }
  }
  return os;
}

std::ostream& operator<<(std::ostream& os, const policy::PolicyMap& policies) {
  os << "{" << std::endl;
  for (policy::PolicyMap::const_iterator iter = policies.begin();
       iter != policies.end(); ++iter) {
    os << "  \"" << iter->first << "\": " << iter->second << "," << std::endl;
  }
  os << "}";
  return os;
}

std::ostream& operator<<(std::ostream& os, const policy::PolicyMap::Entry& e) {
  std::string value;
  base::JSONWriter::WriteWithOptions(
      *e.value, base::JSONWriter::OPTIONS_PRETTY_PRINT, &value);
  os << "{" << std::endl
     << "  \"level\": " << e.level << "," << std::endl
     << "  \"scope\": " << e.scope << "," << std::endl
     << "  \"value\": " << value
     << "}";
  return os;
}

std::ostream& operator<<(std::ostream& os, const policy::PolicyNamespace& ns) {
  os << ns.domain << "/" << ns.component_id;
  return os;
}
