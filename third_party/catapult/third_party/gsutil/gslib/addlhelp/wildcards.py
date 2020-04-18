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
"""Additional help about wildcards."""

from __future__ import absolute_import

from gslib.help_provider import HelpProvider

_DETAILED_HELP_TEXT = ("""
<B>DESCRIPTION</B>
  gsutil supports URI wildcards. For example, the command:

    gsutil cp gs://bucket/data/abc* .

  will copy all objects that start with gs://bucket/data/abc followed by any
  number of characters within that subdirectory.


<B>DIRECTORY BY DIRECTORY VS RECURSIVE WILDCARDS</B>
  The "*" wildcard only matches up to the end of a path within
  a subdirectory. For example, if bucket contains objects
  named gs://bucket/data/abcd, gs://bucket/data/abcdef,
  and gs://bucket/data/abcxyx, as well as an object in a sub-directory
  (gs://bucket/data/abc/def) the above gsutil cp command would match the
  first 3 object names but not the last one.

  If you want matches to span directory boundaries, use a '**' wildcard:

    gsutil cp gs://bucket/data/abc** .

  will match all four objects above.

  Note that gsutil supports the same wildcards for both objects and file names.
  Thus, for example:

    gsutil cp data/abc* gs://bucket

  will match all names in the local file system. Most command shells also
  support wildcarding, so if you run the above command probably your shell
  is expanding the matches before running gsutil. However, most shells do not
  support recursive wildcards ('**'), and you can cause gsutil's wildcarding
  support to work for such shells by single-quoting the arguments so they
  don't get interpreted by the shell before being passed to gsutil:

    gsutil cp 'data/abc**' gs://bucket


<B>BUCKET WILDCARDS</B>
  You can specify wildcards for bucket names within a single project. For
  example:

    gsutil ls gs://data*.example.com

  will list the contents of all buckets whose name starts with "data" and
  ends with ".example.com" in the default project. The -p option can be used
  to specify a project other than the default.  For example:

    gsutil ls -p other-project gs://data*.example.com

  You can also combine bucket and object name wildcards. For example this
  command will remove all ".txt" files in any of your Google Cloud Storage
  buckets in the default project:

    gsutil rm gs://*/**.txt


<B>OTHER WILDCARD CHARACTERS</B>
  In addition to '*', you can use these wildcards:

  ?
    Matches a single character. For example "gs://bucket/??.txt"
    only matches objects with two characters followed by .txt.

  [chars]
    Match any of the specified characters. For example
    "gs://bucket/[aeiou].txt" matches objects that contain a single vowel
    character followed by .txt

  [char range]
    Match any of the range of characters. For example
    "gs://bucket/[a-m].txt" matches objects that contain letters
    a, b, c, ... or m, and end with .txt.

  You can combine wildcards to provide more powerful matches, for example:

    gs://bucket/[a-m]??.j*g


<B>DIFFERENT BEHAVIOR FOR "DOT" FILES IN LOCAL FILE SYSTEM</B>
  Per standard Unix behavior, the wildcard "*" only matches files that don't
  start with a "." character (to avoid confusion with the "." and ".."
  directories present in all Unix directories). gsutil provides this same
  behavior when using wildcards over a file system URI, but does not provide
  this behavior over cloud URIs. For example, the following command will copy
  all objects from gs://bucket1 to gs://bucket2:

    gsutil cp gs://bucket1/* gs://bucket2

  but the following command will copy only files that don't start with a "."
  from the directory "dir" to gs://bucket1:

    gsutil cp dir/* gs://bucket1


<B>EFFICIENCY CONSIDERATION: USING WILDCARDS OVER MANY OBJECTS</B>
  It is more efficient, faster, and less network traffic-intensive
  to use wildcards that have a non-wildcard object-name prefix, like:

    gs://bucket/abc*.txt

  than it is to use wildcards as the first part of the object name, like:

    gs://bucket/*abc.txt

  This is because the request for "gs://bucket/abc*.txt" asks the server to send
  back the subset of results whose object name start with "abc" at the bucket
  root, and then gsutil filters the result list for objects whose name ends with
  ".txt".  In contrast, "gs://bucket/*abc.txt" asks the server for the complete
  list of objects in the bucket root, and then filters for those objects whose
  name ends with "abc.txt". This efficiency consideration becomes increasingly
  noticeable when you use buckets containing thousands or more objects. It is
  sometimes possible to set up the names of your objects to fit with expected
  wildcard matching patterns, to take advantage of the efficiency of doing
  server-side prefix requests. See, for example "gsutil help prod" for a
  concrete use case example.


<B>EFFICIENCY CONSIDERATION: USING MID-PATH WILDCARDS</B>
  Suppose you have a bucket with these objects:

    gs://bucket/obj1
    gs://bucket/obj2
    gs://bucket/obj3
    gs://bucket/obj4
    gs://bucket/dir1/obj5
    gs://bucket/dir2/obj6

  If you run the command:

    gsutil ls gs://bucket/*/obj5

  gsutil will perform a /-delimited top-level bucket listing and then one bucket
  listing for each subdirectory, for a total of 3 bucket listings:

    GET /bucket/?delimiter=/
    GET /bucket/?prefix=dir1/obj5&delimiter=/
    GET /bucket/?prefix=dir2/obj5&delimiter=/

  The more bucket listings your wildcard requires, the slower and more expensive
  it will be. The number of bucket listings required grows as:

  - the number of wildcard components (e.g., "gs://bucket/a??b/c*/*/d"
    has 3 wildcard components);
  - the number of subdirectories that match each component; and
  - the number of results (pagination is implemented using one GET
    request per 1000 results, specifying markers for each).

  If you want to use a mid-path wildcard, you might try instead using a
  recursive wildcard, for example:

    gsutil ls gs://bucket/**/obj5

  This will match more objects than "gs://bucket/*/obj5" (since it spans
  directories), but is implemented using a delimiter-less bucket listing
  request (which means fewer bucket requests, though it will list the entire
  bucket and filter locally, so that could require a non-trivial amount of
  network traffic).
""")


class CommandOptions(HelpProvider):
  """Additional help about wildcards."""

  # Help specification. See help_provider.py for documentation.
  help_spec = HelpProvider.HelpSpec(
      help_name='wildcards',
      help_name_aliases=['wildcard', '*', '**'],
      help_type='additional_help',
      help_one_line_summary='Wildcard Names',
      help_text=_DETAILED_HELP_TEXT,
      subcommand_help_text={},
  )
