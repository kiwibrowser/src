#!/usr/bin/env python
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Extracts native methods from a Java file and generates the JNI bindings.
If you change this, please run and update the tests."""

import collections
import errno
import optparse
import os
import re
from string import Template
import subprocess
import sys
import textwrap
import zipfile

CHROMIUM_SRC = os.path.join(
    os.path.dirname(__file__), os.pardir, os.pardir, os.pardir)
BUILD_ANDROID_GYP = os.path.join(
    CHROMIUM_SRC, 'build', 'android', 'gyp')

sys.path.append(BUILD_ANDROID_GYP)

from util import build_utils


# Match single line comments, multiline comments, character literals, and
# double-quoted strings.
_COMMENT_REMOVER_REGEX = re.compile(
    r'//.*?$|/\*.*?\*/|\'(?:\\.|[^\\\'])*\'|"(?:\\.|[^\\"])*"',
    re.DOTALL | re.MULTILINE)

_EXTRACT_NATIVES_REGEX = re.compile(
    r'(@NativeClassQualifiedName'
    r'\(\"(?P<native_class_name>.*?)\"\)\s+)?'
    r'(@NativeCall(\(\"(?P<java_class_name>.*?)\"\))\s+)?'
    r'(?P<qualifiers>\w+\s\w+|\w+|\s+)\s*native '
    r'(?P<return_type>\S*) '
    r'(?P<name>native\w+)\((?P<params>.*?)\);')

_MAIN_DEX_REGEX = re.compile(
    r'^\s*(?:@(?:\w+\.)*\w+\s+)*@MainDex\b',
    re.MULTILINE)

# Use 100 columns rather than 80 because it makes many lines more readable.
_WRAP_LINE_LENGTH = 100
# WrapOutput() is fairly slow. Pre-creating TextWrappers helps a bit.
_WRAPPERS_BY_INDENT = [
    textwrap.TextWrapper(width=_WRAP_LINE_LENGTH, expand_tabs=False,
                         replace_whitespace=False,
                         subsequent_indent=' ' * (indent + 4),
                         break_long_words=False)
    for indent in xrange(50)]  # 50 chosen experimentally.


class ParseError(Exception):
  """Exception thrown when we can't parse the input file."""

  def __init__(self, description, *context_lines):
    Exception.__init__(self)
    self.description = description
    self.context_lines = context_lines

  def __str__(self):
    context = '\n'.join(self.context_lines)
    return '***\nERROR: %s\n\n%s\n***' % (self.description, context)


class Param(object):
  """Describes a param for a method, either java or native."""

  def __init__(self, **kwargs):
    self.datatype = kwargs['datatype']
    self.name = kwargs['name']


class NativeMethod(object):
  """Describes a C/C++ method that is called by Java code"""

  def __init__(self, **kwargs):
    self.static = kwargs['static']
    self.java_class_name = kwargs['java_class_name']
    self.return_type = kwargs['return_type']
    self.name = kwargs['name']
    self.params = kwargs['params']
    if self.params:
      assert type(self.params) is list
      assert type(self.params[0]) is Param
    if (self.params and
        self.params[0].datatype == kwargs.get('ptr_type', 'int') and
        self.params[0].name.startswith('native')):
      self.type = 'method'
      self.p0_type = self.params[0].name[len('native'):]
      if kwargs.get('native_class_name'):
        self.p0_type = kwargs['native_class_name']
    else:
      self.type = 'function'
    self.method_id_var_name = kwargs.get('method_id_var_name', None)


class CalledByNative(object):
  """Describes a java method exported to c/c++"""

  def __init__(self, **kwargs):
    self.system_class = kwargs['system_class']
    self.unchecked = kwargs['unchecked']
    self.static = kwargs['static']
    self.java_class_name = kwargs['java_class_name']
    self.return_type = kwargs['return_type']
    self.name = kwargs['name']
    self.params = kwargs['params']
    self.method_id_var_name = kwargs.get('method_id_var_name', None)
    self.signature = kwargs.get('signature')
    self.is_constructor = kwargs.get('is_constructor', False)
    self.env_call = GetEnvCall(self.is_constructor, self.static,
                               self.return_type)
    self.static_cast = GetStaticCastForReturnType(self.return_type)


class ConstantField(object):
  def __init__(self, **kwargs):
    self.name = kwargs['name']
    self.value = kwargs['value']


def JavaDataTypeToC(java_type):
  """Returns a C datatype for the given java type."""
  java_pod_type_map = {
      'int': 'jint',
      'byte': 'jbyte',
      'char': 'jchar',
      'short': 'jshort',
      'boolean': 'jboolean',
      'long': 'jlong',
      'double': 'jdouble',
      'float': 'jfloat',
  }
  java_type_map = {
      'void': 'void',
      'String': 'jstring',
      'Class': 'jclass',
      'Throwable': 'jthrowable',
      'java/lang/String': 'jstring',
      'java/lang/Class': 'jclass',
      'java/lang/Throwable': 'jthrowable',
  }

  java_type = _StripGenerics(java_type)
  if java_type in java_pod_type_map:
    return java_pod_type_map[java_type]
  elif java_type in java_type_map:
    return java_type_map[java_type]
  elif java_type.endswith('[]'):
    if java_type[:-2] in java_pod_type_map:
      return java_pod_type_map[java_type[:-2]] + 'Array'
    return 'jobjectArray'
  else:
    return 'jobject'


def WrapCTypeForDeclaration(c_type):
  """Wrap the C datatype in a JavaRef if required."""
  if re.match(RE_SCOPED_JNI_TYPES, c_type):
    return 'const base::android::JavaParamRef<' + c_type + '>&'
  else:
    return c_type


def _JavaDataTypeToCForDeclaration(java_type):
  """Returns a JavaRef-wrapped C datatype for the given java type."""
  return WrapCTypeForDeclaration(JavaDataTypeToC(java_type))


def JavaDataTypeToCForCalledByNativeParam(java_type):
  """Returns a C datatype to be when calling from native."""
  if java_type == 'int':
    return 'JniIntWrapper'
  else:
    c_type = JavaDataTypeToC(java_type)
    if re.match(RE_SCOPED_JNI_TYPES, c_type):
      return 'const base::android::JavaRef<' + c_type + '>&'
    else:
      return c_type


