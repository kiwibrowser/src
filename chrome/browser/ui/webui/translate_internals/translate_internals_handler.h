// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_TRANSLATE_INTERNALS_TRANSLATE_INTERNALS_HANDLER_H_
#define CHROME_BROWSER_UI_WEBUI_TRANSLATE_INTERNALS_TRANSLATE_INTERNALS_HANDLER_H_

#include <memory>
#include <string>

#include "base/callback_list.h"
#include "base/macros.h"
#include "components/translate/core/browser/translate_language_list.h"
#include "components/translate/core/browser/translate_manager.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "content/public/browser/web_ui_message_handler.h"
#include "content/public/common/webplugininfo.h"

namespace translate {
struct TranslateErrorDetails;
struct TranslateEventDetails;
}

namespace base {
class ListValue;
class Value;
}

namespace content {
class NotificationDetails;
class NotificationSource;
}

// The handler class for TranslateInternals page operations.
class TranslateInternalsHandler : public content::WebUIMessageHandler,
                                  public content::NotificationObserver {
 public:
  TranslateInternalsHandler();
  ~TranslateInternalsHandler() override;

  // content::WebUIMessageHandler methods:
  void RegisterMessages() override;

 private:
  // content::NotificationObserver implementation:
  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override;

  // Callback for translate errors.
  void OnTranslateError(const translate::TranslateErrorDetails& details);

  // Callback for translate events.
  virtual void OnTranslateEvent(
      const translate::TranslateEventDetails& details);

  // Handles the Javascript message 'removePrefItem'. This message is sent
  // when UI requests to remove an item in the preference.
  void OnRemovePrefItem(const base::ListValue* args);

  // Handles the Javascript message 'overrideCountry'. This message is sent
  // when UI requests to override the stored country.
  void OnOverrideCountry(const base::ListValue* country);

  // Handles the Javascript message 'requestInfo'. This message is sent
  // when UI needs to show information concerned with the translation.
  // For now, this returns only prefs to Javascript.
  // |args| is not used.
  void OnRequestInfo(const base::ListValue* args);

  // Sends a messsage to Javascript.
  void SendMessageToJs(const std::string& message, const base::Value& value);

  // Sends the current preference to Javascript.
  void SendPrefsToJs();

  // Sends the languages currently supported by the server to JavaScript.
  void SendSupportedLanguagesToJs();

  // Sends the stored permanent country to Javascript.
  // |was_updated| tells Javascript if the country has been updated or not.
  void SendCountryToJs(bool was_updated);

  // Subscription for translate events coming from the translate language list.
  std::unique_ptr<
      translate::TranslateLanguageList::EventCallbackList::Subscription>
      event_subscription_;

  // Subscription for translate errors coming from the translate manager.
  std::unique_ptr<
      translate::TranslateManager::TranslateErrorCallbackList::Subscription>
      error_subscription_;

  content::NotificationRegistrar notification_registrar_;

  DISALLOW_COPY_AND_ASSIGN(TranslateInternalsHandler);
};

#endif  // CHROME_BROWSER_UI_WEBUI_TRANSLATE_INTERNALS_TRANSLATE_INTERNALS_HANDLER_H_
