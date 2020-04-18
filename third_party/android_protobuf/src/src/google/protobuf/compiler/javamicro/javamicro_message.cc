// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// http://code.google.com/p/protobuf/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include <algorithm>
#include <google/protobuf/stubs/hash.h>
#include <google/protobuf/compiler/javamicro/javamicro_message.h>
#include <google/protobuf/compiler/javamicro/javamicro_enum.h>
#include <google/protobuf/compiler/javamicro/javamicro_helpers.h>
#include <google/protobuf/stubs/strutil.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/wire_format.h>
#include <google/protobuf/descriptor.pb.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace javamicro {

using internal::WireFormat;
using internal::WireFormatLite;

namespace {

void PrintFieldComment(io::Printer* printer, const FieldDescriptor* field) {
  // Print the field's proto-syntax definition as a comment.  We don't want to
  // print group bodies so we cut off after the first line.
  string def = field->DebugString();
  printer->Print("// $def$\n",
    "def", def.substr(0, def.find_first_of('\n')));
}

struct FieldOrderingByNumber {
  inline bool operator()(const FieldDescriptor* a,
                         const FieldDescriptor* b) const {
    return a->number() < b->number();
  }
};

// Sort the fields of the given Descriptor by number into a new[]'d array
// and return it.
const FieldDescriptor** SortFieldsByNumber(const Descriptor* descriptor) {
  const FieldDescriptor** fields =
    new const FieldDescriptor*[descriptor->field_count()];
  for (int i = 0; i < descriptor->field_count(); i++) {
    fields[i] = descriptor->field(i);
  }
  sort(fields, fields + descriptor->field_count(),
       FieldOrderingByNumber());
  return fields;
}

// Get an identifier that uniquely identifies this type within the file.
// This is used to declare static variables related to this type at the
// outermost file scope.
string UniqueFileScopeIdentifier(const Descriptor* descriptor) {
  return "static_" + StringReplace(descriptor->full_name(), ".", "_", true);
}

// Returns true if the message type has any required fields.  If it doesn't,
// we can optimize out calls to its isInitialized() method.
//
// already_seen is used to avoid checking the same type multiple times
// (and also to protect against recursion).
static bool HasRequiredFields(
    const Descriptor* type,
    hash_set<const Descriptor*>* already_seen) {
  if (already_seen->count(type) > 0) {
    // The type is already in cache.  This means that either:
    // a. The type has no required fields.
    // b. We are in the midst of checking if the type has required fields,
    //    somewhere up the stack.  In this case, we know that if the type
    //    has any required fields, they'll be found when we return to it,
    //    and the whole call to HasRequiredFields() will return true.
    //    Therefore, we don't have to check if this type has required fields
    //    here.
    return false;
  }
  already_seen->insert(type);

  // If the type has extensions, an extension with message type could contain
  // required fields, so we have to be conservative and assume such an
  // extension exists.
  if (type->extension_range_count() > 0) return true;

  for (int i = 0; i < type->field_count(); i++) {
    const FieldDescriptor* field = type->field(i);
    if (field->is_required()) {
      return true;
    }
    if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
      if (HasRequiredFields(field->message_type(), already_seen)) {
        return true;
      }
    }
  }

  return false;
}

static bool HasRequiredFields(const Descriptor* type) {
  hash_set<const Descriptor*> already_seen;
  return HasRequiredFields(type, &already_seen);
}

}  // namespace

// ===================================================================

MessageGenerator::MessageGenerator(const Descriptor* descriptor, const Params& params)
  : params_(params),
    descriptor_(descriptor),
    field_generators_(descriptor, params) {
}

MessageGenerator::~MessageGenerator() {}

void MessageGenerator::GenerateStaticVariables(io::Printer* printer) {
  // Generate static members for all nested types.
  for (int i = 0; i < descriptor_->nested_type_count(); i++) {
    // TODO(kenton):  Reuse MessageGenerator objects?
    MessageGenerator(descriptor_->nested_type(i), params_)
      .GenerateStaticVariables(printer);
  }
}

void MessageGenerator::GenerateStaticVariableInitializers(
    io::Printer* printer) {
  // Generate static member initializers for all nested types.
  for (int i = 0; i < descriptor_->nested_type_count(); i++) {
    // TODO(kenton):  Reuse MessageGenerator objects?
    MessageGenerator(descriptor_->nested_type(i), params_)
      .GenerateStaticVariableInitializers(printer);
  }

  if (descriptor_->extension_count() != 0) {
    GOOGLE_LOG(FATAL) << "Extensions not supported in MICRO_RUNTIME\n";
  }
}

