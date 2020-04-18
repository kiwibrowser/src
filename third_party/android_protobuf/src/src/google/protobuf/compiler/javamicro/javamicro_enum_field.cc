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

#include <map>
#include <string>

#include <google/protobuf/compiler/javamicro/javamicro_enum_field.h>
#include <google/protobuf/stubs/common.h>
#include <google/protobuf/compiler/javamicro/javamicro_helpers.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/wire_format.h>
#include <google/protobuf/stubs/strutil.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace javamicro {

namespace {

// TODO(kenton):  Factor out a "SetCommonFieldVariables()" to get rid of
//   repeat code between this and the other field types.
void SetEnumVariables(const Params& params,
    const FieldDescriptor* descriptor, map<string, string>* variables) {
  (*variables)["name"] =
    UnderscoresToCamelCase(descriptor);
  (*variables)["capitalized_name"] =
    UnderscoresToCapitalizedCamelCase(descriptor);
  (*variables)["number"] = SimpleItoa(descriptor->number());
  (*variables)["type"] = "int";
  (*variables)["default"] = DefaultValue(params, descriptor);
  (*variables)["tag"] = SimpleItoa(internal::WireFormat::MakeTag(descriptor));
  (*variables)["tag_size"] = SimpleItoa(
      internal::WireFormat::TagSize(descriptor->number(), descriptor->type()));
  (*variables)["message_name"] = descriptor->containing_type()->name();
}

}  // namespace

// ===================================================================

EnumFieldGenerator::
EnumFieldGenerator(const FieldDescriptor* descriptor, const Params& params)
  : FieldGenerator(params), descriptor_(descriptor) {
  SetEnumVariables(params, descriptor, &variables_);
}

EnumFieldGenerator::~EnumFieldGenerator() {}

void EnumFieldGenerator::
GenerateMembers(io::Printer* printer) const {
  printer->Print(variables_,
    "private boolean has$capitalized_name$;\n"
    "private int $name$_ = $default$;\n"
    "public boolean has$capitalized_name$() { return has$capitalized_name$; }\n"
    "public int get$capitalized_name$() { return $name$_; }\n"
    "public $message_name$ set$capitalized_name$(int value) {\n"
    "  has$capitalized_name$ = true;\n"
    "  $name$_ = value;\n"
    "  return this;\n"
    "}\n"
    "public $message_name$ clear$capitalized_name$() {\n"
    "  has$capitalized_name$ = false;\n"
    "  $name$_ = $default$;\n"
    "  return this;\n"
    "}\n");
}

void EnumFieldGenerator::
GenerateMergingCode(io::Printer* printer) const {
  printer->Print(variables_,
    "what is other??"
    "if (other.has$capitalized_name$()) {\n"
    "  set$capitalized_name$(other.get$capitalized_name$());\n"
    "}\n");
}

void EnumFieldGenerator::
GenerateParsingCode(io::Printer* printer) const {
  printer->Print(variables_,
    "  set$capitalized_name$(input.readInt32());\n");
}

void EnumFieldGenerator::
GenerateSerializationCode(io::Printer* printer) const {
  printer->Print(variables_,
    "if (has$capitalized_name$()) {\n"
    "  output.writeInt32($number$, get$capitalized_name$());\n"
    "}\n");
}

void EnumFieldGenerator::
GenerateSerializedSizeCode(io::Printer* printer) const {
  printer->Print(variables_,
    "if (has$capitalized_name$()) {\n"
    "  size += com.google.protobuf.micro.CodedOutputStreamMicro\n"
    "    .computeInt32Size($number$, get$capitalized_name$());\n"
    "}\n");
}

string EnumFieldGenerator::GetBoxedType() const {
  return ClassName(params_, descriptor_->enum_type());
}

// ===================================================================

RepeatedEnumFieldGenerator::
RepeatedEnumFieldGenerator(const FieldDescriptor* descriptor, const Params& params)
  : FieldGenerator(params), descriptor_(descriptor) {
  SetEnumVariables(params, descriptor, &variables_);
  if (descriptor_->options().packed()) {
    GOOGLE_LOG(FATAL) << "MicroRuntime does not support packed";
  }
}

RepeatedEnumFieldGenerator::~RepeatedEnumFieldGenerator() {}

void RepeatedEnumFieldGenerator::
GenerateMembers(io::Printer* printer) const {
  if (params_.java_use_vector()) {
    printer->Print(variables_,
      "private java.util.Vector $name$_ = new java.util.Vector();\n"
      "public java.util.Vector get$capitalized_name$List() {\n"
      "  return $name$_;\n"
      "}\n"
      "public int get$capitalized_name$Count() { return $name$_.size(); }\n"
      "public int get$capitalized_name$(int index) {\n"
      "  return ((Integer)$name$_.elementAt(index)).intValue();\n"
      "}\n"
      "public $message_name$ set$capitalized_name$(int index, int value) {\n"
      "  $name$_.setElementAt(new Integer(value), index);\n"
      "  return this;\n"
      "}\n"
      "public $message_name$ add$capitalized_name$(int value) {\n"
      "  $name$_.addElement(new Integer(value));\n"
      "  return this;\n"
      "}\n"
      "public $message_name$ clear$capitalized_name$() {\n"
      "  $name$_.removeAllElements();\n"
      "  return this;\n"
      "}\n");
  } else {
    printer->Print(variables_,
      "private java.util.List<Integer> $name$_ =\n"
      "  java.util.Collections.emptyList();\n"
      "public java.util.List<Integer> get$capitalized_name$List() {\n"
      "  return $name$_;\n"   // note:  unmodifiable list
      "}\n"
      "public int get$capitalized_name$Count() { return $name$_.size(); }\n"
      "public int get$capitalized_name$(int index) {\n"
      "  return $name$_.get(index);\n"
      "}\n"
      "public $message_name$ set$capitalized_name$(int index, int value) {\n"
      "  $name$_.set(index, value);\n"
      "  return this;\n"
      "}\n"
      "public $message_name$ add$capitalized_name$(int value) {\n"
      "  if ($name$_.isEmpty()) {\n"
      "    $name$_ = new java.util.ArrayList<java.lang.Integer>();\n"
      "  }\n"
      "  $name$_.add(value);\n"
      "  return this;\n"
      "}\n"
      "public $message_name$ clear$capitalized_name$() {\n"
      "  $name$_ = java.util.Collections.emptyList();\n"
      "  return this;\n"
      "}\n");
  }
  if (descriptor_->options().packed()) {
    printer->Print(variables_,
      "private int $name$MemoizedSerializedSize;\n");
  }
}

