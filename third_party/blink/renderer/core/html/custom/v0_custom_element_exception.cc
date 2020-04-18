/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer
 *    in the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of Google Inc. nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/core/html/custom/v0_custom_element_exception.h"

#include "third_party/blink/renderer/bindings/core/v8/exception_state.h"
#include "third_party/blink/renderer/core/dom/exception_code.h"

namespace blink {

String V0CustomElementException::Preamble(const AtomicString& type) {
  return "Registration failed for type '" + type + "'. ";
}

void V0CustomElementException::ThrowException(Reason reason,
                                              const AtomicString& type,
                                              ExceptionState& exception_state) {
  switch (reason) {
    case kCannotRegisterFromExtension:
      exception_state.ThrowDOMException(
          kNotSupportedError,
          Preamble(type) + "Elements cannot be registered from extensions.");
      return;

    case kConstructorPropertyNotConfigurable:
      exception_state.ThrowDOMException(
          kNotSupportedError,
          Preamble(type) +
              "Prototype constructor property is not configurable.");
      return;

    case kContextDestroyedCheckingPrototype:
      exception_state.ThrowDOMException(
          kInvalidStateError,
          Preamble(type) + "The context is no longer valid.");
      return;

    case kContextDestroyedCreatingCallbacks:
      exception_state.ThrowDOMException(
          kInvalidStateError,
          Preamble(type) + "The context is no longer valid.");
      return;

    case kContextDestroyedRegisteringDefinition:
      exception_state.ThrowDOMException(
          kInvalidStateError,
          Preamble(type) + "The context is no longer valid.");
      return;

    case kExtendsIsInvalidName:
      exception_state.ThrowDOMException(
          kNotSupportedError,
          Preamble(type) +
              "The tag name specified in 'extends' is not a valid tag name.");
      return;

    case kExtendsIsCustomElementName:
      exception_state.ThrowDOMException(kNotSupportedError,
                                        Preamble(type) +
                                            "The tag name specified in "
                                            "'extends' is a custom element "
                                            "name. Use inheritance instead.");
      return;

    case kInvalidName:
      exception_state.ThrowDOMException(
          kSyntaxError, Preamble(type) + "The type name is invalid.");
      return;

    case kPrototypeInUse:
      exception_state.ThrowDOMException(
          kNotSupportedError, Preamble(type) +
                                  "The prototype is already in-use as "
                                  "an interface prototype object.");
      return;

    case kTypeAlreadyRegistered:
      exception_state.ThrowDOMException(
          kNotSupportedError,
          Preamble(type) + "A type with that name is already registered.");
      return;
  }

  NOTREACHED();
}

}  // namespace blink
