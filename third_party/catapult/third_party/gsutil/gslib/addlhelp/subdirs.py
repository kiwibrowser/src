# -*- coding: utf-8 -*-
# Copyright 2012 Google Inc. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
"""Additional help about subdirectory handling in gsutil."""

from __future__ import absolute_import

from gslib.help_provider import HelpProvider

_DETAILED_HELP_TEXT = ("""
<B>OVERVIEW</B>
  This section provides details about how subdirectories work in gsutil.
  Most users probably don't need to know these details, and can simply use
  the commands (like cp -r) that work with subdirectories. We provide this
  additional documentation to help users understand how gsutil handles
  subdirectories differently than most GUI / web-based tools (e.g., why
  those other tools create "dir_$folder$" objects), and also to explain cost and
  performance implications of the gsutil approach, for those interested in such
  details.

  gsutil provides the illusion of a hierarchical file tree atop the "flat"
  name space supported by the Google Cloud Storage service. To the service,
  the object gs://your-bucket/abc/def.txt is just an object that happens to
  have "/" characters in its name. There is no "abc" directory; just a single
  object with the given name. This diagram:

  .. image::  https://cloud.google.com/storage/images/gsutil-subdirectories.svg

  illustrates how gsutil provides a hierarchical view of objects in a bucket.

  gsutil achieves the hierarchical file tree illusion by applying a variety of
  rules, to try to make naming work the way users would expect. For example, in
  order to determine whether to treat a destination URL as an object name or the
  root of a directory under which objects should be copied gsutil uses these
  rules:

  1. If the destination object ends with a "/" gsutil treats it as a directory.
     For example, if you run the command:

       gsutil cp your-file gs://your-bucket/abc/

     gsutil will create the object gs://your-bucket/abc/your-file.

  2. If the destination object is XYZ and an object exists called XYZ_$folder$
     gsutil treats XYZ as a directory. For example, if you run the command:

       gsutil cp your-file gs://your-bucket/abc

     and there exists an object called abc_$folder$, gsutil will create the
     object gs://your-bucket/abc/your-file.

  3. If you attempt to copy multiple source files to a destination URL, gsutil
     treats the destination URL as a directory. For example, if you run
     the command:

       gsutil cp -r your-dir gs://your-bucket/abc

     gsutil will create objects like gs://your-bucket/abc/your-dir/file1, etc.
     (assuming file1 is a file under the source directory your-dir).

  4. If none of the above rules applies, gsutil performs a bucket listing to
     determine if the target of the operation is a prefix match to the
     specified string. For example, if you run the command:

       gsutil cp your-file gs://your-bucket/abc

     gsutil will make a bucket listing request for the named bucket, using
     delimiter="/" and prefix="abc". It will then examine the bucket listing
     results and determine whether there are objects in the bucket whose path
     starts with gs://your-bucket/abc/, to determine whether to treat the target
     as an object name or a directory name. In turn this impacts the name of the
     object you create: If the above check indicates there is an "abc" directory
     you will end up with the object gs://your-bucket/abc/your-file; otherwise
     you will end up with the object gs://your-bucket/abc. (See
     "HOW NAMES ARE CONSTRUCTED" under "gsutil help cp" for more details.)

  This rule-based approach stands in contrast to the way many tools work, which
  create objects to mark the existence of folders (such as "dir_$folder$").
  gsutil understands several conventions used by such tools but does not
  require such marker objects to implement naming behavior consistent with
  UNIX commands.

  A downside of the gsutil subdirectory naming approach is it requires an extra
  bucket listing before performing the needed cp or mv command. However those
  listings are relatively inexpensive, because they use delimiter and prefix
  parameters to limit result data. Moreover, gsutil makes only one bucket
  listing request per cp/mv command, and thus amortizes the bucket listing cost
  across all transferred objects (e.g., when performing a recursive copy of a
  directory to the cloud).


<B>POTENTIAL FOR SURPRISING DESTINATION SUBDIRECTORY NAMING</B>
  The above rules-based approach for determining how destination paths are
  constructed can lead to the following surprise: Suppose you start by trying to
  upload everything under a local directory to a bucket "subdirectory" that
  doesn't yet exist:

    gsutil cp -r ./your-dir/* gs://your-bucket/new

  where there are directories under your-dir (say, dir1 and dir2). The first
  time you run this command it will create the objects:

    gs://your-bucket/new/dir1/abc
    gs://your-bucket/new/dir2/abc

  because gs://your-bucket/new doesn't yet exist. If you run the same command
  again, because gs://your-bucket/new does now exist, it will create the
  additional objects:

    gs://your-bucket/new/your-dir/dir1/abc
    gs://your-bucket/new/your-dir/dir2/abc

  Beyond the fact that this naming behavior can surprise users, one particular
  case you should be careful about is if you script gsutil uploads with a retry
  loop. If you do this and the first attempt copies some but not all files,
  the second attempt will encounter an already existing source subdirectory
  and result in the above-described naming problem.

  There are a couple of ways to avoid this problem:

  1. Use gsutil rsync. Since rsync doesn't use the Unix cp-defined directory
  naming rules, it will work consistently whether the destination subdirectory
  exists or not.

  2. If using rsync won't work for you, you can start by creating a
  "placeholder" object to establish that the destination is a subdirectory, by
  running a command such as:

    gsutil cp some-file gs://your-bucket/new/placeholder

  At this point running the gsutil cp -r command noted above will
  consistently treat gs://your-bucket/new as a subdirectory. Once you have
  at least one object under that subdirectory you can delete the placeholder
  object and subsequent uploads to that subdirectory will continue to work
  with naming working as you'd expect.
""")


class CommandOptions(HelpProvider):
  """Additional help about subdirectory handling in gsutil."""

  # Help specification. See help_provider.py for documentation.
  help_spec = HelpProvider.HelpSpec(
      help_name='subdirs',
      help_name_aliases=[
          'dirs', 'directory', 'directories', 'folder', 'folders', 'hierarchy',
          'subdir', 'subdirectory', 'subdirectories'],
      help_type='additional_help',
      help_one_line_summary='How Subdirectories Work',
      help_text=_DETAILED_HELP_TEXT,
      subcommand_help_text={},
  )