void RepeatedEnumFieldGenerator::
GenerateMergingCode(io::Printer* printer) const {
  if (params_.java_use_vector()) {
    printer->Print(variables_,
      "if (other.$name$_.size() != 0) {\n"
      "  for (int i = 0; i < other.$name$_.size(); i++)) {\n"
      "    result.$name$_.addElement(other.$name$_.elementAt(i));\n"
      "  }\n"
      "}\n");
  } else {
    printer->Print(variables_,
      "if (!other.$name$_.isEmpty()) {\n"
      "  if (result.$name$_.isEmpty()) {\n"
      "    result.$name$_ = new java.util.ArrayList<java.lang.Integer>();\n"
      "  }\n"
      "  result.$name$_.addAll(other.$name$_);\n"
      "}\n");
  }
}

void RepeatedEnumFieldGenerator::
GenerateParsingCode(io::Printer* printer) const {
  // If packed, set up the while loop
  if (descriptor_->options().packed()) {
    printer->Print(variables_,
      "int length = input.readRawVarint32();\n"
      "int oldLimit = input.pushLimit(length);\n"
      "while(input.getBytesUntilLimit() > 0) {\n");
    printer->Indent();
  }

  // Read and store the enum
  printer->Print(variables_,
    "  add$capitalized_name$(input.readInt32());\n");

  if (descriptor_->options().packed()) {
    printer->Outdent();
    printer->Print(variables_,
      "}\n"
      "input.popLimit(oldLimit);\n");
  }
}

void RepeatedEnumFieldGenerator::
GenerateSerializationCode(io::Printer* printer) const {
  if (descriptor_->options().packed()) {
    printer->Print(variables_,
        "if (get$capitalized_name$List().size() > 0) {\n"
        "  output.writeRawVarint32($tag$);\n"
        "  output.writeRawVarint32($name$MemoizedSerializedSize);\n"
        "}\n");
    if (params_.java_use_vector()) {
      printer->Print(variables_,
        "for (int i = 0; i < get$capitalized_name$List().size(); i++) {\n"
        "  output.writeRawVarint32(get$capitalized_name$(i));\n"
        "}\n");
    } else {
      printer->Print(variables_,
        "for ($type$ element : get$capitalized_name$List()) {\n"
        "  output.writeRawVarint32(element.getNumber());\n"
        "}\n");
    }
  } else {
    if (params_.java_use_vector()) {
      printer->Print(variables_,
        "for (int i = 0; i < get$capitalized_name$List().size(); i++) {\n"
        "  output.writeInt32($number$, (int)get$capitalized_name$(i));\n"
        "}\n");
    } else {
      printer->Print(variables_,
        "for (java.lang.Integer element : get$capitalized_name$List()) {\n"
        "  output.writeInt32($number$, element);\n"
        "}\n");
    }
  }
}

void RepeatedEnumFieldGenerator::
GenerateSerializedSizeCode(io::Printer* printer) const {
  printer->Print(variables_,
    "{\n"
    "  int dataSize = 0;\n");
    printer->Indent();
  if (params_.java_use_vector()) {
    printer->Print(variables_,
      "for (int i = 0; i < get$capitalized_name$List().size(); i++) {\n"
      "  dataSize += com.google.protobuf.micro.CodedOutputStreamMicro\n"
      "    .computeInt32SizeNoTag(get$capitalized_name$(i));\n"
      "}\n");
  } else {
    printer->Print(variables_,
      "for (java.lang.Integer element : get$capitalized_name$List()) {\n"
      "  dataSize += com.google.protobuf.micro.CodedOutputStreamMicro\n"
      "    .computeInt32SizeNoTag(element);\n"
      "}\n");
  }
  printer->Print(
      "size += dataSize;\n");
  if (descriptor_->options().packed()) {
      printer->Print(variables_,
        "if (get$capitalized_name$List().size() != 0) {"
        "  size += $tag_size$;\n"
        "  size += com.google.protobuf.micro.CodedOutputStreamMicro\n"
        "    .computeRawVarint32Size(dataSize);\n"
        "}");
  } else {
    printer->Print(variables_,
        "size += $tag_size$ * get$capitalized_name$List().size();\n");
  }

  // cache the data size for packed fields.
  if (descriptor_->options().packed()) {
    printer->Print(variables_,
      "$name$MemoizedSerializedSize = dataSize;\n");
  }

  printer->Outdent();
  printer->Print("}\n");
}

string RepeatedEnumFieldGenerator::GetBoxedType() const {
  return ClassName(params_, descriptor_->enum_type());
}

}  // namespace javamicro
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
