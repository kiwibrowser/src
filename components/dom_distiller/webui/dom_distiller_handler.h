// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DOM_DISTILLER_WEBUI_DOM_DISTILLER_HANDLER_H_
#define COMPONENTS_DOM_DISTILLER_WEBUI_DOM_DISTILLER_HANDLER_H_

#include <string>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/values.h"
#include "content/public/browser/web_ui_message_handler.h"

namespace dom_distiller {

class DomDistillerService;

// Handler class for DOM Distiller list operations.
class DomDistillerHandler : public content::WebUIMessageHandler {
 public:
  // The lifetime of |service| has to outlive this handler.
  DomDistillerHandler(DomDistillerService* service, const std::string& scheme);
  ~DomDistillerHandler() override;

  // content::WebUIMessageHandler implementation.
  void RegisterMessages() override;

  // Callback from JavaScript for the "requestEntries" message. This
  // requests the list of entries and returns it to the front end by calling
  // "onReceivedEntries". There are no JavaScript arguments to this method.
  void HandleRequestEntries(const base::ListValue* args);

  // Callback from JavaScript for when an article should be added. The first
  // element in |args| should be a string representing the URL to be added.
  void HandleAddArticle(const base::ListValue* args);

  // Callback from JavaScript for when an article is selected. The first element
  // in |args| should be a string representing the ID of the entry to be
  // selected.
  void HandleSelectArticle(const base::ListValue* args);

  // Callback from JavaScript for when viewing a URL is requested. The first
  // element in |args| should be a string representing the URL to be viewed.
  void HandleViewUrl(const base::ListValue* args);

 private:
  // Callback from DomDistillerService when an article is available.
  void OnArticleAdded(bool article_available);

  // The DomDistillerService.
  DomDistillerService* service_;

  // The scheme for DOM distiller articles.
  std::string article_scheme_;

  // Factory for the creating refs in callbacks.
  base::WeakPtrFactory<DomDistillerHandler> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(DomDistillerHandler);
};

}  // namespace dom_distiller

#endif  // COMPONENTS_DOM_DISTILLER_WEBUI_DOM_DISTILLER_HANDLER_H_
