// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_CLASS_PROPERTY_H_
#define UI_BASE_CLASS_PROPERTY_H_

#include <stdint.h>

#include <map>
#include <memory>
#include <set>

#include "base/time/time.h"
#include "ui/base/property_data.h"
#include "ui/base/ui_base_export.h"
#include "ui/base/ui_base_types.h"

// This header should be included by code that defines ClassProperties.
//
// To define a new ClassProperty:
//
//  #include "foo/foo_export.h"
//  #include "ui/base/class_property.h"
//
//  DEFINE_EXPORTED_UI_CLASS_PROPERTY_TYPE(FOO_EXPORT, MyType);
//  namespace foo {
//    // Use this to define an exported property that is primitive,
//    // or a pointer you don't want automatically deleted.
//    DEFINE_UI_CLASS_PROPERTY_KEY(MyType, kMyKey, MyDefault);
//
//    // Use this to define an exported property whose value is a heap
//    // allocated object, and has to be owned and freed by the class.
//    DEFINE_OWNED_UI_CLASS_PROPERTY_KEY(gfx::Rect, kRestoreBoundsKey, nullptr);
//
//    // Use this to define a non exported property that is primitive,
//    // or a pointer you don't want to automatically deleted, and is used
//    // only in a specific file. This will define the property in an unnamed
//    // namespace which cannot be accessed from another file.
//    DEFINE_LOCAL_UI_CLASS_PROPERTY_KEY(MyType, kMyKey, MyDefault);
//
//  }  // foo namespace
//
// To define a new type used for ClassProperty.
//
//  // outside all namespaces:
//  DEFINE_EXPORTED_UI_CLASS_PROPERTY_TYPE(FOO_EXPORT, MyType)
//
// If a property type is not exported, use
// DEFINE_UI_CLASS_PROPERTY_TYPE(MyType) which is a shorthand for
// DEFINE_EXPORTED_UI_CLASS_PROPERTY_TYPE(, MyType).
//
// If the properties are used outside the file where they are defined
// their accessor methods should also be declared in a suitable header
// using DECLARE_EXPORTED_UI_CLASS_PROPERTY_TYPE(FOO_EXPORT, MyType);

namespace ui {

// Type of a function to delete a property that this window owns.
using PropertyDeallocator = void(*)(int64_t value);

template<typename T>
struct ClassProperty {
  T default_value;
  const char* name;
  PropertyDeallocator deallocator;
};

namespace subtle {

class PropertyHelper;

}

class UI_BASE_EXPORT PropertyHandler {
 public:
  PropertyHandler();
  ~PropertyHandler();

  // Sets the |value| of the given class |property|. Setting to the default
  // value (e.g., NULL) removes the property. The caller is responsible for the
  // lifetime of any object set as a property on the class.
  template<typename T>
  void SetProperty(const ClassProperty<T>* property, T value);

  // Returns the value of the given class |property|.  Returns the
  // property-specific default value if the property was not previously set.
  template<typename T>
  T GetProperty(const ClassProperty<T>* property) const;

  // Sets the |property| to its default value. Useful for avoiding a cast when
  // setting to NULL.
  template<typename T>
  void ClearProperty(const ClassProperty<T>* property);

  // Returns the value of all properties with a non-default value.
  std::set<const void*> GetAllPropertyKeys() const;

 protected:
  friend class subtle::PropertyHelper;

  virtual void AfterPropertyChange(const void* key,
                                   int64_t old_value,
                                   std::unique_ptr<PropertyData> data) {}
  virtual std::unique_ptr<PropertyData> BeforePropertyChange(const void* key);
  void ClearProperties();

  // Called by the public {Set,Get,Clear}Property functions.
  int64_t SetPropertyInternal(const void* key,
                              const char* name,
                              PropertyDeallocator deallocator,
                              int64_t value,
                              int64_t default_value);
  int64_t GetPropertyInternal(const void* key, int64_t default_value) const;

 private:
  // Value struct to keep the name and deallocator for this property.
  // Key cannot be used for this purpose because it can be char* or
  // ClassProperty<>.
  struct Value {
    const char* name;
    int64_t value;
    PropertyDeallocator deallocator;
  };

