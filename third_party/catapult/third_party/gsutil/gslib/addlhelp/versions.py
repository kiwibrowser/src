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
"""Additional help about object versioning."""

from __future__ import absolute_import

from gslib.help_provider import HelpProvider

_DETAILED_HELP_TEXT = ("""
<B>OVERVIEW</B>
  Versioning-enabled buckets maintain an archive of objects, providing a way to
  un-delete data that you accidentally deleted, or to retrieve older versions of
  your data. You can turn versioning on or off for a bucket at any time. Turning
  versioning off leaves existing object versions in place, and simply causes the
  bucket to stop accumulating new object versions. In this case, if you upload
  to an existing object the current version is overwritten instead of creating
  a new version.

  Regardless of whether you have enabled versioning on a bucket, every object
  has two associated positive integer fields:

  - the generation, which is updated when the content of an object is
    overwritten.
  - the metageneration, which identifies the metadata generation. It starts
    at 1; is updated every time the metadata (e.g., ACL or Content-Type) for a
    given content generation is updated; and gets reset when the generation
    number changes.

  Of these two integers, only the generation is used when working with versioned
  data. Both generation and metageneration can be used with concurrency control
  (discussed in a later section).

  To work with object versioning in gsutil, you can use a flavor of storage URLs
  that that embed the object generation, which we refer to as version-specific
  URLs. For example, the version-less object URL:

    gs://bucket/object

  might have have two versions, with these version-specific URLs:

    gs://bucket/object#1360383693690000
    gs://bucket/object#1360383802725000

  The following sections discuss how to work with versioning and concurrency
  control.


<B>OBJECT VERSIONING</B>
  You can view, enable, and disable object versioning on a bucket using
  the 'versioning get' and 'versioning set' commands. For example:

    gsutil versioning set on gs://bucket

  will enable versioning for the named bucket. See 'gsutil help versioning'
  for additional details.

  To see all object versions in a versioning-enabled bucket along with
  their generation.metageneration information, use gsutil ls -a:

    gsutil ls -a gs://bucket

  You can also specify particular objects for which you want to find the
  version-specific URL(s), or you can use wildcards:

    gsutil ls -a gs://bucket/object1 gs://bucket/images/*.jpg

  The generation values form a monotonically increasing sequence as you create
  additional object versions.  Because of this, the latest object version is
  always the last one listed in the gsutil ls output for a particular object.
  For example, if a bucket contains these three versions of gs://bucket/object:

    gs://bucket/object#1360035307075000
    gs://bucket/object#1360101007329000
    gs://bucket/object#1360102216114000

  then gs://bucket/object#1360102216114000 is the latest version and
  gs://bucket/object#1360035307075000 is the oldest available version.

  If you specify version-less URLs with gsutil, you will operate only on the
  live version of an object, for example:

    gsutil cp gs://bucket/object ./dir

  or:

    gsutil rm gs://bucket/object

  The same is true when using wildcards like * and **. These will operate only
  on the live version of matching objects. For example, this
  command will remove the live version and create an archived version for each
  object in a bucket:

    gsutil rm gs://bucket/**

  To operate on a specific object version, use a version-specific URL.
  For example, suppose the output of the above gsutil ls -a command is:

    gs://bucket/object#1360035307075000
    gs://bucket/object#1360101007329000

  In this case, the command:

    gsutil cp gs://bucket/object#1360035307075000 ./dir

  will retrieve the second most recent version of the object.

  Note that version-specific URLs cannot be the target of the gsutil cp
  command (trying to do so will result in an error), because writing to a
  versioned object always creates a new version.

  If an object has been deleted, it will not show up in a normal gsutil ls
  listing (i.e., ls without the -a option). You can restore a deleted object by
  running gsutil ls -a to find the available versions, and then copying one of
  the version-specific URLs to the version-less URL, for example:

    gsutil cp gs://bucket/object#1360101007329000 gs://bucket/object

  Note that when you do this it creates a new object version, which will incur
  additional charges. You can get rid of the extra copy by deleting the older
  version-specfic object:

    gsutil rm gs://bucket/object#1360101007329000

  Or you can combine the two steps by using the gsutil mv command:

    gsutil mv gs://bucket/object#1360101007329000 gs://bucket/object

  If you remove the live version of an object in a versioning-enabled bucket,
  an archived version will be preserved:

    gsutil rm gs://bucket/object

  If you remove a version-specific URL for an object (even if it is the live
  version), that version will be deleted permanently:

    gsutil rm gs://bucket/object#1360101007329000

  If you want to remove all versions of an object, use the gsutil rm -a option:

    gsutil rm -a gs://bucket/object

  If you want to remove all versions of all objects in a bucket (and the bucket
  itself), use the rm -r option (-r implies the -a option):

    gsutil rm -r gs://bucket


  Note that there is no limit to the number of older versions of an object you
  will create if you continue to upload to the same object in a versioning-
  enabled bucket. It is your responsibility to delete versions beyond the ones
  you want to retain.


<B>COPYING VERSIONED BUCKETS</B>
  You can copy data between two versioned buckets, using a command like:

    gsutil cp -r -A gs://bucket1/* gs://bucket2

  When run using versioned buckets, this command will cause every object version
  to be copied. The copies made in gs://bucket2 will have different generation
  numbers (since a new generation is assigned when the object copy is made),
  but the object sort order will remain consistent. For example, gs://bucket1
  might contain:

    % gsutil ls -la gs://bucket1 10  2013-06-06T02:33:11Z
    53  2013-02-02T22:30:57Z  gs://bucket1/file#1359844257574000  metageneration=1
    12  2013-02-02T22:30:57Z  gs://bucket1/file#1359844257615000  metageneration=1
    97  2013-02-02T22:30:57Z  gs://bucket1/file#1359844257665000  metageneration=1

  and after the copy, gs://bucket2 might contain:

    % gsutil ls -la gs://bucket2
    53  2013-06-06T02:33:11Z  gs://bucket2/file#1370485991580000  metageneration=1
    12  2013-06-06T02:33:14Z  gs://bucket2/file#1370485994328000  metageneration=1
    97  2013-06-06T02:33:17Z  gs://bucket2/file#1370485997376000  metageneration=1

  Note that the object versions are in the same order (as can be seen by the
  same sequence of sizes in both listings), but the generation numbers (and
  timestamps) are newer in gs://bucket2.



<B>CONCURRENCY CONTROL</B>
  If you are building an application using Google Cloud Storage, you may need to
  be careful about concurrency control. Normally gsutil itself isn't used for
  this purpose, but it's possible to write scripts around gsutil that perform
  concurrency control.

  For example, suppose you want to implement a "rolling update" system using
  gsutil, where a periodic job computes some data and uploads it to the cloud.
  On each run, the job starts with the data that it computed from last run, and
  computes a new value. To make this system robust, you need to have multiple
  machines on which the job can run, which raises the possibility that two
  simultaneous runs could attempt to update an object at the same time. This
  leads to the following potential race condition:

  - job 1 computes the new value to be written
  - job 2 computes the new value to be written
  - job 2 writes the new value
  - job 1 writes the new value

  In this case, the value that job 1 read is no longer current by the time
  it goes to write the updated object, and writing at this point would result
  in stale (or, depending on the application, corrupt) data.

  To prevent this, you can find the version-specific name of the object that was
  created, and then use the information contained in that URL to specify an
  x-goog-if-generation-match header on a subsequent gsutil cp command. You can
  do this in two steps. First, use the gsutil cp -v option at upload time to get
  the version-specific name of the object that was created, for example:

    gsutil cp -v file gs://bucket/object

  might output:

    Created: gs://bucket/object#1360432179236000

  You can extract the generation value from this object and then construct a
  subsequent gsutil command like this:

    gsutil -h x-goog-if-generation-match:1360432179236000 cp newfile \\
        gs://bucket/object

  This command requests Google Cloud Storage to attempt to upload newfile
  but to fail the request if the generation of newfile that is live at the
  time of the upload does not match that specified.

  If the command you use updates object metadata, you will need to find the
  current metageneration for an object. To do this, use the gsutil ls -a and
  -l options. For example, the command:

    gsutil ls -l -a gs://bucket/object

  will output something like:

      64  2013-02-12T19:59:13Z  gs://bucket/object#1360699153986000  metageneration=3
    1521  2013-02-13T02:04:08Z  gs://bucket/object#1360721048778000  metageneration=2

  Given this information, you could use the following command to request setting
  the ACL on the older version of the object, such that the command will fail
  unless that is the current version of the data+metadata:

    gsutil -h x-goog-if-generation-match:1360699153986000 -h \\
      x-goog-if-metageneration-match:3 acl set public-read \\
      gs://bucket/object#1360699153986000

  Without adding these headers, the update would simply overwrite the existing
  ACL. Note that in contrast, the "gsutil acl ch" command uses these headers
  automatically, because it performs a read-modify-write cycle in order to edit
  ACLs.

  If you want to experiment with how generations and metagenerations work, try
  the following. First, upload an object; then use gsutil ls -l -a to list all
  versions of the object, along with each version's metageneration; then re-
  upload the object and repeat the gsutil ls -l -a. You should see two object
  versions, each with metageneration=1. Now try setting the ACL, and rerun the
  gsutil ls -l -a. You should see the most recent object generation now has
  metageneration=2.


<B>FOR MORE INFORMATION</B>
  For more details on how to use versioning and preconditions, see
  https://cloud.google.com/storage/docs/object-versioning
""")


class CommandOptions(HelpProvider):
  """Additional help about object versioning."""

  # Help specification. See help_provider.py for documentation.
  help_spec = HelpProvider.HelpSpec(
      help_name='versions',
      help_name_aliases=['concurrency', 'concurrency control'],
      help_type='additional_help',
      help_one_line_summary='Object Versioning and Concurrency Control',
      help_text=_DETAILED_HELP_TEXT,
      subcommand_help_text={},
  )
