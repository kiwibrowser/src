# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Generate template values for a callback function.

Design doc: http://www.chromium.org/developers/design-documents/idl-compiler
"""

from v8_globals import includes

CALLBACK_FUNCTION_H_INCLUDES = frozenset([
    'platform/bindings/callback_function_base.h',
])
CALLBACK_FUNCTION_CPP_INCLUDES = frozenset([
    'bindings/core/v8/exception_state.h',
    'bindings/core/v8/generated_code_helper.h',
    'bindings/core/v8/native_value_traits_impl.h',
    'bindings/core/v8/to_v8_for_core.h',
    'bindings/core/v8/v8_binding_for_core.h',
    'core/execution_context/execution_context.h',
])


def callback_function_context(callback_function):
    includes.clear()
    includes.update(CALLBACK_FUNCTION_CPP_INCLUDES)
    idl_type = callback_function.idl_type
    idl_type_str = str(idl_type)

    for argument in callback_function.arguments:
        argument.idl_type.add_includes_for_type(
            callback_function.extended_attributes)

    context = {
        # While both |callback_function_name| and |cpp_class| are identical at
        # the moment, the two are being defined because their values may change
        # in the future (e.g. if we support [ImplementedAs=] in callback
        # functions).
        'callback_function_name': callback_function.name,
        'cpp_class': 'V8%s' % callback_function.name,
        'cpp_includes': sorted(includes),
        'forward_declarations': sorted(forward_declarations(callback_function)),
        'header_includes': sorted(CALLBACK_FUNCTION_H_INCLUDES),
        'idl_type': idl_type_str,
        'return_cpp_type': idl_type.cpp_type,
    }

    if idl_type_str != 'void':
        context.update({
            'return_value_conversion': idl_type.v8_value_to_local_cpp_value(
                callback_function.extended_attributes,
                'call_result', 'native_result', isolate='GetIsolate()',
                bailout_return_value='v8::Nothing<%s>()' % context['return_cpp_type']),
        })

    context.update(arguments_context(callback_function.arguments))
    return context


def forward_declarations(callback_function):
    def find_forward_declaration(idl_type):
        if idl_type.is_interface_type or idl_type.is_dictionary:
            return idl_type.implemented_as
        elif idl_type.is_array_or_sequence_type:
            return find_forward_declaration(idl_type.element_type)
        return None

    declarations = set(['ScriptWrappable'])
    for argument in callback_function.arguments:
        name = find_forward_declaration(argument.idl_type)
        if name:
            declarations.add(name)
    return declarations


def arguments_context(arguments):
    def argument_context(argument):
        idl_type = argument.idl_type
        return {
            'cpp_value_to_v8_value': idl_type.cpp_value_to_v8_value(
                argument.name, isolate='GetIsolate()',
                creation_context='argument_creation_context'),
            'enum_type': idl_type.enum_type,
            'enum_values': idl_type.enum_values,
            'name': argument.name,
            'v8_name': 'v8_%s' % argument.name,
        }

    argument_declarations = ['ScriptWrappable* callback_this_value']
    argument_declarations.extend(
        '%s %s' % (argument.idl_type.callback_cpp_type, argument.name)
        for argument in arguments)
    return {
        'argument_declarations': argument_declarations,
        'arguments': [argument_context(argument) for argument in arguments],
    }
