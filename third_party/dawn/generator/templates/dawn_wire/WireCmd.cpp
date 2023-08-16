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

#include "dawn_wire/WireCmd_autogen.h"

#include "common/Assert.h"
#include "dawn_wire/Wire.h"

#include <algorithm>
#include <cstring>
#include <limits>

//* Helper macros so that the main [de]serialization functions can be written in a generic manner.

//* Outputs an rvalue that's the number of elements a pointer member points to.
{% macro member_length(member, record_accessor) -%}
    {%- if member.length == "constant" -%}
        {{member.constant_length}}
    {%- else -%}
        {{record_accessor}}{{as_varName(member.length.name)}}
    {%- endif -%}
{%- endmacro %}

//* Outputs the type that will be used on the wire for the member
{% macro member_transfer_type(member) -%}
    {%- if member.type.category == "object" -%}
        ObjectId
    {%- elif member.type.category == "structure" -%}
        {{as_cType(member.type.name)}}Transfer
    {%- elif member.type.category == "bitmask" -%}
        {{as_cType(member.type.name)}}Flags
    {%- else -%}
        {{as_cType(member.type.name)}}
    {%- endif -%}
{%- endmacro %}

//* Outputs the size of one element of the type that will be used on the wire for the member
{% macro member_transfer_sizeof(member) -%}
    sizeof({{member_transfer_type(member)}})
{%- endmacro %}

//* Outputs the serialization code to put `in` in `out`
{% macro serialize_member(member, in, out) %}
    {%- if member.type.category == "object" -%}
        {%- set Optional = "Optional" if member.optional else "" -%}
        {{out}} = provider.Get{{Optional}}Id({{in}});
    {% elif member.type.category == "structure"%}
        {%- set Provider = ", provider" if member.type.may_have_dawn_object else "" -%}
        {% if member.annotation == "const*const*" %}
            {{as_cType(member.type.name)}}Serialize(*{{in}}, &{{out}}, buffer{{Provider}});
        {% else %}
            {{as_cType(member.type.name)}}Serialize({{in}}, &{{out}}, buffer{{Provider}});
        {% endif %}
    {%- else -%}
        {{out}} = {{in}};
    {%- endif -%}
{% endmacro %}

//* Outputs the deserialization code to put `in` in `out`
{% macro deserialize_member(member, in, out) %}
    {%- if member.type.category == "object" -%}
        {%- set Optional = "Optional" if member.optional else "" -%}
        DESERIALIZE_TRY(resolver.Get{{Optional}}FromId({{in}}, &{{out}}));
    {%- elif member.type.category == "structure" -%}
        DESERIALIZE_TRY({{as_cType(member.type.name)}}Deserialize(&{{out}}, &{{in}}, buffer, size, allocator
            {%- if member.type.may_have_dawn_object -%}
                , resolver
            {%- endif -%}
        ));
    {%- else -%}
        {{out}} = {{in}};
    {%- endif -%}
{% endmacro %}

namespace {

    struct WGPUChainedStructTransfer {
        WGPUSType sType;
        bool hasNext;
    };

}  // anonymous namespace

