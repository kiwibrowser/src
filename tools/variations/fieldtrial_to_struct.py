#!/usr/bin/env python
# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import itertools
import json
import os.path
import sys
import optparse
_script_path = os.path.realpath(__file__)

sys.path.insert(0, os.path.normpath(_script_path + "/../../json_comment_eater"))
try:
  import json_comment_eater
finally:
  sys.path.pop(0)

sys.path.insert(0, os.path.normpath(_script_path + "/../../json_to_struct"))
try:
  import json_to_struct
finally:
  sys.path.pop(0)

def _Load(filename):
  """Loads a JSON file into a Python object and return this object.
  """
  with open(filename, 'r') as handle:
    result = json.loads(json_comment_eater.Nom(handle.read()))
  return result

def _LoadFieldTrialConfig(filename, platform):
  """Loads a field trial config JSON and converts it into a format that can be
  used by json_to_struct.
  """
  return _FieldTrialConfigToDescription(_Load(filename), platform)

def _CreateExperiment(experiment_data):
  experiment = {'name': experiment_data['name']}
  forcing_flags_data = experiment_data.get('forcing_flag')
  if forcing_flags_data:
    experiment['forcing_flag'] = forcing_flags_data
  params_data = experiment_data.get('params')
  if (params_data):
    experiment['params'] = [{'key': param, 'value': params_data[param]}
                          for param in sorted(params_data.keys())];
  enable_features_data = experiment_data.get('enable_features')
  if enable_features_data:
    experiment['enable_features'] = enable_features_data
  disable_features_data = experiment_data.get('disable_features')
  if disable_features_data:
    experiment['disable_features'] = disable_features_data
  return experiment

def _CreateTrial(study_name, experiment_configs, platform):
  """Returns the applicable experiments for |study_name| and |platform|. This
  iterates through all of the experiment_configs for |study_name| and picks out
  the applicable experiments based off of the valid platforms.
  """
  platform_experiment_lists = [
      config['experiments'] for config in experiment_configs
      if platform in config['platforms']]
  platform_experiments = list(itertools.chain.from_iterable(
      platform_experiment_lists))
  return {
    'name': study_name,
    'experiments': [_CreateExperiment(experiment)
                    for experiment in platform_experiments],
  }

def _GenerateTrials(config, platform):
  for study_name in sorted(config.keys()):
    study = _CreateTrial(study_name, config[study_name], platform)
    # To avoid converting studies with empty experiments (e.g. the study doesn't
    # apply to the target platform), this generator only yields studies that
    # have non-empty experiments.
    if study['experiments']:
      yield study

def ConfigToStudies(config, platform):
  """Returns the applicable studies from config for the platform."""
  return [study for study in _GenerateTrials(config, platform)]

def _FieldTrialConfigToDescription(config, platform):
  return {
    'elements': {
      'kFieldTrialConfig': {
        'studies': ConfigToStudies(config, platform)
      }
    }
  }

def main(arguments):
  parser = optparse.OptionParser(
      description='Generates a struct from a JSON description.',
      usage='usage: %prog [option] -s schema -p platform description')
  parser.add_option('-b', '--destbase',
      help='base directory of generated files.')
  parser.add_option('-d', '--destdir',
      help='directory to output generated files, relative to destbase.')
  parser.add_option('-n', '--namespace',
      help='C++ namespace for generated files. e.g search_providers.')
  parser.add_option('-p', '--platform',
      help='target platform for the field trial, mandatory.')
  parser.add_option('-s', '--schema', help='path to the schema file, '
      'mandatory.')
  parser.add_option('-o', '--output', help='output filename, '
      'mandatory.')
  parser.add_option('-y', '--year',
      help='year to put in the copy-right.')
  (opts, args) = parser.parse_args(args=arguments)

  if not opts.schema:
    parser.error('You must specify a --schema.')

  if not opts.platform:
    parser.error('You must specify a --platform.')

  supported_platforms = ['android', 'chromeos', 'fuchsia', 'ios', 'linux',
                         'mac', 'win']
  if opts.platform not in supported_platforms:
    parser.error('\'%s\' is an unknown platform. Supported platforms: %s' %
        (opts.platform, supported_platforms))

  description_filename = os.path.normpath(args[0])
  shortroot = opts.output
  if opts.destdir:
    output_root = os.path.join(os.path.normpath(opts.destdir), shortroot)
  else:
    output_root = shortroot

  if opts.destbase:
    basepath = os.path.normpath(opts.destbase)
  else:
    basepath = ''

  schema = _Load(opts.schema)
  description = _LoadFieldTrialConfig(description_filename, opts.platform)
  json_to_struct.GenerateStruct(
      basepath, output_root, opts.namespace, schema, description,
      os.path.split(description_filename)[1], os.path.split(opts.schema)[1],
      opts.year)

if __name__ == '__main__':
  main(sys.argv[1:])
