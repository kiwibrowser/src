// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/catalog/entry.h"

#include "base/files/file_path.h"
#include "base/path_service.h"
#include "base/values.h"
#include "build/build_config.h"
#include "services/catalog/public/cpp/manifest_parsing_util.h"
#include "services/catalog/store.h"
#include "services/service_manager/public/mojom/interface_provider_spec.mojom.h"

namespace catalog {
namespace {

#if defined(OS_WIN)
const char kServiceExecutableExtension[] = ".service.exe";
#else
const char kServiceExecutableExtension[] = ".service";
#endif

bool ReadStringSet(const base::ListValue& list_value,
                   std::set<std::string>* string_set) {
  DCHECK(string_set);
  for (const auto& value_value : list_value) {
    std::string value;
    if (!value_value.GetAsString(&value)) {
      LOG(ERROR) << "Entry::Deserialize: list member must be a string";
      return false;
    }
    string_set->insert(std::move(value));
  }
  return true;
}

bool ReadStringSetFromValue(const base::Value& value,
                            std::set<std::string>* string_set) {
  const base::ListValue* list_value = nullptr;
  if (!value.GetAsList(&list_value)) {
    LOG(ERROR) << "Entry::Deserialize: Value must be a list.";
    return false;
  }
  return ReadStringSet(*list_value, string_set);
}

// If |key| refers to a dictionary value within |value|, |*out| is set to that
// DictionaryValue. Returns true if either |key| is not present or the
// corresponding value is a dictionary.
bool GetDictionaryValue(const base::DictionaryValue& value,
                        base::StringPiece key,
                        const base::DictionaryValue** out) {
  const base::Value* entry_value = nullptr;
  return !value.Get(key, &entry_value) || entry_value->GetAsDictionary(out);
}

bool BuildInterfaceProviderSpec(
    const base::DictionaryValue& value,
    service_manager::InterfaceProviderSpec* interface_provider_specs) {
  DCHECK(interface_provider_specs);
  const base::DictionaryValue* provides_value = nullptr;
  if (!GetDictionaryValue(value, Store::kInterfaceProviderSpecs_ProvidesKey,
                          &provides_value)) {
    LOG(ERROR) << "Entry::Deserialize: "
               << Store::kInterfaceProviderSpecs_ProvidesKey
               << " must be a dictionary.";
    return false;
  }
  if (provides_value) {
    base::DictionaryValue::Iterator it(*provides_value);
    for(; !it.IsAtEnd(); it.Advance()) {
      service_manager::InterfaceSet interfaces;
      if (!ReadStringSetFromValue(it.value(), &interfaces)) {
        LOG(ERROR) << "Entry::Deserialize: Invalid interface list in provided "
                   << " capabilities dictionary";
        return false;
      }
      interface_provider_specs->provides[it.key()] = std::move(interfaces);
    }
  }

  const base::DictionaryValue* requires_value = nullptr;
  if (!GetDictionaryValue(value, Store::kInterfaceProviderSpecs_RequiresKey,
                          &requires_value)) {
    LOG(ERROR) << "Entry::Deserialize: "
               << Store::kInterfaceProviderSpecs_RequiresKey
               << " must be a dictionary.";
    return false;
  }
  if (requires_value) {
    base::DictionaryValue::Iterator it(*requires_value);
    for (; !it.IsAtEnd(); it.Advance()) {
      service_manager::CapabilitySet capabilities;
      const base::ListValue* entry_value = nullptr;
      if (!it.value().GetAsList(&entry_value)) {
        LOG(ERROR) << "Entry::Deserialize: "
                   << Store::kInterfaceProviderSpecs_RequiresKey
                   << " entry must be a list.";
        return false;
      }
      if (!ReadStringSet(*entry_value, &capabilities)) {
        LOG(ERROR) << "Entry::Deserialize: Invalid capabilities list in "
                   << "requires dictionary.";
        return false;
      }

      interface_provider_specs->requires[it.key()] = std::move(capabilities);
    }
  }
  return true;
}

}  // namespace

Entry::Entry() {}
Entry::Entry(const std::string& name)
    : name_(name),
      display_name_(name) {}
Entry::~Entry() {}

// static
std::unique_ptr<Entry> Entry::Deserialize(const base::Value& manifest_root) {
  const base::DictionaryValue* dictionary_value = nullptr;
  if (!manifest_root.GetAsDictionary(&dictionary_value))
    return nullptr;
  const base::DictionaryValue& value = *dictionary_value;

  auto entry = std::make_unique<Entry>();

  // Name.
  std::string name;
  if (!value.GetString(Store::kNameKey, &name)) {
    LOG(ERROR) << "Entry::Deserialize: dictionary has no "
               << Store::kNameKey << " key";
    return nullptr;
  }
  if (name.empty()) {
    LOG(ERROR) << "Entry::Deserialize: empty service name.";
    return nullptr;
  }
  entry->set_name(std::move(name));

  // By default we assume a standalone service executable. The catalog may
  // override this layer based on configuration external to the service's own
  // manifest.
  base::FilePath service_exe_root;
  CHECK(base::PathService::Get(base::DIR_ASSETS, &service_exe_root));
  entry->set_path(service_exe_root.AppendASCII(entry->name() +
                                               kServiceExecutableExtension));

  // Human-readable name.
  std::string display_name;
  if (!value.GetString(Store::kDisplayNameKey, &display_name)) {
    LOG(ERROR) << "Entry::Deserialize: dictionary has no "
               << Store::kDisplayNameKey << " key";
    return nullptr;
  }
  entry->set_display_name(std::move(display_name));

  // Sandbox type, optional.
  std::string sandbox_type;
  if (value.GetString(Store::kSandboxTypeKey, &sandbox_type))
    entry->set_sandbox_type(std::move(sandbox_type));

  // InterfaceProvider specs.
  const base::DictionaryValue* interface_provider_specs = nullptr;
  if (!value.GetDictionary(Store::kInterfaceProviderSpecsKey,
                           &interface_provider_specs)) {
    LOG(ERROR) << "Entry::Deserialize: dictionary has no "
               << Store::kInterfaceProviderSpecsKey << " key";
    return nullptr;
  }

  base::DictionaryValue::Iterator it(*interface_provider_specs);
  for (; !it.IsAtEnd(); it.Advance()) {
    const base::DictionaryValue* spec_value = nullptr;
    if (!interface_provider_specs->GetDictionary(it.key(), &spec_value)) {
      LOG(ERROR) << "Entry::Deserialize: value of InterfaceProvider map for "
                 << "key: " << it.key() << " not a dictionary.";
      return nullptr;
    }

    service_manager::InterfaceProviderSpec spec;
    if (!BuildInterfaceProviderSpec(*spec_value, &spec)) {
      LOG(ERROR) << "Entry::Deserialize: failed to build InterfaceProvider "
                 << "spec for key: " << it.key();
      return nullptr;
    }
    entry->AddInterfaceProviderSpec(it.key(), std::move(spec));
  }

  // Required files.
  base::Optional<RequiredFileMap> required_files =
      catalog::RetrieveRequiredFiles(value);
  DCHECK(required_files);
  for (auto& iter : *required_files) {
    entry->AddRequiredFilePath(iter.first, std::move(iter.second));
  }

  const base::ListValue* services = nullptr;
  if (value.GetList(Store::kServicesKey, &services)) {
    for (size_t i = 0; i < services->GetSize(); ++i) {
      const base::DictionaryValue* service = nullptr;
      services->GetDictionary(i, &service);
      std::unique_ptr<Entry> child = Entry::Deserialize(*service);
      if (child) {
        child->set_parent(entry.get());
        entry->children().emplace_back(std::move(child));
      }
    }
  }

  return entry;
}

bool Entry::ProvidesCapability(const std::string& capability) const {
  auto it = interface_provider_specs_.find(
      service_manager::mojom::kServiceManager_ConnectorSpec);
  if (it == interface_provider_specs_.end())
    return false;

  const auto& connection_spec = it->second;
  return connection_spec.provides.find(capability) !=
      connection_spec.provides.end();
}

bool Entry::operator==(const Entry& other) const {
  return other.name_ == name_ && other.display_name_ == display_name_ &&
         other.sandbox_type_ == sandbox_type_ &&
         other.interface_provider_specs_ == interface_provider_specs_;
}

void Entry::AddInterfaceProviderSpec(
    const std::string& name,
    service_manager::InterfaceProviderSpec spec) {
  interface_provider_specs_[name] = std::move(spec);
}

void Entry::AddRequiredFilePath(const std::string& name, base::FilePath path) {
  required_file_paths_[name] = std::move(path);
}

}  // catalog

namespace mojo {

// static
catalog::mojom::EntryPtr
    TypeConverter<catalog::mojom::EntryPtr, catalog::Entry>::Convert(
        const catalog::Entry& input) {
  catalog::mojom::EntryPtr result(catalog::mojom::Entry::New());
  result->name = input.name();
  result->display_name = input.display_name();
  return result;
}

}  // namespace mojo
