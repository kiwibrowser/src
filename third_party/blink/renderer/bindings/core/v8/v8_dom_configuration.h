/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
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

#ifndef THIRD_PARTY_BLINK_RENDERER_BINDINGS_CORE_V8_V8_DOM_CONFIGURATION_H_
#define THIRD_PARTY_BLINK_RENDERER_BINDINGS_CORE_V8_V8_DOM_CONFIGURATION_H_

#include "third_party/blink/renderer/bindings/core/v8/generated_code_helper.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_binding_for_core.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/platform/bindings/v8_dom_wrapper.h"
#include "third_party/blink/renderer/platform/bindings/v8_private_property.h"
#include "v8/include/v8.h"

namespace blink {

class CORE_EXPORT V8DOMConfiguration final {
  STATIC_ONLY(V8DOMConfiguration);

 public:
  // The following Configuration structs and install methods are used for
  // setting multiple properties on ObjectTemplate / FunctionTemplate, used
  // from the generated bindings initialization (ConfigureXXXTemplate).
  // This greatly reduces the binary size by moving from code driven setup to
  // data table driven setup.

  // Bitflags to show where the member will be defined.
  enum PropertyLocationConfiguration : unsigned {
    kOnInstance = 1 << 0,
    kOnPrototype = 1 << 1,
    kOnInterface = 1 << 2,
  };

  // TODO(dcheng): Make these enum classes.
  enum HolderCheckConfiguration : unsigned {
    kCheckHolder,
    kDoNotCheckHolder,
  };

  enum AccessCheckConfiguration : unsigned {
    kCheckAccess,
    kDoNotCheckAccess,
  };

  enum SideEffectConfiguration : unsigned {
    kHasSideEffect,
    kHasNoSideEffect,
  };

  // Bit field to select which worlds the member will be defined in.
  enum WorldConfiguration : unsigned {
    kMainWorld = 1 << 0,
    kNonMainWorlds = 1 << 1,
    kAllWorlds = kMainWorld | kNonMainWorlds,
  };

  // AttributeConfiguration translates into calls to SetNativeDataProperty() on
  // either of instance or prototype object (or their object template).
  struct AttributeConfiguration {
    AttributeConfiguration& operator=(const AttributeConfiguration&) = delete;
    DISALLOW_NEW();
    const char* const name;
    v8::AccessorNameGetterCallback getter;
    v8::AccessorNameSetterCallback setter;

    // v8::PropertyAttribute
    unsigned attribute : 8;
    // PropertyLocationConfiguration
    unsigned property_location_configuration : 3;
    // HolderCheckConfiguration
    unsigned holder_check_configuration : 1;
    // SideEffectConfiguration
    unsigned getter_side_effect_type : 1;
    // WorldConfiguration
    unsigned world_configuration : 2;
  };

  static void InstallAttributes(
      v8::Isolate*,
      const DOMWrapperWorld&,
      v8::Local<v8::ObjectTemplate> instance_template,
      v8::Local<v8::ObjectTemplate> prototype_template,
      const AttributeConfiguration*,
      size_t attribute_count);
  static void InstallAttribute(v8::Isolate*,
                               const DOMWrapperWorld&,
                               v8::Local<v8::ObjectTemplate> instance_template,
                               v8::Local<v8::ObjectTemplate> prototype_template,
                               const AttributeConfiguration&);

  // If an empty handle is passed as |instance| or |prototype|, that object is
  // ignored and no properties are installed on that object.
  static void InstallAttributes(v8::Isolate*,
                                const DOMWrapperWorld&,
                                v8::Local<v8::Object> instance,
                                v8::Local<v8::Object> prototype,
                                const AttributeConfiguration*,
                                size_t attribute_count);
  static void InstallAttribute(v8::Isolate*,
                               const DOMWrapperWorld&,
                               v8::Local<v8::Object> instance,
                               v8::Local<v8::Object> prototype,
                               const AttributeConfiguration&);

  // A lazy data attribute is like one of the attributes added via
  // installAttributes(), however, V8 will attempt to replace it with the value
  // returned by the getter callback, turning it into a real data value.
  //
  // This also means that the AttributeConfiguration must not specify a setter,
  // nor any non-default attributes.
  static void InstallLazyDataAttributes(
      v8::Isolate*,
      const DOMWrapperWorld&,
      v8::Local<v8::ObjectTemplate> instance_template,
      v8::Local<v8::ObjectTemplate> prototype_template,
      const AttributeConfiguration*,
      size_t attribute_count);

