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

#include <google/protobuf/compiler/javamicro/javamicro_primitive_field.h>
#include <google/protobuf/stubs/common.h>
#include <google/protobuf/compiler/javamicro/javamicro_helpers.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/wire_format.h>
#include <google/protobuf/stubs/strutil.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace javamicro {

using internal::WireFormat;
using internal::WireFormatLite;

namespace {

const char* PrimitiveTypeName(JavaType type) {
  switch (type) {
    case JAVATYPE_INT    : return "int";
    case JAVATYPE_LONG   : return "long";
    case JAVATYPE_FLOAT  : return "float";
    case JAVATYPE_DOUBLE : return "double";
    case JAVATYPE_BOOLEAN: return "boolean";
    case JAVATYPE_STRING : return "java.lang.String";
    case JAVATYPE_BYTES  : return "com.google.protobuf.micro.ByteStringMicro";
    case JAVATYPE_ENUM   : return NULL;
    case JAVATYPE_MESSAGE: return NULL;

    // No default because we want the compiler to complain if any new
    // JavaTypes are added.
  }

  GOOGLE_LOG(FATAL) << "Can't get here.";
  return NULL;
}

bool IsReferenceType(JavaType type) {
  switch (type) {
    case JAVATYPE_INT    : return false;
    case JAVATYPE_LONG   : return false;
    case JAVATYPE_FLOAT  : return false;
    case JAVATYPE_DOUBLE : return false;
    case JAVATYPE_BOOLEAN: return false;
    case JAVATYPE_STRING : return true;
    case JAVATYPE_BYTES  : return true;
    case JAVATYPE_ENUM   : return false;
    case JAVATYPE_MESSAGE: return true;

    // No default because we want the compiler to complain if any new
    // JavaTypes are added.
  }

  GOOGLE_LOG(FATAL) << "Can't get here.";
  return false;
}

const char* GetCapitalizedType(const FieldDescriptor* field) {
  switch (field->type()) {
    case FieldDescriptor::TYPE_INT32   : return "Int32"   ;
    case FieldDescriptor::TYPE_UINT32  : return "UInt32"  ;
    case FieldDescriptor::TYPE_SINT32  : return "SInt32"  ;
    case FieldDescriptor::TYPE_FIXED32 : return "Fixed32" ;
    case FieldDescriptor::TYPE_SFIXED32: return "SFixed32";
    case FieldDescriptor::TYPE_INT64   : return "Int64"   ;
    case FieldDescriptor::TYPE_UINT64  : return "UInt64"  ;
    case FieldDescriptor::TYPE_SINT64  : return "SInt64"  ;
    case FieldDescriptor::TYPE_FIXED64 : return "Fixed64" ;
    case FieldDescriptor::TYPE_SFIXED64: return "SFixed64";
    case FieldDescriptor::TYPE_FLOAT   : return "Float"   ;
    case FieldDescriptor::TYPE_DOUBLE  : return "Double"  ;
    case FieldDescriptor::TYPE_BOOL    : return "Bool"    ;
    case FieldDescriptor::TYPE_STRING  : return "String"  ;
    case FieldDescriptor::TYPE_BYTES   : return "Bytes"   ;
    case FieldDescriptor::TYPE_ENUM    : return "Enum"    ;
    case FieldDescriptor::TYPE_GROUP   : return "Group"   ;
    case FieldDescriptor::TYPE_MESSAGE : return "Message" ;

    // No default because we want the compiler to complain if any new
    // types are added.
  }

  GOOGLE_LOG(FATAL) << "Can't get here.";
  return NULL;
}

// For encodings with fixed sizes, returns that size in bytes.  Otherwise
// returns -1.
int FixedSize(FieldDescriptor::Type type) {
  switch (type) {
    case FieldDescriptor::TYPE_INT32   : return -1;
    case FieldDescriptor::TYPE_INT64   : return -1;
    case FieldDescriptor::TYPE_UINT32  : return -1;
    case FieldDescriptor::TYPE_UINT64  : return -1;
    case FieldDescriptor::TYPE_SINT32  : return -1;
    case FieldDescriptor::TYPE_SINT64  : return -1;
    case FieldDescriptor::TYPE_FIXED32 : return WireFormatLite::kFixed32Size;
    case FieldDescriptor::TYPE_FIXED64 : return WireFormatLite::kFixed64Size;
    case FieldDescriptor::TYPE_SFIXED32: return WireFormatLite::kSFixed32Size;
    case FieldDescriptor::TYPE_SFIXED64: return WireFormatLite::kSFixed64Size;
    case FieldDescriptor::TYPE_FLOAT   : return WireFormatLite::kFloatSize;
    case FieldDescriptor::TYPE_DOUBLE  : return WireFormatLite::kDoubleSize;

    case FieldDescriptor::TYPE_BOOL    : return WireFormatLite::kBoolSize;
    case FieldDescriptor::TYPE_ENUM    : return -1;

    case FieldDescriptor::TYPE_STRING  : return -1;
    case FieldDescriptor::TYPE_BYTES   : return -1;
    case FieldDescriptor::TYPE_GROUP   : return -1;
    case FieldDescriptor::TYPE_MESSAGE : return -1;

    // No default because we want the compiler to complain if any new
    // types are added.
  }
  GOOGLE_LOG(FATAL) << "Can't get here.";
  return -1;
}

// Return true if the type is a that has variable length
// for instance String's.
bool IsVariableLenType(JavaType type) {
  switch (type) {
    case JAVATYPE_INT    : return false;
    case JAVATYPE_LONG   : return false;
    case JAVATYPE_FLOAT  : return false;
    case JAVATYPE_DOUBLE : return false;
    case JAVATYPE_BOOLEAN: return false;
    case JAVATYPE_STRING : return true;
    case JAVATYPE_BYTES  : return true;
    case JAVATYPE_ENUM   : return false;
    case JAVATYPE_MESSAGE: return true;

    // No default because we want the compiler to complain if any new
    // JavaTypes are added.
  }

  GOOGLE_LOG(FATAL) << "Can't get here.";
  return false;
}

bool IsFastStringHandling(const FieldDescriptor* descriptor,
      const Params params) {
  return ((params.optimization() == JAVAMICRO_OPT_SPEED)
      && (GetJavaType(descriptor) == JAVATYPE_STRING));
}

void SetPrimitiveVariables(const FieldDescriptor* descriptor, const Params params,
                           map<string, string>* variables) {
  (*variables)["name"] =
    UnderscoresToCamelCase(descriptor);
  (*variables)["capitalized_name"] =
    UnderscoresToCapitalizedCamelCase(descriptor);
  (*variables)["number"] = SimpleItoa(descriptor->number());
  (*variables)["type"] = PrimitiveTypeName(GetJavaType(descriptor));
  (*variables)["default"] = DefaultValue(params, descriptor);
  (*variables)["boxed_type"] = BoxedPrimitiveTypeName(GetJavaType(descriptor));
  (*variables)["capitalized_type"] = GetCapitalizedType(descriptor);
  (*variables)["tag"] = SimpleItoa(WireFormat::MakeTag(descriptor));
  (*variables)["tag_size"] = SimpleItoa(
      WireFormat::TagSize(descriptor->number(), descriptor->type()));
  if (IsReferenceType(GetJavaType(descriptor))) {
    (*variables)["null_check"] =
        "  if (value == null) {\n"
        "    throw new NullPointerException();\n"
        "  }\n";
  } else {
    (*variables)["null_check"] = "";
  }
  int fixed_size = FixedSize(descriptor->type());
  if (fixed_size != -1) {
    (*variables)["fixed_size"] = SimpleItoa(fixed_size);
  }
  (*variables)["message_name"] = descriptor->containing_type()->name();
}
}  // namespace

