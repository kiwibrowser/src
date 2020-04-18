#!/usr/bin/python
# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Convert GN Xcode projects to platform and configuration independent targets.

GN generates Xcode projects that build one configuration only. However, typical
iOS development involves using the Xcode IDE to toggle the platform and
configuration. This script replaces the 'gn' configuration with 'Debug',
'Release' and 'Profile', and changes the ninja invokation to honor these
configurations.
"""

import argparse
import collections
import copy
import filecmp
import json
import hashlib
import os
import plistlib
import random
import shutil
import subprocess
import sys
import tempfile


XCTEST_PRODUCT_TYPE = 'com.apple.product-type.bundle.unit-test'


class XcodeProject(object):

  def __init__(self, objects, counter = 0):
    self.objects = objects
    self.counter = 0

  def AddObject(self, parent_name, obj):
    while True:
      self.counter += 1
      str_id = "%s %s %d" % (parent_name, obj['isa'], self.counter)
      new_id = hashlib.sha1(str_id).hexdigest()[:24].upper()

      # Make sure ID is unique. It's possible there could be an id conflict
      # since this is run after GN runs.
      if new_id not in self.objects:
        self.objects[new_id] = obj
        return new_id


def CopyFileIfChanged(source_path, target_path):
  """Copy |source_path| to |target_path| is different."""
  target_dir = os.path.dirname(target_path)
  if not os.path.isdir(target_dir):
    os.makedirs(target_dir)
  if not os.path.exists(target_path) or \
      not filecmp.cmp(source_path, target_path):
    shutil.copyfile(source_path, target_path)


def LoadXcodeProjectAsJSON(path):
  """Return Xcode project at |path| as a JSON string."""
  return subprocess.check_output([
      'plutil', '-convert', 'json', '-o', '-', path])


def WriteXcodeProject(output_path, json_string):
  """Save Xcode project to |output_path| as XML."""
  with tempfile.NamedTemporaryFile() as temp_file:
    temp_file.write(json_string)
    temp_file.flush()
    subprocess.check_call(['plutil', '-convert', 'xml1', temp_file.name])
    CopyFileIfChanged(temp_file.name, output_path)


def UpdateProductsProject(file_input, file_output, configurations):
  """Update Xcode project to support multiple configurations.

  Args:
    file_input: path to the input Xcode project
    file_output: path to the output file
    configurations: list of string corresponding to the configurations that
      need to be supported by the tweaked Xcode projects, must contains at
      least one value.
  """
  json_data = json.loads(LoadXcodeProjectAsJSON(file_input))
  project = XcodeProject(json_data['objects'])

  objects_to_remove = []
  for value in project.objects.values():
    isa = value['isa']

    # TODO(crbug.com/619072): gn does not write the min deployment target in the
    # generated Xcode project, so add it while doing the conversion, only if it
    # is not present. Remove this code and comment once the bug is fixed and gn
    # has rolled past it.
    if isa == 'XCBuildConfiguration':
      build_settings = value['buildSettings']
      if 'IPHONEOS_DEPLOYMENT_TARGET' not in build_settings:
        build_settings['IPHONEOS_DEPLOYMENT_TARGET'] = '9.0'

    # Teach build shell script to look for the configuration and platform.
    if isa == 'PBXShellScriptBuildPhase':
      value['shellScript'] = value['shellScript'].replace(
          'ninja -C .',
          'ninja -C "../${CONFIGURATION}${EFFECTIVE_PLATFORM_NAME}"')

    # Configure BUNDLE_LOADER and TEST_HOST for xctest targets (if not yet
    # configured by gn). Old convention was to name the test dynamic module
    # "foo" and the host "foo_host" while the new convention is to name the
    # test "foo_module" and the host "foo". Decide which convention to use
    # by inspecting the target name.
    if isa == 'PBXNativeTarget' and value['productType'] == XCTEST_PRODUCT_TYPE:
      configuration_list = project.objects[value['buildConfigurationList']]
      for config_name in configuration_list['buildConfigurations']:
        config = project.objects[config_name]
        if not config['buildSettings'].get('BUNDLE_LOADER'):
          assert value['name'].endswith('_module')
          host_name = value['name'][:-len('_module')]
          config['buildSettings']['BUNDLE_LOADER'] = '$(TEST_HOST)'
          config['buildSettings']['TEST_HOST'] = \
              '${BUILT_PRODUCTS_DIR}/%s.app/%s' % (host_name, host_name)

    # Add new configuration, using the first one as default.
    if isa == 'XCConfigurationList':
      value['defaultConfigurationName'] = configurations[0]
      objects_to_remove.extend(value['buildConfigurations'])

      build_config_template = project.objects[value['buildConfigurations'][0]]
      build_config_template['buildSettings']['CONFIGURATION_BUILD_DIR'] = \
          '$(PROJECT_DIR)/../$(CONFIGURATION)$(EFFECTIVE_PLATFORM_NAME)'

      value['buildConfigurations'] = []
      for configuration in configurations:
        new_build_config = copy.copy(build_config_template)
        new_build_config['name'] = configuration
        value['buildConfigurations'].append(
            project.AddObject('products', new_build_config))

  for object_id in objects_to_remove:
    del project.objects[object_id]

  objects = collections.OrderedDict(sorted(project.objects.iteritems()))
  WriteXcodeProject(file_output, json.dumps(json_data))


def ConvertGnXcodeProject(input_dir, output_dir, configurations):
  '''Tweak the Xcode project generated by gn to support multiple configurations.

  The Xcode projects generated by "gn gen --ide" only supports a single
  platform and configuration (as the platform and configuration are set
  per output directory). This method takes as input such projects and
  add support for multiple configurations and platforms (to allow devs
  to select them in Xcode).

  Args:
    input_dir: directory containing the XCode projects created by "gn gen --ide"
    output_dir: directory where the tweaked Xcode projects will be saved
    configurations: list of string corresponding to the configurations that
      need to be supported by the tweaked Xcode projects, must contains at
      least one value.
  '''
  # Update products project.
  products = os.path.join('products.xcodeproj', 'project.pbxproj')
  product_input = os.path.join(input_dir, products)
  product_output = os.path.join(output_dir, products)
  UpdateProductsProject(product_input, product_output, configurations)

  # Copy all workspace.
  xcwspace = os.path.join('all.xcworkspace', 'contents.xcworkspacedata')
  CopyFileIfChanged(os.path.join(input_dir, xcwspace),
                    os.path.join(output_dir, xcwspace))
  # TODO(crbug.com/679110): gn has been modified to remove 'sources.xcodeproj'
  # and keep 'all.xcworkspace' and 'products.xcodeproj'. The following code is
  # here to support both old and new projects setup and will be removed once gn
  # has rolled past it.
  sources = os.path.join('sources.xcodeproj', 'project.pbxproj')
  if os.path.isfile(os.path.join(input_dir, sources)):
    CopyFileIfChanged(os.path.join(input_dir, sources),
                      os.path.join(output_dir, sources))

def Main(args):
  parser = argparse.ArgumentParser(
      description='Convert GN Xcode projects for iOS.')
  parser.add_argument(
      'input',
      help='directory containing [product|all] Xcode projects.')
  parser.add_argument(
      'output',
      help='directory where to generate the iOS configuration.')
  parser.add_argument(
      '--add-config', dest='configurations', default=[], action='append',
      help='configuration to add to the Xcode project')
  args = parser.parse_args(args)

  if not os.path.isdir(args.input):
    sys.stderr.write('Input directory does not exists.\n')
    return 1

  required = set(['products.xcodeproj', 'all.xcworkspace'])
  if not required.issubset(os.listdir(args.input)):
    sys.stderr.write(
        'Input directory does not contain all necessary Xcode projects.\n')
    return 1

  if not args.configurations:
    sys.stderr.write('At least one configuration required, see --add-config.\n')
    return 1

  ConvertGnXcodeProject(args.input, args.output, args.configurations)

if __name__ == '__main__':
  sys.exit(Main(sys.argv[1:]))


