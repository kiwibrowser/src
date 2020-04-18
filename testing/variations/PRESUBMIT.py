# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Presubmit script validating field trial configs.

See http://dev.chromium.org/developers/how-tos/depottools/presubmit-scripts
for more details on the presubmit API built into depot_tools.
"""

import copy
import json
import sys

from collections import OrderedDict

VALID_EXPERIMENT_KEYS = ['name',
                         'forcing_flag',
                         'params',
                         'enable_features',
                         'disable_features',
                         '//0',
                         '//1',
                         '//2',
                         '//3',
                         '//4',
                         '//5',
                         '//6',
                         '//7',
                         '//8',
                         '//9']

def PrettyPrint(contents):
  """Pretty prints a fieldtrial configuration.

  Args:
    contents: File contents as a string.

  Returns:
    Pretty printed file contents.
  """

  # We have a preferred ordering of the fields (e.g. platforms on top). This
  # code loads everything into OrderedDicts and then tells json to dump it out.
  # The JSON dumper will respect the dict ordering.
  #
  # The ordering is as follows:
  # {
  #     'StudyName Alphabetical': [
  #         {
  #             'platforms': [sorted platforms]
  #             'groups': [
  #                 {
  #                     name: ...
  #                     forcing_flag: "forcing flag string"
  #                     params: {sorted dict}
  #                     enable_features: [sorted features]
  #                     disable_features: [sorted features]
  #                     (Unexpected extra keys will be caught by the validator)
  #                 }
  #             ],
  #             ....
  #         },
  #         ...
  #     ]
  #     ...
  # }
  config = json.loads(contents)
  ordered_config = OrderedDict()
  for key in sorted(config.keys()):
    study = copy.deepcopy(config[key])
    ordered_study = []
    for experiment_config in study:
      ordered_experiment_config = OrderedDict([
          ('platforms', experiment_config['platforms']),
          ('experiments', [])])
      for experiment in experiment_config['experiments']:
        ordered_experiment = OrderedDict()
        for index in xrange(0, 10):
          comment_key = '//' + str(index)
          if comment_key in experiment:
            ordered_experiment[comment_key] = experiment[comment_key]
        ordered_experiment['name'] = experiment['name']
        if 'forcing_flag' in experiment:
          ordered_experiment['forcing_flag'] = experiment['forcing_flag']
        if 'params' in experiment:
          ordered_experiment['params'] = OrderedDict(
              sorted(experiment['params'].items(), key=lambda t: t[0]))
        if 'enable_features' in experiment:
          ordered_experiment['enable_features'] = \
              sorted(experiment['enable_features'])
        if 'disable_features' in experiment:
          ordered_experiment['disable_features'] = \
              sorted(experiment['disable_features'])
        ordered_experiment_config['experiments'].append(ordered_experiment)
      ordered_study.append(ordered_experiment_config)
    ordered_config[key] = ordered_study
  return json.dumps(ordered_config,
                    sort_keys=False, indent=4,
                    separators=(',', ': ')) + '\n'

def ValidateData(json_data, file_path, message_type):
  """Validates the format of a fieldtrial configuration.

  Args:
    json_data: Parsed JSON object representing the fieldtrial config.
    file_path: String representing the path to the JSON file.
    message_type: Type of message from |output_api| to return in the case of
        errors/warnings.

  Returns:
    A list of |message_type| messages. In the case of all tests passing with no
    warnings/errors, this will return [].
  """
  if not isinstance(json_data, dict):
    return _CreateMalformedConfigMessage(message_type, file_path,
                                         'Expecting dict')
  for (study, experiment_configs) in json_data.iteritems():
    if not isinstance(study, unicode):
      return _CreateMalformedConfigMessage(message_type, file_path,
          'Expecting keys to be string, got %s', type(study))
    if not isinstance(experiment_configs, list):
      return _CreateMalformedConfigMessage(message_type, file_path,
          'Expecting list for study %s', study)
    for experiment_config in experiment_configs:
      if not isinstance(experiment_config, dict):
        return _CreateMalformedConfigMessage(message_type, file_path,
            'Expecting dict for experiment config in Study[%s]', study)
      if not 'experiments' in experiment_config:
        return _CreateMalformedConfigMessage(message_type, file_path,
            'Missing valid experiments for experiment config in Study[%s]',
            study)
      if not isinstance(experiment_config['experiments'], list):
        return _CreateMalformedConfigMessage(message_type, file_path,
            'Expecting list for experiments in Study[%s]', study)
      for experiment in experiment_config['experiments']:
        if not 'name' in experiment or not isinstance(experiment['name'],
                                                      unicode):
          return _CreateMalformedConfigMessage(message_type, file_path,
              'Missing valid name for experiment in Study[%s]', study)
        if 'params' in experiment:
          params = experiment['params']
          if not isinstance(params, dict):
            return _CreateMalformedConfigMessage(message_type, file_path,
                'Expected dict for params for Experiment[%s] in Study[%s]',
                experiment['name'], study)
          for (key, value) in params.iteritems():
            if not isinstance(key, unicode) or not isinstance(value, unicode):
              return _CreateMalformedConfigMessage(message_type, file_path,
                  'Invalid param (%s: %s) for Experiment[%s] in Study[%s]',
                  key, value, experiment['name'], study)
        for key in experiment.keys():
          if key not in VALID_EXPERIMENT_KEYS:
            return _CreateMalformedConfigMessage(message_type, file_path,
                'Key[%s] in Experiment[%s] in Study[%s] is not a valid key.',
                key, experiment['name'], study)
      if not 'platforms' in experiment_config:
        return _CreateMalformedConfigMessage(message_type, file_path,
            'Missing valid platforms for experiment config in Study[%s]', study)
      if not isinstance(experiment_config['platforms'], list):
        return _CreateMalformedConfigMessage(message_type, file_path,
            'Expecting list for platforms in Study[%s]', study)
      supported_platforms = ['android', 'chromeos', 'ios', 'linux', 'mac',
                             'win']
      experiment_platforms = experiment_config['platforms']
      unsupported_platforms = list(set(experiment_platforms).difference(
                                       supported_platforms))
      if unsupported_platforms:
        return _CreateMalformedConfigMessage(message_type, file_path,
                              'Unsupported platforms %s in Study[%s]',
                              unsupported_platforms, study)

  return []

def _CreateMalformedConfigMessage(message_type, file_path, message_format,
                                  *args):
  """Returns a list containing one |message_type| with the error message.

  Args:
    message_type: Type of message from |output_api| to return in the case of
        errors/warnings.
    message_format: The error message format string.
    file_path: The path to the config file.
    *args: The args for message_format.

  Returns:
    A list containing a message_type with a formatted error message and
    'Malformed config file [file]: ' prepended to it.
  """
  error_message_format = 'Malformed config file %s: ' + message_format
  format_args = (file_path,) + args
  return [message_type(error_message_format % format_args)]

def CheckPretty(contents, file_path, message_type):
  """Validates the pretty printing of fieldtrial configuration.

  Args:
    contents: File contents as a string.
    file_path: String representing the path to the JSON file.
    message_type: Type of message from |output_api| to return in the case of
        errors/warnings.

  Returns:
    A list of |message_type| messages. In the case of all tests passing with no
    warnings/errors, this will return [].
  """
  pretty = PrettyPrint(contents)
  if contents != pretty:
    return [message_type(
        'Pretty printing error: Run '
        'python testing/variations/PRESUBMIT.py %s' % file_path)]
  return []

def CommonChecks(input_api, output_api):
  affected_files = input_api.AffectedFiles(
      include_deletes=False,
      file_filter=lambda x: x.LocalPath().endswith('.json'))
  for f in affected_files:
    contents = input_api.ReadFile(f)
    try:
      json_data = input_api.json.loads(contents)
      result = ValidateData(json_data, f.LocalPath(), output_api.PresubmitError)
      if len(result):
        return result
      result = CheckPretty(contents, f.LocalPath(), output_api.PresubmitError)
      if len(result):
        return result
    except ValueError:
      return [output_api.PresubmitError(
          'Malformed JSON file: %s' % f.LocalPath())]
  return []

def CheckChangeOnUpload(input_api, output_api):
  return CommonChecks(input_api, output_api)

def CheckChangeOnCommit(input_api, output_api):
  return CommonChecks(input_api, output_api)


def main(argv):
  content = open(argv[1]).read()
  pretty = PrettyPrint(content)
  open(argv[1], 'wb').write(pretty)

if __name__ == "__main__":
  sys.exit(main(sys.argv))