// ===================================================================

PrimitiveFieldGenerator::
PrimitiveFieldGenerator(const FieldDescriptor* descriptor, const Params& params)
  : FieldGenerator(params), descriptor_(descriptor) {
  SetPrimitiveVariables(descriptor, params, &variables_);
}

PrimitiveFieldGenerator::~PrimitiveFieldGenerator() {}

void PrimitiveFieldGenerator::
GenerateMembers(io::Printer* printer) const {
  printer->Print(variables_,
    "private boolean has$capitalized_name$;\n"
    "private $type$ $name$_ = $default$;\n"
    "public $type$ get$capitalized_name$() { return $name$_; }\n"
    "public boolean has$capitalized_name$() { return has$capitalized_name$; }\n");
  if (IsFastStringHandling(descriptor_, params_)) {
    printer->Print(variables_,
      "private byte [] $name$Utf8_ = null;\n"
      "public $message_name$ set$capitalized_name$($type$ value) {\n"
      "  has$capitalized_name$ = true;\n"
      "  $name$_ = value;\n"
      "  $name$Utf8_ = null;\n"
      "  return this;\n"
      "}\n"
      "public $message_name$ clear$capitalized_name$() {\n"
      "  has$capitalized_name$ = false;\n"
      "  $name$_ = $default$;\n"
      "  $name$Utf8_ = null;\n"
      "  return this;\n"
      "}\n");
  } else {
    if (IsVariableLenType(GetJavaType(descriptor_))) {
      printer->Print(variables_,
        "public $message_name$ set$capitalized_name$($type$ value) {\n"
        "  has$capitalized_name$ = true;\n"
        "  $name$_ = value;\n"
        "  return this;\n"
        "}\n"
        "public $message_name$ clear$capitalized_name$() {\n"
        "  has$capitalized_name$ = false;\n"
        "  $name$_ = $default$;\n"
        "  return this;\n"
        "}\n");
    } else {
      printer->Print(variables_,
        "public $message_name$ set$capitalized_name$($type$ value) {\n"
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
  }
}

void PrimitiveFieldGenerator::
GenerateMergingCode(io::Printer* printer) const {
  printer->Print(variables_,
    "if (other.has$capitalized_name$()) {\n"
    "  set$capitalized_name$(other.get$capitalized_name$());\n"
    "}\n");
}

void PrimitiveFieldGenerator::
GenerateParsingCode(io::Printer* printer) const {
  printer->Print(variables_,
    "set$capitalized_name$(input.read$capitalized_type$());\n");
}

void PrimitiveFieldGenerator::
GenerateSerializationCode(io::Printer* printer) const {
  if (IsFastStringHandling(descriptor_, params_)) {
    printer->Print(variables_,
      "if (has$capitalized_name$()) {\n"
      "  output.writeByteArray($number$, $name$Utf8_);\n"
      "}\n");
  } else {
    printer->Print(variables_,
      "if (has$capitalized_name$()) {\n"
      "  output.write$capitalized_type$($number$, get$capitalized_name$());\n"
      "}\n");
  }
}

void PrimitiveFieldGenerator::
GenerateSerializedSizeCode(io::Printer* printer) const {
  if (IsFastStringHandling(descriptor_, params_)) {
    printer->Print(variables_,
      "if (has$capitalized_name$()) {\n"
      "  try {\n"
      "    $name$Utf8_ = $name$_.getBytes(\"UTF-8\");\n"
      "  } catch (java.io.UnsupportedEncodingException e) {\n"
      "    throw new RuntimeException(\"UTF-8 not supported.\");\n"
      "  }\n"
      "  size += com.google.protobuf.micro.CodedOutputStreamMicro\n"
      "    .computeByteArraySize($number$, $name$Utf8_);\n"
      "}\n");
  } else {
    printer->Print(variables_,
      "if (has$capitalized_name$()) {\n"
      "  size += com.google.protobuf.micro.CodedOutputStreamMicro\n"
      "    .compute$capitalized_type$Size($number$, get$capitalized_name$());\n"
      "}\n");
  }
}

string PrimitiveFieldGenerator::GetBoxedType() const {
  return BoxedPrimitiveTypeName(GetJavaType(descriptor_));
}

// ===================================================================

RepeatedPrimitiveFieldGenerator::
RepeatedPrimitiveFieldGenerator(const FieldDescriptor* descriptor, const Params& params)
  : FieldGenerator(params), descriptor_(descriptor) {
  SetPrimitiveVariables(descriptor, params, &variables_);
  if (descriptor_->options().packed()) {
    GOOGLE_LOG(FATAL) << "MicroRuntime does not support packed";
  }
}

RepeatedPrimitiveFieldGenerator::~RepeatedPrimitiveFieldGenerator() {}

void RepeatedPrimitiveFieldGenerator::
GenerateMembers(io::Printer* printer) const {
  if (IsFastStringHandling(descriptor_, params_)) {
    if (params_.java_use_vector()) {
      printer->Print(variables_,
        "private java.util.Vector $name$_ = new java.util.Vector();\n"
        "public java.util.Vector get$capitalized_name$List() {\n"
        "  return $name$_;\n"
        "}\n"
        "private java.util.Vector $name$Utf8_ = new java.util.Vector();\n"
        "public int get$capitalized_name$Count() { return $name$_.size(); }\n"
        "public $type$ get$capitalized_name$(int index) {\n"
        "  return (($type$)$name$_.elementAt(index));\n"
        "}\n"
        "public $message_name$ set$capitalized_name$(int index, $type$ value) {\n"
        "$null_check$"
        "  $name$_.setElementAt(value, index);\n"
        "  $name$Utf8_ = null;\n"
        "  return this;\n"
        "}\n"
        "public $message_name$ add$capitalized_name$($type$ value) {\n"
        "$null_check$"
        "  $name$_.addElement(value);\n"
        "  $name$Utf8_ = null;\n"
        "  return this;\n"
        "}\n"
        "public $message_name$ clear$capitalized_name$() {\n"
        "  $name$_.removeAllElements();\n"
        "  $name$Utf8_ = null;\n"
        "  return this;\n"
        "}\n");
    } else {
      printer->Print(variables_,
        "private java.util.List<$type$> $name$_ =\n"
        "  java.util.Collections.emptyList();\n"
        "public java.util.List<$type$> get$capitalized_name$List() {\n"
        "  return $name$_;\n"   // note:  unmodifiable list
        "}\n"
        "private java.util.List<byte []> $name$Utf8_ = null;\n"
        "public int get$capitalized_name$Count() { return $name$_.size(); }\n"
        "public $type$ get$capitalized_name$(int index) {\n"
        "  return $name$_.get(index);\n"
        "}\n"
        "public $message_name$ set$capitalized_name$(int index, $type$ value) {\n"
        "$null_check$"
        "  $name$_.set(index, value);\n"
        "  $name$Utf8_ = null;\n"
        "  return this;\n"
        "}\n"
        "public $message_name$ add$capitalized_name$($type$ value) {\n"
        "$null_check$"
        "  if ($name$_.isEmpty()) {\n"
        "    $name$_ = new java.util.ArrayList<$type$>();\n"
        "  }\n"
        "  $name$_.add(value);\n"
        "  $name$Utf8_ = null;\n"
        "  return this;\n"
        "}\n"
        "public $message_name$ clear$capitalized_name$() {\n"
        "  $name$_ = java.util.Collections.emptyList();\n"
        "  $name$Utf8_ = null;\n"
        "  return this;\n"
        "}\n");
    }
  } else if (params_.java_use_vector()) {
    if (IsReferenceType(GetJavaType(descriptor_))) {
      printer->Print(variables_,
        "private java.util.Vector $name$_ = new java.util.Vector();\n"
        "public java.util.Vector get$capitalized_name$List() {\n"
        "  return $name$_;\n"
        "}\n"
        "public int get$capitalized_name$Count() { return $name$_.size(); }\n"
        "public $type$ get$capitalized_name$(int index) {\n"
        "  return ($type$) $name$_.elementAt(index);\n"
        "}\n"
        "public $message_name$ set$capitalized_name$(int index, $type$ value) {\n"
        "$null_check$"
        "  $name$_.setElementAt(value, index);\n"
        "  return this;\n"
        "}\n"
        "public $message_name$ add$capitalized_name$($type$ value) {\n"
        "$null_check$"
        "  $name$_.addElement(value);\n"
        "  return this;\n"
        "}\n"
        "public $message_name$ clear$capitalized_name$() {\n"
        "  $name$_.removeAllElements();\n"
        "  return this;\n"
        "}\n");
    } else {
      printer->Print(variables_,
        "private java.util.Vector $name$_ = new java.util.Vector();\n"
        "public java.util.Vector get$capitalized_name$List() {\n"
        "  return $name$_;\n"
        "}\n"
        "public int get$capitalized_name$Count() { return $name$_.size(); }\n"
        "public $type$ get$capitalized_name$(int index) {\n"
        "  return (($boxed_type$)$name$_.elementAt(index)).$type$Value();\n"
        "}\n"
        "public $message_name$ set$capitalized_name$(int index, $type$ value) {\n"
        "$null_check$"
        "  $name$_.setElementAt(new $boxed_type$(value), index);\n"
        "  return this;\n"
        "}\n"
        "public $message_name$ add$capitalized_name$($type$ value) {\n"
        "$null_check$"
        "  $name$_.addElement(new $boxed_type$(value));\n"
        "  return this;\n"
        "}\n"
        "public $message_name$ clear$capitalized_name$() {\n"
        "  $name$_.removeAllElements();\n"
        "  return this;\n"
        "}\n");
    }
  } else {
    printer->Print(variables_,
      "private java.util.List<$boxed_type$> $name$_ =\n"
      "  java.util.Collections.emptyList();\n"
      "public java.util.List<$boxed_type$> get$capitalized_name$List() {\n"
      "  return $name$_;\n"   // note:  unmodifiable list
      "}\n"
      "public int get$capitalized_name$Count() { return $name$_.size(); }\n"
      "public $type$ get$capitalized_name$(int index) {\n"
      "  return $name$_.get(index);\n"
      "}\n"
      "public $message_name$ set$capitalized_name$(int index, $type$ value) {\n"
      "$null_check$"
      "  $name$_.set(index, value);\n"
      "  return this;\n"
      "}\n"
      "public $message_name$ add$capitalized_name$($type$ value) {\n"
      "$null_check$"
      "  if ($name$_.isEmpty()) {\n"
      "    $name$_ = new java.util.ArrayList<$boxed_type$>();\n"
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

void RepeatedPrimitiveFieldGenerator::
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
      "    result.$name$_ = new java.util.ArrayList<$type$>();\n"
      "  }\n"
      "  result.$name$_.addAll(other.$name$_);\n"
      "}\n");
  }
}

void RepeatedPrimitiveFieldGenerator::
GenerateParsingCode(io::Printer* printer) const {
  if (descriptor_->options().packed()) {
    printer->Print(variables_,
      "int length = input.readRawVarint32();\n"
      "int limit = input.pushLimit(length);\n"
      "while (input.getBytesUntilLimit() > 0) {\n"
      "  add$capitalized_name$(input.read$capitalized_type$());\n"
      "}\n"
      "input.popLimit(limit);\n");
  } else {
    printer->Print(variables_,
      "add$capitalized_name$(input.read$capitalized_type$());\n");
  }
}

void RepeatedPrimitiveFieldGenerator::
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
        "  output.write$capitalized_type$NoTag(get$capitalized_name$(i));\n"
        "}\n");
    } else {
      printer->Print(variables_,
        "for ($type$ element : get$capitalized_name$List()) {\n"
        "  output.write$capitalized_type$NoTag(element);\n"
        "}\n");
    }
  } else {
    if (params_.java_use_vector()) {
      if (IsFastStringHandling(descriptor_, params_)) {
        printer->Print(variables_,
          "for (int i = 0; i < $name$Utf8_.size(); i++) {\n"
          "  output.writeByteArray($number$, (byte []) $name$Utf8_.get(i));\n"
          "}\n");
      } else {
        printer->Print(variables_,
          "for (int i = 0; i < get$capitalized_name$List().size(); i++) {\n"
          "  output.write$capitalized_type$($number$, get$capitalized_name$(i));\n"
          "}\n");
      }
    } else {
      if (IsFastStringHandling(descriptor_, params_)) {
        printer->Print(variables_,
          "for (byte [] element : $name$Utf8_) {\n"
          "  output.writeByteArray($number$, element);\n"
          "}\n");
      } else {
        printer->Print(variables_,
          "for ($type$ element : get$capitalized_name$List()) {\n"
          "  output.write$capitalized_type$($number$, element);\n"
          "}\n");
      }
    }
  }
}

