#!/usr/bin/env python
# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""protoc plugin to create C++ reader/writer for JSON-encoded protobufs

The reader/writer use Chrome's base::Values.
"""

import os
import sys

from util import plugin_protos, types, writer


class CppConverterWriter(writer.CodeWriter):
  def WriteProtoFile(self, proto_file, output_dir):
    err = proto_file.CheckSupported()
    if err:
      self.AddError(err)
      return

    self.WriteCStyleHeader()

    self.Output('#include "{output_dir}{generated_pb_h}"',
                output_dir=output_dir + '/' if output_dir else '',
                generated_pb_h=proto_file.CppBaseHeader())
    self.Output('')

    # import is not supported
    assert [] == proto_file.GetDependencies()

    self.Output('// base dependencies')
    self.Output('#include "base/values.h"')
    self.Output('')
    self.Output('#include <memory>')
    self.Output('#include <string>')
    self.Output('#include <utility>')
    self.Output('')

    namespaces = proto_file.ProtoNamespaces() + ['json']
    for name in namespaces:
      self.Output('namespace {name} {{', name=name)
      self.IncreaseIndent()

    for message in proto_file.GetMessages():
      self.WriteMessage(message)

    # Nothing to do for enums

    for name in namespaces:
      self.DecreaseIndent()
      self.Output('}}')

  def WriteMessage(self, message):
    self.Output('class {class_name} {{',
                class_name=message.CppConverterClassName())
    self.Output(' public:')
    with self.AddIndent():
      for nested_class in message.GetMessages():
        self.WriteMessage(nested_class)

      generated_class_name = message.QualifiedTypes().cpp_base
      # Nothing to write for enums.

      self.Output(
          'static bool ReadFromValue(const base::Value* json, {generated_class_name}* message) {{\n'
          '  const base::DictionaryValue* dict;\n'
          '  if (!json->GetAsDictionary(&dict)) goto error;\n'
          '',
          generated_class_name=generated_class_name)

      with self.AddIndent():
        for field_proto in message.GetFields():
          self.WriteFieldRead(field_proto)

      self.Output(
          '  return true;\n'
          '\n'
          'error:\n'
          '  return false;\n'
          '}}\n'
          '\n'
          'static std::unique_ptr<base::DictionaryValue> WriteToValue(const {generated_class_name}& message) {{\n'
          '  std::unique_ptr<base::DictionaryValue> dict(new base::DictionaryValue());\n'
          '',
          generated_class_name=generated_class_name)

      with self.AddIndent():
        for field_proto in message.GetFields():
          self.FieldWriteToValue(field_proto)

      self.Output(
          '  return dict;\n'
          '',
          generated_class_name=generated_class_name)
      self.Output('}}')

    self.Output('}};')
    self.Output('')

  def FieldWriteToValue(self, field):
    if field.IsRepeated():
      self.Output('{{')
    else:
      self.Output('if (message.has_{field_name}()) {{\n', field_name=field.name)

    with self.AddIndent():
      if field.IsRepeated():
        self.RepeatedMemberFieldWriteToValue(field)
      else:
        self.OptionalMemberFieldWriteToValue(field)

    self.Output('}}')

  def RepeatedMemberFieldWriteToValue(self, field):
    prologue = (
        'auto field_list = std::make_unique<base::ListValue>();\n'
        'for (int i = 0; i < message.{field_name}_size(); ++i) {{\n'
    )

    if field.IsClassType():
      middle = (
          'std::unique_ptr<base::Value> inner_message_value = \n'
          '    {inner_class_converter}::WriteToValue(message.{field_name}(i));\n'
          'field_list->Append(std::move(inner_message_value));\n'
      )
    else:
      middle = (
          'field_list->Append{value_type}(message.{field_name}(i));\n'
      )

    epilogue = (
        '\n}}\n'
        'dict->Set("{field_number}", std::move(field_list));'
    )
    self.Output(
        prologue + Indented(middle) + epilogue,
        field_number=field.JavascriptIndex(),
        field_name=field.name,
        value_type=field.CppValueType() if not field.IsClassType() else None,
        inner_class_converter=field.CppConverterType()
    )

  def OptionalMemberFieldWriteToValue(self, field):
    if field.IsClassType():
      body = (
          'std::unique_ptr<base::Value> inner_message_value = \n'
          '    {inner_class_converter}::WriteToValue(message.{field_name}());\n'
          'dict->Set("{field_number}", std::move(inner_message_value));\n'
      )
    else:
      body = (
          'dict->Set{value_type}("{field_number}", message.{field_name}());\n'
      )

    self.Output(
        body,
        field_number=field.JavascriptIndex(),
        field_name=field.name,
        value_type=field.CppValueType() if not field.IsClassType() else None,
        inner_class_converter=field.CppConverterType(),
    )

  def WriteFieldRead(self, field):
    self.Output('if (dict->HasKey("{field_number}")) {{',
                field_number=field.JavascriptIndex())

    with self.AddIndent():
      if field.IsRepeated():
        self.RepeatedMemberFieldRead(field)
      else:
        self.OptionalMemberFieldRead(field)

    self.Output('}}')

  def RepeatedMemberFieldRead(self, field):
    prologue = (
        'const base::ListValue* field_list;\n'
        'if (!dict->GetList("{field_number}", &field_list)) {{\n'
        '  goto error;\n'
        '}}\n'
        'for (size_t i = 0; i < field_list->GetSize(); ++i) {{\n'
    )

    if field.IsClassType():
      middle = (
          'const base::Value* inner_message_value;\n'
          'if (!field_list->Get(i, &inner_message_value)) {{\n'
          '  goto error;\n'
          '}}\n'
          'if (!{inner_class_parser}::ReadFromValue(inner_message_value, message->add_{field_name}())) {{\n'
          '  goto error;\n'
          '}}\n'
      )
    else:
      middle = (
          '{cpp_type} field_value;\n'
          'if (!field_list->Get{value_type}(i, &field_value)) {{\n'
          '  goto error;\n'
          '}}\n'
          'message->add_{field_name}(field_value);\n'
      )

    self.Output(
        prologue + Indented(middle) + '\n}}',
        field_number=field.JavascriptIndex(),
        field_name=field.name,
        cpp_type=field.CppPrimitiveType() if not field.IsClassType() else None,
        value_type=field.CppValueType() if not field.IsClassType() else None,
        inner_class_parser=field.CppConverterType()
    )

  def OptionalMemberFieldRead(self, field):
    if field.IsClassType():
      self.Output(
          'const base::Value* inner_message_value;\n'
          'if (!dict->Get("{field_number}", &inner_message_value)) {{\n'
          '  goto error;\n'
          '}}\n'
          'if (!{inner_class_parser}::ReadFromValue(inner_message_value, message->mutable_{field_name}())) {{\n'
          '  goto error;\n'
          '}}\n'
          '',
          field_number=field.JavascriptIndex(),
          field_name=field.name,
          inner_class_parser=field.CppConverterType()
      )
    else:
      self.Output(
          '{cpp_type} field_value;\n'
          'if (!dict->Get{value_type}("{field_number}", &field_value)) {{\n'
          '  goto error;\n'
          '}}\n'
          'message->set_{field_name}(field_value);\n'
          '',
          field_number=field.JavascriptIndex(),
          field_name=field.name,
          cpp_type=field.CppPrimitiveType(),
          value_type=field.CppValueType()
      )


def Indented(s, indent=2):
  return '\n'.join((' ' * indent) + p for p in s.rstrip('\n').split('\n'))


def SetBinaryStdio():
  import platform
  if platform.system() == 'Windows':
    import msvcrt
    msvcrt.setmode(sys.stdin.fileno(), os.O_BINARY)
    msvcrt.setmode(sys.stdout.fileno(), os.O_BINARY)


def ReadRequestFromStdin():
  data = sys.stdin.read()
  return plugin_protos.PluginRequestFromString(data)


def main():
  SetBinaryStdio()
  request = ReadRequestFromStdin()
  response = plugin_protos.PluginResponse()

  output_dir = request.GetArgs().get('output_dir', '')

  for proto_file in request.GetAllFiles():
    types.RegisterProtoFile(proto_file)

    cppwriter = CppConverterWriter()
    cppwriter.WriteProtoFile(proto_file, output_dir)

    converter_filename = proto_file.CppConverterFilename()
    if output_dir:
      converter_filename = os.path.join(output_dir,
                                        os.path.split(converter_filename)[1])

    response.AddFileWithContent(converter_filename, cppwriter.GetValue())
    if cppwriter.GetErrors():
      response.AddError('\n'.join(cppwriter.GetErrors()))

  response.WriteToStdout()


if __name__ == '__main__':
  main()
