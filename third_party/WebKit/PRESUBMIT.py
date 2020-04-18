# Copyright (c) 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Top-level presubmit script for Blink.

See http://dev.chromium.org/developers/how-tos/depottools/presubmit-scripts
for more details about the presubmit API built into gcl.
"""

import os


def _CommonChecks(input_api, output_api):
    """Checks common to both upload and commit."""
    # We should figure out what license checks we actually want to use.
    license_header = r'.*'

    results = []
    results.extend(input_api.canned_checks.PanProjectChecks(
        input_api, output_api, maxlen=800, license_header=license_header))
    return results


def _CheckStyle(input_api, output_api):
    style_checker_path = input_api.os_path.join(input_api.PresubmitLocalPath(), '..', 'blink',
                                                'tools', 'check_blink_style.py')
    args = [input_api.python_executable, style_checker_path, '--diff-files']
    files = []
    for f in input_api.AffectedFiles():
        file_path = f.LocalPath()
        # Filter out changes in LayoutTests.
        if 'LayoutTests' + input_api.os_path.sep in file_path and 'TestExpectations' not in file_path:
            continue
        files.append(input_api.os_path.join('..', '..', file_path))
    # Do not call check_blink_style.py with empty affected file list if all
    # input_api.AffectedFiles got filtered.
    if not files:
        return []
    args += files
    results = []

    try:
        child = input_api.subprocess.Popen(args,
                                           stderr=input_api.subprocess.PIPE)
        _, stderrdata = child.communicate()
        if child.returncode != 0:
            results.append(output_api.PresubmitError(
                'check_blink_style.py failed', [stderrdata]))
    except Exception as e:
        results.append(output_api.PresubmitNotifyResult(
            'Could not run check_blink_style.py', [str(e)]))

    return results


def CheckChangeOnUpload(input_api, output_api):
    results = []
    results.extend(_CommonChecks(input_api, output_api))
    results.extend(_CheckStyle(input_api, output_api))
    return results


def CheckChangeOnCommit(input_api, output_api):
    results = []
    results.extend(_CommonChecks(input_api, output_api))
    results.extend(input_api.canned_checks.CheckTreeIsOpen(
        input_api, output_api,
        json_url='http://chromium-status.appspot.com/current?format=json'))
    results.extend(input_api.canned_checks.CheckChangeHasDescription(
        input_api, output_api))
    return results


def _ArePaintOrCompositingDirectoriesModified(change):  # pylint: disable=C0103
    """Checks whether CL has changes to paint or compositing directories."""
    paint_or_compositing_paths = [
        os.path.join('third_party', 'WebKit', 'LayoutTests', 'FlagExpectations',
                     'enable-slimming-paint-v2'),
        os.path.join('third_party', 'WebKit', 'LayoutTests', 'flag-specific',
                     'enable-slimming-paint-v2'),
        os.path.join('third_party', 'WebKit', 'LayoutTests', 'FlagExpectations',
                     'enable-slimming-paint-v175'),
        os.path.join('third_party', 'WebKit', 'LayoutTests', 'flag-specific',
                     'enable-slimming-paint-v175'),
    ]
    for affected_file in change.AffectedFiles():
        file_path = affected_file.LocalPath()
        if any(x in file_path for x in paint_or_compositing_paths):
            return True
    return False


def _AreLayoutNGDirectoriesModified(change):  # pylint: disable=C0103
    """Checks whether CL has changes to a layout ng directory."""
    layout_ng_paths = [
        os.path.join('third_party', 'WebKit', 'LayoutTests', 'FlagExpectations',
                     'enable-blink-features=LayoutNG'),
        os.path.join('third_party', 'WebKit', 'LayoutTests', 'flag-specific',
                     'enable-blink-features=LayoutNG'),
    ]
    for affected_file in change.AffectedFiles():
        file_path = affected_file.LocalPath()
        if any(x in file_path for x in layout_ng_paths):
            return True
    return False


def PostUploadHook(cl, change, output_api):  # pylint: disable=C0103
    """git cl upload will call this hook after the issue is created/modified.

    This hook adds extra try bots to the CL description in order to run slimming
    paint v2 tests or LayoutNG tests in addition to the CQ try bots if the
    change contains changes in a relevant direcotry (see:
    _ArePaintOrCompositingDirectoriesModified and
    _AreLayoutNGDirectoriesModified). For more information about
    slimming-paint-v2 tests see https://crbug.com/601275 and for information
    about the LayoutNG tests see https://crbug.com/706183.
    """
    results = []
    if _ArePaintOrCompositingDirectoriesModified(change):
        results.extend(output_api.EnsureCQIncludeTrybotsAreAdded(
            cl,
            ['master.tryserver.chromium.linux:'
             'linux_layout_tests_slimming_paint_v2',
             'master.tryserver.blink:linux_trusty_blink_rel'],
            'Automatically added linux_layout_tests_slimming_paint_v2 and '
            'linux_trusty_blink_rel to run on CQ due to changes in paint or '
            'compositing directories.'))
    if _AreLayoutNGDirectoriesModified(change):
        results.extend(output_api.EnsureCQIncludeTrybotsAreAdded(
            cl,
            ['master.tryserver.chromium.linux:'
             'linux_layout_tests_layout_ng'],
            'Automatically added linux_layout_tests_layout_ng to run on CQ due '
            'to changes in LayoutNG directories.'))
    return results
