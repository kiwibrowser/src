// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_CATALOG_ENTRY_H_
#define SERVICES_CATALOG_ENTRY_H_

#include <memory>
#include <string>
#include <vector>

#include "base/component_export.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "services/catalog/public/mojom/catalog.mojom.h"
#include "services/service_manager/public/cpp/interface_provider_spec.h"

namespace base {
class Value;
}

namespace catalog {

// Static information about a service package known to the Catalog.
class COMPONENT_EXPORT(CATALOG) Entry {
 public:
  Entry();
  explicit Entry(const std::string& name);
  ~Entry();

  static std::unique_ptr<Entry> Deserialize(const base::Value& manifest_root);

  bool ProvidesCapability(const std::string& capability) const;

  bool operator==(const Entry& other) const;

  const std::string& name() const { return name_; }
  void set_name(std::string name) { name_ = std::move(name); }

  const base::FilePath& path() const { return path_; }
  void set_path(base::FilePath path) { path_ = std::move(path); }

  const std::string& display_name() const { return display_name_; }
  void set_display_name(std::string display_name) {
    display_name_ = std::move(display_name);
  }

  const std::string& sandbox_type() const { return sandbox_type_; }
  void set_sandbox_type(std::string sandbox_type) {
    sandbox_type_ = std::move(sandbox_type);
  }

  const Entry* parent() const { return parent_; }
  void set_parent(const Entry* parent) { parent_ = parent; }

  const std::vector<std::unique_ptr<Entry>>& children() const {
    return children_;
  }
  std::vector<std::unique_ptr<Entry>>& children() { return children_; }
  void set_children(std::vector<std::unique_ptr<Entry>> children) {
    children_ = std::move(children);
  }

  void AddInterfaceProviderSpec(const std::string& name,
                                service_manager::InterfaceProviderSpec spec);
  const service_manager::InterfaceProviderSpecMap&
      interface_provider_specs() const {
    return interface_provider_specs_;
  }

  void AddRequiredFilePath(const std::string& name, base::FilePath path);
  const std::map<std::string, base::FilePath>& required_file_paths() const {
    return required_file_paths_;
  }

 private:
  std::string name_;
  base::FilePath path_;
  std::string display_name_;
  std::string sandbox_type_;
  service_manager::InterfaceProviderSpecMap interface_provider_specs_;
  std::map<std::string, base::FilePath> required_file_paths_;
  const Entry* parent_ = nullptr;
  std::vector<std::unique_ptr<Entry>> children_;

  DISALLOW_COPY_AND_ASSIGN(Entry);
};

}  // namespace catalog

namespace mojo {

template<>
struct TypeConverter<catalog::mojom::EntryPtr, catalog::Entry> {
  static catalog::mojom::EntryPtr Convert(const catalog::Entry& input);
};

}  // namespace mojo

#endif  // SERVICES_CATALOG_ENTRY_H_