  // AccessorConfiguration translates into calls to SetAccessorProperty() on
  // either of instance, prototype, or interface object (or their object
  // template).
  struct AccessorConfiguration {
    AccessorConfiguration& operator=(const AccessorConfiguration&) = delete;
    DISALLOW_NEW();
    const char* const name;
    v8::FunctionCallback getter;
    v8::FunctionCallback setter;
    // V8PrivateProperty::CachedAccessorSymbol
    unsigned cached_property_key : 1;
    // v8::PropertyAttribute
    unsigned attribute : 8;
    // PropertyLocationConfiguration
    unsigned property_location_configuration : 3;
    // HolderCheckConfiguration
    unsigned holder_check_configuration : 1;
    // SideEffectConfiguration
    unsigned getter_side_effect_type : 1;
    // WorldConfiguration
    unsigned world_configuration : 2;
  };

  static void InstallAccessors(
      v8::Isolate*,
      const DOMWrapperWorld&,
      v8::Local<v8::ObjectTemplate> instance_template,
      v8::Local<v8::ObjectTemplate> prototype_template,
      v8::Local<v8::FunctionTemplate> interface_template,
      v8::Local<v8::Signature>,
      const AccessorConfiguration*,
      size_t accessor_count);
  static void InstallAccessor(
      v8::Isolate*,
      const DOMWrapperWorld&,
      v8::Local<v8::ObjectTemplate> instance_template,
      v8::Local<v8::ObjectTemplate> prototype_template,
      v8::Local<v8::FunctionTemplate> interface_template,
      v8::Local<v8::Signature>,
      const AccessorConfiguration&);

  // If an empty handle is passed as |instance|, |prototype|, or |interface|,
  // then that object is ignored and no properties are installed on that object.
  static void InstallAccessors(v8::Isolate*,
                               const DOMWrapperWorld&,
                               v8::Local<v8::Object> instance,
                               v8::Local<v8::Object> prototype,
                               v8::Local<v8::Function> interface,
                               v8::Local<v8::Signature>,
                               const AccessorConfiguration*,
                               size_t accessor_count);
  static void InstallAccessor(v8::Isolate*,
                              const DOMWrapperWorld&,
                              v8::Local<v8::Object> instance,
                              v8::Local<v8::Object> prototype,
                              v8::Local<v8::Function> interface,
                              v8::Local<v8::Signature>,
                              const AccessorConfiguration&);

  enum ConstantType {
    kConstantTypeShort,
    kConstantTypeLong,
    kConstantTypeUnsignedShort,
    kConstantTypeUnsignedLong,
    kConstantTypeFloat,
    kConstantTypeDouble
  };

  // ConstantConfiguration translates into calls to Set() for setting up an
  // object's constants. It sets the constant on both the FunctionTemplate and
  // the ObjectTemplate. PropertyAttributes is always ReadOnly.
  struct ConstantConfiguration {
    constexpr ConstantConfiguration(const char* name,
                                    ConstantType type,
                                    int value)
        : name(name), type(type), ivalue(value) {}
    constexpr ConstantConfiguration(const char* name,
                                    ConstantType type,
                                    double value)
        : name(name), type(type), dvalue(value) {}
    ConstantConfiguration& operator=(const ConstantConfiguration&) = delete;
    DISALLOW_NEW();
    const char* const name;
    ConstantType type;
    union {
      int ivalue;
      double dvalue;
    };
  };

  // Constant installation
  //
  // installConstants and installConstant are used for simple constants. They
  // install constants using v8::Template::Set(), which results in a property
  // that is much faster to access from scripts.
  // installConstantWithGetter is used when some C++ code needs to be executed
  // when the constant is accessed, e.g. to handle deprecation or measuring
  // usage. The property appears the same to scripts, but is slower to access.
  static void InstallConstants(
      v8::Isolate*,
      v8::Local<v8::FunctionTemplate> interface_template,
      v8::Local<v8::ObjectTemplate> prototype_template,
      const ConstantConfiguration*,
      size_t constant_count);
  static void InstallConstant(
      v8::Isolate*,
      v8::Local<v8::FunctionTemplate> interface_template,
      v8::Local<v8::ObjectTemplate> prototype_template,
      const ConstantConfiguration&);

  static void InstallConstant(v8::Isolate*,
                              v8::Local<v8::Function> interface,
                              v8::Local<v8::Object> prototype,
                              const ConstantConfiguration&);