//* The main [de]serialization macro
//* Methods are very similar to structures that have one member corresponding to each arguments.
//* This macro takes advantage of the similarity to output [de]serialization code for a record
//* that is either a structure or a method, with some special cases for each.
{% macro write_record_serialization_helpers(record, name, members, is_cmd=False, is_return_command=False) %}
    {% set Return = "Return" if is_return_command else "" %}
    {% set Cmd = "Cmd" if is_cmd else "" %}

    //* Structure for the wire format of each of the records. Members that are values
    //* are embedded directly in the structure. Other members are assumed to be in the
    //* memory directly following the structure in the buffer.
    struct {{Return}}{{name}}Transfer {
        static_assert({{[is_cmd, record.extensible, record.chained].count(True)}} <= 1,
                      "Record must be at most one of is_cmd, extensible, and chained.");
        {% if is_cmd %}
            //* Start the transfer structure with the command ID, so that casting to WireCmd gives the ID.
            {{Return}}WireCmd commandId;
        {% elif record.extensible %}
            bool hasNextInChain;
        {% elif record.chained %}
            WGPUChainedStructTransfer chain;
        {% endif %}

        //* Value types are directly in the command, objects being replaced with their IDs.
        {% for member in members if member.annotation == "value" %}
            {{member_transfer_type(member)}} {{as_varName(member.name)}};
        {% endfor %}

        //* const char* have their length embedded directly in the command.
        {% for member in members if member.length == "strlen" %}
            size_t {{as_varName(member.name)}}Strlen;
        {% endfor %}

        {% for member in members if member.optional and member.annotation != "value" and member.type.category != "object" %}
            bool has_{{as_varName(member.name)}};
        {% endfor %}
    };

    {% if record.chained %}
        static_assert(offsetof({{Return}}{{name}}Transfer, chain) == 0, "");
    {% endif %}

    //* Returns the required transfer size for `record` in addition to the transfer structure.
    DAWN_DECLARE_UNUSED size_t {{Return}}{{name}}GetExtraRequiredSize(const {{Return}}{{name}}{{Cmd}}& record) {
        DAWN_UNUSED(record);

        size_t result = 0;

        //* Gather how much space will be needed for the extension chain.
        {% if record.extensible %}
            if (record.nextInChain != nullptr) {
                result += GetChainedStructExtraRequiredSize(record.nextInChain);
            }
        {% endif %}

        //* Special handling of const char* that have their length embedded directly in the command
        {% for member in members if member.length == "strlen" %}
            {% set memberName = as_varName(member.name) %}

            {% if member.optional %}
                bool has_{{memberName}} = record.{{memberName}} != nullptr;
                if (has_{{memberName}})
            {% endif %}
            {
            result += std::strlen(record.{{memberName}});
            }
        {% endfor %}

        //* Gather how much space will be needed for pointer members.
        {% for member in members if member.length != "strlen" and not member.skip_serialize %}
            {% if member.type.category != "object" and member.optional %}
                if (record.{{as_varName(member.name)}} != nullptr)
            {% endif %}
            {
                {% if member.annotation != "value" %}
                    size_t memberLength = {{member_length(member, "record.")}};
                    result += memberLength * {{member_transfer_sizeof(member)}};
                    //* Structures might contain more pointers so we need to add their extra size as well.
                    {% if member.type.category == "structure" %}
                        for (size_t i = 0; i < memberLength; ++i) {
                            {% if member.annotation == "const*const*" %}
                                result += {{as_cType(member.type.name)}}GetExtraRequiredSize(*record.{{as_varName(member.name)}}[i]);
                            {% else %}
                                {{assert(member.annotation == "const*")}}
                                result += {{as_cType(member.type.name)}}GetExtraRequiredSize(record.{{as_varName(member.name)}}[i]);
                            {% endif %}
                        }
                    {% endif %}
                {% elif member.type.category == "structure" %}
                    result += {{as_cType(member.type.name)}}GetExtraRequiredSize(record.{{as_varName(member.name)}});
                {% endif %}
            }
        {% endfor %}

        return result;
    }
    // GetExtraRequiredSize isn't used for structures that are value members of other structures
    // because we assume they cannot contain pointers themselves.
    DAWN_UNUSED_FUNC({{Return}}{{name}}GetExtraRequiredSize);

    //* Serializes `record` into `transfer`, using `buffer` to get more space for pointed-to data
    //* and `provider` to serialize objects.
    DAWN_DECLARE_UNUSED void {{Return}}{{name}}Serialize(const {{Return}}{{name}}{{Cmd}}& record, {{Return}}{{name}}Transfer* transfer,
                           char** buffer
        {%- if record.may_have_dawn_object -%}
            , const ObjectIdProvider& provider
        {%- endif -%}
    ) {
        DAWN_UNUSED(buffer);

        //* Handle special transfer members of methods.
        {% if is_cmd %}
            transfer->commandId = {{Return}}WireCmd::{{name}};
        {% endif %}

        //* Value types are directly in the transfer record, objects being replaced with their IDs.
        {% for member in members if member.annotation == "value" %}
            {% set memberName = as_varName(member.name) %}
            {{serialize_member(member, "record." + memberName, "transfer->" + memberName)}}
        {% endfor %}

        {% if record.extensible %}
            if (record.nextInChain != nullptr) {
                transfer->hasNextInChain = true;
                SerializeChainedStruct(record.nextInChain, buffer, provider);
            } else {
                transfer->hasNextInChain = false;
            }
        {% endif %}

        {% if record.chained %}
            //* Should be set by the root descriptor's call to SerializeChainedStruct.
            ASSERT(transfer->chain.sType == {{as_cEnum(types["s type"].name, record.name)}});
            ASSERT(transfer->chain.hasNext == (record.chain.next != nullptr));
        {% endif %}

        //* Special handling of const char* that have their length embedded directly in the command
        {% for member in members if member.length == "strlen" %}
            {% set memberName = as_varName(member.name) %}

            {% if member.optional %}
                bool has_{{memberName}} = record.{{memberName}} != nullptr;
                transfer->has_{{memberName}} = has_{{memberName}};
                if (has_{{memberName}})
            {% endif %}
            {
            transfer->{{memberName}}Strlen = std::strlen(record.{{memberName}});

            memcpy(*buffer, record.{{memberName}}, transfer->{{memberName}}Strlen);
            *buffer += transfer->{{memberName}}Strlen;
            }
        {% endfor %}

        //* Allocate space and write the non-value arguments in it.
        {% for member in members if member.annotation != "value" and member.length != "strlen" and not member.skip_serialize %}
            {% set memberName = as_varName(member.name) %}

            {% if member.type.category != "object" and member.optional %}
                bool has_{{memberName}} = record.{{memberName}} != nullptr;
                transfer->has_{{memberName}} = has_{{memberName}};
                if (has_{{memberName}})
            {% endif %}
            {
                size_t memberLength = {{member_length(member, "record.")}};
                auto memberBuffer = reinterpret_cast<{{member_transfer_type(member)}}*>(*buffer);
                *buffer += memberLength * {{member_transfer_sizeof(member)}};

                for (size_t i = 0; i < memberLength; ++i) {
                    {{serialize_member(member, "record." + memberName + "[i]", "memberBuffer[i]" )}}
                }
            }
        {% endfor %}
    }
    DAWN_UNUSED_FUNC({{Return}}{{name}}Serialize);

    //* Deserializes `transfer` into `record` getting more serialized data from `buffer` and `size`
    //* if needed, using `allocator` to store pointed-to values and `resolver` to translate object
    //* Ids to actual objects.
    DAWN_DECLARE_UNUSED DeserializeResult {{Return}}{{name}}Deserialize({{Return}}{{name}}{{Cmd}}* record, const volatile {{Return}}{{name}}Transfer* transfer,
                                          const volatile char** buffer, size_t* size, DeserializeAllocator* allocator
        {%- if record.may_have_dawn_object -%}
            , const ObjectIdResolver& resolver
        {%- endif -%}
    ) {
        DAWN_UNUSED(allocator);
        DAWN_UNUSED(buffer);
        DAWN_UNUSED(size);

        {% if is_cmd %}
            ASSERT(transfer->commandId == {{Return}}WireCmd::{{name}});
        {% endif %}

        {% if record.derived_method %}
            record->selfId = transfer->self;
        {% endif %}

        //* Value types are directly in the transfer record, objects being replaced with their IDs.
        {% for member in members if member.annotation == "value" %}
            {% set memberName = as_varName(member.name) %}
            {{deserialize_member(member, "transfer->" + memberName, "record->" + memberName)}}
        {% endfor %}

        {% if record.extensible %}
            record->nextInChain = nullptr;
            if (transfer->hasNextInChain) {
                DESERIALIZE_TRY(DeserializeChainedStruct(&record->nextInChain, buffer, size, allocator, resolver));
            }
        {% endif %}

        {% if record.chained %}
            //* Should be set by the root descriptor's call to DeserializeChainedStruct.
            //* Don't check |record->chain.next| matches because it is not set until the
            //* next iteration inside DeserializeChainedStruct.
            ASSERT(record->chain.sType == {{as_cEnum(types["s type"].name, record.name)}});
            ASSERT(record->chain.next == nullptr);
        {% endif %}

        //* Special handling of const char* that have their length embedded directly in the command
        {% for member in members if member.length == "strlen" %}
            {% set memberName = as_varName(member.name) %}

            {% if member.optional %}
                bool has_{{memberName}} = transfer->has_{{memberName}};
                record->{{memberName}} = nullptr;
                if (has_{{memberName}})
            {% endif %}
            {
                size_t stringLength = transfer->{{memberName}}Strlen;
                const volatile char* stringInBuffer = nullptr;
                DESERIALIZE_TRY(GetPtrFromBuffer(buffer, size, stringLength, &stringInBuffer));

                char* copiedString = nullptr;
                DESERIALIZE_TRY(GetSpace(allocator, stringLength + 1, &copiedString));
                std::copy(stringInBuffer, stringInBuffer + stringLength, copiedString);
                copiedString[stringLength] = '\0';
                record->{{memberName}} = copiedString;
            }
        {% endfor %}

        //* Get extra buffer data, and copy pointed to values in extra allocated space.
        {% for member in members if member.annotation != "value" and member.length != "strlen" %}
            {% set memberName = as_varName(member.name) %}

            {% if member.type.category != "object" and member.optional %}
                bool has_{{memberName}} = transfer->has_{{memberName}};
                record->{{memberName}} = nullptr;
                if (has_{{memberName}})
            {% endif %}
            {
                size_t memberLength = {{member_length(member, "record->")}};
                auto memberBuffer = reinterpret_cast<const volatile {{member_transfer_type(member)}}*>(buffer);
                DESERIALIZE_TRY(GetPtrFromBuffer(buffer, size, memberLength, &memberBuffer));

                {{as_cType(member.type.name)}}* copiedMembers = nullptr;
                DESERIALIZE_TRY(GetSpace(allocator, memberLength, &copiedMembers));
                {% if member.annotation == "const*const*" %}
                    {{as_cType(member.type.name)}}** pointerArray = nullptr;
                    DESERIALIZE_TRY(GetSpace(allocator, memberLength, &pointerArray));
                    for (size_t i = 0; i < memberLength; ++i) {
                        pointerArray[i] = &copiedMembers[i];
                    }
                    record->{{memberName}} = pointerArray;
                {% else %}
                    record->{{memberName}} = copiedMembers;
                {% endif %}

                for (size_t i = 0; i < memberLength; ++i) {
                    {{deserialize_member(member, "memberBuffer[i]", "copiedMembers[i]")}}
                }
            }
        {% endfor %}

        return DeserializeResult::Success;
    }
    DAWN_UNUSED_FUNC({{Return}}{{name}}Deserialize);
{% endmacro %}

