// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SERVICES_FONT_FONT_SERVICE_APP_H_
#define COMPONENTS_SERVICES_FONT_FONT_SERVICE_APP_H_

#include <stdint.h>
#include <vector>

#include "base/macros.h"
#include "components/services/font/public/interfaces/font_service.mojom.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "services/service_manager/public/cpp/service.h"
#include "skia/ext/skia_utils_base.h"

namespace font_service {

class FontServiceApp : public service_manager::Service,
                       public mojom::FontService {
 public:
  FontServiceApp();
  ~FontServiceApp() override;

 private:
  // service_manager::Service:
  void OnStart() override;
  void OnBindInterface(const service_manager::BindSourceInfo& source_info,
                       const std::string& interface_name,
                       mojo::ScopedMessagePipeHandle interface_pipe) override;

  // FontService:
  void MatchFamilyName(const std::string& family_name,
                       mojom::TypefaceStylePtr requested_style,
                       MatchFamilyNameCallback callback) override;
  void OpenStream(uint32_t id_number, OpenStreamCallback callback) override;

  void Create(mojom::FontServiceRequest request);

  int FindOrAddPath(const SkString& path);

  service_manager::BinderRegistry registry_;
  mojo::BindingSet<mojom::FontService> bindings_;

  // We don't want to leak paths to our callers; we thus enumerate the paths of
  // fonts.
  std::vector<SkString> paths_;

  DISALLOW_COPY_AND_ASSIGN(FontServiceApp);
};

}  // namespace font_service

#endif  // COMPONENTS_SERVICES_FONT_FONT_SERVICE_APP_H_
