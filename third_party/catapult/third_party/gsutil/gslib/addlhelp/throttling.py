# -*- coding: utf-8 -*-
# Copyright 2015 Google Inc. All Rights Reserved.
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
"""Additional help text for throttling gsutil."""

from __future__ import absolute_import

from gslib.help_provider import HelpProvider

_DETAILED_HELP_TEXT = ("""
<B>OVERVIEW</B>
  Particularly when used with the -m (multi-threading) option, gsutil can
  consume a significant amount of network bandwidth. In some cases this can
  cause problems, for example if you start a large rsync operation over a
  network link that's also used by a number of other important jobs.

  While gsutil has no built-in support for throttling requests, there are
  various tools available on Linux and MacOS that can be used to throttle
  gsutil requests.

  One tool is `trickle <https://github.com/mariusae/trickle>`_ (available via
  apt-get on Ubuntu systems), which will let you limit how much bandwidth gsutil
  consumes. For example, the following command would limit upload and download
  bandwidth consumed by gsutil rsync to 100 KBps:

      trickle -d 100 -u 100 gsutil rsync -r ./dir gs://some bucket

  Note that we recommend against using the -m flag with gsutil when running via
  trickle, as this may cause resource starvation and prevent your command from
  finishing.

  Another tool is
  `ionice <http://www.tutorialspoint.com/unix_commands/ionice.htm>`_ (built
  in to many Linux systems), which will let you limit how much I/O capacity
  gsutil consumes (e.g., to avoid letting it monopolize your local disk). For
  example, the following command would reduce I/O priority of gsutil so it
  doesn't monopolize your local disk:

      ionice -c 2 -n 7 gsutil -m rsync -r ./dir gs://some bucket
""")


class CommandOptions(HelpProvider):
  """Additional help text for throttling gsutil."""

  # Help specification. See help_provider.py for documentation.
  help_spec = HelpProvider.HelpSpec(
      help_name='throttling',
      help_name_aliases=['bandwidth', 'limit', 'nice'],
      help_type='additional_help',
      help_one_line_summary='Throttling gsutil',
      help_text=_DETAILED_HELP_TEXT,
      subcommand_help_text={},
  )