{% macro write_command_serialization_methods(command, is_return) %}
    {% set Return = "Return" if is_return else "" %}
    {% set Name = Return + command.name.CamelCase() %}
    {% set Cmd = Name + "Cmd" %}

    size_t {{Cmd}}::GetRequiredSize() const {
        size_t size = sizeof({{Name}}Transfer) + {{Name}}GetExtraRequiredSize(*this);
        return size;
    }

    void {{Cmd}}::Serialize(char* buffer
        {%- if not is_return -%}
            , const ObjectIdProvider& objectIdProvider
        {%- endif -%}
    ) const {
        auto transfer = reinterpret_cast<{{Name}}Transfer*>(buffer);
        buffer += sizeof({{Name}}Transfer);

        {{Name}}Serialize(*this, transfer, &buffer
            {%- if command.may_have_dawn_object -%}
                , objectIdProvider
            {%- endif -%}
        );
    }

    DeserializeResult {{Cmd}}::Deserialize(const volatile char** buffer, size_t* size, DeserializeAllocator* allocator
        {%- if command.may_have_dawn_object -%}
            , const ObjectIdResolver& resolver
        {%- endif -%}
    ) {
        const volatile {{Name}}Transfer* transfer = nullptr;
        DESERIALIZE_TRY(GetPtrFromBuffer(buffer, size, 1, &transfer));

        return {{Name}}Deserialize(this, transfer, buffer, size, allocator
            {%- if command.may_have_dawn_object -%}
                , resolver
            {%- endif -%}
        );
    }
{% endmacro %}

