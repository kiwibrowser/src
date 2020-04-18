/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY GOOGLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/core/dom/dom_exception.h"

#include "third_party/blink/renderer/core/dom/exception_code.h"

namespace blink {

static const struct CoreException {
  const char* const name;
  const char* const message;
  const int code;
} kCoreExceptions[] = {
    // This list must be kept in sync with the one in ExceptionCode.h
    {"IndexSizeError",
     "Index or size was negative, or greater than the allowed value.", 1},
    {"HierarchyRequestError",
     "A Node was inserted somewhere it doesn't belong.", 3},
    {"WrongDocumentError",
     "A Node was used in a different document than the one that created it "
     "(that doesn't support it).",
     4},
    {"InvalidCharacterError", "The string contains invalid characters.", 5},
    {"NoModificationAllowedError",
     "An attempt was made to modify an object where modifications are not "
     "allowed.",
     7},
    {"NotFoundError",
     "An attempt was made to reference a Node in a context where it does not "
     "exist.",
     8},
    {"NotSupportedError",
     "The implementation did not support the requested type of object or "
     "operation.",
     9},
    {"InUseAttributeError",
     "An attempt was made to add an attribute that is already in use "
     "elsewhere.",
     10},
    {"InvalidStateError",
     "An attempt was made to use an object that is not, or is no longer, "
     "usable.",
     11},
    {"SyntaxError", "An invalid or illegal string was specified.", 12},
    {"InvalidModificationError", "The object can not be modified in this way.",
     13},
    {"NamespaceError",
     "An attempt was made to create or change an object in a way which is "
     "incorrect with regard to namespaces.",
     14},
    {"InvalidAccessError",
     "A parameter or an operation was not supported by the underlying object.",
     15},
    {"TypeMismatchError",
     "The type of an object was incompatible with the expected type of the "
     "parameter associated to the object.",
     17},
    {"SecurityError",
     "An attempt was made to break through the security policy of the user "
     "agent.",
     18},
    {"NetworkError", "A network error occurred.", 19},
    {"AbortError", "The user aborted a request.", 20},
    {"URLMismatchError",
     "A worker global scope represented an absolute URL that is not equal to "
     "the resulting absolute URL.",
     21},
    {"QuotaExceededError",
     "An attempt was made to add something to storage that exceeded the quota.",
     22},
    {"TimeoutError", "A timeout occurred.", 23},
    {"InvalidNodeTypeError",
     "The supplied node is invalid or has an invalid ancestor for this "
     "operation.",
     24},
    {"DataCloneError", "An object could not be cloned.", 25},

    // Indexed DB
    {"UnknownError", "An unknown error occurred within Indexed Database.", 0},
    {"ConstraintError",
     "A mutation operation in the transaction failed because a constraint was "
     "not satisfied.",
     0},
    {"DataError", "The data provided does not meet requirements.", 0},
    {"TransactionInactiveError",
     "A request was placed against a transaction which is either currently not "
     "active, or which is finished.",
     0},
    {"ReadOnlyError",
     "A write operation was attempted in a read-only transaction.", 0},
    {"VersionError",
     "An attempt was made to open a database using a lower version than the "
     "existing version.",
     0},

    // File system
    {"NotReadableError",
     "The requested file could not be read, typically due to permission "
     "problems that have occurred after a reference to a file was acquired.",
     0},
    {"EncodingError",
     "A URI supplied to the API was malformed, or the resulting Data URL has "
     "exceeded the URL length limitations for Data URLs.",
     0},
    {"PathExistsError",
     "An attempt was made to create a file or directory where an element "
     "already exists.",
     0},

    // SQL
    {"DatabaseError",
     "The operation failed for some reason related to the database.", 0},

    // Web Crypto
    {"OperationError", "The operation failed for an operation-specific reason",
     0},

    // Push API
    {"PermissionDeniedError", "User or security policy denied the request.", 0},

    // Used by HTML and Media Session API.
    {"NotAllowedError",
     "The request is not allowed by the user agent or the platform in the "
     "current context.",
     0},

    // Pointer Event
    {"InvalidPointerId", "PointerId was invalid.", 0},
};

static const CoreException* GetErrorEntry(ExceptionCode ec) {
  size_t table_size = arraysize(kCoreExceptions);
  size_t table_index = ec - kIndexSizeError;

  return table_index < table_size ? &kCoreExceptions[table_index] : nullptr;
}

static int GetErrorCode(const String& name) {
  for (const CoreException& entry : kCoreExceptions) {
    if (entry.name == name)
      return entry.code;
  }
  return 0;
}

DOMException::DOMException(unsigned short code,
                           const String& name,
                           const String& sanitized_message,
                           const String& unsanitized_message) {
  DCHECK(name);
  code_ = code;
  name_ = name;
  sanitized_message_ = sanitized_message;
  unsanitized_message_ = unsanitized_message;
}

DOMException* DOMException::Create(ExceptionCode ec,
                                   const String& sanitized_message,
                                   const String& unsanitized_message) {
  const CoreException* entry = GetErrorEntry(ec);
  DCHECK(entry);
  return new DOMException(
      entry->code, entry->name ? entry->name : "Error",
      sanitized_message.IsNull() ? String(entry->message) : sanitized_message,
      unsanitized_message);
}

DOMException* DOMException::Create(const String& message, const String& name) {
  return new DOMException(GetErrorCode(name), name, message, message);
}

String DOMException::ToStringForConsole() const {
  return name() + ": " + MessageForConsole();
}

String DOMException::GetErrorName(ExceptionCode ec) {
  const CoreException* entry = GetErrorEntry(ec);
  DCHECK(entry);
  if (!entry)
    return "UnknownError";

  return entry->name;
}

String DOMException::GetErrorMessage(ExceptionCode ec) {
  const CoreException* entry = GetErrorEntry(ec);
  DCHECK(entry);
  if (!entry)
    return "Unknown error.";

  return entry->message;
}

}  // namespace blink
