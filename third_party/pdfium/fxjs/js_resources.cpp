// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "fxjs/js_resources.h"

WideString JSGetStringFromID(JSMessage msg) {
  switch (msg) {
    case JSMessage::kAlert:
      return L"Alert";
    case JSMessage::kParamError:
      return L"Incorrect number of parameters passed to function.";
    case JSMessage::kInvalidInputError:
      return L"The input value is invalid.";
    case JSMessage::kParamTooLongError:
      return L"The input value is too long.";
    case JSMessage::kParseDateError:
      return L"The input value can't be parsed as a valid date/time (%s).";
    case JSMessage::kRangeBetweenError:
      return L"The input value must be greater than or equal to %s"
             L" and less than or equal to %s.";
    case JSMessage::kRangeGreaterError:
      return L"The input value must be greater than or equal to %s.";
    case JSMessage::kRangeLessError:
      return L"The input value must be less than or equal to %s.";
    case JSMessage::kNotSupportedError:
      return L"Operation not supported.";
    case JSMessage::kBusyError:
      return L"System is busy.";
    case JSMessage::kDuplicateEventError:
      return L"Duplicate formfield event found.";
    case JSMessage::kRunSuccess:
      return L"Script ran successfully.";
    case JSMessage::kSecondParamNotDateError:
      return L"The second parameter can't be converted to a Date.";
    case JSMessage::kSecondParamInvalidDateError:
      return L"The second parameter is an invalid Date!";
    case JSMessage::kGlobalNotFoundError:
      return L"Global value not found.";
    case JSMessage::kReadOnlyError:
      return L"Cannot assign to readonly property.";
    case JSMessage::kTypeError:
      return L"Incorrect parameter type.";
    case JSMessage::kValueError:
      return L"Incorrect parameter value.";
    case JSMessage::kPermissionError:
      return L"Permission denied.";
    case JSMessage::kBadObjectError:
      return L"Object no longer exists.";
    case JSMessage::kTooManyOccurances:
      return L"Too many occurances";
  }
  NOTREACHED();
  return L"";
}

WideString JSFormatErrorString(const char* class_name,
                               const char* property_name,
                               const WideString& details) {
  WideString result = WideString::FromLocal(class_name);
  if (property_name) {
    result += L".";
    result += WideString::FromLocal(property_name);
  }
  result += L": ";
  result += details;
  return result;
}