  std::map<const void*, Value> prop_map_;
};

namespace {

// No single new-style cast works for every conversion to/from int64_t, so we
// need this helper class. A third specialization is needed for bool because
// MSVC warning C4800 (forcing value to bool) is not suppressed by an explicit
// cast (!).
template<typename T>
class ClassPropertyCaster {
 public:
  static int64_t ToInt64(T x) { return static_cast<int64_t>(x); }
  static T FromInt64(int64_t x) { return static_cast<T>(x); }
};
template<typename T>
class ClassPropertyCaster<T*> {
 public:
  static int64_t ToInt64(T* x) { return reinterpret_cast<int64_t>(x); }
  static T* FromInt64(int64_t x) { return reinterpret_cast<T*>(x); }
};
template<>
class ClassPropertyCaster<bool> {
 public:
  static int64_t ToInt64(bool x) { return static_cast<int64_t>(x); }
  static bool FromInt64(int64_t x) { return x != 0; }
};
template <>
class ClassPropertyCaster<base::TimeDelta> {
 public:
  static int64_t ToInt64(base::TimeDelta x) { return x.InMicroseconds(); }
  static base::TimeDelta FromInt64(int64_t x) {
    return base::TimeDelta::FromMicroseconds(x);
  }
};

}  // namespace

namespace subtle {

class UI_BASE_EXPORT PropertyHelper {
 public:
  template<typename T>
  static void Set(::ui::PropertyHandler* handler,
                  const ::ui::ClassProperty<T>* property, T value) {
    int64_t old = handler->SetPropertyInternal(
        property, property->name,
        value == property->default_value ? nullptr : property->deallocator,
        ClassPropertyCaster<T>::ToInt64(value),
        ClassPropertyCaster<T>::ToInt64(property->default_value));
    if (property->deallocator &&
        old != ClassPropertyCaster<T>::ToInt64(property->default_value)) {
      (*property->deallocator)(old);
    }
  }
  template<typename T>
  static T Get(const ::ui::PropertyHandler* handler,
               const ::ui::ClassProperty<T>* property) {
    return ClassPropertyCaster<T>::FromInt64(handler->GetPropertyInternal(
        property, ClassPropertyCaster<T>::ToInt64(property->default_value)));
  }
  template<typename T>
  static void Clear(::ui::PropertyHandler* handler,
                    const ::ui::ClassProperty<T>* property) {
    handler->SetProperty(property, property->default_value);
  }
};

}  // namespace subtle

}  // namespace ui

// Macros to declare the property getter/setter template functions.
#define DECLARE_EXPORTED_UI_CLASS_PROPERTY_TYPE(EXPORT, T)                   \
  namespace ui {                                                             \
  template <>                                                                \
  EXPORT void PropertyHandler::SetProperty(const ClassProperty<T>* property, \
                                           T value);                         \
  template <>                                                                \
  EXPORT T                                                                   \
  PropertyHandler::GetProperty(const ClassProperty<T>* property) const;      \
  template <>                                                                \
  EXPORT void PropertyHandler::ClearProperty(                                \
      const ClassProperty<T>* property);                                     \
  }  // namespace ui

// Macros to instantiate the property getter/setter template functions.
#define DEFINE_EXPORTED_UI_CLASS_PROPERTY_TYPE(EXPORT, T)                    \
  namespace ui {                                                             \
  template <>                                                                \
  EXPORT void PropertyHandler::SetProperty(const ClassProperty<T>* property, \
                                           T value) {                        \
    subtle::PropertyHelper::Set<T>(this, property, value);                   \
  }                                                                          \
  template <>                                                                \
  EXPORT T                                                                   \
  PropertyHandler::GetProperty(const ClassProperty<T>* property) const {     \
    return subtle::PropertyHelper::Get<T>(this, property);                   \
  }                                                                          \
  template <>                                                                \
  EXPORT void PropertyHandler::ClearProperty(                                \
      const ClassProperty<T>* property) {                                    \
    subtle::PropertyHelper::Clear<T>(this, property);                        \
  }                                                                          \
  }  // namespace ui

#define DEFINE_UI_CLASS_PROPERTY_TYPE(T) \
  DEFINE_EXPORTED_UI_CLASS_PROPERTY_TYPE(, T)

#define DEFINE_UI_CLASS_PROPERTY_KEY(TYPE, NAME, DEFAULT)                    \
  static_assert(sizeof(TYPE) <= sizeof(int64_t), "property type too large"); \
  namespace {                                                                \
  const ::ui::ClassProperty<TYPE> NAME##_Value = {DEFAULT, #NAME, nullptr};  \
  } /* namespace */                                                          \
  const ::ui::ClassProperty<TYPE>* const NAME = &NAME##_Value;

#define DEFINE_LOCAL_UI_CLASS_PROPERTY_KEY(TYPE, NAME, DEFAULT)              \
  static_assert(sizeof(TYPE) <= sizeof(int64_t), "property type too large"); \
  namespace {                                                                \
  const ::ui::ClassProperty<TYPE> NAME##_Value = {DEFAULT, #NAME, nullptr};  \
  const ::ui::ClassProperty<TYPE>* const NAME = &NAME##_Value;               \
  }  // namespace

#define DEFINE_OWNED_UI_CLASS_PROPERTY_KEY(TYPE, NAME, DEFAULT)         \
  namespace {                                                           \
  void Deallocator##NAME(int64_t p) {                                   \
    enum { type_must_be_complete = sizeof(TYPE) };                      \
    delete ::ui::ClassPropertyCaster<TYPE*>::FromInt64(p);              \
  }                                                                     \
  const ::ui::ClassProperty<TYPE*> NAME##_Value = {DEFAULT, #NAME,      \
                                                   &Deallocator##NAME}; \
  } /* namespace */                                                     \
  const ::ui::ClassProperty<TYPE*>* const NAME = &NAME##_Value;

#endif  // UI_BASE_CLASS_PROPERTY_H_