def JavaReturnValueToC(java_type):
  """Returns a valid C return value for the given java type."""
  java_pod_type_map = {
      'int': '0',
      'byte': '0',
      'char': '0',
      'short': '0',
      'boolean': 'false',
      'long': '0',
      'double': '0',
      'float': '0',
      'void': ''
  }
  return java_pod_type_map.get(java_type, 'NULL')


def _GetJNIFirstParamType(native):
  if native.type == 'function' and native.static:
    return 'jclass'
  return 'jobject'


def _GetJNIFirstParam(native, for_declaration):
  c_type = _GetJNIFirstParamType(native)
  if for_declaration:
    c_type = WrapCTypeForDeclaration(c_type)
  return [c_type + ' jcaller']


def _GetParamsInDeclaration(native):
  """Returns the params for the forward declaration.

  Args:
    native: the native dictionary describing the method.

  Returns:
    A string containing the params.
  """
  return ',\n    '.join(_GetJNIFirstParam(native, True) +
                        [_JavaDataTypeToCForDeclaration(param.datatype) + ' ' +
                         param.name
                         for param in native.params])


def GetParamsInStub(native):
  """Returns the params for the stub declaration.

  Args:
    native: the native dictionary describing the method.

  Returns:
    A string containing the params.
  """
  params = [JavaDataTypeToC(p.datatype) + ' ' + p.name for p in native.params]
  return ',\n    '.join(_GetJNIFirstParam(native, False) + params)


def _StripGenerics(value):
  """Strips Java generics from a string."""
  nest_level = 0  # How deeply we are nested inside the generics.
  start_index = 0  # Starting index of the last non-generic region.
  out = []

  for i, c in enumerate(value):
    if c == '<':
      if nest_level == 0:
        out.append(value[start_index:i])
      nest_level += 1
    elif c == '>':
      start_index = i + 1
      nest_level -= 1
  out.append(value[start_index:])

  return ''.join(out)


class JniParams(object):
  """Get JNI related parameters."""

  def __init__(self, fully_qualified_class):
    self._fully_qualified_class = 'L' + fully_qualified_class
    self._package = '/'.join(fully_qualified_class.split('/')[:-1])
    self._imports = []
    self._inner_classes = []
    self._implicit_imports = []

  def ExtractImportsAndInnerClasses(self, contents):
    contents = contents.replace('\n', '')
    re_import = re.compile(r'import.*?(?P<class>\S*?);')
    for match in re.finditer(re_import, contents):
      self._imports += ['L' + match.group('class').replace('.', '/')]

    re_inner = re.compile(r'(class|interface|enum)\s+?(?P<name>\w+?)\W')
    for match in re.finditer(re_inner, contents):
      inner = match.group('name')
      if not self._fully_qualified_class.endswith(inner):
        self._inner_classes += [self._fully_qualified_class + '$' +
                                     inner]

    re_additional_imports = re.compile(
        r'@JNIAdditionalImport\(\s*{?(?P<class_names>.*?)}?\s*\)')
    for match in re.finditer(re_additional_imports, contents):
      for class_name in match.group('class_names').split(','):
        self._AddAdditionalImport(class_name.strip())

  def JavaToJni(self, param):
    """Converts a java param into a JNI signature type."""
    pod_param_map = {
        'int': 'I',
        'boolean': 'Z',
        'char': 'C',
        'short': 'S',
        'long': 'J',
        'double': 'D',
        'float': 'F',
        'byte': 'B',
        'void': 'V',
    }
    object_param_list = [
        'Ljava/lang/Boolean',
        'Ljava/lang/Integer',
        'Ljava/lang/Long',
        'Ljava/lang/Object',
        'Ljava/lang/String',
        'Ljava/lang/Class',
        'Ljava/lang/CharSequence',
        'Ljava/lang/Runnable',
        'Ljava/lang/Throwable',
    ]

    prefix = ''
    # Array?
    while param[-2:] == '[]':
      prefix += '['
      param = param[:-2]
    # Generic?
    if '<' in param:
      param = param[:param.index('<')]
    if param in pod_param_map:
      return prefix + pod_param_map[param]
    if '/' in param:
      # Coming from javap, use the fully qualified param directly.
      return prefix + 'L' + param + ';'

    for qualified_name in (object_param_list +
                           [self._fully_qualified_class] + self._inner_classes):
      if (qualified_name.endswith('/' + param) or
          qualified_name.endswith('$' + param.replace('.', '$')) or
          qualified_name == 'L' + param):
        return prefix + qualified_name + ';'

    # Is it from an import? (e.g. referecing Class from import pkg.Class;
    # note that referencing an inner class Inner from import pkg.Class.Inner
    # is not supported).
    for qualified_name in self._imports:
      if qualified_name.endswith('/' + param):
        # Ensure it's not an inner class.
        components = qualified_name.split('/')
        if len(components) > 2 and components[-2][0].isupper():
          raise SyntaxError('Inner class (%s) can not be imported '
                            'and used by JNI (%s). Please import the outer '
                            'class and use Outer.Inner instead.' %
                            (qualified_name, param))
        return prefix + qualified_name + ';'

    # Is it an inner class from an outer class import? (e.g. referencing
    # Class.Inner from import pkg.Class).
    if '.' in param:
      components = param.split('.')
      outer = '/'.join(components[:-1])
      inner = components[-1]
      for qualified_name in self._imports:
        if qualified_name.endswith('/' + outer):
          return (prefix + qualified_name + '$' + inner + ';')
      raise SyntaxError('Inner class (%s) can not be '
                        'used directly by JNI. Please import the outer '
                        'class, probably:\n'
                        'import %s.%s;' %
                        (param, self._package.replace('/', '.'),
                         outer.replace('/', '.')))

    self._CheckImplicitImports(param)

    # Type not found, falling back to same package as this class.
    return (prefix + 'L' + self._package + '/' + param + ';')

  def _AddAdditionalImport(self, class_name):
    assert class_name.endswith('.class')
    raw_class_name = class_name[:-len('.class')]
    if '.' in raw_class_name:
      raise SyntaxError('%s cannot be used in @JNIAdditionalImport. '
                        'Only import unqualified outer classes.' % class_name)
    new_import = 'L%s/%s' % (self._package, raw_class_name)
    if new_import in self._imports:
      raise SyntaxError('Do not use JNIAdditionalImport on an already '
                        'imported class: %s' % (new_import.replace('/', '.')))
    self._imports += [new_import]

  def _CheckImplicitImports(self, param):
    # Ensure implicit imports, such as java.lang.*, are not being treated
    # as being in the same package.
    if not self._implicit_imports:
      # This file was generated from android.jar and lists
      # all classes that are implicitly imported.
      with file(os.path.join(os.path.dirname(sys.argv[0]),
                             'android_jar.classes'), 'r') as f:
        self._implicit_imports = f.readlines()
    for implicit_import in self._implicit_imports:
      implicit_import = implicit_import.strip().replace('.class', '')
      implicit_import = implicit_import.replace('/', '.')
      if implicit_import.endswith('.' + param):
        raise SyntaxError('Ambiguous class (%s) can not be used directly '
                          'by JNI.\nPlease import it, probably:\n\n'
                          'import %s;' %
                          (param, implicit_import))

  def Signature(self, params, returns):
    """Returns the JNI signature for the given datatypes."""
    items = ['(']
    items += [self.JavaToJni(param.datatype) for param in params]
    items += [')']
    items += [self.JavaToJni(returns)]
    return '"{}"'.format(''.join(items))

  @staticmethod
  def ParseJavaPSignature(signature_line):
    prefix = 'Signature: '
    index = signature_line.find(prefix)
    if index == -1:
      prefix = 'descriptor: '
      index = signature_line.index(prefix)
    return '"%s"' % signature_line[index + len(prefix):]

  @staticmethod
  def Parse(params):
    """Parses the params into a list of Param objects."""
    if not params:
      return []
    ret = []
    params = _StripGenerics(params)
    for p in params.split(','):
      items = p.split()

      # Remove @Annotations from parameters.
      while items[0].startswith('@'):
        del items[0]

      if 'final' in items:
        items.remove('final')

      param = Param(
          datatype=items[0],
          name=(items[1] if len(items) > 1 else 'p%s' % len(ret)),
      )
      ret += [param]
    return ret