  static void InstallConstantWithGetter(
      v8::Isolate*,
      v8::Local<v8::FunctionTemplate> interface_template,
      v8::Local<v8::ObjectTemplate> prototype_template,
      const char* name,
      v8::AccessorNameGetterCallback);

  // MethodConfiguration translates into calls to Set() for setting up an
  // object's callbacks. It sets a method on instance, prototype or
  // interface object (or their object tepmplate).
  struct MethodConfiguration {
    MethodConfiguration& operator=(const MethodConfiguration&) = delete;
    DISALLOW_NEW();
    v8::Local<v8::Name> MethodName(v8::Isolate* isolate) const {
      return V8AtomicString(isolate, name);
    }

    const char* const name;
    v8::FunctionCallback callback;
    int length;
    // v8::PropertyAttribute
    unsigned attribute : 8;
    // PropertyLocationConfiguration
    unsigned property_location_configuration : 3;
    // HolderCheckConfiguration
    unsigned holder_check_configuration : 1;
    // AccessCheckConfiguration
    unsigned access_check_configuration : 1;
    // SideEffectConfiguration
    unsigned side_effect_type : 1;
    // WorldConfiguration
    unsigned world_configuration : 2;
  };

  struct SymbolKeyedMethodConfiguration {
    SymbolKeyedMethodConfiguration& operator=(
        const SymbolKeyedMethodConfiguration&) = delete;
    DISALLOW_NEW();
    v8::Local<v8::Name> MethodName(v8::Isolate* isolate) const {
      return get_symbol(isolate);
    }

    v8::Local<v8::Symbol> (*get_symbol)(v8::Isolate*);
    const char* const symbol_alias;
    v8::FunctionCallback callback;
    // SymbolKeyedMethodConfiguration doesn't support per-world bindings.
    int length;
    // v8::PropertyAttribute
    unsigned attribute : 8;
    // PropertyLocationConfiguration
    unsigned property_location_configuration : 3;
    // HolderCheckConfiguration
    unsigned holder_check_configuration : 1;
    // AccessCheckConfiguration
    unsigned access_check_configuration : 1;
    // SideEffectConfiguration
    unsigned side_effect_type : 1;
  };

  static void InstallMethods(v8::Isolate*,
                             const DOMWrapperWorld&,
                             v8::Local<v8::ObjectTemplate> instance_template,
                             v8::Local<v8::ObjectTemplate> prototype_template,
                             v8::Local<v8::FunctionTemplate> interface_template,
                             v8::Local<v8::Signature>,
                             const MethodConfiguration*,
                             size_t method_count);
  static void InstallMethod(v8::Isolate*,
                            const DOMWrapperWorld&,
                            v8::Local<v8::ObjectTemplate> instance_template,
                            v8::Local<v8::ObjectTemplate> prototype_template,
                            v8::Local<v8::FunctionTemplate> interface_template,
                            v8::Local<v8::Signature>,
                            const MethodConfiguration&);
  static void InstallMethod(v8::Isolate*,
                            const DOMWrapperWorld&,
                            v8::Local<v8::ObjectTemplate>,
                            v8::Local<v8::Signature>,
                            const SymbolKeyedMethodConfiguration&);

  // If an empty handle is passed as |instance|, |prototype|, or |interface|,
  // then that object is ignored and no properties are installed on that object.
  static void InstallMethod(v8::Isolate*,
                            const DOMWrapperWorld&,
                            v8::Local<v8::Object> instance,
                            v8::Local<v8::Object> prototype,
                            v8::Local<v8::Function> interface,
                            v8::Local<v8::Signature>,
                            const MethodConfiguration&);

  static void InitializeDOMInterfaceTemplate(
      v8::Isolate*,
      v8::Local<v8::FunctionTemplate> interface_template,
      const char* interface_name,
      v8::Local<v8::FunctionTemplate> parent_interface_template,
      size_t v8_internal_field_count);

  static v8::Local<v8::FunctionTemplate> DomClassTemplate(
      v8::Isolate*,
      const DOMWrapperWorld&,
      WrapperTypeInfo*,
      InstallTemplateFunction);

  // Sets the class string of platform objects, interface prototype objects,
  // etc.  See also http://heycam.github.io/webidl/#dfn-class-string
  static void SetClassString(v8::Isolate*,
                             v8::Local<v8::ObjectTemplate>,
                             const char* class_string);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_BINDINGS_CORE_V8_V8_DOM_CONFIGURATION_H_
