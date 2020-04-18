#!/usr/bin/env python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Generates C++ source and header files defining function to create an
in-memory representation of a static catalog manifest at runtime."""


import argparse
import imp
import json
import os.path
import sys


_H_FILE_TEMPLATE = "catalog.h.tmpl"
_CC_FILE_TEMPLATE = "catalog.cc.tmpl"


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
  sys.path.append(os.path.join(_GetDirAbove("services"), "third_party"))
import jinja2


def ApplyTemplate(path_to_template, output_path, global_vars, **kwargs):
  def make_ascii(maybe_unicode):
    if type(maybe_unicode) is str:
      return maybe_unicode
    assert type(maybe_unicode) is unicode
    return maybe_unicode.encode("ascii", "ignore")

  with open(output_path, "w") as output_file:
    jinja_env = jinja2.Environment(
        loader=jinja2.FileSystemLoader(os.path.dirname(__file__)),
        keep_trailing_newline=True, **kwargs)
    jinja_env.globals.update(global_vars)
    jinja_env.filters.update({
      "is_dict": lambda x : type(x) is dict,
      "is_list": lambda x : type(x) is list,
      "is_number": lambda x : type(x) is int or type(x) is float,
      "is_bool": lambda x: type(x) is bool,
      "is_string": lambda x: type(x) is str,
      "is_unicode": lambda x: type(x) is unicode,
      "make_ascii": make_ascii,
    })
    output_file.write(jinja_env.get_template(path_to_template).render())


def main():
  parser = argparse.ArgumentParser(
      description="Generates a C++ constant containing a catalog manifest.")
  parser.add_argument("--input")
  parser.add_argument("--output-filename-base")
  parser.add_argument("--output-function-name")
  parser.add_argument("--module-path")
  args, _ = parser.parse_known_args()

  if args.input is None:
    raise Exception("--input is required")
  if args.output_filename_base is None:
    raise Exception("--output-filename-base is required")
  if args.output_function_name is None:
    raise Exception("--output-function-name is required")
  if args.module_path is None:
    raise Exception("--module-path is required")

  with open(args.input, "r") as input_file:
    catalog = json.load(input_file)

  qualified_function_name = args.output_function_name.split("::")
  namespaces = qualified_function_name[0:-1]
  function_name = qualified_function_name[-1]

  def raise_error(error, value):
    raise Exception(error)

  global_vars = {
    "catalog": catalog,
    "function_name": function_name,
    "namespaces": namespaces,
    "path": args.module_path,
    "raise": raise_error,
  }

  input_h_filename = _H_FILE_TEMPLATE
  output_h_filename = "%s.h" % args.output_filename_base
  ApplyTemplate(input_h_filename, output_h_filename, global_vars)

  input_cc_filename = _CC_FILE_TEMPLATE
  output_cc_filename = "%s.cc" % args.output_filename_base
  ApplyTemplate(input_cc_filename, output_cc_filename, global_vars)

  return 0

if __name__ == "__main__":
  sys.exit(main())