namespace dawn_wire {

    // Macro to simplify error handling, similar to DAWN_TRY but for DeserializeResult.
#define DESERIALIZE_TRY(EXPR) \
    do { \
        DeserializeResult exprResult = EXPR; \
        if (exprResult != DeserializeResult::Success) { \
            return exprResult; \
        } \
    } while (0)

    ObjectHandle::ObjectHandle() = default;
    ObjectHandle::ObjectHandle(ObjectId id, ObjectGeneration generation)
        : id(id), generation(generation) {
    }

    ObjectHandle::ObjectHandle(const volatile ObjectHandle& rhs)
        : id(rhs.id), generation(rhs.generation) {
    }
    ObjectHandle& ObjectHandle::operator=(const volatile ObjectHandle& rhs) {
        id = rhs.id;
        generation = rhs.generation;
        return *this;
    }

    ObjectHandle& ObjectHandle::AssignFrom(const ObjectHandle& rhs) {
        id = rhs.id;
        generation = rhs.generation;
        return *this;
    }
    ObjectHandle& ObjectHandle::AssignFrom(const volatile ObjectHandle& rhs) {
        id = rhs.id;
        generation = rhs.generation;
        return *this;
    }

    namespace {

        // Consumes from (buffer, size) enough memory to contain T[count] and return it in data.
        // Returns FatalError if not enough memory was available
        template <typename T>
        DeserializeResult GetPtrFromBuffer(const volatile char** buffer, size_t* size, size_t count, const volatile T** data) {
            constexpr size_t kMaxCountWithoutOverflows = std::numeric_limits<size_t>::max() / sizeof(T);
            if (count > kMaxCountWithoutOverflows) {
                return DeserializeResult::FatalError;
            }

            size_t totalSize = sizeof(T) * count;
            if (totalSize > *size) {
                return DeserializeResult::FatalError;
            }

            *data = reinterpret_cast<const volatile T*>(*buffer);
            *buffer += totalSize;
            *size -= totalSize;

            return DeserializeResult::Success;
        }

