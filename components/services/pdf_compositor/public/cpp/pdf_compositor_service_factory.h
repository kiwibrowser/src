// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SERVICES_PDF_COMPOSITOR_PUBLIC_CPP_PDF_COMPOSITOR_SERVICE_FACTORY_H_
#define COMPONENTS_SERVICES_PDF_COMPOSITOR_PUBLIC_CPP_PDF_COMPOSITOR_SERVICE_FACTORY_H_

#include <memory>
#include <string>

#include "services/service_manager/public/cpp/service.h"

namespace printing {

std::unique_ptr<service_manager::Service> CreatePdfCompositorService(
    const std::string& creator);

}  // namespace printing

#endif  // COMPONENTS_SERVICES_PDF_COMPOSITOR_PUBLIC_CPP_PDF_COMPOSITOR_SERVICE_FACTORY_H_