def ExtractJNINamespace(contents):
  re_jni_namespace = re.compile('.*?@JNINamespace\("(.*?)"\)')
  m = re.findall(re_jni_namespace, contents)
  if not m:
    return ''
  return m[0]


def ExtractFullyQualifiedJavaClassName(java_file_name, contents):
  re_package = re.compile('.*?package (.*?);')
  matches = re.findall(re_package, contents)
  if not matches:
    raise SyntaxError('Unable to find "package" line in %s' % java_file_name)
  return (matches[0].replace('.', '/') + '/' +
          os.path.splitext(os.path.basename(java_file_name))[0])


def ExtractNatives(contents, ptr_type):
  """Returns a list of dict containing information about a native method."""
  contents = contents.replace('\n', '')
  natives = []
  for match in _EXTRACT_NATIVES_REGEX.finditer(contents):
    native = NativeMethod(
        static='static' in match.group('qualifiers'),
        java_class_name=match.group('java_class_name'),
        native_class_name=match.group('native_class_name'),
        return_type=match.group('return_type'),
        name=match.group('name').replace('native', ''),
        params=JniParams.Parse(match.group('params')),
        ptr_type=ptr_type)
    natives += [native]
  return natives


def IsMainDexJavaClass(contents):
  """Returns True if the class or any of its methods are annotated as @MainDex.

  JNI registration doesn't always need to be completed for non-browser processes
  since most Java code is only used by the browser process. Classes that are
  needed by non-browser processes must explicitly be annotated with @MainDex
  to force JNI registration.
  """
  return bool(_MAIN_DEX_REGEX.search(contents))


def GetBinaryClassName(fully_qualified_class):
  """Returns a string concatenating the Java package and class."""
  escaped = fully_qualified_class.replace('_', '_1')
  return escaped.replace('/', '_').replace('$', '_00024')


def GetRegistrationFunctionName(fully_qualified_class):
  """Returns the register name with a given class."""
  return 'RegisterNative_' + GetBinaryClassName(fully_qualified_class)


def GetStaticCastForReturnType(return_type):
  type_map = { 'String' : 'jstring',
               'java/lang/String' : 'jstring',
               'Class': 'jclass',
               'java/lang/Class': 'jclass',
               'Throwable': 'jthrowable',
               'java/lang/Throwable': 'jthrowable',
               'boolean[]': 'jbooleanArray',
               'byte[]': 'jbyteArray',
               'char[]': 'jcharArray',
               'short[]': 'jshortArray',
               'int[]': 'jintArray',
               'long[]': 'jlongArray',
               'float[]': 'jfloatArray',
               'double[]': 'jdoubleArray' }
  return_type = _StripGenerics(return_type)
  ret = type_map.get(return_type, None)
  if ret:
    return ret
  if return_type.endswith('[]'):
    return 'jobjectArray'
  return None


def GetEnvCall(is_constructor, is_static, return_type):
  """Maps the types availabe via env->Call__Method."""
  if is_constructor:
    return 'NewObject'
  env_call_map = {'boolean': 'Boolean',
                  'byte': 'Byte',
                  'char': 'Char',
                  'short': 'Short',
                  'int': 'Int',
                  'long': 'Long',
                  'float': 'Float',
                  'void': 'Void',
                  'double': 'Double',
                  'Object': 'Object',
                 }
  call = env_call_map.get(return_type, 'Object')
  if is_static:
    call = 'Static' + call
  return 'Call' + call + 'Method'


def GetMangledParam(datatype):
  """Returns a mangled identifier for the datatype."""
  if len(datatype) <= 2:
    return datatype.replace('[', 'A')
  ret = ''
  for i in range(1, len(datatype)):
    c = datatype[i]
    if c == '[':
      ret += 'A'
    elif c.isupper() or datatype[i - 1] in ['/', 'L']:
      ret += c.upper()
  return ret


def GetMangledMethodName(jni_params, name, params, return_type):
  """Returns a mangled method name for the given signature.

     The returned name can be used as a C identifier and will be unique for all
     valid overloads of the same method.

  Args:
     jni_params: JniParams object.
     name: string.
     params: list of Param.
     return_type: string.

  Returns:
      A mangled name.
  """
  mangled_items = []
  for datatype in [return_type] + [x.datatype for x in params]:
    mangled_items += [GetMangledParam(jni_params.JavaToJni(datatype))]
  mangled_name = name + '_'.join(mangled_items)
  assert re.match(r'[0-9a-zA-Z_]+', mangled_name)
  return mangled_name


