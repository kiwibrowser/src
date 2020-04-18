// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/printing/cloud_print/privet_constants.h"

namespace cloud_print {

const char kPrivetKeyError[] = "error";
const char kPrivetInfoKeyToken[] = "x-privet-token";
const char kPrivetInfoKeyAPIList[] = "api";
const char kPrivetInfoKeyID[] = "id";
const char kPrivetKeyDeviceID[] = "device_id";
const char kPrivetKeyClaimURL[] = "claim_url";
const char kPrivetKeyClaimToken[] = "token";
const char kPrivetKeyTimeout[] = "timeout";

const char kPrivetActionNameInfo[] = "info";

const char kPrivetInfoPath[] = "/privet/info";
const char kPrivetRegisterPath[] = "/privet/register";
const char kPrivetCapabilitiesPath[] = "/privet/capabilities";
const char kPrivetSubmitdocPath[] = "/privet/printer/submitdoc";
const char kPrivetCreatejobPath[] = "/privet/printer/createjob";

const char kPrivetErrorDeviceBusy[] = "device_busy";
const char kPrivetErrorPrinterBusy[] = "printer_busy";
const char kPrivetErrorInvalidPrintJob[] = "invalid_print_job";
const char kPrivetErrorInvalidDocumentType[] = "invalid_document_type";
const char kPrivetErrorPendingUserAction[] = "pending_user_action";
const char kPrivetErrorInvalidXPrivetToken[] = "invalid_x_privet_token";
const char kPrivetErrorTimeout[] = "confirmation_timeout";
const char kPrivetErrorCancel[] = "user_cancel";

const char kPrivetActionStart[] = "start";
const char kPrivetActionGetClaimToken[] = "getClaimToken";
const char kPrivetActionComplete[] = "complete";
const char kPrivetActionCancel[] = "cancel";

const char kPrivetDefaultDeviceType[] = "_privet._tcp.local";

const char kPrivetTypePrinter[] = "printer";

const char kPrivetTxtKeyName[] = "ty";
const char kPrivetTxtKeyDescription[] = "note";
const char kPrivetTxtKeyVersion[] = "txtvers";
const char kPrivetTxtKeyType[] = "type";
const char kPrivetTxtKeyID[] = "id";
const char kPrivetTxtKeyGcdID[] = "gcd_id";
const char kPrivetTxtKeyDevicesClass[] = "class";

}  // namespace cloud_print
