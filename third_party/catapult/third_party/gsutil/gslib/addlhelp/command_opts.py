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
"""Additional help about gsutil command-level options."""

from __future__ import absolute_import

from gslib.help_provider import HelpProvider

_DETAILED_HELP_TEXT = ("""
<B>DESCRIPTION</B>
  gsutil supports separate options for the top-level gsutil command and
  the individual sub-commands (like cp, rm, etc.) The top-level options
  control behavior of gsutil that apply across commands. For example, in
  the command:

    gsutil -m cp -p file gs://bucket/obj

  the -m option applies to gsutil, while the -p option applies to the cp
  sub-command.


<B>OPTIONS</B>
  -D          Shows HTTP requests/headers and additional debug info needed when
              posting support requests, including exception stack traces.

  -DD         Shows HTTP requests/headers, additional debug info,
              exception stack traces, plus HTTP upstream payload.

  -h          Allows you to specify certain HTTP headers, for example:

                gsutil -h "Cache-Control:public,max-age=3600" \\
                       -h "Content-Type:text/html" cp ...

              Note that you need to quote the headers/values that
              contain spaces (such as "Content-Disposition: attachment;
              filename=filename.ext"), to avoid having the shell split them
              into separate arguments.

              The following headers are stored as object metadata and used
              in future requests on the object:

                Cache-Control
                Content-Disposition
                Content-Encoding
                Content-Language
                Content-Type

              The following headers are used to check data integrity:

                Content-MD5

              gsutil also supports custom metadata headers with a matching
              Cloud Storage Provider prefix, such as:

                x-goog-meta-

              Note that for gs:// URLs, the Cache Control header is specific to
              the API being used. The XML API will accept any cache control
              headers and return them during object downloads.  The JSON API
              respects only the public, private, no-cache, and max-age cache
              control headers, and may add its own no-transform directive even
              if it was not specified. See 'gsutil help apis' for more
              information on gsutil's interaction with APIs.

              See also "gsutil help setmeta" for the ability to set metadata
              fields on objects after they have been uploaded.

  -m          Causes supported operations (acl ch, acl set, cp, mv, rm, rsync,
              and setmeta) to run in parallel. This can significantly improve
              performance if you are performing operations on a large number of
              files over a reasonably fast network connection.

              gsutil performs the specified operation using a combination of
              multi-threading and multi-processing, using a number of threads
              and processors determined by the parallel_thread_count and
              parallel_process_count values set in the boto configuration
              file. You might want to experiment with these values, as the
              best values can vary based on a number of factors, including
              network speed, number of CPUs, and available memory.

              Using the -m option may make your performance worse if you
              are using a slower network, such as the typical network speeds
              offered by non-business home network plans. It can also make
              your performance worse for cases that perform all operations
              locally (e.g., gsutil rsync, where both source and destination
              URLs are on the local disk), because it can "thrash" your local
              disk.

              If a download or upload operation using parallel transfer fails
              before the entire transfer is complete (e.g. failing after 300 of
              1000 files have been transferred), you will need to restart the
              entire transfer.

              Also, although most commands will normally fail upon encountering
              an error when the -m flag is disabled, all commands will
              continue to try all operations when -m is enabled with multiple
              threads or processes, and the number of failed operations (if any)
              will be reported at the end of the command's execution.

  -o          Set/override values in the boto configuration value, in the format
              <section>:<name>=<value>, e.g. gsutil -o "Boto:proxy=host" ...
              This will not pass the option to gsutil integration tests, which
              run in a separate process.

  -q          Causes gsutil to perform operations quietly, i.e., without
              reporting progress indicators of files being copied or removed,
              etc. Errors are still reported. This option can be useful for
              running gsutil from a cron job that logs its output to a file, for
              which the only information desired in the log is failures.

  -u          Allows you to specify a user project to be billed for the request.
              For example:

                gsutil -u "bill-this-project" cp ...
""")


class CommandOptions(HelpProvider):
  """Additional help about gsutil command-level options."""

  # Help specification. See help_provider.py for documentation.
  help_spec = HelpProvider.HelpSpec(
      help_name='options',
      help_name_aliases=['arg', 'args', 'cli', 'opt', 'opts'],
      help_type='additional_help',
      help_one_line_summary='Top-Level Command-Line Options',
      help_text=_DETAILED_HELP_TEXT,
      subcommand_help_text={},
  )