def MangleCalledByNatives(jni_params, called_by_natives, always_mangle):
  """Mangles all the overloads from the call_by_natives list."""
  method_counts = collections.defaultdict(
      lambda: collections.defaultdict(lambda: 0))
  for called_by_native in called_by_natives:
    java_class_name = called_by_native.java_class_name
    name = called_by_native.name
    method_counts[java_class_name][name] += 1
  for called_by_native in called_by_natives:
    java_class_name = called_by_native.java_class_name
    method_name = called_by_native.name
    method_id_var_name = method_name
    if always_mangle or method_counts[java_class_name][method_name] > 1:
      method_id_var_name = GetMangledMethodName(jni_params, method_name,
                                                called_by_native.params,
                                                called_by_native.return_type)
    called_by_native.method_id_var_name = method_id_var_name
  return called_by_natives


# Regex to match the JNI types that should be wrapped in a JavaRef.
RE_SCOPED_JNI_TYPES = re.compile('jobject|jclass|jstring|jthrowable|.*Array')


# Regex to match a string like "@CalledByNative public void foo(int bar)".
RE_CALLED_BY_NATIVE = re.compile(
    r'@CalledByNative(?P<Unchecked>(?:Unchecked)?)(?:\("(?P<annotation>.*)"\))?'
    r'(?:\s+@\w+(?:\(.*\))?)*'  # Ignore any other annotations.
    r'\s+(?P<prefix>('
    r'(private|protected|public|static|abstract|final|default|synchronized)'
    r'\s*)*)'
    r'(?:\s*@\w+)?'  # Ignore annotations in return types.
    r'\s*(?P<return_type>\S*?)'
    r'\s*(?P<name>\w+)'
    r'\s*\((?P<params>[^\)]*)\)')

# Removes empty lines that are indented (i.e. start with 2x spaces).
def RemoveIndentedEmptyLines(string):
  return re.sub('^(?: {2})+$\n', '', string, flags=re.MULTILINE)


def ExtractCalledByNatives(jni_params, contents, always_mangle):
  """Parses all methods annotated with @CalledByNative.

  Args:
    jni_params: JniParams object.
    contents: the contents of the java file.

  Returns:
    A list of dict with information about the annotated methods.
    TODO(bulach): return a CalledByNative object.

  Raises:
    ParseError: if unable to parse.
  """
  called_by_natives = []
  for match in re.finditer(RE_CALLED_BY_NATIVE, contents):
    return_type = match.group('return_type')
    name = match.group('name')
    if not return_type:
      is_constructor = True
      return_type = name
      name = "Constructor"
    else:
      is_constructor = False

    called_by_natives += [CalledByNative(
        system_class=False,
        unchecked='Unchecked' in match.group('Unchecked'),
        static='static' in match.group('prefix'),
        java_class_name=match.group('annotation') or '',
        return_type=return_type,
        name=name,
        is_constructor=is_constructor,
        params=JniParams.Parse(match.group('params')))]
  # Check for any @CalledByNative occurrences that weren't matched.
  unmatched_lines = re.sub(RE_CALLED_BY_NATIVE, '', contents).split('\n')
  for line1, line2 in zip(unmatched_lines, unmatched_lines[1:]):
    if '@CalledByNative' in line1:
      raise ParseError('could not parse @CalledByNative method signature',
                       line1, line2)
  return MangleCalledByNatives(jni_params, called_by_natives, always_mangle)


def RemoveComments(contents):
  # We need to support both inline and block comments, and we need to handle
  # strings that contain '//' or '/*'.
  # TODO(bulach): This is a bit hacky. It would be cleaner to use a real Java
  # parser. Maybe we could ditch JNIFromJavaSource and just always use
  # JNIFromJavaP; or maybe we could rewrite this script in Java and use APT.
  # http://code.google.com/p/chromium/issues/detail?id=138941
  def replacer(match):
    # Replace matches that are comments with nothing; return literals/strings
    # unchanged.
    s = match.group(0)
    if s.startswith('/'):
      return ''
    else:
      return s
  return _COMMENT_REMOVER_REGEX.sub(replacer, contents)


class JNIFromJavaP(object):
  """Uses 'javap' to parse a .class file and generate the JNI header file."""

  def __init__(self, contents, options):
    self.contents = contents
    self.namespace = options.namespace
    for line in contents:
      class_name = re.match(
          '.*?(public).*?(class|interface) (?P<class_name>\S+?)( |\Z)',
          line)
      if class_name:
        self.fully_qualified_class = class_name.group('class_name')
        break
    self.fully_qualified_class = self.fully_qualified_class.replace('.', '/')
    # Java 7's javap includes type parameters in output, like HashSet<T>. Strip
    # away the <...> and use the raw class name that Java 6 would've given us.
    self.fully_qualified_class = self.fully_qualified_class.split('<', 1)[0]
    self.jni_params = JniParams(self.fully_qualified_class)
    self.java_class_name = self.fully_qualified_class.split('/')[-1]
    if not self.namespace:
      self.namespace = 'JNI_' + self.java_class_name
    re_method = re.compile('(?P<prefix>.*?)(?P<return_type>\S+?) (?P<name>\w+?)'
                           '\((?P<params>.*?)\)')
    self.called_by_natives = []
    for lineno, content in enumerate(contents[2:], 2):
      match = re.match(re_method, content)
      if not match:
        continue
      self.called_by_natives += [CalledByNative(
          system_class=True,
          unchecked=False,
          static='static' in match.group('prefix'),
          java_class_name='',
          return_type=match.group('return_type').replace('.', '/'),
          name=match.group('name'),
          params=JniParams.Parse(match.group('params').replace('.', '/')),
          signature=JniParams.ParseJavaPSignature(contents[lineno + 1]))]
    re_constructor = re.compile('(.*?)public ' +
                                self.fully_qualified_class.replace('/', '.') +
                                '\((?P<params>.*?)\)')
    for lineno, content in enumerate(contents[2:], 2):
      match = re.match(re_constructor, content)
      if not match:
        continue
      self.called_by_natives += [CalledByNative(
          system_class=True,
          unchecked=False,
          static=False,
          java_class_name='',
          return_type=self.fully_qualified_class,
          name='Constructor',
          params=JniParams.Parse(match.group('params').replace('.', '/')),
          signature=JniParams.ParseJavaPSignature(contents[lineno + 1]),
          is_constructor=True)]
    self.called_by_natives = MangleCalledByNatives(self.jni_params,
                                                   self.called_by_natives, options.always_mangle)
    self.constant_fields = []
    re_constant_field = re.compile('.*?public static final int (?P<name>.*?);')
    re_constant_field_value = re.compile(
        '.*?Constant(Value| value): int (?P<value>(-*[0-9]+)?)')
    for lineno, content in enumerate(contents[2:], 2):
      match = re.match(re_constant_field, content)
      if not match:
        continue
      value = re.match(re_constant_field_value, contents[lineno + 2])
      if not value:
        value = re.match(re_constant_field_value, contents[lineno + 3])
      if value:
        self.constant_fields.append(
            ConstantField(name=match.group('name'),
                          value=value.group('value')))

    self.inl_header_file_generator = InlHeaderFileGenerator(
        self.namespace, self.fully_qualified_class, [], self.called_by_natives,
        self.constant_fields, self.jni_params, options)

  def GetContent(self):
    return self.inl_header_file_generator.GetContent()

  @staticmethod
  def CreateFromClass(class_file, options):
    class_name = os.path.splitext(os.path.basename(class_file))[0]
    p = subprocess.Popen(args=[options.javap, '-c', '-verbose',
                               '-s', class_name],
                         cwd=os.path.dirname(class_file),
                         stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE)
    stdout, _ = p.communicate()
    jni_from_javap = JNIFromJavaP(stdout.split('\n'), options)
    return jni_from_javap


