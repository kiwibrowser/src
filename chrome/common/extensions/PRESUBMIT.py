# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Presubmit script for changes affecting extensions.

See http://dev.chromium.org/developers/how-tos/depottools/presubmit-scripts
for more details about the presubmit API built into depot_tools.
"""
import fnmatch
import os
import re

EXTENSIONS_PATH = os.path.join('chrome', 'common', 'extensions')
DOCS_PATH = os.path.join(EXTENSIONS_PATH, 'docs')
SERVER2_PATH = os.path.join(DOCS_PATH, 'server2')
API_PATH = os.path.join(EXTENSIONS_PATH, 'api')
TEMPLATES_PATH = os.path.join(DOCS_PATH, 'templates')
PRIVATE_TEMPLATES_PATH = os.path.join(TEMPLATES_PATH, 'private')
PUBLIC_TEMPLATES_PATH = os.path.join(TEMPLATES_PATH, 'public')
INTROS_PATH = os.path.join(TEMPLATES_PATH, 'intros')
ARTICLES_PATH = os.path.join(TEMPLATES_PATH, 'articles')

LOCAL_PUBLIC_TEMPLATES_PATH = os.path.join('docs',
                                           'templates',
                                           'public')

EXTENSIONS_TO_REMOVE_FOR_CLEAN_URLS = ('.md', '.html')

def _ReadFile(filename):
  with open(filename) as f:
    return f.read()

def _ListFilesInPublic():
  all_files = []
  for path, dirs, files in os.walk(LOCAL_PUBLIC_TEMPLATES_PATH):
    all_files.extend(
        os.path.join(path, filename)[len(LOCAL_PUBLIC_TEMPLATES_PATH + os.sep):]
        for filename in files)
  return all_files

def _UnixName(name):
  name = os.path.splitext(name)[0]
  s1 = re.sub('([a-z])([A-Z])', r'\1_\2', name)
  s2 = re.sub('([A-Z]+)([A-Z][a-z])', r'\1_\2', s1)
  return s2.replace('.', '_').lower()

def _FindMatchingTemplates(template_name, template_path_list):
  matches = []
  unix_name = _UnixName(template_name)
  for template in template_path_list:
    if unix_name == _UnixName(template.split(os.sep)[-1]):
      basename, ext = os.path.splitext(template)
      # The docserver expects clean (extensionless) template URLs, so we
      # strip some extensions here when generating the list of matches.
      if ext in EXTENSIONS_TO_REMOVE_FOR_CLEAN_URLS:
        matches.append(basename)
      else:
        matches.append(template)
  return matches

def _SanitizeAPIName(name, api_path):
  if not api_path.endswith(os.sep):
    api_path += os.sep
  filename = os.path.splitext(name)[0][len(api_path):].replace(os.sep, '_')
  if 'experimental' in filename:
    filename = 'experimental_' + filename.replace('experimental_', '')
  return filename

def _CreateIntegrationTestArgs(affected_files):
  if (any(fnmatch.fnmatch(name, '%s*.py' % SERVER2_PATH)
         for name in affected_files) or
      any(fnmatch.fnmatch(name, '%s*' % PRIVATE_TEMPLATES_PATH)
          for name in affected_files)):
    return ['-a']
  args = []
  for name in affected_files:
    if (fnmatch.fnmatch(name, '%s*' % PUBLIC_TEMPLATES_PATH) or
        fnmatch.fnmatch(name, '%s*' % INTROS_PATH) or
        fnmatch.fnmatch(name, '%s*' % ARTICLES_PATH)):
      args.extend(_FindMatchingTemplates(name.split(os.sep)[-1],
                                         _ListFilesInPublic()))
    if fnmatch.fnmatch(name, '%s*' % API_PATH):
      args.extend(_FindMatchingTemplates(_SanitizeAPIName(name, API_PATH),
                                         _ListFilesInPublic()))
  return args

def _CheckHeadingIDs(input_api):
  ids_re = re.compile('<h[23].*id=.*?>')
  headings_re = re.compile('<h[23].*?>')
  bad_files = []
  for name in input_api.AbsoluteLocalPaths():
    if not os.path.exists(name):
      continue
    if (fnmatch.fnmatch(name, '*%s*' % INTROS_PATH) or
        fnmatch.fnmatch(name, '*%s*' % ARTICLES_PATH)):
      contents = input_api.ReadFile(name)
      if (len(re.findall(headings_re, contents)) !=
          len(re.findall(ids_re, contents))):
        bad_files.append(name)
  return bad_files

def _CheckLinks(input_api, output_api, results):
  for affected_file in input_api.AffectedFiles():
    name = affected_file.LocalPath()
    absolute_path = affected_file.AbsoluteLocalPath()
    if not os.path.exists(absolute_path):
      continue
    if (fnmatch.fnmatch(name, '%s*' % PUBLIC_TEMPLATES_PATH) or
        fnmatch.fnmatch(name, '%s*' % INTROS_PATH) or
        fnmatch.fnmatch(name, '%s*' % ARTICLES_PATH) or
        fnmatch.fnmatch(name, '%s*' % API_PATH)):
      contents = _ReadFile(absolute_path)
      args = []
      if input_api.platform == 'win32':
        args = [input_api.python_executable]
      args.extend([os.path.join('docs', 'server2', 'link_converter.py'),
                   '-o',
                   '-f',
                   absolute_path])
      output = input_api.subprocess.check_output(
          args,
          cwd=input_api.PresubmitLocalPath(),
          universal_newlines=True)
      if output != contents:
        changes = ''
        for i, (line1, line2) in enumerate(
            zip(contents.split('\n'), output.split('\n'))):
          if line1 != line2:
            changes = ('%s\nLine %d:\n-%s\n+%s\n' %
                (changes, i + 1, line1, line2))
        if changes:
          results.append(output_api.PresubmitPromptWarning(
              'File %s may have an old-style <a> link to an API page. Please '
              'run docs/server2/link_converter.py to convert the link[s], or '
              'convert them manually.\n\nSuggested changes are: %s' %
              (name, changes)))

def _CheckChange(input_api, output_api):
  results = [
      output_api.PresubmitError('File %s needs an id for each heading.' % name)
      for name in _CheckHeadingIDs(input_api)]
  try:
    integration_test = []
    # From depot_tools/presubmit_canned_checks.py:529
    if input_api.platform == 'win32':
      integration_test = [input_api.python_executable]
    integration_test.append(
        os.path.join('docs', 'server2', 'integration_test.py'))
    integration_test.extend(_CreateIntegrationTestArgs(input_api.LocalPaths()))
    input_api.subprocess.check_call(integration_test,
                                    cwd=input_api.PresubmitLocalPath())
  except input_api.subprocess.CalledProcessError:
    results.append(output_api.PresubmitError('IntegrationTest failed!'))

  # TODO(kalman): Re-enable this check, or decide to delete it forever. Now
  # that we have multiple directories it no longer works.
  # See http://crbug.com/297178.
  #_CheckLinks(input_api, output_api, results)

  return results

def CheckChangeOnUpload(input_api, output_api):
  results = []
  results += _CheckChange(input_api, output_api)
  return results

def CheckChangeOnCommit(input_api, output_api):
  return _CheckChange(input_api, output_api)
