// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_API_PASSWORDS_PRIVATE_PASSWORDS_PRIVATE_API_H_
#define CHROME_BROWSER_EXTENSIONS_API_PASSWORDS_PRIVATE_PASSWORDS_PRIVATE_API_H_

#include <string>

#include "base/macros.h"
#include "chrome/browser/extensions/api/passwords_private/passwords_private_delegate.h"
#include "chrome/browser/ui/passwords/password_manager_presenter.h"
#include "extensions/browser/extension_function.h"

namespace extensions {

class PasswordsPrivateRemoveSavedPasswordFunction :
    public UIThreadExtensionFunction {
 public:
  PasswordsPrivateRemoveSavedPasswordFunction() {}
  DECLARE_EXTENSION_FUNCTION("passwordsPrivate.removeSavedPassword",
                             PASSWORDSPRIVATE_REMOVESAVEDPASSWORD);

 protected:
  ~PasswordsPrivateRemoveSavedPasswordFunction() override;

  // ExtensionFunction overrides.
  ResponseAction Run() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(PasswordsPrivateRemoveSavedPasswordFunction);
};

class PasswordsPrivateRemovePasswordExceptionFunction :
    public UIThreadExtensionFunction {
 public:
  PasswordsPrivateRemovePasswordExceptionFunction() {}
  DECLARE_EXTENSION_FUNCTION("passwordsPrivate.removePasswordException",
                             PASSWORDSPRIVATE_REMOVEPASSWORDEXCEPTION);

 protected:
  ~PasswordsPrivateRemovePasswordExceptionFunction() override;

  // ExtensionFunction overrides.
  ResponseAction Run() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(PasswordsPrivateRemovePasswordExceptionFunction);
};

class PasswordsPrivateUndoRemoveSavedPasswordOrExceptionFunction
    : public UIThreadExtensionFunction {
 public:
  PasswordsPrivateUndoRemoveSavedPasswordOrExceptionFunction() {}
  DECLARE_EXTENSION_FUNCTION(
      "passwordsPrivate.undoRemoveSavedPasswordOrException",
      PASSWORDSPRIVATE_UNDOREMOVESAVEDPASSWORDOREXCEPTION);

 protected:
  ~PasswordsPrivateUndoRemoveSavedPasswordOrExceptionFunction() override;

  // ExtensionFunction overrides.
  ResponseAction Run() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(
      PasswordsPrivateUndoRemoveSavedPasswordOrExceptionFunction);
};

class PasswordsPrivateRequestPlaintextPasswordFunction :
    public UIThreadExtensionFunction {
 public:
  PasswordsPrivateRequestPlaintextPasswordFunction() {}
  DECLARE_EXTENSION_FUNCTION("passwordsPrivate.requestPlaintextPassword",
                             PASSWORDSPRIVATE_REQUESTPLAINTEXTPASSWORD);

 protected:
  ~PasswordsPrivateRequestPlaintextPasswordFunction() override;

  // ExtensionFunction overrides.
  ResponseAction Run() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(PasswordsPrivateRequestPlaintextPasswordFunction);
};

class PasswordsPrivateGetSavedPasswordListFunction
    : public UIThreadExtensionFunction {
 public:
  PasswordsPrivateGetSavedPasswordListFunction() {}
  DECLARE_EXTENSION_FUNCTION("passwordsPrivate.getSavedPasswordList",
                             PASSWORDSPRIVATE_GETSAVEDPASSWORDLIST);

 protected:
  ~PasswordsPrivateGetSavedPasswordListFunction() override;

  // ExtensionFunction overrides.
  ResponseAction Run() override;

 private:
  void GetList();
  void GotList(const PasswordsPrivateDelegate::UiEntries& entries);

  DISALLOW_COPY_AND_ASSIGN(PasswordsPrivateGetSavedPasswordListFunction);
};

class PasswordsPrivateGetPasswordExceptionListFunction
    : public UIThreadExtensionFunction {
 public:
  PasswordsPrivateGetPasswordExceptionListFunction() {}
  DECLARE_EXTENSION_FUNCTION("passwordsPrivate.getPasswordExceptionList",
                             PASSWORDSPRIVATE_GETPASSWORDEXCEPTIONLIST);

 protected:
  ~PasswordsPrivateGetPasswordExceptionListFunction() override;

  // ExtensionFunction overrides.
  ResponseAction Run() override;

 private:
  void GetList();
  void GotList(const PasswordsPrivateDelegate::ExceptionEntries& entries);

  DISALLOW_COPY_AND_ASSIGN(PasswordsPrivateGetPasswordExceptionListFunction);
};

class PasswordsPrivateImportPasswordsFunction
    : public UIThreadExtensionFunction {
 public:
  PasswordsPrivateImportPasswordsFunction() {}
  DECLARE_EXTENSION_FUNCTION("passwordsPrivate.importPasswords",
                             PASSWORDSPRIVATE_IMPORTPASSWORDS);

 protected:
  ~PasswordsPrivateImportPasswordsFunction() override;

  // ExtensionFunction overrides.
  ResponseAction Run() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(PasswordsPrivateImportPasswordsFunction);
};

class PasswordsPrivateExportPasswordsFunction
    : public UIThreadExtensionFunction {
 public:
  PasswordsPrivateExportPasswordsFunction() {}
  DECLARE_EXTENSION_FUNCTION("passwordsPrivate.exportPasswords",
                             PASSWORDSPRIVATE_EXPORTPASSWORDS);

 protected:
  ~PasswordsPrivateExportPasswordsFunction() override;

  // ExtensionFunction overrides.
  ResponseAction Run() override;

 private:
  void ExportRequestCompleted(const std::string& error);

  DISALLOW_COPY_AND_ASSIGN(PasswordsPrivateExportPasswordsFunction);
};

class PasswordsPrivateCancelExportPasswordsFunction
    : public UIThreadExtensionFunction {
 public:
  PasswordsPrivateCancelExportPasswordsFunction() {}
  DECLARE_EXTENSION_FUNCTION("passwordsPrivate.cancelExportPasswords",
                             PASSWORDSPRIVATE_CANCELEXPORTPASSWORDS);

 protected:
  ~PasswordsPrivateCancelExportPasswordsFunction() override;

  // ExtensionFunction overrides.
  ResponseAction Run() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(PasswordsPrivateCancelExportPasswordsFunction);
};

class PasswordsPrivateRequestExportProgressStatusFunction
    : public UIThreadExtensionFunction {
 public:
  PasswordsPrivateRequestExportProgressStatusFunction() {}
  DECLARE_EXTENSION_FUNCTION("passwordsPrivate.requestExportProgressStatus",
                             PASSWORDSPRIVATE_REQUESTEXPORTPROGRESSSTATUS);

 protected:
  ~PasswordsPrivateRequestExportProgressStatusFunction() override;

  // ExtensionFunction overrides.
  ResponseAction Run() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(PasswordsPrivateRequestExportProgressStatusFunction);
};

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_API_PASSWORDS_PRIVATE_PASSWORDS_PRIVATE_API_H_
