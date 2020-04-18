// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DOM_DISTILLER_WEBUI_DOM_DISTILLER_UI_H_
#define COMPONENTS_DOM_DISTILLER_WEBUI_DOM_DISTILLER_UI_H_

#include <string>

#include "base/macros.h"
#include "content/public/browser/web_ui_controller.h"

namespace dom_distiller {

class DomDistillerService;

// The WebUI controller for chrome://dom-distiller.
class DomDistillerUi : public content::WebUIController {
 public:
  DomDistillerUi(content::WebUI* web_ui,
                 DomDistillerService* service,
                 const std::string& scheme);
  ~DomDistillerUi() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(DomDistillerUi);
};

}  // namespace dom_distiller

#endif  // COMPONENTS_DOM_DISTILLER_WEBUI_DOM_DISTILLER_UI_H_