class JNIFromJavaSource(object):
  """Uses the given java source file to generate the JNI header file."""

  def __init__(self, contents, fully_qualified_class, options):
    contents = RemoveComments(contents)
    self.jni_params = JniParams(fully_qualified_class)
    self.jni_params.ExtractImportsAndInnerClasses(contents)
    jni_namespace = ExtractJNINamespace(contents) or options.namespace
    natives = ExtractNatives(contents, options.ptr_type)
    called_by_natives = ExtractCalledByNatives(self.jni_params, contents, options.always_mangle)
    if len(natives) == 0 and len(called_by_natives) == 0:
      raise SyntaxError('Unable to find any JNI methods for %s.' %
                        fully_qualified_class)
    inl_header_file_generator = InlHeaderFileGenerator(
        jni_namespace, fully_qualified_class, natives, called_by_natives, [],
        self.jni_params, options)
    self.content = inl_header_file_generator.GetContent()

  def GetContent(self):
    return self.content

  @staticmethod
  def CreateFromFile(java_file_name, options):
    contents = file(java_file_name).read()
    fully_qualified_class = ExtractFullyQualifiedJavaClassName(java_file_name,
                                                               contents)
    return JNIFromJavaSource(contents, fully_qualified_class, options)


class HeaderFileGeneratorHelper(object):
  """Include helper methods for header generators."""

  def __init__(self, class_name, fully_qualified_class):
    self.class_name = class_name
    self.fully_qualified_class = fully_qualified_class

  def GetStubName(self, native):
    """Return the name of the stub function for this native method.

    Args:
      native: the native dictionary describing the method.

    Returns:
      A string with the stub function name (used by the JVM).
    """
    template = Template("Java_${JAVA_NAME}_native${NAME}")

    java_name = self.fully_qualified_class
    if native.java_class_name:
      java_name += '$' + native.java_class_name

    values = {'NAME': native.name,
              'JAVA_NAME': GetBinaryClassName(java_name)}
    return template.substitute(values)

  def GetUniqueClasses(self, origin):
    ret = {self.class_name: self.fully_qualified_class}
    for entry in origin:
      class_name = self.class_name
      jni_class_path = self.fully_qualified_class
      if entry.java_class_name:
        class_name = entry.java_class_name
        jni_class_path = self.fully_qualified_class + '$' + class_name
      ret[class_name] = jni_class_path
    return ret

  def GetClassPathLines(self, classes, declare_only=False):
    """Returns the ClassPath constants."""
    ret = []
    if declare_only:
      template = Template("""
extern const char kClassPath_${JAVA_CLASS}[];
""")
    else:
      template = Template("""
JNI_REGISTRATION_EXPORT extern const char kClassPath_${JAVA_CLASS}[];
const char kClassPath_${JAVA_CLASS}[] = \
"${JNI_CLASS_PATH}";
""")

    for full_clazz in classes.itervalues():
      values = {
          'JAVA_CLASS': GetBinaryClassName(full_clazz),
          'JNI_CLASS_PATH': full_clazz,
      }
      ret += [template.substitute(values)]

    class_getter = """\
#ifndef ${JAVA_CLASS}_clazz_defined
#define ${JAVA_CLASS}_clazz_defined
inline jclass ${JAVA_CLASS}_clazz(JNIEnv* env) {
  return base::android::LazyGetClass(env, kClassPath_${JAVA_CLASS}, \
&g_${JAVA_CLASS}_clazz);
}
#endif
"""
    if declare_only:
      template = Template("""\
extern base::subtle::AtomicWord g_${JAVA_CLASS}_clazz;
""" + class_getter)
    else:
      template = Template("""\
// Leaking this jclass as we cannot use LazyInstance from some threads.
JNI_REGISTRATION_EXPORT base::subtle::AtomicWord g_${JAVA_CLASS}_clazz = 0;
""" + class_getter)

    for full_clazz in classes.itervalues():
      values = {
          'JAVA_CLASS': GetBinaryClassName(full_clazz),
      }
      ret += [template.substitute(values)]

    return ''.join(ret)