        // Allocates enough space from allocator to countain T[count] and return it in out.
        // Return FatalError if the allocator couldn't allocate the memory.
        template <typename T>
        DeserializeResult GetSpace(DeserializeAllocator* allocator, size_t count, T** out) {
            constexpr size_t kMaxCountWithoutOverflows = std::numeric_limits<size_t>::max() / sizeof(T);
            if (count > kMaxCountWithoutOverflows) {
                return DeserializeResult::FatalError;
            }

            size_t totalSize = sizeof(T) * count;
            *out = static_cast<T*>(allocator->GetSpace(totalSize));
            if (*out == nullptr) {
                return DeserializeResult::FatalError;
            }

            return DeserializeResult::Success;
        }

        size_t GetChainedStructExtraRequiredSize(const WGPUChainedStruct* chainedStruct);
        void SerializeChainedStruct(WGPUChainedStruct const* chainedStruct,
                                    char** buffer,
                                    const ObjectIdProvider& provider);
        DeserializeResult DeserializeChainedStruct(const WGPUChainedStruct** outChainNext,
                                                   const volatile char** buffer,
                                                   size_t* size,
                                                   DeserializeAllocator* allocator,
                                                   const ObjectIdResolver& resolver);

        //* Output structure [de]serialization first because it is used by commands.
        {% for type in by_category["structure"] %}
            {% set name = as_cType(type.name) %}
            {% if type.name.CamelCase() not in client_side_structures %}
                {{write_record_serialization_helpers(type, name, type.members,
                  is_cmd=False)}}
            {% endif %}
        {% endfor %}

