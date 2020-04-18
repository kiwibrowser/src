// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MASH_CATALOG_VIEWER_CATALOG_VIEWER_H_
#define MASH_CATALOG_VIEWER_CATALOG_VIEWER_H_

#include <map>
#include <memory>

#include "base/callback.h"
#include "base/macros.h"
#include "mash/public/mojom/launchable.mojom.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "services/service_manager/public/cpp/service.h"

namespace views {
class AuraInit;
class Widget;
}

namespace mash {
namespace catalog_viewer {

class CatalogViewer : public service_manager::Service,
                      public mojom::Launchable {
 public:
  CatalogViewer();
  ~CatalogViewer() override;

  void RemoveWindow(views::Widget* window);

 private:
  // service_manager::Service:
  void OnStart() override;
  void OnBindInterface(const service_manager::BindSourceInfo& source_info,
                       const std::string& interface_name,
                       mojo::ScopedMessagePipeHandle interface_pipe) override;

  // mojom::Launchable:
  void Launch(uint32_t what, mojom::LaunchMode how) override;

  void Create(mojom::LaunchableRequest request);

  mojo::BindingSet<mojom::Launchable> bindings_;
  std::vector<views::Widget*> windows_;

  service_manager::BinderRegistry registry_;

  std::unique_ptr<views::AuraInit> aura_init_;

  DISALLOW_COPY_AND_ASSIGN(CatalogViewer);
};

}  // namespace catalog_viewer
}  // namespace mash

#endif  // MASH_CATALOG_VIEWER_CATALOG_VIEWER_H_
