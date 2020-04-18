// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_EXTENSIONS_INPUT_METHOD_API_H_
#define CHROME_BROWSER_CHROMEOS_EXTENSIONS_INPUT_METHOD_API_H_

#include <memory>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "chrome/common/extensions/api/input_method_private.h"
#include "extensions/browser/browser_context_keyed_api_factory.h"
#include "extensions/browser/event_router.h"
#include "extensions/browser/extension_function.h"

namespace chromeos {
class ExtensionDictionaryEventRouter;
class ExtensionInputMethodEventRouter;
class ExtensionImeMenuEventRouter;
}

namespace extensions {

// Implements the inputMethodPrivate.getInputMethodConfig  method.
class InputMethodPrivateGetInputMethodConfigFunction
    : public UIThreadExtensionFunction {
 public:
  InputMethodPrivateGetInputMethodConfigFunction() {}

 protected:
  ~InputMethodPrivateGetInputMethodConfigFunction() override {}

  ResponseAction Run() override;

 private:
  DECLARE_EXTENSION_FUNCTION("inputMethodPrivate.getInputMethodConfig",
                             INPUTMETHODPRIVATE_GETINPUTMETHODCONFIG)
  DISALLOW_COPY_AND_ASSIGN(InputMethodPrivateGetInputMethodConfigFunction);
};

// Implements the inputMethodPrivate.getCurrentInputMethod method.
class InputMethodPrivateGetCurrentInputMethodFunction
    : public UIThreadExtensionFunction {
 public:
  InputMethodPrivateGetCurrentInputMethodFunction() {}

 protected:
  ~InputMethodPrivateGetCurrentInputMethodFunction() override {}

  ResponseAction Run() override;

 private:
  DECLARE_EXTENSION_FUNCTION("inputMethodPrivate.getCurrentInputMethod",
                             INPUTMETHODPRIVATE_GETCURRENTINPUTMETHOD)
  DISALLOW_COPY_AND_ASSIGN(InputMethodPrivateGetCurrentInputMethodFunction);
};

// Implements the inputMethodPrivate.setCurrentInputMethod method.
class InputMethodPrivateSetCurrentInputMethodFunction
    : public UIThreadExtensionFunction {
 public:
  InputMethodPrivateSetCurrentInputMethodFunction() {}

 protected:
  ~InputMethodPrivateSetCurrentInputMethodFunction() override {}

  ResponseAction Run() override;

 private:
  DECLARE_EXTENSION_FUNCTION("inputMethodPrivate.setCurrentInputMethod",
                             INPUTMETHODPRIVATE_SETCURRENTINPUTMETHOD)
  DISALLOW_COPY_AND_ASSIGN(InputMethodPrivateSetCurrentInputMethodFunction);
};

// Implements the inputMethodPrivate.getInputMethods method.
class InputMethodPrivateGetInputMethodsFunction
    : public UIThreadExtensionFunction {
 public:
  InputMethodPrivateGetInputMethodsFunction() {}

 protected:
  ~InputMethodPrivateGetInputMethodsFunction() override {}

  ResponseAction Run() override;

 private:
  DECLARE_EXTENSION_FUNCTION("inputMethodPrivate.getInputMethods",
                             INPUTMETHODPRIVATE_GETINPUTMETHODS)
  DISALLOW_COPY_AND_ASSIGN(InputMethodPrivateGetInputMethodsFunction);
};

// Implements the inputMethodPrivate.fetchAllDictionaryWords method.
class InputMethodPrivateFetchAllDictionaryWordsFunction
    : public UIThreadExtensionFunction {
 public:
  InputMethodPrivateFetchAllDictionaryWordsFunction() {}

 protected:
  ~InputMethodPrivateFetchAllDictionaryWordsFunction() override {}

  ResponseAction Run() override;

 private:
  DECLARE_EXTENSION_FUNCTION("inputMethodPrivate.fetchAllDictionaryWords",
                             INPUTMETHODPRIVATE_FETCHALLDICTIONARYWORDS)
  DISALLOW_COPY_AND_ASSIGN(InputMethodPrivateFetchAllDictionaryWordsFunction);
};

// Implements the inputMethodPrivate.addWordToDictionary method.
class InputMethodPrivateAddWordToDictionaryFunction
    : public UIThreadExtensionFunction {
 public:
  InputMethodPrivateAddWordToDictionaryFunction() {}

 protected:
  ~InputMethodPrivateAddWordToDictionaryFunction() override {}

  ResponseAction Run() override;

 private:
  DECLARE_EXTENSION_FUNCTION("inputMethodPrivate.addWordToDictionary",
                             INPUTMETHODPRIVATE_ADDWORDTODICTIONARY)
  DISALLOW_COPY_AND_ASSIGN(InputMethodPrivateAddWordToDictionaryFunction);
};

// Implements the inputMethodPrivate.getEncryptSyncEnabled method.
class InputMethodPrivateGetEncryptSyncEnabledFunction
    : public UIThreadExtensionFunction {
 public:
  InputMethodPrivateGetEncryptSyncEnabledFunction() {}

 protected:
  ~InputMethodPrivateGetEncryptSyncEnabledFunction() override {}

  ResponseAction Run() override;

 private:
  DECLARE_EXTENSION_FUNCTION("inputMethodPrivate.getEncryptSyncEnabled",
                             INPUTMETHODPRIVATE_GETENCRYPTSYNCENABLED)
  DISALLOW_COPY_AND_ASSIGN(InputMethodPrivateGetEncryptSyncEnabledFunction);
};

// Implements the inputMethodPrivate.setXkbLayout method.
class InputMethodPrivateSetXkbLayoutFunction
    : public UIThreadExtensionFunction {
 public:
  InputMethodPrivateSetXkbLayoutFunction() {}

 protected:
  ~InputMethodPrivateSetXkbLayoutFunction() override {}

  ResponseAction Run() override;

 private:
  DECLARE_EXTENSION_FUNCTION("inputMethodPrivate.setXkbLayout",
                             INPUTMETHODPRIVATE_SETXKBLAYOUT)
  DISALLOW_COPY_AND_ASSIGN(InputMethodPrivateSetXkbLayoutFunction);
};

// Implements the inputMethodPrivate.showInputView method.
class InputMethodPrivateShowInputViewFunction
    : public UIThreadExtensionFunction {
 public:
  InputMethodPrivateShowInputViewFunction() {}

 protected:
  ~InputMethodPrivateShowInputViewFunction() override {}

  ResponseAction Run() override;

 private:
  DECLARE_EXTENSION_FUNCTION("inputMethodPrivate.showInputView",
                             INPUTMETHODPRIVATE_SHOWINPUTVIEW)
  DISALLOW_COPY_AND_ASSIGN(InputMethodPrivateShowInputViewFunction);
};

// Implements the inputMethodPrivate.openOptionsPage method.
class InputMethodPrivateOpenOptionsPageFunction
    : public UIThreadExtensionFunction {
 public:
  InputMethodPrivateOpenOptionsPageFunction() {}

 protected:
  ~InputMethodPrivateOpenOptionsPageFunction() override {}

  ResponseAction Run() override;

 private:
  DECLARE_EXTENSION_FUNCTION("inputMethodPrivate.openOptionsPage",
                             INPUTMETHODPRIVATE_OPENOPTIONSPAGE)
  DISALLOW_COPY_AND_ASSIGN(InputMethodPrivateOpenOptionsPageFunction);
};

class InputMethodAPI : public BrowserContextKeyedAPI,
                       public extensions::EventRouter::Observer {
 public:
  explicit InputMethodAPI(content::BrowserContext* context);
  ~InputMethodAPI() override;

  // Returns input method name for the given XKB (X keyboard extensions in X
  // Window System) id.
  static std::string GetInputMethodForXkb(const std::string& xkb_id);

  // BrowserContextKeyedAPI implementation.
  static BrowserContextKeyedAPIFactory<InputMethodAPI>* GetFactoryInstance();

  // BrowserContextKeyedAPI implementation.
  void Shutdown() override;

  // EventRouter::Observer implementation.
  void OnListenerAdded(const extensions::EventListenerInfo& details) override;

 private:
  friend class BrowserContextKeyedAPIFactory<InputMethodAPI>;

  // BrowserContextKeyedAPI implementation.
  static const char* service_name() {
    return "InputMethodAPI";
  }
  static const bool kServiceIsNULLWhileTesting = true;

  content::BrowserContext* const context_;

  // Created lazily upon OnListenerAdded.
  std::unique_ptr<chromeos::ExtensionInputMethodEventRouter>
      input_method_event_router_;
  std::unique_ptr<chromeos::ExtensionDictionaryEventRouter>
      dictionary_event_router_;
  std::unique_ptr<chromeos::ExtensionImeMenuEventRouter> ime_menu_event_router_;

  DISALLOW_COPY_AND_ASSIGN(InputMethodAPI);
};

}  // namespace extensions

#endif  // CHROME_BROWSER_CHROMEOS_EXTENSIONS_INPUT_METHOD_API_H_
