# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

""" Set of basic operations/utilities that are used by the build. """

from contextlib import contextmanager
import ast
import cStringIO
import copy
import errno
import fnmatch
import glob
import json
import os
import re
import shutil
import socket
import stat
import subprocess
import sys
import threading
import time
import traceback


BUILD_DIR = os.path.realpath(os.path.join(
    os.path.dirname(__file__), os.pardir, os.pardir))


# Local errors.
class MissingArgument(Exception):
  pass
class PathNotFound(Exception):
  pass
class ExternalError(Exception):
  pass

def IsWindows():
  return sys.platform == 'cygwin' or sys.platform.startswith('win')

def IsLinux():
  return sys.platform.startswith('linux')

def IsMac():
  return sys.platform.startswith('darwin')


def convert_json(option, _, value, parser):
  """Provide an OptionParser callback to unmarshal a JSON string."""
  setattr(parser.values, option.dest, json.loads(value))


def AddPropertiesOptions(option_parser):
  """Registers command line options for parsing build and factory properties.

  After parsing, the options object will have the 'build_properties' and
  'factory_properties' attributes. The corresponding values will be python
  dictionaries containing the properties. If the options are not given on
  the command line, the dictionaries will be empty.

  Args:
    option_parser: An optparse.OptionParser to register command line options
                   for build and factory properties.
  """
  option_parser.add_option('--build-properties', action='callback',
                           callback=convert_json, type='string',
                           nargs=1, default={},
                           help='build properties in JSON format')
  option_parser.add_option('--factory-properties', action='callback',
                           callback=convert_json, type='string',
                           nargs=1, default={},
                           help='factory properties in JSON format')
