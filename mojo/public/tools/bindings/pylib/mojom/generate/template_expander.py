# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Based on third_party/WebKit/Source/build/scripts/template_expander.py.

import imp
import os.path
import sys

# Disable lint check for finding modules:
# pylint: disable=F0401

def _GetDirAbove(dirname):
  """Returns the directory "above" this file containing |dirname| (which must
  also be "above" this file)."""
  path = os.path.abspath(__file__)
  while True:
    path, tail = os.path.split(path)
    assert tail
    if tail == dirname:
      return path

try:
  imp.find_module("jinja2")
except ImportError:
  sys.path.append(os.path.join(_GetDirAbove("mojo"), "third_party"))
import jinja2


def ApplyTemplate(mojo_generator, path_to_template, params, **kwargs):
  loader = jinja2.ModuleLoader(os.path.join(
      mojo_generator.bytecode_path, "%s.zip" % mojo_generator.GetTemplatePrefix(
      )))
  final_kwargs = dict(mojo_generator.GetJinjaParameters())
  final_kwargs.update(kwargs)
  jinja_env = jinja2.Environment(loader=loader,
                                 keep_trailing_newline=True,
                                 **final_kwargs)
  jinja_env.globals.update(mojo_generator.GetGlobals())
  jinja_env.filters.update(mojo_generator.GetFilters())
  template = jinja_env.get_template(path_to_template)
  return template.render(params)


def UseJinja(path_to_template, **kwargs):
  def RealDecorator(generator):
    def GeneratorInternal(*args, **kwargs2):
      parameters = generator(*args, **kwargs2)
      return ApplyTemplate(args[0], path_to_template, parameters, **kwargs)
    GeneratorInternal.func_name = generator.func_name
    return GeneratorInternal
  return RealDecorator


def PrecompileTemplates(generator_modules, output_dir):
  for module in generator_modules.values():
    generator = module.Generator(None)
    jinja_env = jinja2.Environment(loader=jinja2.FileSystemLoader([os.path.join(
        os.path.dirname(module.__file__), generator.GetTemplatePrefix())]))
    jinja_env.filters.update(generator.GetFilters())
    jinja_env.compile_templates(
        os.path.join(output_dir, "%s.zip" % generator.GetTemplatePrefix()),
        extensions=["tmpl"],
        zip="stored",
        py_compile=True,
        ignore_errors=False)
