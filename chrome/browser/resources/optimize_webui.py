#!/usr/bin/env python
# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import argparse
import itertools
import json
import os
import platform
import re
import shutil
import sys
import tempfile


_HERE_PATH = os.path.dirname(__file__)
_SRC_PATH = os.path.normpath(os.path.join(_HERE_PATH, '..', '..', '..'))
_CWD = os.getcwd()  # NOTE(dbeam): this is typically out/<gn_name>/.

sys.path.append(os.path.join(_SRC_PATH, 'third_party', 'node'))
import node
import node_modules


_RESOURCES_PATH = os.path.join(_SRC_PATH, 'ui', 'webui', 'resources')


_CR_ELEMENTS_PATH = os.path.join(_RESOURCES_PATH, 'cr_elements')


_CR_COMPONENTS_PATH = os.path.join(_RESOURCES_PATH, 'cr_components')


_CSS_RESOURCES_PATH = os.path.join(_RESOURCES_PATH, 'css')


_HTML_RESOURCES_PATH = os.path.join(_RESOURCES_PATH, 'html')


_JS_RESOURCES_PATH = os.path.join(_RESOURCES_PATH, 'js')


_POLYMER_PATH = os.path.join(
    _SRC_PATH, 'third_party', 'polymer', 'v1_0', 'components-chromium')


_VULCANIZE_BASE_ARGS = [
  # These files are already combined and minified.
  '--exclude', 'chrome://resources/html/polymer.html',
  '--exclude', 'chrome://resources/polymer/v1_0/polymer/polymer.html',
  '--exclude', 'chrome://resources/polymer/v1_0/polymer/polymer-micro.html',
  '--exclude', 'chrome://resources/polymer/v1_0/polymer/polymer-mini.html',
  '--exclude', 'chrome://resources/polymer/v1_0/web-animations-js/' +
      'web-animations-next-lite.min.js',

  '--exclude', 'chrome://resources/css/roboto.css',
  '--exclude', 'chrome://resources/css/text_defaults.css',
  '--exclude', 'chrome://resources/css/text_defaults_md.css',
  '--exclude', 'chrome://resources/js/load_time_data.js',

  '--inline-css',
  '--inline-scripts',
  '--rewrite-urls-in-templates',
  '--strip-comments',
]


_URL_MAPPINGS = [
    ('chrome://resources/cr_components/', _CR_COMPONENTS_PATH),
    ('chrome://resources/cr_elements/', _CR_ELEMENTS_PATH),
    ('chrome://resources/css/', _CSS_RESOURCES_PATH),
    ('chrome://resources/html/', _HTML_RESOURCES_PATH),
    ('chrome://resources/js/', _JS_RESOURCES_PATH),
    ('chrome://resources/polymer/v1_0/', _POLYMER_PATH)
]


_VULCANIZE_REDIRECT_ARGS = list(itertools.chain.from_iterable(map(
    lambda m: ['--redirect', '"%s|%s"' % (m[0], m[1])], _URL_MAPPINGS)))


def _undo_mapping(mappings, url):
  for (redirect_url, file_path) in mappings:
    if url.startswith(redirect_url):
      return url.replace(redirect_url, file_path + os.sep, 1)
  # TODO(dbeam): can we make this stricter?
  return url

def _request_list_path(out_path, host):
  return os.path.join(out_path, host + '_requestlist.txt')

# Get a list of all files that were bundled with polymer-bundler and update the
# depfile accordingly such that Ninja knows when to re-trigger.
def _update_dep_file(in_folder, args, manifest):
  in_path = os.path.join(_CWD, in_folder)

  # Gather the dependencies of all bundled root HTML files.
  request_list = []
  for html_file in manifest:
    request_list += manifest[html_file]

  # Add a slash in front of every dependency that is not a chrome:// URL, so
  # that we can map it to the correct source file path below.
  request_list = map(
      lambda dep: '/' + dep if not dep.startswith('chrome://') else dep,
      request_list)

  # Undo the URL mappings applied by vulcanize to get file paths relative to
  # current working directory.
  url_mappings = _URL_MAPPINGS + [
      ('/', os.path.relpath(in_path, _CWD)),
      ('chrome://%s/' % args.host, os.path.relpath(in_path, _CWD)),
  ]

  deps = [_undo_mapping(url_mappings, u) for u in request_list]
  deps = map(os.path.normpath, deps)

  # If the input was a folder holding an unpacked .pak file, the generated
  # depfile should not list files already in the .pak file.
  if args.input.endswith('.unpak'):
    filter_url = args.input
    deps = [d for d in deps if not d.startswith(filter_url)]

  with open(os.path.join(_CWD, args.depfile), 'w') as f:
    deps_file_header = os.path.join(args.out_folder, args.html_out_files[0])
    f.write(deps_file_header + ': ' + ' '.join(deps))


