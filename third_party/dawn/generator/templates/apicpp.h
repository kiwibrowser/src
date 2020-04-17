//* Copyright 2017 The Dawn Authors
//*
//* Licensed under the Apache License, Version 2.0 (the "License");
//* you may not use this file except in compliance with the License.
//* You may obtain a copy of the License at
//*
//*     http://www.apache.org/licenses/LICENSE-2.0
//*
//* Unless required by applicable law or agreed to in writing, software
//* distributed under the License is distributed on an "AS IS" BASIS,
//* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//* See the License for the specific language governing permissions and
//* limitations under the License.

#ifndef DAWN_DAWNCPP_H_
#define DAWN_DAWNCPP_H_

#include "dawn/dawn.h"
#include "dawn/dawn_export.h"
#include "dawn/EnumClassBitmasks.h"

namespace dawn {

    {% for type in by_category["enum"] %}
        enum class {{as_cppType(type.name)}} : uint32_t {
            {% for value in type.values %}
                {{as_cppEnum(value.name)}} = 0x{{format(value.value, "08X")}},
            {% endfor %}
        };

    {% endfor %}

    {% for type in by_category["bitmask"] %}
        enum class {{as_cppType(type.name)}} : uint32_t {
            {% for value in type.values %}
                {{as_cppEnum(value.name)}} = 0x{{format(value.value, "08X")}},
            {% endfor %}
        };

    {% endfor %}

    {% for type in by_category["bitmask"] %}
        template<>
        struct IsDawnBitmask<{{as_cppType(type.name)}}> {
            static constexpr bool enable = true;
        };

    {% endfor %}

    {% for type in by_category["natively defined"] %}
        using {{as_cppType(type.name)}} = {{as_cType(type.name)}};
    {% endfor %}

    {% for type in by_category["object"] %}
        class {{as_cppType(type.name)}};
    {% endfor %}

    {% for type in by_category["structure"] %}
        struct {{as_cppType(type.name)}};
    {% endfor %}

    template<typename Derived, typename CType>
    class ObjectBase {
        public:
            ObjectBase() = default;
            ObjectBase(CType handle): mHandle(handle) {
                if (mHandle) Derived::DawnReference(mHandle);
            }
            ~ObjectBase() {
                if (mHandle) Derived::DawnRelease(mHandle);
            }

            ObjectBase(ObjectBase const& other)
                : ObjectBase(other.Get()) {
            }
            Derived& operator=(ObjectBase const& other) {
                if (&other != this) {
                    if (mHandle) Derived::DawnRelease(mHandle);
                    mHandle = other.mHandle;
                    if (mHandle) Derived::DawnReference(mHandle);
                }

                return static_cast<Derived&>(*this);
            }

            ObjectBase(ObjectBase&& other) {
                mHandle = other.mHandle;
                other.mHandle = 0;
            }
            Derived& operator=(ObjectBase&& other) {
                if (&other != this) {
                    if (mHandle) Derived::DawnRelease(mHandle);
                    mHandle = other.mHandle;
                    other.mHandle = 0;
                }

                return static_cast<Derived&>(*this);
            }

            ObjectBase(std::nullptr_t) {}
            Derived& operator=(std::nullptr_t) {
                if (mHandle != nullptr) {
                    Derived::DawnRelease(mHandle);
                    mHandle = nullptr;
                }
                return static_cast<Derived&>(*this);
            }

            explicit operator bool() const {
                return mHandle != nullptr;
            }
            CType Get() const {
                return mHandle;
            }
            CType Release() {
                CType result = mHandle;
                mHandle = 0;
                return result;
            }
            static Derived Acquire(CType handle) {
                Derived result;
                result.mHandle = handle;
                return result;
            }

        protected:
            CType mHandle = nullptr;
    };

    {% macro render_cpp_method_declaration(type, method) %}
        {% set CppType = as_cppType(type.name) %}
        DAWN_EXPORT {{as_cppType(method.return_type.name)}} {{method.name.CamelCase()}}(
            {%- for arg in method.arguments -%}
                {%- if not loop.first %}, {% endif -%}
                {%- if arg.type.category == "object" and arg.annotation == "value" -%}
                    {{as_cppType(arg.type.name)}} const& {{as_varName(arg.name)}}
                {%- else -%}
                    {{as_annotated_cppType(arg)}}
                {%- endif -%}
            {%- endfor -%}
        ) const
    {%- endmacro %}

    {% for type in by_category["object"] %}
        {% set CppType = as_cppType(type.name) %}
        {% set CType = as_cType(type.name) %}
        class {{CppType}} : public ObjectBase<{{CppType}}, {{CType}}> {
            public:
                using ObjectBase::ObjectBase;
                using ObjectBase::operator=;

                {% for method in native_methods(type) %}
                    {{render_cpp_method_declaration(type, method)}};
                {% endfor %}

            private:
                friend ObjectBase<{{CppType}}, {{CType}}>;
                static DAWN_EXPORT void DawnReference({{CType}} handle);
                static DAWN_EXPORT void DawnRelease({{CType}} handle);
        };

    {% endfor %}

    {% for type in by_category["structure"] %}
        struct {{as_cppType(type.name)}} {
            {% if type.extensible %}
                const void* nextInChain = nullptr;
            {% endif %}
            {% for member in type.members %}
                {{as_annotated_cppType(member)}};
            {% endfor %}
        };

    {% endfor %}

} // namespace dawn

#endif // DAWN_DAWNCPP_H_
