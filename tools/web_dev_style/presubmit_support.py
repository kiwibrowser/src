# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


import css_checker
import html_checker
import js_checker
import resource_checker


def IsResource(f):
  return f.LocalPath().endswith(('.html', '.css', '.js'))


def CheckStyle(input_api, output_api, file_filter=lambda f: True):
  apis = input_api, output_api
  wrapped_filter = lambda f: file_filter(f) and IsResource(f)
  checkers = [
      css_checker.CSSChecker(*apis, file_filter=wrapped_filter),
      html_checker.HtmlChecker(*apis, file_filter=wrapped_filter),
      js_checker.JSChecker(*apis, file_filter=wrapped_filter),
      resource_checker.ResourceChecker(*apis, file_filter=wrapped_filter),
  ]
  results = []
  for checker in checkers:
    results.extend(checker.RunChecks())
  return results


def CheckStyleESLint(input_api, output_api):
  is_js = lambda f: f.LocalPath().endswith('.js')
  js_files = input_api.AffectedFiles(file_filter=is_js, include_deletes=False)
  if not js_files:
    return []
  return js_checker.JSChecker(input_api, output_api).RunEsLintChecks(js_files)


def DisallowIncludes(input_api, output_api, msg):
  return resource_checker.ResourceChecker(
      input_api, output_api, file_filter=IsResource).DisallowIncludes(msg)