void MessageGenerator::Generate(io::Printer* printer) {
  const string& file_name = descriptor_->file()->name();
  bool is_own_file =
    params_.java_multiple_files(file_name)
      && descriptor_->containing_type() == NULL;

  if ((descriptor_->extension_count() != 0)
      || (descriptor_->extension_range_count() != 0)) {
    GOOGLE_LOG(FATAL) << "Extensions not supported in MICRO_RUNTIME\n";
  }

  // Note: Fields (which will be emitted in the loop, below) may have the same names as fields in
  // the inner or outer class.  This causes Java warnings, but is not fatal, so we suppress those
  // warnings here in the class declaration.
  printer->Print(
    "@SuppressWarnings(\"hiding\")\n"
    "public $modifiers$ final class $classname$ extends\n"
    "    com.google.protobuf.micro.MessageMicro {\n",
    "modifiers", is_own_file ? "" : "static",
    "classname", descriptor_->name());
  printer->Indent();
  printer->Print(
    "public $classname$() {}\n"
    "\n",
    "classname", descriptor_->name());

  // Nested types and extensions
  for (int i = 0; i < descriptor_->enum_type_count(); i++) {
    EnumGenerator(descriptor_->enum_type(i), params_).Generate(printer);
  }

  for (int i = 0; i < descriptor_->nested_type_count(); i++) {
    MessageGenerator(descriptor_->nested_type(i), params_).Generate(printer);
  }

  // Fields
  for (int i = 0; i < descriptor_->field_count(); i++) {
    PrintFieldComment(printer, descriptor_->field(i));
    printer->Print("public static final int $constant_name$ = $number$;\n",
      "constant_name", FieldConstantName(descriptor_->field(i)),
      "number", SimpleItoa(descriptor_->field(i)->number()));
    field_generators_.get(descriptor_->field(i)).GenerateMembers(printer);
    printer->Print("\n");
  }

  GenerateClear(printer);
  GenerateIsInitialized(printer);
  GenerateMessageSerializationMethods(printer);
  GenerateMergeFromMethods(printer);
  GenerateParseFromMethods(printer);

  printer->Outdent();
  printer->Print("}\n\n");
}

// ===================================================================

void MessageGenerator::
GenerateMessageSerializationMethods(io::Printer* printer) {
  scoped_array<const FieldDescriptor*> sorted_fields(
    SortFieldsByNumber(descriptor_));

  if (descriptor_->extension_range_count() != 0) {
    GOOGLE_LOG(FATAL) << "Extensions not supported in MICRO_RUNTIME\n";
  }

  // writeTo only throws an exception if it contains one or more fields to write
  if (descriptor_->field_count() > 0) {
    printer->Print(
      "@Override\n"
      "public void writeTo(com.google.protobuf.micro.CodedOutputStreamMicro output)\n"
      "                    throws java.io.IOException {\n");
  } else {
    printer->Print(
      "@Override\n"
      "public void writeTo(com.google.protobuf.micro.CodedOutputStreamMicro output) {\n");
  }
  printer->Indent();

  // Output the fields in sorted order
  for (int i = 0; i < descriptor_->field_count(); i++) {
      GenerateSerializeOneField(printer, sorted_fields[i]);
  }

  printer->Outdent();
  printer->Print(
    "}\n"
    "\n"
    "private int cachedSize = -1;\n"
    "@Override\n"
    "public int getCachedSize() {\n"
    "  if (cachedSize < 0) {\n"
    "    // getSerializedSize sets cachedSize\n"
    "    getSerializedSize();\n"
    "  }\n"
    "  return cachedSize;\n"
    "}\n"
    "\n"
    "@Override\n"
    "public int getSerializedSize() {\n"
    "  int size = 0;\n");
  printer->Indent();

  for (int i = 0; i < descriptor_->field_count(); i++) {
    field_generators_.get(sorted_fields[i]).GenerateSerializedSizeCode(printer);
  }

  printer->Outdent();
  printer->Print(
    "  cachedSize = size;\n"
    "  return size;\n"
    "}\n"
    "\n");
}

void MessageGenerator::GenerateMergeFromMethods(io::Printer* printer) {
  scoped_array<const FieldDescriptor*> sorted_fields(
    SortFieldsByNumber(descriptor_));

  if (params_.java_use_vector()) {
    printer->Print(
      "@Override\n"
      "public com.google.protobuf.micro.MessageMicro mergeFrom(\n"
      "    com.google.protobuf.micro.CodedInputStreamMicro input)\n"
      "    throws java.io.IOException {\n",
      "classname", descriptor_->name());
  } else {
    printer->Print(
      "@Override\n"
      "public $classname$ mergeFrom(\n"
      "    com.google.protobuf.micro.CodedInputStreamMicro input)\n"
      "    throws java.io.IOException {\n",
      "classname", descriptor_->name());
  }
  printer->Indent();

  printer->Print(
    "while (true) {\n");
  printer->Indent();

  printer->Print(
    "int tag = input.readTag();\n"
    "switch (tag) {\n");
  printer->Indent();

  printer->Print(
    "case 0:\n"          // zero signals EOF / limit reached
    "  return this;\n"
    "default: {\n"
    "  if (!parseUnknownField(input, tag)) {\n"
    "    return this;\n"   // it's an endgroup tag
    "  }\n"
    "  break;\n"
    "}\n");

  for (int i = 0; i < descriptor_->field_count(); i++) {
    const FieldDescriptor* field = sorted_fields[i];
    uint32 tag = WireFormatLite::MakeTag(field->number(),
      WireFormat::WireTypeForField(field));

    printer->Print(
      "case $tag$: {\n",
      "tag", SimpleItoa(tag));
    printer->Indent();

    field_generators_.get(field).GenerateParsingCode(printer);

    printer->Outdent();
    printer->Print(
      "  break;\n"
      "}\n");
  }

  printer->Outdent();
  printer->Outdent();
  printer->Outdent();
  printer->Print(
    "    }\n"     // switch (tag)
    "  }\n"       // while (true)
    "}\n"
    "\n");
}

