/*
 *  Copyright (C) 2006, 2007 Apple Inc. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 * USA
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_DOM_EXCEPTION_CODE_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_DOM_EXCEPTION_CODE_H_

namespace blink {

// The DOM standards use unsigned short for exception codes.
// In our DOM implementation we use int instead, and use different
// numerical ranges for different types of DOM exception, so that
// an exception of any type can be expressed with a single integer.
typedef int ExceptionCode;

// This list must be in sync with |coreExceptions| in DOMExceptions.cpp.
// Some of these are considered historical since they have been
// changed or removed from the specifications.
enum {
  kIndexSizeError = 1,
  kHierarchyRequestError,
  kWrongDocumentError,
  kInvalidCharacterError,
  kNoModificationAllowedError,
  kNotFoundError,
  kNotSupportedError,
  kInUseAttributeError,  // Historical. Only used in setAttributeNode etc which
                         // have been removed from the DOM specs.

  // Introduced in DOM Level 2:
  kInvalidStateError,
  kSyntaxError,
  kInvalidModificationError,
  kNamespaceError,
  kInvalidAccessError,

  // Introduced in DOM Level 3:
  kTypeMismatchError,  // Historical; use TypeError instead

  // XMLHttpRequest extension:
  kSecurityError,

  // Others introduced in HTML5:
  kNetworkError,
  kAbortError,
  kURLMismatchError,
  kQuotaExceededError,
  kTimeoutError,
  kInvalidNodeTypeError,
  kDataCloneError,

  // The operation failed for an unknown transient reason (e.g. out of memory).
  // Note: Rethrowed V8 exception will also have this code.
  kUnknownError,

  // These are IDB-specific.
  kConstraintError,
  kDataError,
  kTransactionInactiveError,
  kReadOnlyError,
  kVersionError,

  // File system
  kNotReadableError,
  kEncodingError,
  kPathExistsError,

  // SQL
  kSQLDatabaseError,  // Naming conflict with DatabaseError class.

  // Web Crypto
  kOperationError,

  // Push API
  kPermissionDeniedError,

  kNotAllowedError,

  // Pointer Events
  kInvalidPointerId,
};

enum V8ErrorType {
  kV8Error = 1000,
  kV8TypeError,
  kV8RangeError,
  kV8SyntaxError,
  kV8ReferenceError,
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_DOM_EXCEPTION_CODE_H_