class InlHeaderFileGenerator(object):
  """Generates an inline header file for JNI integration."""

  def __init__(self, namespace, fully_qualified_class, natives,
               called_by_natives, constant_fields, jni_params, options):
    self.namespace = namespace
    self.fully_qualified_class = fully_qualified_class
    self.class_name = self.fully_qualified_class.split('/')[-1]
    self.natives = natives
    self.called_by_natives = called_by_natives
    self.header_guard = fully_qualified_class.replace('/', '_') + '_JNI'
    self.constant_fields = constant_fields
    self.jni_params = jni_params
    self.options = options
    self.helper = HeaderFileGeneratorHelper(
        self.class_name, fully_qualified_class)


  def GetContent(self):
    """Returns the content of the JNI binding file."""
    template = Template("""\
// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.


// This file is autogenerated by
//     ${SCRIPT_NAME}
// For
//     ${FULLY_QUALIFIED_CLASS}

#ifndef ${HEADER_GUARD}
#define ${HEADER_GUARD}

#include <jni.h>

${INCLUDES}

// Step 1: Forward declarations.
$CLASS_PATH_DEFINITIONS

// Step 2: Constants (optional).

$CONSTANT_FIELDS\

// Step 3: Method stubs.
$METHOD_STUBS

#endif  // ${HEADER_GUARD}
""")
    values = {
        'SCRIPT_NAME': self.options.script_name,
        'FULLY_QUALIFIED_CLASS': self.fully_qualified_class,
        'CLASS_PATH_DEFINITIONS': self.GetClassPathDefinitionsString(),
        'CONSTANT_FIELDS': self.GetConstantFieldsString(),
        'METHOD_STUBS': self.GetMethodStubsString(),
        'HEADER_GUARD': self.header_guard,
        'INCLUDES': self.GetIncludesString(),
    }
    open_namespace = self.GetOpenNamespaceString()
    if open_namespace:
      close_namespace = self.GetCloseNamespaceString()
      values['METHOD_STUBS'] = '\n'.join([
            open_namespace, values['METHOD_STUBS'], close_namespace])

      constant_fields = values['CONSTANT_FIELDS']
      if constant_fields:
        values['CONSTANT_FIELDS'] = '\n'.join([
            open_namespace, constant_fields, close_namespace])

    return WrapOutput(template.substitute(values))

  def GetClassPathDefinitionsString(self):
    classes = self.helper.GetUniqueClasses(self.called_by_natives)
    classes.update(self.helper.GetUniqueClasses(self.natives))
    return self.helper.GetClassPathLines(classes)

  def GetConstantFieldsString(self):
    if not self.constant_fields:
      return ''
    ret = ['enum Java_%s_constant_fields {' % self.class_name]
    for c in self.constant_fields:
      ret += ['  %s = %s,' % (c.name, c.value)]
    ret += ['};', '']
    return '\n'.join(ret)

  def GetMethodStubsString(self):
    """Returns the code corresponding to method stubs."""
    ret = []
    for native in self.natives:
      ret += [self.GetNativeStub(native)]
    ret += self.GetLazyCalledByNativeMethodStubs()
    return '\n'.join(ret)

  def GetLazyCalledByNativeMethodStubs(self):
    return [self.GetLazyCalledByNativeMethodStub(called_by_native)
            for called_by_native in self.called_by_natives]

  def GetIncludesString(self):
    if not self.options.includes:
      return ''
    includes = self.options.includes.split(',')
    return '\n'.join('#include "%s"' % x for x in includes) + '\n'

  def GetOpenNamespaceString(self):
    if self.namespace:
      all_namespaces = ['namespace %s {' % ns
                        for ns in self.namespace.split('::')]
      return '\n'.join(all_namespaces) + '\n'
    return ''

  def GetCloseNamespaceString(self):
    if self.namespace:
      all_namespaces = ['}  // namespace %s' % ns
                        for ns in self.namespace.split('::')]
      all_namespaces.reverse()
      return '\n' + '\n'.join(all_namespaces)
    return ''

  def GetCalledByNativeParamsInDeclaration(self, called_by_native):
    return ',\n    '.join([
        JavaDataTypeToCForCalledByNativeParam(param.datatype) + ' ' +
        param.name
        for param in called_by_native.params])

  def GetJavaParamRefForCall(self, c_type, name):
    return Template(
        'base::android::JavaParamRef<${TYPE}>(env, ${NAME})').substitute({
        'TYPE': c_type,
        'NAME': name,
    })

  def GetJNIFirstParamForCall(self, native):
    c_type = _GetJNIFirstParamType(native)
    return [self.GetJavaParamRefForCall(c_type, 'jcaller')]

  def GetImplementationMethodName(self, native):
    class_name = self.class_name
    if native.java_class_name is not None:
      # Inner class
      class_name = native.java_class_name
    return "JNI_%s_%s" % (class_name, native.name)

  def GetNativeStub(self, native):
    is_method = native.type == 'method'

    if is_method:
      params = native.params[1:]
    else:
      params = native.params
    params_in_call = ['env'] + self.GetJNIFirstParamForCall(native)
    for p in params:
      c_type = JavaDataTypeToC(p.datatype)
      if re.match(RE_SCOPED_JNI_TYPES, c_type):
        params_in_call.append(self.GetJavaParamRefForCall(c_type, p.name))
      else:
        params_in_call.append(p.name)
    params_in_call = ', '.join(params_in_call)

    return_type = return_declaration = JavaDataTypeToC(native.return_type)
    post_call = ''
    if re.match(RE_SCOPED_JNI_TYPES, return_type):
      post_call = '.Release()'
      return_declaration = ('base::android::ScopedJavaLocalRef<' + return_type +
                            '>')
    profiling_entered_native = ''
    if self.options.enable_profiling:
      profiling_entered_native = '  JNI_LINK_SAVED_FRAME_POINTER;\n'
    values = {
        'RETURN': return_type,
        'RETURN_DECLARATION': return_declaration,
        'NAME': native.name,
        'IMPL_METHOD_NAME': self.GetImplementationMethodName(native),
        'PARAMS': _GetParamsInDeclaration(native),
        'PARAMS_IN_STUB': GetParamsInStub(native),
        'PARAMS_IN_CALL': params_in_call,
        'POST_CALL': post_call,
        'STUB_NAME': self.helper.GetStubName(native),
        'PROFILING_ENTERED_NATIVE': profiling_entered_native,
        'TRACE_EVENT': '',
    }

    namespace_qual = self.namespace + '::' if self.namespace else ''
    if is_method:
      optional_error_return = JavaReturnValueToC(native.return_type)
      if optional_error_return:
        optional_error_return = ', ' + optional_error_return
      values.update({
          'OPTIONAL_ERROR_RETURN': optional_error_return,
          'PARAM0_NAME': native.params[0].name,
          'P0_TYPE': native.p0_type,
      })
      if self.options.enable_tracing:
        values['TRACE_EVENT'] = self.GetTraceEventForNameTemplate(
            namespace_qual + '${P0_TYPE}::${NAME}', values);
      template = Template("""\
JNI_GENERATOR_EXPORT ${RETURN} ${STUB_NAME}(
    JNIEnv* env,
    ${PARAMS_IN_STUB}) {
${PROFILING_ENTERED_NATIVE}\
${TRACE_EVENT}\
  ${P0_TYPE}* native = reinterpret_cast<${P0_TYPE}*>(${PARAM0_NAME});
  CHECK_NATIVE_PTR(env, jcaller, native, "${NAME}"${OPTIONAL_ERROR_RETURN});
  return native->${NAME}(${PARAMS_IN_CALL})${POST_CALL};
}
""")
    else:
      if self.options.enable_tracing:
        values['TRACE_EVENT'] = self.GetTraceEventForNameTemplate(
            namespace_qual + '${IMPL_METHOD_NAME}', values)
      template = Template("""\
static ${RETURN_DECLARATION} ${IMPL_METHOD_NAME}(JNIEnv* env, ${PARAMS});

JNI_GENERATOR_EXPORT ${RETURN} ${STUB_NAME}(
    JNIEnv* env,
    ${PARAMS_IN_STUB}) {
${PROFILING_ENTERED_NATIVE}\
${TRACE_EVENT}\
  return ${IMPL_METHOD_NAME}(${PARAMS_IN_CALL})${POST_CALL};
}
""")

    return RemoveIndentedEmptyLines(template.substitute(values))

  def GetArgument(self, param):
    if param.datatype == 'int':
      return 'as_jint(' + param.name + ')'
    elif re.match(RE_SCOPED_JNI_TYPES, JavaDataTypeToC(param.datatype)):
      return param.name + '.obj()'
    else:
      return param.name

  def GetArgumentsInCall(self, params):
    """Return a string of arguments to call from native into Java"""
    return [self.GetArgument(p) for p in params]

  def GetCalledByNativeValues(self, called_by_native):
    """Fills in necessary values for the CalledByNative methods."""
    java_class_only = called_by_native.java_class_name or self.class_name
    java_class = self.fully_qualified_class
    if called_by_native.java_class_name:
      java_class += '$' + called_by_native.java_class_name

    if called_by_native.static or called_by_native.is_constructor:
      first_param_in_declaration = ''
      first_param_in_call = ('%s_clazz(env)' % GetBinaryClassName(java_class))
    else:
      first_param_in_declaration = (
          ', const base::android::JavaRef<jobject>& obj')
      first_param_in_call = 'obj.obj()'
    params_in_declaration = self.GetCalledByNativeParamsInDeclaration(
        called_by_native)
    if params_in_declaration:
      params_in_declaration = ', ' + params_in_declaration
    params_in_call = ', '.join(self.GetArgumentsInCall(called_by_native.params))
    if params_in_call:
      params_in_call = ', ' + params_in_call
    pre_call = ''
    post_call = ''
    if called_by_native.static_cast:
      pre_call = 'static_cast<%s>(' % called_by_native.static_cast
      post_call = ')'
    check_exception = ''
    if not called_by_native.unchecked:
      check_exception = 'jni_generator::CheckException(env);'
    return_type = JavaDataTypeToC(called_by_native.return_type)
    optional_error_return = JavaReturnValueToC(called_by_native.return_type)
    if optional_error_return:
      optional_error_return = ', ' + optional_error_return
    return_declaration = ''
    return_clause = ''
    if return_type != 'void':
      pre_call = ' ' + pre_call
      return_declaration = return_type + ' ret ='
      if re.match(RE_SCOPED_JNI_TYPES, return_type):
        return_type = 'base::android::ScopedJavaLocalRef<' + return_type + '>'
        return_clause = 'return ' + return_type + '(env, ret);'
      else:
        return_clause = 'return ret;'
    profiling_leaving_native = ''
    if self.options.enable_profiling:
      profiling_leaving_native = '  JNI_SAVE_FRAME_POINTER;\n'
    jni_name = called_by_native.name
    jni_return_type = called_by_native.return_type
    if called_by_native.is_constructor:
      jni_name = '<init>'
      jni_return_type = 'void'
    if called_by_native.signature:
      jni_signature = called_by_native.signature
    else:
      jni_signature = self.jni_params.Signature(
          called_by_native.params, jni_return_type)
    java_name_full = java_class.replace('/', '.') + '.' + jni_name
    return {
        'JAVA_CLASS_ONLY': java_class_only,
        'JAVA_CLASS': GetBinaryClassName(java_class),
        'RETURN_TYPE': return_type,
        'OPTIONAL_ERROR_RETURN': optional_error_return,
        'RETURN_DECLARATION': return_declaration,
        'RETURN_CLAUSE': return_clause,
        'FIRST_PARAM_IN_DECLARATION': first_param_in_declaration,
        'PARAMS_IN_DECLARATION': params_in_declaration,
        'PRE_CALL': pre_call,
        'POST_CALL': post_call,
        'ENV_CALL': called_by_native.env_call,
        'FIRST_PARAM_IN_CALL': first_param_in_call,
        'PARAMS_IN_CALL': params_in_call,
        'CHECK_EXCEPTION': check_exception,
        'PROFILING_LEAVING_NATIVE': profiling_leaving_native,
        'JNI_NAME': jni_name,
        'JNI_SIGNATURE': jni_signature,
        'METHOD_ID_VAR_NAME': called_by_native.method_id_var_name,
        'METHOD_ID_TYPE': 'STATIC' if called_by_native.static else 'INSTANCE',
        'JAVA_NAME_FULL': java_name_full,
    }

  def GetLazyCalledByNativeMethodStub(self, called_by_native):
    """Returns a string."""
    function_signature_template = Template("""\
static ${RETURN_TYPE} Java_${JAVA_CLASS_ONLY}_${METHOD_ID_VAR_NAME}(\
JNIEnv* env${FIRST_PARAM_IN_DECLARATION}${PARAMS_IN_DECLARATION})""")
    function_header_template = Template("""\
${FUNCTION_SIGNATURE} {""")
    function_header_with_unused_template = Template("""\
${FUNCTION_SIGNATURE} __attribute__ ((unused));
${FUNCTION_SIGNATURE} {""")
    template = Template("""
static base::subtle::AtomicWord g_${JAVA_CLASS}_${METHOD_ID_VAR_NAME} = 0;
${FUNCTION_HEADER}
  CHECK_CLAZZ(env, ${FIRST_PARAM_IN_CALL},
      ${JAVA_CLASS}_clazz(env)${OPTIONAL_ERROR_RETURN});
  jmethodID method_id = base::android::MethodID::LazyGet<
      base::android::MethodID::TYPE_${METHOD_ID_TYPE}>(
          env, ${JAVA_CLASS}_clazz(env),
          "${JNI_NAME}",
          ${JNI_SIGNATURE},
          &g_${JAVA_CLASS}_${METHOD_ID_VAR_NAME});

${TRACE_EVENT}\
${PROFILING_LEAVING_NATIVE}\
  ${RETURN_DECLARATION}
     ${PRE_CALL}env->${ENV_CALL}(${FIRST_PARAM_IN_CALL},
          method_id${PARAMS_IN_CALL})${POST_CALL};
  ${CHECK_EXCEPTION}
  ${RETURN_CLAUSE}
}""")
    values = self.GetCalledByNativeValues(called_by_native)
    values['FUNCTION_SIGNATURE'] = (
        function_signature_template.substitute(values))
    if called_by_native.system_class:
      values['FUNCTION_HEADER'] = (
          function_header_with_unused_template.substitute(values))
    else:
      values['FUNCTION_HEADER'] = function_header_template.substitute(values)
    if self.options.enable_tracing:
      values['TRACE_EVENT'] = self.GetTraceEventForNameTemplate(
          '${JAVA_NAME_FULL}', values)
    else:
      values['TRACE_EVENT'] = ''
    return RemoveIndentedEmptyLines(template.substitute(values))

  def GetTraceEventForNameTemplate(self, name_template, values):
    name = Template(name_template).substitute(values)
    return '  TRACE_EVENT0("jni", "%s");' % name