void MessageGenerator::
GenerateParseFromMethods(io::Printer* printer) {
  // Note:  These are separate from GenerateMessageSerializationMethods()
  //   because they need to be generated even for messages that are optimized
  //   for code size.
  printer->Print(
    "public static $classname$ parseFrom(byte[] data)\n"
    "    throws com.google.protobuf.micro.InvalidProtocolBufferMicroException {\n"
    "  return ($classname$) (new $classname$().mergeFrom(data));\n"
    "}\n"
    "\n"
    "public static $classname$ parseFrom(\n"
    "        com.google.protobuf.micro.CodedInputStreamMicro input)\n"
    "    throws java.io.IOException {\n"
    "  return new $classname$().mergeFrom(input);\n"
    "}\n"
    "\n",
    "classname", descriptor_->name());
}

void MessageGenerator::GenerateSerializeOneField(
    io::Printer* printer, const FieldDescriptor* field) {
  field_generators_.get(field).GenerateSerializationCode(printer);
}

void MessageGenerator::GenerateClear(io::Printer* printer) {
  printer->Print(
    "public final $classname$ clear() {\n",
    "classname", descriptor_->name());
  printer->Indent();

  // Call clear for all of the fields.
  for (int i = 0; i < descriptor_->field_count(); i++) {
    const FieldDescriptor* field = descriptor_->field(i);

    printer->Print(
      "clear$name$();\n",
      "name", UnderscoresToCapitalizedCamelCase(field));
  }

  printer->Outdent();
  printer->Print(
    "  cachedSize = -1;\n"
    "  return this;\n"
    "}\n"
    "\n");
}


void MessageGenerator::GenerateIsInitialized(io::Printer* printer) {
  printer->Print(
    "public final boolean isInitialized() {\n");
  printer->Indent();

  // Check that all required fields in this message are set.
  // TODO(kenton):  We can optimize this when we switch to putting all the
  //   "has" fields into a single bitfield.
  for (int i = 0; i < descriptor_->field_count(); i++) {
    const FieldDescriptor* field = descriptor_->field(i);

    if (field->is_required()) {
      printer->Print(
        "if (!has$name$) return false;\n",
        "name", UnderscoresToCapitalizedCamelCase(field));
    }
  }

  // Now check that all embedded messages are initialized.
  for (int i = 0; i < descriptor_->field_count(); i++) {
    const FieldDescriptor* field = descriptor_->field(i);
    if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE &&
        HasRequiredFields(field->message_type())) {
      switch (field->label()) {
        case FieldDescriptor::LABEL_REQUIRED:
          printer->Print(
            "if (!get$name$().isInitialized()) return false;\n",
            "type", ClassName(params_, field->message_type()),
            "name", UnderscoresToCapitalizedCamelCase(field));
          break;
        case FieldDescriptor::LABEL_OPTIONAL:
          printer->Print(
            "if (has$name$()) {\n"
            "  if (!get$name$().isInitialized()) return false;\n"
            "}\n",
            "type", ClassName(params_, field->message_type()),
            "name", UnderscoresToCapitalizedCamelCase(field));
          break;
        case FieldDescriptor::LABEL_REPEATED:
          if (params_.java_use_vector()) {
            printer->Print(
              "for (int i = 0; i < get$name$List().size(); i++) {\n"
              "  if (get$name$(i).isInitialized()) return false;\n"
              "}\n",
              "type", ClassName(params_, field->message_type()),
              "name", UnderscoresToCapitalizedCamelCase(field));
          } else {
            printer->Print(
              "for ($type$ element : get$name$List()) {\n"
              "  if (!element.isInitialized()) return false;\n"
              "}\n",
              "type", ClassName(params_, field->message_type()),
              "name", UnderscoresToCapitalizedCamelCase(field));
          }
          break;
      }
    }
  }

  if (descriptor_->extension_range_count() > 0) {
    printer->Print(
      "if (!extensionsAreInitialized()) return false;\n");
  }

  printer->Outdent();
  printer->Print(
    "  return true;\n"
    "}\n"
    "\n");
}

// ===================================================================

}  // namespace javamicro
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