        size_t GetChainedStructExtraRequiredSize(const WGPUChainedStruct* chainedStruct) {
            ASSERT(chainedStruct != nullptr);
            size_t result = 0;
            while (chainedStruct != nullptr) {
                switch (chainedStruct->sType) {
                    {% for sType in types["s type"].values if sType.valid and sType.name.CamelCase() not in client_side_structures %}
                        case {{as_cEnum(types["s type"].name, sType.name)}}: {
                            const auto& typedStruct = *reinterpret_cast<{{as_cType(sType.name)}} const *>(chainedStruct);
                            result += sizeof({{as_cType(sType.name)}}Transfer);
                            result += {{as_cType(sType.name)}}GetExtraRequiredSize(typedStruct);
                            chainedStruct = typedStruct.chain.next;
                            break;
                        }
                    {% endfor %}
                    default:
                        // Invalid enum. Reserve space just for the transfer header (sType and hasNext).
                        // Stop iterating because this is an error.
                        // TODO(crbug.com/dawn/369): Unknown sTypes are silently discarded.
                        ASSERT(chainedStruct->sType == WGPUSType_Invalid);
                        result += sizeof(WGPUChainedStructTransfer);
                        return result;
                }
            }
            return result;
        }

        void SerializeChainedStruct(WGPUChainedStruct const* chainedStruct,
                                    char** buffer,
                                    const ObjectIdProvider& provider) {
            ASSERT(chainedStruct != nullptr);
            ASSERT(buffer != nullptr);
            do {
                switch (chainedStruct->sType) {
                    {% for sType in types["s type"].values if sType.valid and sType.name.CamelCase() not in client_side_structures %}
                        {% set CType = as_cType(sType.name) %}
                        case {{as_cEnum(types["s type"].name, sType.name)}}: {

                            auto* transfer = reinterpret_cast<{{CType}}Transfer*>(*buffer);
                            transfer->chain.sType = chainedStruct->sType;
                            transfer->chain.hasNext = chainedStruct->next != nullptr;

                            *buffer += sizeof({{CType}}Transfer);
                            {{CType}}Serialize(*reinterpret_cast<{{CType}} const*>(chainedStruct), transfer, buffer
                                {%- if types[sType.name.get()].may_have_dawn_object -%}
                                , provider
                                {%- endif -%}
                            );

                            chainedStruct = chainedStruct->next;
                        } break;
                    {% endfor %}
                    default: {
                        // Invalid enum. Serialize just the transfer header with Invalid as the sType.
                        // TODO(crbug.com/dawn/369): Unknown sTypes are silently discarded.
                        ASSERT(chainedStruct->sType == WGPUSType_Invalid);
                        WGPUChainedStructTransfer* transfer = reinterpret_cast<WGPUChainedStructTransfer*>(*buffer);
                        transfer->sType = WGPUSType_Invalid;
                        transfer->hasNext = false;

                        *buffer += sizeof(WGPUChainedStructTransfer);
                        return;
                    }
                }
            } while (chainedStruct != nullptr);
        }