def WrapOutput(output):
  ret = []
  for line in output.splitlines():
    # Do not wrap preprocessor directives or comments.
    if len(line) < _WRAP_LINE_LENGTH or line[0] == '#' or line.startswith('//'):
      ret.append(line)
    else:
      # Assumes that the line is not already indented as a continuation line,
      # which is not always true (oh well).
      first_line_indent = (len(line) - len(line.lstrip()))
      wrapper = _WRAPPERS_BY_INDENT[first_line_indent]
      ret.extend(wrapper.wrap(line))
  ret += ['']
  return '\n'.join(ret)


def ExtractJarInputFile(jar_file, input_file, out_dir):
  """Extracts input file from jar and returns the filename.

  The input file is extracted to the same directory that the generated jni
  headers will be placed in.  This is passed as an argument to script.

  Args:
    jar_file: the jar file containing the input files to extract.
    input_files: the list of files to extract from the jar file.
    out_dir: the name of the directories to extract to.

  Returns:
    the name of extracted input file.
  """
  jar_file = zipfile.ZipFile(jar_file)

  out_dir = os.path.join(out_dir, os.path.dirname(input_file))
  try:
    os.makedirs(out_dir)
  except OSError as e:
    if e.errno != errno.EEXIST:
      raise
  extracted_file_name = os.path.join(out_dir, os.path.basename(input_file))
  with open(extracted_file_name, 'w') as outfile:
    outfile.write(jar_file.read(input_file))

  return extracted_file_name