def _optimize(in_folder, args):
  in_path = os.path.normpath(os.path.join(_CWD, in_folder))
  out_path = os.path.join(_CWD, args.out_folder)
  manifest_out_path = _request_list_path(out_path, args.host)

  exclude_args = []
  for f in args.exclude or []:
    exclude_args.append('--exclude')
    exclude_args.append(f)

  in_html_args = []
  for f in args.html_in_files:
    in_html_args.append('--in-html')
    in_html_args.append(f)

  tmp_out_dir = os.path.join(out_path, 'bundled')
  node.RunNode(
      [node_modules.PathToBundler()] +
      _VULCANIZE_BASE_ARGS + _VULCANIZE_REDIRECT_ARGS + exclude_args +
      [# This file is dynamically created by C++. Need to specify an exclusion
       # URL for both the relative URL and chrome:// URL syntax.
       '--exclude', 'strings.js',
       '--exclude', 'chrome://%s/strings.js' % args.host,

       '--manifest-out', manifest_out_path,
       '--root', in_path,
       '--redirect', '"chrome://%s/|%s"' % (args.host, in_path),
       '--out-dir', os.path.relpath(tmp_out_dir, _CWD),
       '--shell', args.html_in_files[0],
      ] + in_html_args)

  for index, html_file in enumerate(args.html_in_files):
    with open(
        os.path.join(os.path.relpath(tmp_out_dir, _CWD), html_file), 'r') as f:
      output = f.read()

      # Grit includes are not supported, use HTML imports instead.
      output = output.replace('<include src="', '<include src-disabled="')

      if args.insert_in_head:
        assert '<head>' in output
        # NOTE(dbeam): polymer-bundler eats <base> tags after processing. This
        # undoes that by adding a <base> tag to the (post-processed) generated
        # output.
        output = output.replace('<head>', '<head>' + args.insert_in_head)

    # Open file again with 'w' such that the previous contents are overwritten.
    with open(
        os.path.join(os.path.relpath(tmp_out_dir, _CWD), html_file), 'w') as f:
      f.write(output)
      f.close()

  try:
    crisper_html_out_paths = []
    for index, html_in_file in enumerate(args.html_in_files):
      crisper_html_out_paths.append(
          os.path.join(tmp_out_dir, args.html_out_files[index]))
      js_out_file = args.js_out_files[index]

      # Run crisper to separate the JS from the HTML file.
      node.RunNode([node_modules.PathToCrisper(),
                   '--source', os.path.join(tmp_out_dir, html_in_file),
                   '--script-in-head', 'false',
                   '--html', crisper_html_out_paths[index],
                   '--js', os.path.join(tmp_out_dir, js_out_file)])

      # Pass the JS file through Uglify and write the output to its final
      # destination.
      node.RunNode([node_modules.PathToUglify(),
                    os.path.join(tmp_out_dir, js_out_file),
                    '--comments', '"/Copyright|license|LICENSE|\<\/?if/"',
                    '--output', os.path.join(out_path, js_out_file)])

    # Run polymer-css-build and write the output HTML files to their final
    # destination.
    pcb_html_out_paths = [
        os.path.join(out_path, f) for f in args.html_out_files]
    node.RunNode([node_modules.PathToPolymerCssBuild()] +
                 ['--no-inline-includes', '-f'] +
                 crisper_html_out_paths + ['-o'] + pcb_html_out_paths)
  finally:
    shutil.rmtree(tmp_out_dir)
  return manifest_out_path


def main(argv):
  parser = argparse.ArgumentParser()
  parser.add_argument('--depfile', required=True)
  parser.add_argument('--exclude', nargs='*')
  parser.add_argument('--host', required=True)
  parser.add_argument('--html_in_files', nargs='*', required=True)
  parser.add_argument('--html_out_files', nargs='*', required=True)
  parser.add_argument('--input', required=True)
  parser.add_argument('--insert_in_head')
  parser.add_argument('--js_out_files', nargs='*', required=True)
  parser.add_argument('--out_folder', required=True)
  args = parser.parse_args(argv)

  # NOTE(dbeam): on Windows, GN can send dirs/like/this. When joined, you might
  # get dirs/like/this\file.txt. This looks odd to windows. Normalize to right
  # the slashes.
  args.depfile = os.path.normpath(args.depfile)
  args.input = os.path.normpath(args.input)
  args.out_folder = os.path.normpath(args.out_folder)

  manifest_out_path = _optimize(args.input, args)

  # Prior call to _optimize() generated an output manifest file, containing
  # information about all files that were bundled. Grab it from there.
  manifest = json.loads(open(manifest_out_path, 'r').read())

  # polymer-bundler reports any missing files in the output manifest, instead of
  # directly failing. Ensure that no such files were encountered.
  if '_missing' in manifest:
    raise Exception(
        'polymer-bundler could not find files for the following URLs:\n' +
        '\n'.join(manifest['_missing']))

  _update_dep_file(args.input, args, manifest)


if __name__ == '__main__':
  main(sys.argv[1:])
