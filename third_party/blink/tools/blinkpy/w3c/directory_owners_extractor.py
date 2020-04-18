# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""A limited finder & parser for Chromium OWNERS files.

This module is intended to be used within LayoutTests/external and is
informative only. For authoritative uses, please rely on `git cl owners`.
For example, it does not support directives other than email addresses.
"""

import collections
import re

from blinkpy.common.memoized import memoized
from blinkpy.common.path_finder import PathFinder
from blinkpy.common.system.filesystem import FileSystem


# Format of OWNERS files can be found at //src/third_party/depot_tools/owners.py
# In our use case (under external/wpt), we only process the first enclosing
# OWNERS file for any given path (i.e. always assuming "set noparent"), and we
# ignore "per-file:" lines, "file:" directives, etc.

# Recognizes 'X@Y' email addresses. Very simplistic. (from owners.py)
BASIC_EMAIL_REGEXP = r'^[\w\-\+\%\.]+\@[\w\-\+\%\.]+$'
WPT_NOTIFY_REGEXP = r'^# *WPT-NOTIFY: *true$'
COMPONENT_REGEXP = r'^# *COMPONENT: *(.+)$'


class DirectoryOwnersExtractor(object):

    def __init__(self, filesystem=None):
        self.filesystem = filesystem or FileSystem()
        self.finder = PathFinder(filesystem)
        self.owner_map = None

    def list_owners(self, changed_files):
        """Looks up the owners for the given set of changed files.

        Args:
            changed_files: A list of file paths relative to the repository root.

        Returns:
            A dict mapping tuples of owner email addresses to lists of
            owned directories (paths relative to the root of layout tests).
        """
        email_map = collections.defaultdict(set)
        external_root_owners = self.finder.path_from_layout_tests('external', 'OWNERS')
        for relpath in changed_files:
            # Try to find the first *non-empty* OWNERS file.
            absolute_path = self.finder.path_from_chromium_base(relpath)
            owners = None
            owners_file = self.find_owners_file(absolute_path)
            while owners_file:
                owners = self.extract_owners(owners_file)
                if owners:
                    break
                # Found an empty OWNERS file. Try again from the parent directory.
                absolute_path = self.filesystem.dirname(self.filesystem.dirname(owners_file))
                owners_file = self.find_owners_file(absolute_path)
            # Skip LayoutTests/external/OWNERS.
            if not owners or owners_file == external_root_owners:
                continue

            owned_directory = self.filesystem.dirname(owners_file)
            owned_directory_relpath = self.filesystem.relpath(owned_directory, self.finder.layout_tests_dir())
            email_map[tuple(owners)].add(owned_directory_relpath)
        return {owners: sorted(owned_directories) for owners, owned_directories in email_map.iteritems()}

    def find_owners_file(self, start_path):
        """Finds the first enclosing OWNERS file for a given path.

        Starting from the given path, walks up the directory tree until the
        first OWNERS file is found or LayoutTests/external is reached.

        Args:
            start_path: A relative path from the root of the repository, or an
                absolute path. The path can be a file or a directory.

        Returns:
            The absolute path to the first OWNERS file found; None if not found
            or if start_path is outside of LayoutTests/external.
        """
        abs_start_path = (start_path if self.filesystem.isabs(start_path)
                          else self.finder.path_from_chromium_base(start_path))
        directory = (abs_start_path if self.filesystem.isdir(abs_start_path)
                     else self.filesystem.dirname(abs_start_path))
        external_root = self.finder.path_from_layout_tests('external')
        if not directory.startswith(external_root):
            return None
        # Stop at LayoutTests, which is the parent of external_root.
        while directory != self.finder.layout_tests_dir():
            owners_file = self.filesystem.join(directory, 'OWNERS')
            if self.filesystem.isfile(self.finder.path_from_chromium_base(owners_file)):
                return owners_file
            directory = self.filesystem.dirname(directory)
        return None

    def extract_owners(self, owners_file):
        """Extracts owners from an OWNERS file.

        Args:
            owners_file: An absolute path to an OWNERS file.

        Returns:
            A list of valid owners (email addresses).
        """
        contents = self._read_text_file(owners_file)
        email_regexp = re.compile(BASIC_EMAIL_REGEXP)
        addresses = []
        for line in contents.splitlines():
            line = line.strip()
            if email_regexp.match(line):
                addresses.append(line)
        return addresses

    def extract_component(self, owners_file):
        """Extracts the component from an OWNERS file.

        Args:
            owners_file: An absolute path to an OWNERS file.

        Returns:
            A string, or None if not found.
        """
        contents = self._read_text_file(owners_file)
        search = re.search(COMPONENT_REGEXP, contents, re.MULTILINE)
        if search:
            return search.group(1)
        return None

    def is_wpt_notify_enabled(self, owners_file):
        """Checks if the OWNERS file enables WPT-NOTIFY.

        Args:
            owners_file: An absolute path to an OWNERS file.

        Returns:
            A boolean.
        """
        contents = self._read_text_file(owners_file)
        return bool(re.search(WPT_NOTIFY_REGEXP, contents, re.MULTILINE))

    @memoized
    def _read_text_file(self, path):
        return self.filesystem.read_text_file(path)
