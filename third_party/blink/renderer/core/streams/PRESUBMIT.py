# Copyright (c) 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# pylint: disable=invalid-name,import-error

"""Run eslint on the Streams API Javascript files.

See http://dev.chromium.org/developers/how-tos/depottools/presubmit-scripts
for more details about the presubmit API built into depot_tools, and see
https://chromium.googlesource.com/chromium/src/+/master/styleguide/web/web.md
for the rules we're checking against here.
"""


def CheckChangeOnUpload(input_api, output_api):
    return common_checks(input_api, output_api)


def CheckChangeOnCommit(input_api, output_api):
    return common_checks(input_api, output_api)


def common_checks(input_api, output_api):
    import sys
    web_dev_style_path = input_api.os_path.join(input_api.change.RepositoryRoot(),
                                                'tools')
    oldpath = sys.path
    sys.path = [input_api.PresubmitLocalPath(), web_dev_style_path] + sys.path
    from web_dev_style import js_checker
    sys.path = oldpath

    def is_resource(maybe_resource):
        return maybe_resource.AbsoluteLocalPath().endswith('.js')

    return js_checker.JSChecker(input_api, output_api,
                                file_filter=is_resource).RunChecks()