void RepeatedPrimitiveFieldGenerator::
GenerateSerializedSizeCode(io::Printer* printer) const {
  printer->Print(variables_,
    "{\n"
    "  int dataSize = 0;\n");
  printer->Indent();

  if (FixedSize(descriptor_->type()) == -1) {
    if (params_.java_use_vector()) {
      if (IsFastStringHandling(descriptor_, params_)) {
        printer->Print(variables_,
          "$name$Utf8_ = new java.util.Vector();\n"
          "byte[] bytes = null;\n"
          "int sizeArray = get$capitalized_name$List().size();\n"
          "for (int i = 0; i < sizeArray; i++) {\n"
          "  $type$ element = ($type$)$name$_.elementAt(i);\n"
          "  try {\n"
          "    bytes = element.getBytes(\"UTF-8\");\n"
          "  } catch (java.io.UnsupportedEncodingException e) {\n"
          "    throw new RuntimeException(\"UTF-8 not supported.\");\n"
          "  }\n"
          "  $name$Utf8_.addElement(bytes);\n"
          "  dataSize += com.google.protobuf.micro.CodedOutputStreamMicro\n"
          "    .computeByteArraySizeNoTag(bytes);\n"
          "}\n");
      } else {
        printer->Print(variables_,
          "for (int i = 0; i < get$capitalized_name$List().size(); i++) {\n"
          "  dataSize += com.google.protobuf.micro.CodedOutputStreamMicro\n"
          "    .compute$capitalized_type$SizeNoTag(($type$)get$capitalized_name$(i));\n"
          "}\n");
      }
    } else {
      if (IsFastStringHandling(descriptor_, params_)) {
          printer->Print(variables_,
            "$name$Utf8_ = new java.util.ArrayList<byte[]>();\n"
            "byte[] bytes = null;\n"
            "int sizeArray = get$capitalized_name$List().size();\n"
            "for (int i = 0; i < sizeArray; i++) {\n"
            "   $type$ element = get$capitalized_name$(i);\n"
            "  try {\n"
            "    bytes = element.getBytes(\"UTF-8\");\n"
            "  } catch (java.io.UnsupportedEncodingException e) {\n"
            "    throw new RuntimeException(\"UTF-8 not supported.\");\n"
            "  }\n"
            "  $name$Utf8_.add(bytes);\n"
            "  dataSize += com.google.protobuf.micro.CodedOutputStreamMicro\n"
            "    .computeByteArraySizeNoTag(bytes);\n"
            "}\n");
      } else {
        printer->Print(variables_,
            "for ($type$ element : get$capitalized_name$List()) {\n"
            "  dataSize += com.google.protobuf.micro.CodedOutputStreamMicro\n"
            "    .compute$capitalized_type$SizeNoTag(element);\n"
          "}\n");
      }
    }
  } else {
    printer->Print(variables_,
      "dataSize = $fixed_size$ * get$capitalized_name$List().size();\n");
  }

  printer->Print(
      "size += dataSize;\n");

  if (descriptor_->options().packed()) {
    if (params_.java_use_vector()) {
      printer->Print(variables_,
        "if (get$capitalized_name$List().size() != 0) {\n");
    } else {
      printer->Print(variables_,
        "if (!get$capitalized_name$List().isEmpty()) {\n");
    }
    printer->Print(variables_,
        "  size += $tag_size$;\n"
        "  size += com.google.protobuf.micro.CodedOutputStreamMicro\n"
        "      .computeInt32SizeNoTag(dataSize);\n"
        "}\n");
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

string RepeatedPrimitiveFieldGenerator::GetBoxedType() const {
  return BoxedPrimitiveTypeName(GetJavaType(descriptor_));
}

}  // namespace javamicro
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