def GenerateJNIHeader(input_file, output_file, options):
  try:
    if os.path.splitext(input_file)[1] == '.class':
      jni_from_javap = JNIFromJavaP.CreateFromClass(input_file, options)
      content = jni_from_javap.GetContent()
    else:
      jni_from_java_source = JNIFromJavaSource.CreateFromFile(
          input_file, options)
      content = jni_from_java_source.GetContent()
  except ParseError, e:
    print e
    sys.exit(1)
  if output_file:
    WriteOutput(output_file, content)
  else:
    print content


def WriteOutput(output_file, content):
  if os.path.exists(output_file):
    with open(output_file) as f:
      existing_content = f.read()
      if existing_content == content:
        return
  with open(output_file, 'w') as f:
    f.write(content)


def GetScriptName():
  script_components = os.path.abspath(sys.argv[0]).split(os.path.sep)
  base_index = 0
  for idx, value in enumerate(script_components):
    if value == 'base' or value == 'third_party':
      base_index = idx
      break
  return os.sep.join(script_components[base_index:])


def main(argv):
  usage = """usage: %prog [OPTIONS]
This script will parse the given java source code extracting the native
declarations and print the header file to stdout (or a file).
See SampleForTests.java for more details.
  """
  option_parser = optparse.OptionParser(usage=usage)
  build_utils.AddDepfileOption(option_parser)

  option_parser.add_option('-j', '--jar_file', dest='jar_file',
                           help='Extract the list of input files from'
                           ' a specified jar file.'
                           ' Uses javap to extract the methods from a'
                           ' pre-compiled class. --input should point'
                           ' to pre-compiled Java .class files.')
  option_parser.add_option('-n', dest='namespace',
                           help='Uses as a namespace in the generated header '
                           'instead of the javap class name, or when there is '
                           'no JNINamespace annotation in the java source.')
  option_parser.add_option('--input_file',
                           help='Single input file name. The output file name '
                           'will be derived from it. Must be used with '
                           '--output_dir.')
  option_parser.add_option('--output_dir',
                           help='The output directory. Must be used with '
                           '--input')
  option_parser.add_option('--script_name', default=GetScriptName(),
                           help='The name of this script in the generated '
                           'header.')
  option_parser.add_option('--includes',
                           help='The comma-separated list of header files to '
                           'include in the generated header.')
  option_parser.add_option('--ptr_type', default='int',
                           type='choice', choices=['int', 'long'],
                           help='The type used to represent native pointers in '
                           'Java code. For 32-bit, use int; '
                           'for 64-bit, use long.')
  option_parser.add_option('--cpp', default='cpp',
                           help='The path to cpp command.')
  option_parser.add_option('--javap', default='javap',
                           help='The path to javap command.')
  option_parser.add_option('--enable_profiling', action='store_true',
                           help='Add additional profiling instrumentation.')
  option_parser.add_option('--enable_tracing', action='store_true',
                           help='Add TRACE_EVENTs to generated functions.')
  option_parser.add_option('--always_mangle', action='store_true',
                           help='Mangle all function names')
  options, args = option_parser.parse_args(argv)
  if options.jar_file:
    input_file = ExtractJarInputFile(options.jar_file, options.input_file,
                                     options.output_dir)
  elif options.input_file:
    input_file = options.input_file
  else:
    option_parser.print_help()
    print '\nError: Must specify --jar_file or --input_file.'
    return 1
  output_file = None
  if options.output_dir:
    root_name = os.path.splitext(os.path.basename(input_file))[0]
    output_file = os.path.join(options.output_dir, root_name) + '_jni.h'
  GenerateJNIHeader(input_file, output_file, options)

  if options.depfile:
    build_utils.WriteDepfile(options.depfile, output_file)


if __name__ == '__main__':
  sys.exit(main(sys.argv))