        DeserializeResult DeserializeChainedStruct(const WGPUChainedStruct** outChainNext,
                                                   const volatile char** buffer,
                                                   size_t* size,
                                                   DeserializeAllocator* allocator,
                                                   const ObjectIdResolver& resolver) {
            bool hasNext;
            do {
                if (*size < sizeof(WGPUChainedStructTransfer)) {
                    return DeserializeResult::FatalError;
                }
                WGPUSType sType =
                    reinterpret_cast<const volatile WGPUChainedStructTransfer*>(*buffer)->sType;
                switch (sType) {
                    {% for sType in types["s type"].values if sType.valid and sType.name.CamelCase() not in client_side_structures %}
                        {% set CType = as_cType(sType.name) %}
                        case {{as_cEnum(types["s type"].name, sType.name)}}: {
                            const volatile {{CType}}Transfer* transfer = nullptr;
                            DESERIALIZE_TRY(GetPtrFromBuffer(buffer, size, 1, &transfer));

                            {{CType}}* outStruct = nullptr;
                            DESERIALIZE_TRY(GetSpace(allocator, sizeof({{CType}}), &outStruct));
                            outStruct->chain.sType = sType;
                            outStruct->chain.next = nullptr;

                            *outChainNext = &outStruct->chain;
                            outChainNext = &outStruct->chain.next;

                            DESERIALIZE_TRY({{CType}}Deserialize(outStruct, transfer, buffer, size, allocator
                                {%- if types[sType.name.get()].may_have_dawn_object -%}
                                    , resolver
                                {%- endif -%}
                            ));

                            hasNext = transfer->chain.hasNext;
                        } break;
                    {% endfor %}
                    default:
                        return DeserializeResult::FatalError;
                }
            } while (hasNext);

            return DeserializeResult::Success;
        }

        //* Output [de]serialization helpers for commands
        {% for command in cmd_records["command"] %}
            {% set name = command.name.CamelCase() %}
            {{write_record_serialization_helpers(command, name, command.members,
              is_cmd=True)}}
        {% endfor %}

        //* Output [de]serialization helpers for return commands
        {% for command in cmd_records["return command"] %}
            {% set name = command.name.CamelCase() %}
            {{write_record_serialization_helpers(command, name, command.members,
              is_cmd=True, is_return_command=True)}}
        {% endfor %}
    }  // anonymous namespace

    {% for command in cmd_records["command"] %}
        {{ write_command_serialization_methods(command, False) }}
    {% endfor %}

    {% for command in cmd_records["return command"] %}
        {{ write_command_serialization_methods(command, True) }}
    {% endfor %}

        // Implementations of serialization/deserialization of WPGUDeviceProperties.
        size_t SerializedWGPUDevicePropertiesSize(const WGPUDeviceProperties* deviceProperties) {
            return sizeof(WGPUDeviceProperties) +
                   WGPUDevicePropertiesGetExtraRequiredSize(*deviceProperties);
        }

        void SerializeWGPUDeviceProperties(const WGPUDeviceProperties* deviceProperties,
                                           char* serializeBuffer) {
            size_t devicePropertiesSize = SerializedWGPUDevicePropertiesSize(deviceProperties);
            WGPUDevicePropertiesTransfer* transfer =
                reinterpret_cast<WGPUDevicePropertiesTransfer*>(serializeBuffer);
            serializeBuffer += devicePropertiesSize;

            WGPUDevicePropertiesSerialize(*deviceProperties, transfer, &serializeBuffer);
        }

        bool DeserializeWGPUDeviceProperties(WGPUDeviceProperties* deviceProperties,
                                             const volatile char* deserializeBuffer) {
            size_t devicePropertiesSize = SerializedWGPUDevicePropertiesSize(deviceProperties);
            const volatile WGPUDevicePropertiesTransfer* transfer = nullptr;
            if (GetPtrFromBuffer(&deserializeBuffer, &devicePropertiesSize, 1, &transfer) !=
                DeserializeResult::Success) {
                return false;
            }

            return WGPUDevicePropertiesDeserialize(deviceProperties, transfer, &deserializeBuffer,
                                                   &devicePropertiesSize,
                                                   nullptr) == DeserializeResult::Success;
        }

}  // namespace dawn_wire
